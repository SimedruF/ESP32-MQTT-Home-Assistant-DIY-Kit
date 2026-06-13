#include "WiFiWebManager.h"
#include "../../include/SerialLog.h"

WiFiWebManager::WiFiWebManager(const char* apSSID, const char* apPassword, int serverPort)
    : _apSSID(apSSID), _apPassword(apPassword), _serverPort(serverPort),
      _wifiConfigured(false), _wifiConnected(false), _isAPMode(false) {
    _server = new WebServer(_serverPort);
}

bool WiFiWebManager::begin() {
    serialLog.println("\n=== WiFi Web Manager Initialization ===");

    // Make the board easy to identify in the router's DHCP client list.
    WiFi.setHostname("esp32-ha-kit");
    
    // Load saved credentials
    loadCredentials();
    
    // Try to connect to WiFi if configured
    if (_wifiConfigured && connectToWiFi()) {
        _isAPMode = false;
        serialLog.println("✅ Mode: WiFi Client");
        serialLog.println("Open in browser: http://" + WiFi.localIP().toString());
    } else {
        // Start Access Point mode
        startAccessPoint();
        _isAPMode = true;
        serialLog.println("📡 Mode: Access Point");
        serialLog.println("Open in browser: http://" + WiFi.softAPIP().toString());
    }
    
    // Setup default routes for WiFi management
    setupDefaultRoutes();
    
    // Start web server
    _server->begin();
    serialLog.println("✅ Web server started!");
    serialLog.println("=======================================\n");
    
    return true;
}

void WiFiWebManager::handleClient() {
    _server->handleClient();
}

void WiFiWebManager::on(const String& uri, HTTPMethod method, RouteCallback handler) {
    _server->on(uri, method, [this, handler]() {
        handler(*_server);
    });
}

bool WiFiWebManager::isConnected() const {
    return _wifiConnected;
}

String WiFiWebManager::getIPAddress() const {
    if (_wifiConnected) {
        return WiFi.localIP().toString();
    } else {
        return WiFi.softAPIP().toString();
    }
}

String WiFiWebManager::getSSID() const {
    if (_wifiConnected) {
        return WiFi.SSID();
    } else {
        return _apSSID;
    }
}

bool WiFiWebManager::isAPMode() const {
    return _isAPMode;
}

WebServer& WiFiWebManager::getServer() {
    return *_server;
}

void WiFiWebManager::saveCredentials(const String& ssid, const String& password) {
    if (!_preferences.begin("wifi", false)) {
        serialLog.println("Eroare NVS: credentialele WiFi nu au putut fi salvate");
        return;
    }
    _preferences.putString("ssid", ssid);
    _preferences.putString("password", password);
    _preferences.putBool("configured", true);
    _preferences.end();
    
    _savedSSID = ssid;
    _savedPassword = password;
    _wifiConfigured = true;
    
    serialLog.println("WiFi credentials saved to EEPROM");
}

void WiFiWebManager::clearCredentials() {
    if (!_preferences.begin("wifi", false)) {
        serialLog.println("Eroare NVS: credentialele WiFi nu au putut fi sterse");
        return;
    }
    _preferences.clear();
    _preferences.end();
    
    _wifiConfigured = false;
    _wifiConnected = false;
    _savedSSID = "";
    _savedPassword = "";
    
    serialLog.println("WiFi credentials cleared from EEPROM");
}

String WiFiWebManager::getSavedSSID() const {
    return _savedSSID;
}

void WiFiWebManager::restart() {
    serialLog.println("Restarting ESP32...");
    delay(1000);
    ESP.restart();
}

// Private methods

void WiFiWebManager::loadCredentials() {
    // A read-write open creates the namespace after flash erase/first boot.
    // Read-only would log ESP_ERR_NVS_NOT_FOUND for a valid empty NVS.
    if (!_preferences.begin("wifi", false)) {
        _wifiConfigured = false;
        _savedSSID = "";
        _savedPassword = "";
        serialLog.println("NVS indisponibil; nu pot incarca setarile WiFi");
        return;
    }
    _wifiConfigured = _preferences.getBool("configured", false);
    
    if (_wifiConfigured) {
        _savedSSID = _preferences.getString("ssid", "");
        _savedPassword = _preferences.getString("password", "");
        serialLog.println("WiFi credentials loaded from EEPROM");
        serialLog.println("SSID: " + _savedSSID);
    } else {
        serialLog.println("No saved WiFi credentials");
    }
    
    _preferences.end();
}

bool WiFiWebManager::connectToWiFi() {
    if (!_wifiConfigured || _savedSSID.length() == 0) {
        return false;
    }
    
    serialLog.println("Attempting to connect to WiFi: " + _savedSSID);
    WiFi.mode(WIFI_STA);
    WiFi.begin(_savedSSID.c_str(), _savedPassword.c_str());
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        serialLog.print(".");
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        _wifiConnected = true;
        serialLog.println("\n✅ Connected to WiFi!");
        serialLog.print("IP Address: ");
        serialLog.println(WiFi.localIP());
        return true;
    } else {
        serialLog.println("\n❌ Could not connect to WiFi");
        return false;
    }
}

void WiFiWebManager::startAccessPoint() {
    serialLog.println("Starting Access Point...");
    WiFi.mode(WIFI_AP);
    WiFi.softAP(_apSSID.c_str(), _apPassword.c_str());
    
    IPAddress IP = WiFi.softAPIP();
    serialLog.print("Access Point created! SSID: ");
    serialLog.println(_apSSID);
    serialLog.print("Password: ");
    serialLog.println(_apPassword);
    serialLog.print("IP Address: ");
    serialLog.println(IP);
}

void WiFiWebManager::setupDefaultRoutes() {
    // WiFi status endpoint
    _server->on("/wifi_status", HTTP_GET, [this]() {
        handleWiFiStatus();
    });
    
    // WiFi configuration endpoint
    _server->on("/wifi_config", HTTP_POST, [this]() {
        handleWiFiConfig();
    });
    
    // Clear WiFi credentials endpoint
    _server->on("/wifi_clear", HTTP_GET, [this]() {
        handleWiFiClear();
    });
}

void WiFiWebManager::handleWiFiStatus() {
    String json = "{";
    json += "\"connected\":" + String(_wifiConnected ? "true" : "false") + ",";
    json += "\"ssid\":\"" + getSSID() + "\",";
    json += "\"ip\":\"" + getIPAddress() + "\",";
    json += "\"saved_ssid\":\"" + _savedSSID + "\"";
    json += "}";
    
    _server->send(200, "application/json", json);
}

void WiFiWebManager::handleWiFiConfig() {
    if (_server->method() != HTTP_POST) {
        _server->send(405, "text/plain", "Method Not Allowed");
        return;
    }
    
    String ssid = _server->arg("ssid");
    String password = _server->arg("password");
    
    if (ssid.length() == 0) {
        _server->send(200, "application/json", 
            "{\"success\":false,\"message\":\"Invalid SSID!\"}");
        return;
    }
    
    // Save credentials
    saveCredentials(ssid, password);
    
    _server->send(200, "application/json", 
        "{\"success\":true,\"message\":\"Configuration saved! ESP32 will restart...\"}");
    
    // Restart after 3 seconds
    delay(3000);
    restart();
}

void WiFiWebManager::handleWiFiClear() {
    clearCredentials();
    _server->send(200, "application/json", 
        "{\"success\":true,\"message\":\"WiFi configuration cleared!\"}");
}

String WiFiWebManager::getWiFiConfigHTML() {
    // This method can be used to generate a basic WiFi config page
    // For now, it's a placeholder - the main app can provide its own HTML
    return "";
}

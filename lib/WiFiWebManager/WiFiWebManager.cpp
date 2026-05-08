#include "WiFiWebManager.h"

WiFiWebManager::WiFiWebManager(const char* apSSID, const char* apPassword, int serverPort)
    : _apSSID(apSSID), _apPassword(apPassword), _serverPort(serverPort),
      _wifiConfigured(false), _wifiConnected(false), _isAPMode(false) {
    _server = new WebServer(_serverPort);
}

bool WiFiWebManager::begin() {
    Serial.println("\n=== WiFi Web Manager Initialization ===");
    
    // Load saved credentials
    loadCredentials();
    
    // Try to connect to WiFi if configured
    if (_wifiConfigured && connectToWiFi()) {
        _isAPMode = false;
        Serial.println("✅ Mode: WiFi Client");
        Serial.println("Open in browser: http://" + WiFi.localIP().toString());
    } else {
        // Start Access Point mode
        startAccessPoint();
        _isAPMode = true;
        Serial.println("📡 Mode: Access Point");
        Serial.println("Open in browser: http://" + WiFi.softAPIP().toString());
    }
    
    // Setup default routes for WiFi management
    setupDefaultRoutes();
    
    // Start web server
    _server->begin();
    Serial.println("✅ Web server started!");
    Serial.println("=======================================\n");
    
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
    _preferences.begin("wifi", false);
    _preferences.putString("ssid", ssid);
    _preferences.putString("password", password);
    _preferences.putBool("configured", true);
    _preferences.end();
    
    _savedSSID = ssid;
    _savedPassword = password;
    _wifiConfigured = true;
    
    Serial.println("WiFi credentials saved to EEPROM");
}

void WiFiWebManager::clearCredentials() {
    _preferences.begin("wifi", false);
    _preferences.clear();
    _preferences.end();
    
    _wifiConfigured = false;
    _wifiConnected = false;
    _savedSSID = "";
    _savedPassword = "";
    
    Serial.println("WiFi credentials cleared from EEPROM");
}

String WiFiWebManager::getSavedSSID() const {
    return _savedSSID;
}

void WiFiWebManager::restart() {
    Serial.println("Restarting ESP32...");
    delay(1000);
    ESP.restart();
}

// Private methods

void WiFiWebManager::loadCredentials() {
    _preferences.begin("wifi", true);
    _wifiConfigured = _preferences.getBool("configured", false);
    
    if (_wifiConfigured) {
        _savedSSID = _preferences.getString("ssid", "");
        _savedPassword = _preferences.getString("password", "");
        Serial.println("WiFi credentials loaded from EEPROM");
        Serial.println("SSID: " + _savedSSID);
    } else {
        Serial.println("No saved WiFi credentials");
    }
    
    _preferences.end();
}

bool WiFiWebManager::connectToWiFi() {
    if (!_wifiConfigured || _savedSSID.length() == 0) {
        return false;
    }
    
    Serial.println("Attempting to connect to WiFi: " + _savedSSID);
    WiFi.mode(WIFI_STA);
    WiFi.begin(_savedSSID.c_str(), _savedPassword.c_str());
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        Serial.print(".");
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        _wifiConnected = true;
        Serial.println("\n✅ Connected to WiFi!");
        Serial.print("IP Address: ");
        Serial.println(WiFi.localIP());
        return true;
    } else {
        Serial.println("\n❌ Could not connect to WiFi");
        return false;
    }
}

void WiFiWebManager::startAccessPoint() {
    Serial.println("Starting Access Point...");
    WiFi.mode(WIFI_AP);
    WiFi.softAP(_apSSID.c_str(), _apPassword.c_str());
    
    IPAddress IP = WiFi.softAPIP();
    Serial.print("Access Point created! SSID: ");
    Serial.println(_apSSID);
    Serial.print("Password: ");
    Serial.println(_apPassword);
    Serial.print("IP Address: ");
    Serial.println(IP);
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

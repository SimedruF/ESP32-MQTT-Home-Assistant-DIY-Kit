#ifndef WIFIWEBMANAGER_H
#define WIFIWEBMANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <functional>

/**
 * @brief WiFi and WebServer Manager Class
 * 
 * Manages WiFi connectivity (both AP and Client modes) and web server functionality.
 * Supports persistent WiFi credentials storage in EEPROM and custom route handlers.
 */
class WiFiWebManager {
public:
    /**
     * @brief Callback function type for custom route handlers
     * @param server Reference to the WebServer instance
     */
    typedef std::function<void(WebServer&)> RouteCallback;
    
    /**
     * @brief Constructor
     * @param apSSID Access Point SSID (default: "ESP32_AP")
     * @param apPassword Access Point password (default: "12345678")
     * @param serverPort Web server port (default: 80)
     */
    WiFiWebManager(const char* apSSID = "ESP32_AP", 
                   const char* apPassword = "12345678",
                   int serverPort = 80);
    
    /**
     * @brief Initialize WiFi and start web server
     * @return true if initialization successful
     */
    bool begin();
    
    /**
     * @brief Handle client requests (call in loop())
     */
    void handleClient();
    
    /**
     * @brief Register a custom route handler
     * @param uri The URI path (e.g., "/data")
     * @param method HTTP method (HTTP_GET, HTTP_POST, etc.)
     * @param handler Callback function for the route
     */
    void on(const String& uri, HTTPMethod method, RouteCallback handler);
    
    /**
     * @brief Check if WiFi is connected (as client)
     * @return true if connected to WiFi network
     */
    bool isConnected() const;
    
    /**
     * @brief Get current IP address
     * @return IP address as String
     */
    String getIPAddress() const;
    
    /**
     * @brief Get current SSID
     * @return SSID as String
     */
    String getSSID() const;
    
    /**
     * @brief Check if in Access Point mode
     * @return true if in AP mode
     */
    bool isAPMode() const;
    
    /**
     * @brief Get reference to WebServer for direct access
     * @return Reference to WebServer instance
     */
    WebServer& getServer();
    
    /**
     * @brief Save WiFi credentials to EEPROM
     * @param ssid Network SSID
     * @param password Network password
     */
    void saveCredentials(const String& ssid, const String& password);
    
    /**
     * @brief Clear saved WiFi credentials
     */
    void clearCredentials();
    
    /**
     * @brief Get saved SSID from EEPROM
     * @return Saved SSID or empty string
     */
    String getSavedSSID() const;
    
    /**
     * @brief Restart ESP32
     */
    void restart();
    
private:
    // WiFi settings
    String _apSSID;
    String _apPassword;
    String _savedSSID;
    String _savedPassword;
    bool _wifiConfigured;
    bool _wifiConnected;
    bool _isAPMode;
    
    // Web server
    WebServer* _server;
    int _serverPort;
    
    // Preferences for EEPROM
    Preferences _preferences;
    
    // Internal methods
    void loadCredentials();
    bool connectToWiFi();
    void startAccessPoint();
    void setupDefaultRoutes();
    
    // Default route handlers
    void handleWiFiStatus();
    void handleWiFiConfig();
    void handleWiFiClear();
    
    // HTML for WiFi configuration
    String getWiFiConfigHTML();
};

#endif // WIFIWEBMANAGER_H

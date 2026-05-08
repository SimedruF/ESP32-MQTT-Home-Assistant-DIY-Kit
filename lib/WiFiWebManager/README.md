# WiFiWebManager Library

A reusable WiFi and Web Server management library for ESP32 projects.

## 📋 Overview

`WiFiWebManager` is a C++ class that simplifies WiFi connectivity and web server management for ESP32 projects. It handles:

- **WiFi Client Mode**: Connect to existing WiFi networks
- **Access Point Mode**: Create a WiFi hotspot when client connection fails
- **Credential Management**: Store/retrieve WiFi credentials in EEPROM
- **Web Server**: Built-in web server with customizable routes
- **Default Routes**: Pre-configured routes for WiFi configuration

## 🚀 Features

- ✅ Automatic fallback to AP mode if WiFi connection fails
- ✅ Persistent WiFi credentials storage
- ✅ Easy-to-use callback system for custom routes
- ✅ RESTful API endpoints for WiFi management
- ✅ Zero-configuration setup with sensible defaults
- ✅ Direct access to WebServer for advanced use cases

## 📦 Installation

### Option 1: Copy to your project

1. Copy `WiFiWebManager.h` to your `include/` folder
2. Copy `WiFiWebManager.cpp` to your `src/` folder

### Option 2: Use as library

1. Create a folder structure:
   ```
   lib/
   └── WiFiWebManager/
       ├── WiFiWebManager.h
       └── WiFiWebManager.cpp
   ```

2. Copy the files into the `WiFiWebManager` folder

## 🔧 Usage

### Basic Example

```cpp
#include <Arduino.h>
#include "WiFiWebManager.h"

// Create WiFiWebManager instance
WiFiWebManager wifiManager("MyESP32_AP", "password123", 80);

void handleRoot(WebServer& server) {
    server.send(200, "text/html", "<h1>Hello from ESP32!</h1>");
}

void handleData(WebServer& server) {
    String json = "{\"message\":\"Hello World\"}";
    server.send(200, "application/json", json);
}

void setup() {
    Serial.begin(115200);
    
    // Initialize WiFi and Web Server
    wifiManager.begin();
    
    // Register custom routes
    wifiManager.on("/", HTTP_GET, handleRoot);
    wifiManager.on("/data", HTTP_GET, handleData);
    
    Serial.println("Setup complete!");
}

void loop() {
    // Handle web server requests
    wifiManager.handleClient();
    
    // Your application code here
    delay(10);
}
```

### Advanced Example with WiFi Management

```cpp
#include <Arduino.h>
#include "WiFiWebManager.h"

WiFiWebManager wifiManager;

void handleStatus(WebServer& server) {
    String json = "{";
    json += "\"connected\":" + String(wifiManager.isConnected() ? "true" : "false") + ",";
    json += "\"ip\":\"" + wifiManager.getIPAddress() + "\",";
    json += "\"ssid\":\"" + wifiManager.getSSID() + "\"";
    json += "}";
    
    server.send(200, "application/json", json);
}

void setup() {
    Serial.begin(115200);
    
    // Initialize with custom settings
    wifiManager.begin();
    
    // Register routes
    wifiManager.on("/", HTTP_GET, [](WebServer& server) {
        server.send(200, "text/plain", "ESP32 Server");
    });
    
    wifiManager.on("/status", HTTP_GET, handleStatus);
    
    // Check connection status
    if (wifiManager.isConnected()) {
        Serial.println("Connected to WiFi!");
        Serial.println("IP: " + wifiManager.getIPAddress());
    } else {
        Serial.println("Running in AP mode");
    }
}

void loop() {
    wifiManager.handleClient();
    delay(10);
}
```

## 🎛️ API Reference

### Constructor

```cpp
WiFiWebManager(const char* apSSID = "ESP32_AP", 
               const char* apPassword = "12345678",
               int serverPort = 80);
```

Creates a new WiFiWebManager instance.

**Parameters:**
- `apSSID`: Access Point SSID (default: "ESP32_AP")
- `apPassword`: Access Point password (default: "12345678")
- `serverPort`: Web server port (default: 80)

### Public Methods

#### `bool begin()`

Initialize WiFi and start the web server. Returns `true` if successful.

```cpp
wifiManager.begin();
```

#### `void handleClient()`

Handle incoming web server requests. **Must be called in `loop()`**.

```cpp
void loop() {
    wifiManager.handleClient();
}
```

#### `void on(const String& uri, HTTPMethod method, RouteCallback handler)`

Register a custom route handler.

```cpp
wifiManager.on("/sensor", HTTP_GET, [](WebServer& server) {
    int value = analogRead(A0);
    server.send(200, "text/plain", String(value));
});
```

**Supported HTTP Methods:**
- `HTTP_GET`
- `HTTP_POST`
- `HTTP_PUT`
- `HTTP_DELETE`
- `HTTP_PATCH`
- etc.

#### `bool isConnected()`

Check if connected to WiFi as a client.

```cpp
if (wifiManager.isConnected()) {
    Serial.println("WiFi connected!");
}
```

#### `String getIPAddress()`

Get current IP address (either client IP or AP IP).

```cpp
Serial.println("IP: " + wifiManager.getIPAddress());
```

#### `String getSSID()`

Get current SSID (either connected network or AP SSID).

```cpp
Serial.println("SSID: " + wifiManager.getSSID());
```

#### `bool isAPMode()`

Check if running in Access Point mode.

```cpp
if (wifiManager.isAPMode()) {
    Serial.println("Running as Access Point");
}
```

#### `WebServer& getServer()`

Get direct access to the WebServer instance for advanced configuration.

```cpp
WebServer& server = wifiManager.getServer();
server.on("/custom", HTTP_POST, customHandler);
```

#### `void saveCredentials(const String& ssid, const String& password)`

Save WiFi credentials to EEPROM.

```cpp
wifiManager.saveCredentials("MyNetwork", "MyPassword");
```

#### `void clearCredentials()`

Clear saved WiFi credentials from EEPROM.

```cpp
wifiManager.clearCredentials();
```

#### `String getSavedSSID()`

Get the saved SSID from EEPROM.

```cpp
String ssid = wifiManager.getSavedSSID();
```

#### `void restart()`

Restart the ESP32.

```cpp
wifiManager.restart();
```

## 🌐 Default Routes

WiFiWebManager provides three default routes for WiFi management:

### `GET /wifi_status`

Returns WiFi connection status.

**Response:**
```json
{
    "connected": true,
    "ssid": "MyNetwork",
    "ip": "192.168.1.100",
    "saved_ssid": "MyNetwork"
}
```

### `POST /wifi_config`

Configure WiFi credentials.

**Request (form-encoded):**
```
ssid=MyNetwork&password=MyPassword
```

**Response:**
```json
{
    "success": true,
    "message": "Configuration saved! ESP32 will restart..."
}
```

### `GET /wifi_clear`

Clear saved WiFi credentials.

**Response:**
```json
{
    "success": true,
    "message": "WiFi configuration cleared!"
}
```

## 📝 Callback Function Signature

Route handlers must follow this signature:

```cpp
void handlerFunction(WebServer& server) {
    // Access server methods:
    // - server.send()
    // - server.arg()
    // - server.method()
    // - etc.
}
```

## 💡 Tips & Best Practices

### 1. Always call `handleClient()` in `loop()`

```cpp
void loop() {
    wifiManager.handleClient();  // Required!
    // Your code here
}
```

### 2. Use lambda functions for simple routes

```cpp
wifiManager.on("/hello", HTTP_GET, [](WebServer& server) {
    server.send(200, "text/plain", "Hello World!");
});
```

### 3. Check connection status before WiFi operations

```cpp
if (wifiManager.isConnected()) {
    // Perform internet-dependent operations
}
```

### 4. Access control for sensitive routes

```cpp
wifiManager.on("/admin", HTTP_GET, [](WebServer& server) {
    if (!authenticateUser(server)) {
        server.send(401, "text/plain", "Unauthorized");
        return;
    }
    server.send(200, "text/html", adminPage);
});
```

## 🔒 Security Considerations

1. **Change default AP password**: Always use a strong password for production
2. **Implement authentication**: Add authentication for sensitive routes
3. **Use HTTPS**: Consider using HTTPS ESP32 libraries for secure communication
4. **Validate input**: Always validate and sanitize user input

## 🐛 Troubleshooting

### WiFi not connecting

- Check SSID and password are correct
- Verify WiFi signal strength
- Ensure 2.4GHz WiFi (ESP32 doesn't support 5GHz)
- Check router settings (MAC filtering, etc.)

### Web server not responding

- Verify IP address with Serial Monitor
- Check firewall settings
- Ensure `handleClient()` is called in `loop()`
- Try accessing from different browser/device

### EEPROM issues

- Call `clearCredentials()` to reset
- Check available EEPROM space
- Verify Preferences library is available

## 📄 Dependencies

- `Arduino.h`
- `WiFi.h` (ESP32)
- `WebServer.h` (ESP32)
- `Preferences.h` (ESP32)

All dependencies are included with ESP32 Arduino core.

## 📜 License

This library is open source and available under the MIT License.

## 🤝 Contributing

Contributions are welcome! Feel free to:
- Report bugs
- Suggest features
- Submit pull requests

## 👤 Author

Created as part of the ESP32_VibrationSensor project.

---

**Happy Coding! 🚀**

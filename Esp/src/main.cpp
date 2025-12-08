#include <WiFi.h>
#include <WebServer.h>
#include <WiFiManager.h>
#include <ArduinoJson.h>
#include <time.h>
#include <PubSubClient.h>
#include <EEPROM.h>
#include <SPIFFS.h>
#include <WiFiClientSecure.h>

// HiveMQ Cloud Configuration
#include "hivemq_config.h"
#include "hivemq_cert.h"



// ========== WIFI MANAGER ==========
WiFiManager wm;

// ========== EEPROM CONFIG ==========
#define EEPROM_SIZE 512
#define SSID_ADDR 0
#define PASS_ADDR 32
#define SETUP_FLAG_ADDR 64

// ========== HARDWARE CONFIG ==========
HardwareSerial UnoSerial(2);
const int RXD2 = 16;
const int TXD2 = 17;

// ========== SYSTEM STATE ==========
int soilMoisture = 0;
int rainStatus = 0;
bool pumpStatus = false;
bool autoMode = true;
int pumpSpeed = 50;
bool wifiConfigured = false;
bool setupCompleted = false;
bool mqttConnected = false;

// ========== NETWORK OBJECTS ==========
WebServer server(80);
WiFiClientSecure espClient;  // Changed to WiFiClientSecure for TLS
PubSubClient mqttClient(espClient);

// ========== GLOBAL VARIABLES ==========
const char* ntpServer = "pool.ntp.org";
const char* TZ_INFO = "Asia/Bangkok";
String eventLog = "";
String savedSSID = "";
String savedPassword = "";

// ========== PREVIOUS STATE TRACKING ==========
bool prevPumpStatus = false;
bool prevAutoMode = true;
int prevPumpSpeed = 50;

// ========== EVENT LOG DUPLICATE PREVENTION ==========
String lastLogMessage = "";

// ========== MEMORY PROTECTION ==========
const int MAX_LOG_LENGTH = 2000;  // Reduced from 3000 for better memory management
const int LOG_TRIM_LENGTH = 1500; // Trim when log exceeds this length
const int MIN_FREE_HEAP = 10000;  // Minimum free heap threshold (10KB)
const int MAX_STRING_LENGTH = 256; // Maximum string length for safety



// ========== FUNCTION PROTOTYPES ==========
void scanWiFi();
void connectToWiFi(String ssid, String password);

void setupWebServer();
void setupMQTT();
void handleRoot();
void handleWiFiConfig();
void handleWiFiScan();
void handleWiFiConnect();
void handleGetData();
void handleControlPump();
void handleSetMode();
void handleSetSpeed();
void addLog(String message);
void readUARTData();
void checkAutoWatering();
int getCurrentHour();
void mqttCallback(char* topic, byte* payload, unsigned int length);
void reconnectMQTT();
void publishData();
void publishPumpStatus(String reason = "");
void publishSystemStatus();
void publishLog(String logMessage);
void loadSetupFlag();
void saveSetupFlag(bool completed);
void handleResetSetup();



// ========== SETUP ==========
void setup() {
    Serial.begin(115200);
    UnoSerial.begin(9600, SERIAL_8N1, RXD2, TXD2);
    
    EEPROM.begin(EEPROM_SIZE);
    
    // Initialize SPIFFS
    if (!SPIFFS.begin(true)) {
        Serial.println("❌ SPIFFS Mount Failed");
        return;
    }
    Serial.println("✅ SPIFFS Mounted Successfully");
    
    Serial.println("\n🚀 ESP32 Smart Irrigation System Starting...");
    
    // Load saved WiFi credentials and setup flag
    loadWiFiCreds();
    loadSetupFlag();

    // WiFiManager auto-connect
    if (wm.autoConnect("SmartIrrigation_AP", "12345678")) {
        Serial.println("✅ Connected to WiFi using WiFiManager");
        Serial.println("🌐 Access web interface at: http://" + WiFi.localIP().toString());
        addLog("STA Mode: Connected to " + WiFi.SSID());
        addLog("IP: " + WiFi.localIP().toString());
        wifiConfigured = true;


    } else {
        Serial.println("❌ Failed to connect to WiFi, starting AP mode");
        Serial.println("🌐 Access captive portal at: http://" + WiFi.softAPIP().toString());
        addLog("AP Mode: SmartIrrigation_AP (12345678)");
    }
    
    // Initialize time
    configTzTime(TZ_INFO, ntpServer);
    
    // Setup web server
    setupWebServer();
    
    // Setup MQTT
    setupMQTT();
    
    Serial.println("✅ System initialized successfully");
}

// ========== MAIN LOOP ==========
void loop() {
    server.handleClient();
    readUARTData();

    if (WiFi.status() == WL_CONNECTED) {
        if (!mqttClient.connected()) {
            reconnectMQTT();
        }
        mqttClient.loop();

        // Publish sensor data every 5 seconds
        static unsigned long lastPublish = 0;
        if (millis() - lastPublish > SENSOR_PUBLISH_INTERVAL) {
            publishData();
            lastPublish = millis();
        }

        // Publish system status every 30 seconds
        static unsigned long lastStatusPublish = 0;
        if (millis() - lastStatusPublish > STATUS_PUBLISH_INTERVAL) {
            publishSystemStatus();
            lastStatusPublish = millis();
        }

        if (autoMode) {
            checkAutoWatering();
        }
    }

    // Removed delay(100) for real-time updates
}

// ========== WIFI FUNCTIONS ==========
void scanWiFi() {
    Serial.println("📡 Scanning for WiFi networks...");
    int count = WiFi.scanNetworks();
    
    if (count == 0) {
        Serial.println("❌ No networks found");
    } else {
        Serial.print("✅ Found ");
        Serial.print(count);
        Serial.println(" networks:");
        
        for (int i = 0; i < count && i < 20; i++) {
            Serial.print("  ");
            Serial.print(i + 1);
            Serial.print(": ");
            Serial.println(WiFi.SSID(i));
        }
    }
}

void connectToWiFi(String ssid, String password) {
    WiFi.begin(ssid.c_str(), password.c_str());
    
    Serial.print("🔗 Connecting to: ");
    Serial.println(ssid);
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 30) {
        delay(500);
        Serial.print(".");
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\n✅ Connected successfully!");
        saveWiFiCreds(ssid, password);
        wifiConfigured = true;
        addLog("Connected to WiFi: " + ssid);
    } else {
        Serial.println("\n❌ Connection failed!");
        addLog("Failed to connect: " + ssid);
    }
}

void saveWiFiCreds(String ssid, String password) {
    // Clear EEPROM first
    for (int i = 0; i < 64; i++) {
        EEPROM.write(SSID_ADDR + i, 0);
        EEPROM.write(PASS_ADDR + i, 0);
    }
    
    // Save SSID
    for (int i = 0; i < ssid.length() && i < 32; i++) {
        EEPROM.write(SSID_ADDR + i, ssid[i]);
    }
    EEPROM.write(SSID_ADDR + ssid.length(), '\0');
    
    // Save Password
    for (int i = 0; i < password.length() && i < 32; i++) {
        EEPROM.write(PASS_ADDR + i, password[i]);
    }
    EEPROM.write(PASS_ADDR + password.length(), '\0');
    
    EEPROM.commit();
    Serial.println("💾 WiFi credentials saved to EEPROM");
}

void loadWiFiCreds() {
    String ssid = "";
    String password = "";
    
    // Load SSID
    for (int i = 0; i < 32; i++) {
        char c = EEPROM.read(SSID_ADDR + i);
        if (c == '\0') break;
        if (c > 0) ssid += c;
    }
    
    // Load Password
    for (int i = 0; i < 32; i++) {
        char c = EEPROM.read(PASS_ADDR + i);
        if (c == '\0') break;
        if (c > 0) password += c;
    }
    
    if (ssid.length() > 0) {
        savedSSID = ssid;
        savedPassword = password;
        wifiConfigured = true;
        Serial.println("📖 Loaded WiFi credentials from EEPROM");
        Serial.print("SSID: ");
        Serial.println(savedSSID);
    } else {
        Serial.println("📭 No saved WiFi credentials found");
    }
}

// ========== WEB SERVER HANDLERS ==========
void setupWebServer() {
    server.on("/", HTTP_GET, handleRoot);
    server.on("/style.css", HTTP_GET, []() {
        File file = SPIFFS.open("/style.css", "r");
        if (file) {
            server.streamFile(file, "text/css");
            file.close();
        } else {
            server.send(404, "text/plain", "CSS file not found");
        }
    });
    server.on("/script.js", HTTP_GET, []() {
        File file = SPIFFS.open("/script.js", "r");
        if (file) {
            server.streamFile(file, "application/javascript");
            file.close();
        } else {
            server.send(404, "text/plain", "JS file not found");
        }
    });
    server.on("/wifi-config", HTTP_GET, handleWiFiConfig);
    server.on("/wifi-scan", HTTP_GET, handleWiFiScan);
    server.on("/wifi-connect", HTTP_POST, handleWiFiConnect);
    server.on("/getData", HTTP_GET, handleGetData);
    server.on("/controlPump", HTTP_GET, handleControlPump);
    server.on("/setMode", HTTP_GET, handleSetMode);
    server.on("/setSpeed", HTTP_GET, handleSetSpeed);
    server.on("/reset-setup", HTTP_GET, handleResetSetup);
    server.onNotFound([]() {
        server.send(404, "text/plain", "404: Not Found");
    });
    
    server.begin();
    Serial.println("🌐 Web server started on port 80");
}

void handleRoot() {
    File file = SPIFFS.open("/index.html", "r");
    if (!file) {
        server.send(404, "text/plain", "index.html not found");
        return;
    }
    
    server.streamFile(file, "text/html");
    file.close();
}

void handleWiFiConfig() {
    // Serve WiFi configuration page from SPIFFS
    File file = SPIFFS.open("/wifi-config.html", "r");
    if (!file) {
        server.send(404, "text/plain", "wifi-config.html not found");
        return;
    }
    
    server.streamFile(file, "text/html");
    file.close();
}

void handleWiFiScan() {
    int count = WiFi.scanNetworks();
    String json = "[";
    for (int i = 0; i < count && i < 20; i++) {
        if (i > 0) json += ",";
        json += "\"" + WiFi.SSID(i) + "\"";
    }
    json += "]";
    server.send(200, "application/json", json);
}

void handleWiFiConnect() {
    if (server.hasArg("ssid") && server.hasArg("password")) {
        String ssid = server.arg("ssid");
        String password = server.arg("password");

        connectToWiFi(ssid, password);

        if (WiFi.status() == WL_CONNECTED) {
            server.send(200, "application/json", "{\"status\":\"success\",\"ip\":\"" + WiFi.localIP().toString() + "\",\"ssid\":\"" + ssid + "\"}");
        } else {
            server.send(200, "application/json", "{\"status\":\"failed\"}");
        }
    } else {
        server.send(400, "text/plain", "Missing parameters");
    }
}

void handleGetData() {
    StaticJsonDocument<200> doc;
    doc["soil"] = soilMoisture;
    doc["rain"] = rainStatus;
    doc["pump"] = pumpStatus;
    doc["autoMode"] = autoMode;
    doc["speed"] = pumpSpeed;
    
    String json;
    serializeJson(doc, json);
    server.send(200, "application/json", json);
}

void handleControlPump() {
    if (server.hasArg("state")) {
        String state = server.arg("state");
        bool newPumpStatus = (state == "on");

        if (newPumpStatus != prevPumpStatus) {
            if (newPumpStatus) {
                UnoSerial.println("PUMP_ON");
                pumpStatus = true;
                addLog("Manual: Pump ON");
                publishPumpStatus("Manual control: ON");
            } else {
                UnoSerial.println("PUMP_OFF");
                pumpStatus = false;
                addLog("Manual: Pump OFF");
                publishPumpStatus("Manual control: OFF");
            }
            prevPumpStatus = newPumpStatus;
        }
        server.send(200, "text/plain", "OK");
    } else {
        server.send(400, "text/plain", "Missing state parameter");
    }
}

void handleSetMode() {
    if (server.hasArg("mode")) {
        autoMode = (server.arg("mode") == "auto");
        addLog("Mode: " + String(autoMode ? "Auto" : "Manual"));
        server.send(200, "text/plain", "OK");
    } else {
        server.send(400, "text/plain", "Missing mode parameter");
    }
}

void handleSetSpeed() {
    if (server.hasArg("speed")) {
        pumpSpeed = server.arg("speed").toInt();
        server.send(200, "text/plain", "OK");
    } else {
        server.send(400, "text/plain", "Missing speed parameter");
    }
}

// ========== MQTT FUNCTIONS ==========
void setupMQTT() {
    Serial.println("📡 Initializing MQTT with HiveMQ Cloud...");
    
    // Configure WiFiClientSecure with certificate
    espClient.setCACert(hivemq_root_ca);
    Serial.println("✅ Certificate loaded");
    
    // Set MQTT server
    mqttClient.setServer(HIVEMQ_HOST, HIVEMQ_PORT);
    mqttClient.setCallback(mqttCallback);
    mqttClient.setBufferSize(MQTT_BUFFER_SIZE);
    mqttClient.setKeepAlive(MQTT_KEEPALIVE);
    
    Serial.printf("🔐 MQTT Broker: %s:%d\n", HIVEMQ_HOST, HIVEMQ_PORT);
    Serial.printf("👤 Client ID: %s\n", MQTT_CLIENT_ID);
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
    // Convert payload to string
    char message[length + 1];
    memcpy(message, payload, length);
    message[length] = '\0';
    
    String topicStr = String(topic);
    String messageStr = String(message);
    
    #if MQTT_DEBUG
    Serial.printf("📥 MQTT Message received on [%s]: %s\n", topic, message);
    #endif
    
    // Parse JSON message
    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, message);
    
    if (error) {
        Serial.printf("❌ JSON parse error: %s\n", error.c_str());
        return;
    }
    
    // Handle pump control
    if (topicStr == TOPIC_PUMP_CONTROL) {
        String command = doc["command"] | "";
        
        if (command == "turn_on") {
            UnoSerial.println("PUMP_ON");
            pumpStatus = true;
            
            if (doc.containsKey("speed")) {
                pumpSpeed = doc["speed"];
            }
            
            addLog("MQTT: Pump ON (Speed: " + String(pumpSpeed) + "%)");
        } 
        else if (command == "turn_off") {
            UnoSerial.println("PUMP_OFF");
            pumpStatus = false;
            addLog("MQTT: Pump OFF");
        }
    }
    
    // Handle mode control
    else if (topicStr == TOPIC_MODE_CONTROL) {
        String mode = doc["mode"] | "";
        
        if (mode == "AUTO") {
            autoMode = true;
            addLog("MQTT: Mode changed to AUTO");
        } 
        else if (mode == "MANUAL") {
            autoMode = false;
            if (doc.containsKey("speed")) {
                pumpSpeed = doc["speed"];
            }
            addLog("MQTT: Mode changed to MANUAL");
        }
    }
    
    // Handle config update
    else if (topicStr == TOPIC_CONFIG) {
        // Reserved for future configuration updates
        Serial.println("📝 Config update received");
    }
}

void reconnectMQTT() {
    static unsigned long lastAttempt = 0;
    
    // Don't retry too quickly
    if (millis() - lastAttempt < MQTT_RECONNECT_DELAY) {
        return;
    }
    
    lastAttempt = millis();
    
    if (!mqttClient.connected()) {
        Serial.printf("🔄 Attempting MQTT connection to %s...\n", HIVEMQ_HOST);
        
        // Attempt to connect
        if (mqttClient.connect(MQTT_CLIENT_ID, MQTT_USERNAME, MQTT_PASSWORD)) {
            Serial.println("✅ MQTT Connected!");
            mqttConnected = true;
            addLog("MQTT: Connected to HiveMQ Cloud");
            
            // Subscribe to control topics
            mqttClient.subscribe(TOPIC_PUMP_CONTROL, MQTT_QOS);
            Serial.printf("📤 Subscribed to: %s\n", TOPIC_PUMP_CONTROL);
            
            mqttClient.subscribe(TOPIC_MODE_CONTROL, MQTT_QOS);
            Serial.printf("📤 Subscribed to: %s\n", TOPIC_MODE_CONTROL);
            
            mqttClient.subscribe(TOPIC_CONFIG, MQTT_QOS);
            Serial.printf("📤 Subscribed to: %s\n", TOPIC_CONFIG);
            
            // Publish initial status
            publishSystemStatus();
        } else {
            mqttConnected = false;
            int state = mqttClient.state();
            Serial.printf("❌ MQTT Connection failed, rc=%d\n", state);
            
            // Error codes:
            // -4 : MQTT_CONNECTION_TIMEOUT
            // -3 : MQTT_CONNECTION_LOST
            // -2 : MQTT_CONNECT_FAILED
            // -1 : MQTT_DISCONNECTED
            //  1 : MQTT_CONNECT_BAD_PROTOCOL
            //  2 : MQTT_CONNECT_BAD_CLIENT_ID
            //  3 : MQTT_CONNECT_UNAVAILABLE
            //  4 : MQTT_CONNECT_BAD_CREDENTIALS
            //  5 : MQTT_CONNECT_UNAUTHORIZED
            
            switch(state) {
                case -4:
                    Serial.println("   → Timeout connecting to server");
                    break;
                case -2:
                    Serial.println("   → Connect failed");
                    break;
                case 4:
                    Serial.println("   → Bad credentials (check username/password)");
                    break;
                case 5:
                    Serial.println("   → Unauthorized");
                    break;
                default:
                    Serial.printf("   → Unknown error: %d\n", state);
            }
        }
    }
}

void publishData() {
    if (!mqttClient.connected()) {
        return;
    }
    
    // Create JSON document
    StaticJsonDocument<256> doc;
    doc["timestamp"] = time(nullptr);
    doc["soil_moisture"] = soilMoisture;
    doc["rain_status"] = rainStatus;
    doc["pump_status"] = pumpStatus;
    doc["auto_mode"] = autoMode;
    doc["pump_speed"] = pumpSpeed;
    
    // Serialize to string
    String payload;
    serializeJson(doc, payload);
    
    // Publish to HiveMQ Cloud
    bool published = mqttClient.publish(TOPIC_SENSOR_DATA, payload.c_str(), MQTT_RETAIN);
    
    #if MQTT_DEBUG
    if (published) {
        Serial.printf("📊 Published sensor data: %s\n", payload.c_str());
    } else {
        Serial.println("❌ Failed to publish sensor data");
    }
    #endif
}

void publishPumpStatus(String reason = "") {
    if (!mqttClient.connected()) {
        return;
    }
    
    StaticJsonDocument<256> doc;
    doc["timestamp"] = time(nullptr);
    doc["status"] = pumpStatus ? "ON" : "OFF";
    doc["speed"] = pumpSpeed;
    doc["mode"] = autoMode ? "AUTO" : "MANUAL";
    if (reason.length() > 0) {
        doc["reason"] = reason;
    }
    
    String payload;
    serializeJson(doc, payload);
    
    mqttClient.publish(TOPIC_PUMP_STATUS, payload.c_str(), true); // Retain = true
    
    #if MQTT_DEBUG
    Serial.printf("📊 Published pump status: %s\n", payload.c_str());
    #endif
}

void publishSystemStatus() {
    if (!mqttClient.connected()) {
        return;
    }
    
    StaticJsonDocument<512> doc;
    doc["timestamp"] = time(nullptr);
    doc["client_id"] = MQTT_CLIENT_ID;
    doc["uptime"] = millis() / 1000; // seconds
    doc["free_heap"] = ESP.getFreeHeap();
    doc["wifi_rssi"] = WiFi.RSSI();
    doc["wifi_ssid"] = WiFi.SSID();
    doc["ip_address"] = WiFi.localIP().toString();
    
    String payload;
    serializeJson(doc, payload);
    
    mqttClient.publish(TOPIC_SYSTEM_STATUS, payload.c_str());
    
    #if MQTT_DEBUG
    Serial.printf("📊 Published system status: %s\n", payload.c_str());
    #endif
}

void publishLog(String logMessage) {
    if (!mqttClient.connected()) {
        return;
    }
    
    StaticJsonDocument<256> doc;
    doc["timestamp"] = time(nullptr);
    doc["message"] = logMessage;
    doc["level"] = "INFO";
    
    String payload;
    serializeJson(doc, payload);
    
    mqttClient.publish(TOPIC_SYSTEM_LOG, payload.c_str());
}

// ========== HELPER FUNCTIONS ==========
void addLog(String message) {
    // Prevent duplicate consecutive prints
    if (message == lastLogMessage) {
        return;  // Skip adding duplicate message
    }
    lastLogMessage = message;  // Update last message

    time_t now = time(nullptr);
    struct tm timeinfo;

    // Reserve memory for eventLog to prevent fragmentation
    if (eventLog.length() == 0) {
        eventLog.reserve(MAX_LOG_LENGTH + 100);  // Reserve extra space
    }

    if (localtime_r(&now, &timeinfo)) {
        char timeStr[20];
        strftime(timeStr, sizeof(timeStr), "%H:%M:%S", &timeinfo);
        eventLog = "[" + String(timeStr) + "] " + message + "<br>" + eventLog;
    } else {
        eventLog = "[--:--:--] " + message + "<br>" + eventLog;
    }

    // More aggressive log trimming for memory protection
    if (eventLog.length() > LOG_TRIM_LENGTH) {
        int lastIndex = eventLog.lastIndexOf("<br>", LOG_TRIM_LENGTH - 500);
        if (lastIndex != -1) {
            eventLog = eventLog.substring(0, lastIndex);
        }
    }

    // Monitor memory usage
    Serial.printf("📝 %s | Log size: %d/%d bytes | Free heap: %d bytes\n",
                  message.c_str(), eventLog.length(), MAX_LOG_LENGTH, ESP.getFreeHeap());
}

void readUARTData() {
    while (UnoSerial.available()) {
        String data = UnoSerial.readStringUntil('\n');
        data.trim();
        
        if (data.startsWith("{")) {
            StaticJsonDocument<256> doc;
            DeserializationError error = deserializeJson(doc, data);
            
            if (!error) {
                if (doc.containsKey("soil_moisture")) {
                    soilMoisture = doc["soil_moisture"];
                }
                if (doc.containsKey("rain")) {
                    rainStatus = doc["rain"];
                }
                if (doc.containsKey("pump_status")) {
                    pumpStatus = doc["pump_status"];
                }
            }
        } else if (data == "PUMP_ON_ACK") {
            pumpStatus = true;
            addLog("Pump ON acknowledged");
        } else if (data == "PUMP_OFF_ACK") {
            pumpStatus = false;
            addLog("Pump OFF acknowledged");
        }
    }
}

int getCurrentHour() {
    time_t now = time(nullptr);
    struct tm timeinfo;
    
    if (localtime_r(&now, &timeinfo)) {
        return timeinfo.tm_hour;
    }
    return -1;
}

void checkAutoWatering() {
    static unsigned long lastCheck = 0;
    if (millis() - lastCheck < 30000) return;
    lastCheck = millis();
    
    int currentHour = getCurrentHour();
    
    if (currentHour == -1) return;
    
    // Auto watering logic
    if ((currentHour == 6 || currentHour == 17) && rainStatus == 0 && soilMoisture < 40) {
        if (!pumpStatus) {
            UnoSerial.println("PUMP_ON");
            pumpStatus = true;
            String reason = "Auto: Hour " + String(currentHour) + ", Soil " + String(soilMoisture) + "%";
            addLog(reason);
            publishPumpStatus(reason);
        }
    } else {
        if (pumpStatus) {
            UnoSerial.println("PUMP_OFF");
            pumpStatus = false;
            addLog("Auto: Pump OFF");
            publishPumpStatus("Auto: conditions not met");
        }
    }
}

void loadSetupFlag() {
    setupCompleted = (EEPROM.read(SETUP_FLAG_ADDR) == 1);
    Serial.printf("📖 Loaded setup flag: %s\n", setupCompleted ? "COMPLETED" : "NOT COMPLETED");
}

void saveSetupFlag(bool completed) {
    EEPROM.write(SETUP_FLAG_ADDR, completed ? 1 : 0);
    EEPROM.commit();
    setupCompleted = completed;
    Serial.printf("💾 Setup flag saved: %s\n", completed ? "COMPLETED" : "NOT COMPLETED");
}

void handleResetSetup() {
    // Reset WiFi credentials
    for (int i = 0; i < 64; i++) {
        EEPROM.write(SSID_ADDR + i, 0);
        EEPROM.write(PASS_ADDR + i, 0);
    }
    EEPROM.write(SETUP_FLAG_ADDR, 0);
    EEPROM.commit();

    // Reset variables
    savedSSID = "";
    savedPassword = "";
    wifiConfigured = false;
    setupCompleted = false;

    Serial.println("🔄 Setup reset completed");
    addLog("Setup configuration reset");
}

// Note: HTML/CSS/JS content removed - now served from SPIFFS files
// The web interface files (index.html, style.css, script.js) are located in the data/ directory
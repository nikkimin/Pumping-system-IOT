#include <WiFi.h>
#include <WebServer.h>
#include <WiFiManager.h>
#include <ArduinoJson.h>
#include <time.h>
#include <PubSubClient.h>
#include <EEPROM.h>

// ========== CẤU HÌNH BLYNK MQTT ==========
#define BLYNK_TEMPLATE_ID "TMPL6vBXbZ3DJ"
#define BLYNK_TEMPLATE_NAME "Smart Pumping system"
#define BLYNK_AUTH_TOKEN "09AjF1sQoCmf3KEJKHswN219at_noSyN"
#define VIRTUAL_PIN_SOIL 0
#define VIRTUAL_PIN_RAIN 1
#define VIRTUAL_PIN_PUMP 2
#define VIRTUAL_PIN_MODE 3
#define VIRTUAL_PIN_SPEED 4
#define VIRTUAL_PIN_STATE 5

#include <BlynkSimpleEsp32.h>

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
WiFiClient espClient;
PubSubClient mqttClient(espClient);

// ========== GLOBAL VARIABLES ==========
const char* mqttServer = "mqtt.blynk.cc";
const int mqttPort = 1883;
const char* ntpServer = "pool.ntp.org";
const char* TZ_INFO = "Asia/Bangkok";
String eventLog = "";
String availableWifi[20];
int wifiCount = 0;
String savedSSID = "";
String savedPassword = "";

// ========== PREVIOUS STATE TRACKING ==========
bool prevPumpStatus = false;
bool prevAutoMode = true;
int prevPumpSpeed = 50;

// ========== EVENT LOG DUPLICATE PREVENTION ==========
String lastLogMessage = "";

// ========== MEMORY PROTECTION ==========
const int MAX_LOG_LENGTH = 2000;
const int LOG_TRIM_LENGTH = 1500;
const int MIN_FREE_HEAP = 10000;
const int MAX_STRING_LENGTH = 256;

// ========== HTML INTERFACE FILES ==========
#include "web_interface.h"
#include "wifi_config.h"

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
String getWiFiListHTML();
void loadSetupFlag();
void saveSetupFlag(bool completed);
void handleResetSetup();

// ========== BLYNK FUNCTIONS ==========
BLYNK_WRITE(VIRTUAL_PIN_PUMP) {
    int pinValue = param.asInt();
    if (pinValue == 1) {
        UnoSerial.println("PUMP_ON");
        pumpStatus = true;
        addLog("Blynk: Pump ON");
    } else {
        UnoSerial.println("PUMP_OFF");
        pumpStatus = false;
        addLog("Blynk: Pump OFF");
    }
}

BLYNK_WRITE(VIRTUAL_PIN_MODE) {
    int pinValue = param.asInt();
    autoMode = (pinValue == 1);
    addLog("Blynk: Mode " + String(autoMode ? "Auto" : "Manual"));
}

BLYNK_WRITE(VIRTUAL_PIN_SPEED) {
    int pinValue = param.asInt();
    pumpSpeed = pinValue;
    addLog("Blynk: Speed " + String(pumpSpeed) + "%");
}

// ========== SETUP ==========
void setup() {
    Serial.begin(115200);
    UnoSerial.begin(9600, SERIAL_8N1, RXD2, TXD2);
    
    EEPROM.begin(EEPROM_SIZE);
    
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

        // Initialize Blynk after WiFi connection
        Blynk.config(BLYNK_AUTH_TOKEN);
        Blynk.connect();
        Serial.println("✅ Blynk initialized");
        addLog("Blynk initialized");
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

        // Run Blynk
        Blynk.run();

        static unsigned long lastPublish = 0;
        if (millis() - lastPublish > 5000) {
            publishData();
            lastPublish = millis();
        }

        if (autoMode) {
            checkAutoWatering();
        }
    }
}

// ========== WIFI FUNCTIONS ==========
void scanWiFi() {
    Serial.println("📡 Scanning for WiFi networks...");
    wifiCount = WiFi.scanNetworks();
    
    if (wifiCount == 0) {
        Serial.println("❌ No networks found");
    } else {
        Serial.print("✅ Found ");
        Serial.print(wifiCount);
        Serial.println(" networks:");
        
        for (int i = 0; i < wifiCount && i < 20; i++) {
            availableWifi[i] = WiFi.SSID(i);
            Serial.print("  ");
            Serial.print(i + 1);
            Serial.print(": ");
            Serial.println(availableWifi[i]);
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
    String html = String(htmlContent);
    
    // Replace placeholders with actual values
    if (WiFi.status() == WL_CONNECTED) {
        html.replace("%WIFI_STATUS%", "Đã kết nối - " + WiFi.SSID());
        html.replace("%WIFI_MODE%", "STA");
    } else {
        html.replace("%WIFI_STATUS%", "Chưa kết nối");
        html.replace("%WIFI_MODE%", "AP");
    }
    
    server.send(200, "text/html", html);
}

void handleWiFiConfig() {
    String html = String(wifiConfigHTML);
    
    // Replace ESP_IP placeholder
    if (WiFi.status() == WL_CONNECTED) {
        html.replace("%ESP_IP%", WiFi.localIP().toString());
    } else {
        html.replace("%ESP_IP%", WiFi.softAPIP().toString());
    }
    
    html.replace("{{WIFI_LIST}}", getWiFiListHTML());
    server.send(200, "text/html", html);
}

void handleWiFiScan() {
    scanWiFi();
    String json = "[";
    for (int i = 0; i < wifiCount && i < 20; i++) {
        if (i > 0) json += ",";
        json += "\"" + availableWifi[i] + "\"";
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
            } else {
                UnoSerial.println("PUMP_OFF");
                pumpStatus = false;
                addLog("Manual: Pump OFF");
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
    mqttClient.setServer(mqttServer, mqttPort);
    mqttClient.setCallback(mqttCallback);
    Serial.println("📡 MQTT client configured");
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
    String message;
    for (unsigned int i = 0; i < length; i++) {
        message += (char)payload[i];
    }
    
    String topicStr = String(topic);
    
    if (topicStr.endsWith("/v" + String(VIRTUAL_PIN_PUMP))) {
        if (message == "1") {
            UnoSerial.println("PUMP_ON");
            pumpStatus = true;
            addLog("MQTT: Pump ON");
        } else {
            UnoSerial.println("PUMP_OFF");
            pumpStatus = false;
            addLog("MQTT: Pump OFF");
        }
    } else if (topicStr.endsWith("/v" + String(VIRTUAL_PIN_MODE))) {
        autoMode = (message == "1");
        addLog("MQTT: Mode " + String(autoMode ? "Auto" : "Manual"));
    } else if (topicStr.endsWith("/v" + String(VIRTUAL_PIN_SPEED))) {
        pumpSpeed = message.toInt();
        addLog("MQTT: Speed " + String(pumpSpeed) + "%");
    }
}

void reconnectMQTT() {
    if (!mqttClient.connected()) {
        Serial.print("🔗 Connecting to MQTT...");
        
        if (mqttClient.connect("ESP32Client", BLYNK_AUTH_TOKEN, "")) {
            Serial.println("✅ MQTT connected");
            
            String topicPump = String(BLYNK_AUTH_TOKEN) + "/v" + String(VIRTUAL_PIN_PUMP);
            mqttClient.subscribe(topicPump.c_str());
            
            String topicMode = String(BLYNK_AUTH_TOKEN) + "/v" + String(VIRTUAL_PIN_MODE);
            mqttClient.subscribe(topicMode.c_str());
            
            String topicSpeed = String(BLYNK_AUTH_TOKEN) + "/v" + String(VIRTUAL_PIN_SPEED);
            mqttClient.subscribe(topicSpeed.c_str());
            
            addLog("MQTT connected & subscribed");
        } else {
            Serial.print("❌ MQTT failed, rc=");
            Serial.println(mqttClient.state());
        }
    }
}

void publishData() {
    if (!mqttClient.connected()) return;

    mqttClient.publish((String(BLYNK_AUTH_TOKEN) + "/v" + String(VIRTUAL_PIN_SOIL)).c_str(),
                       String(soilMoisture).c_str());
    mqttClient.publish((String(BLYNK_AUTH_TOKEN) + "/v" + String(VIRTUAL_PIN_RAIN)).c_str(),
                       String(rainStatus).c_str());
    mqttClient.publish((String(BLYNK_AUTH_TOKEN) + "/v" + String(VIRTUAL_PIN_STATE)).c_str(),
                       String(pumpStatus).c_str());

    // Send data to Blynk dashboard
    if (Blynk.connected()) {
        Blynk.virtualWrite(VIRTUAL_PIN_SOIL, soilMoisture);
        Blynk.virtualWrite(VIRTUAL_PIN_RAIN, rainStatus);
        Blynk.virtualWrite(VIRTUAL_PIN_PUMP, pumpStatus);
        Blynk.virtualWrite(VIRTUAL_PIN_MODE, autoMode);
        Blynk.virtualWrite(VIRTUAL_PIN_SPEED, pumpSpeed);
        Blynk.virtualWrite(VIRTUAL_PIN_STATE, pumpStatus);
    }
}

// ========== HELPER FUNCTIONS ==========
void addLog(String message) {
    // Prevent duplicate consecutive prints
    if (message == lastLogMessage) {
        return;
    }
    lastLogMessage = message;

    time_t now = time(nullptr);
    struct tm timeinfo;

    // Reserve memory for eventLog to prevent fragmentation
    if (eventLog.length() == 0) {
        eventLog.reserve(MAX_LOG_LENGTH + 100);
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
            addLog("Auto: Pump ON (Hour: " + String(currentHour) + ", Soil: " + String(soilMoisture) + "%)");
        }
    } else {
        if (pumpStatus) {
            UnoSerial.println("PUMP_OFF");
            pumpStatus = false;
            addLog("Auto: Pump OFF");
        }
    }
}

String getWiFiListHTML() {
    String html = "";
    for (int i = 0; i < wifiCount && i < 20; i++) {
        html += "<option value=\"" + availableWifi[i] + "\">" + availableWifi[i] + "</option>";
    }
    return html;
}

// ========== SETUP FLAG MANAGEMENT ==========
void loadSetupFlag() {
    int flag = EEPROM.read(SETUP_FLAG_ADDR);
    setupCompleted = (flag == 1);
    Serial.printf("📖 Setup flag loaded: %s\n", setupCompleted ? "COMPLETED" : "NOT COMPLETED");
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
    
    server.send(200, "text/plain", "Setup reset successfully");
}

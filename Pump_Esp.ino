#include <WiFi.h>
#include <WebServer.h>
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

// ========== EEPROM CONFIG ==========
#define EEPROM_SIZE 512
#define SSID_ADDR 0
#define PASS_ADDR 32

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

// ========== ADD THESE MISSING VARIABLES ==========
String savedSSID = "";
String savedPassword = "";

// ========== HTML CONTENTS (DECLARE HERE) ==========
extern const char* wifiConfigHTML;
extern const char* htmlContent;

// ========== FUNCTION PROTOTYPES ==========
void setupWiFi();
void scanWiFi();
void connectToWiFi(String ssid, String password);
void saveWiFiCreds(String ssid, String password);
void loadWiFiCreds();
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

// ========== SETUP ==========
void setup() {
    Serial.begin(115200);
    UnoSerial.begin(9600, SERIAL_8N1, RXD2, TXD2);
    
    EEPROM.begin(EEPROM_SIZE);
    
    Serial.println("\n🚀 ESP32 Smart Irrigation System Starting...");
    
    // Load saved WiFi credentials
    loadWiFiCreds();
    
    // Try to connect to saved WiFi
    if (wifiConfigured && savedSSID.length() > 0) {
        setupWiFi();
    } else {
        Serial.println("📶 WiFi not configured. Starting AP mode...");
        WiFi.softAP("SmartIrrigation_AP", "12345678");
        Serial.print("📡 AP IP: ");
        Serial.println(WiFi.softAPIP());
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
        
        static unsigned long lastPublish = 0;
        if (millis() - lastPublish > 5000) {
            publishData();
            lastPublish = millis();
        }
        
        if (autoMode) {
            checkAutoWatering();
        }
    }
    
    delay(100);
}

// ========== WIFI FUNCTIONS ==========
void setupWiFi() {
    WiFi.mode(WIFI_STA);
    WiFi.begin(savedSSID.c_str(), savedPassword.c_str());
    
    Serial.print("📶 Connecting to WiFi: ");
    Serial.println(savedSSID);
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 30) {
        delay(500);
        Serial.print(".");
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\n✅ WiFi Connected!");
        Serial.print("📡 IP Address: ");
        Serial.println(WiFi.localIP());
        addLog("Connected to: " + savedSSID);
        addLog("IP: " + WiFi.localIP().toString());
    } else {
        Serial.println("\n❌ WiFi Connection Failed!");
        addLog("WiFi connection failed");
    }
}

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
            server.send(200, "application/json", "{\"status\":\"success\",\"ip\":\"" + WiFi.localIP().toString() + "\"}");
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
        if (state == "on") {
            UnoSerial.println("PUMP_ON");
            pumpStatus = true;
            addLog("Manual: Pump ON");
        } else {
            UnoSerial.println("PUMP_OFF");
            pumpStatus = false;
            addLog("Manual: Pump OFF");
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
}

// ========== HELPER FUNCTIONS ==========
void addLog(String message) {
    time_t now = time(nullptr);
    struct tm timeinfo;
    
    if (localtime_r(&now, &timeinfo)) {
        char timeStr[20];
        strftime(timeStr, sizeof(timeStr), "%H:%M:%S", &timeinfo);
        eventLog = "[" + String(timeStr) + "] " + message + "<br>" + eventLog;
    } else {
        eventLog = "[--:--:--] " + message + "<br>" + eventLog;
    }
    
    if (eventLog.length() > 3000) {
        int lastIndex = eventLog.lastIndexOf("<br>", 2000);
        if (lastIndex != -1) {
            eventLog = eventLog.substring(0, lastIndex);
        }
    }
    
    Serial.println("📝 " + message);
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

// ========== HTML CONTENTS (DEFINE HERE) ==========
const char* wifiConfigHTML = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>WiFi Configuration</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            padding: 20px;
            min-height: 100vh;
        }
        .container {
            max-width: 500px;
            margin: 0 auto;
            background: white;
            padding: 30px;
            border-radius: 20px;
            box-shadow: 0 8px 32px rgba(0,0,0,0.1);
        }
        h1 {
            color: #333;
            text-align: center;
            margin-bottom: 30px;
        }
        .form-group {
            margin-bottom: 20px;
        }
        label {
            display: block;
            margin-bottom: 5px;
            font-weight: bold;
            color: #555;
        }
        select, input {
            width: 100%;
            padding: 12px;
            border: 2px solid #ddd;
            border-radius: 10px;
            font-size: 16px;
            transition: border 0.3s;
        }
        select:focus, input:focus {
            border-color: #667eea;
            outline: none;
        }
        .btn {
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
            border: none;
            padding: 15px;
            border-radius: 10px;
            font-size: 16px;
            font-weight: bold;
            cursor: pointer;
            width: 100%;
            transition: transform 0.3s;
        }
        .btn:hover {
            transform: translateY(-2px);
        }
        .btn-secondary {
            background: #6c757d;
            margin-top: 10px;
        }
        .status {
            padding: 10px;
            border-radius: 5px;
            margin: 10px 0;
            text-align: center;
            font-weight: bold;
        }
        .success {
            background: #d4edda;
            color: #155724;
        }
        .error {
            background: #f8d7da;
            color: #721c24;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>🔧 WiFi Configuration</h1>
        
        <div class="form-group">
            <label for="wifiSelect">Select WiFi Network:</label>
            <select id="wifiSelect">
                <option value="">-- Select Network --</option>
            </select>
        </div>
        
        <div class="form-group">
            <label for="ssid">Or Enter SSID:</label>
            <input type="text" id="ssid" placeholder="WiFi SSID">
        </div>
        
        <div class="form-group">
            <label for="password">Password:</label>
            <input type="password" id="password" placeholder="WiFi Password">
        </div>
        
        <div id="status"></div>
        
        <button class="btn" onclick="connectWiFi()">🔗 Connect to WiFi</button>
        <button class="btn btn-secondary" onclick="scanWiFi()">🔍 Scan Networks</button>
        <button class="btn btn-secondary" onclick="goToMain()">🏠 Back to Main</button>
        
        <div style="margin-top: 20px; text-align: center;">
            <small>Current IP: %ESP_IP%</small><br>
            <small>AP SSID: SmartIrrigation_AP</small><br>
            <small>AP Password: 12345678</small>
        </div>
    </div>
    
    <script>
        function scanWiFi() {
            fetch('/wifi-scan')
                .then(response => response.json())
                .then(networks => {
                    const select = document.getElementById('wifiSelect');
                    select.innerHTML = '<option value="">-- Select Network --</option>';
                    networks.forEach(network => {
                        const option = document.createElement('option');
                        option.value = network;
                        option.textContent = network;
                        select.appendChild(option);
                    });
                    showStatus('Found ' + networks.length + ' networks', 'success');
                })
                .catch(error => {
                    showStatus('Scan failed: ' + error, 'error');
                });
        }
        
        function connectWiFi() {
            const ssid = document.getElementById('ssid').value || 
                        document.getElementById('wifiSelect').value;
            const password = document.getElementById('password').value;
            
            if (!ssid || !password) {
                showStatus('Please enter SSID and password', 'error');
                return;
            }
            
            const formData = new FormData();
            formData.append('ssid', ssid);
            formData.append('password', password);
            
            showStatus('Connecting...', 'success');
            
            fetch('/wifi-connect', {
                method: 'POST',
                body: formData
            })
            .then(response => response.json())
            .then(data => {
                if (data.status === 'success') {
                    showStatus('Connected! IP: ' + data.ip, 'success');
                    setTimeout(() => {
                        window.location.href = '/';
                    }, 3000);
                } else {
                    showStatus('Connection failed', 'error');
                }
            })
            .catch(error => {
                showStatus('Error: ' + error, 'error');
            });
        }
        
        function goToMain() {
            window.location.href = '/';
        }
        
        function showStatus(message, type) {
            const statusDiv = document.getElementById('status');
            statusDiv.textContent = message;
            statusDiv.className = 'status ' + type;
        }
        
        // Auto scan on load
        window.onload = scanWiFi;
    </script>
</body>
</html>
)rawliteral";

// Main HTML interface (giữ nguyên từ code cũ nhưng thêm nút WiFi Config)
const char* htmlContent = R"rawliteral(
<!DOCTYPE html>
<html lang="vi">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Hệ Thống Tưới Cây Thông Minh</title>
    <style>
        /* Giữ nguyên toàn bộ CSS từ code cũ */
        :root {
            --primary: #667eea;
            --secondary: #764ba2;
            --success: #28a745;
            --danger: #dc3545;
            --warning: #ffc107;
            --info: #17a2b8;
            --light: #f8f9fa;
            --dark: #343a40;
        }
        
        * {
            box-sizing: border-box;
            margin: 0;
            padding: 0;
        }
        
        body {
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            min-height: 100vh;
            padding: 20px;
        }
        
        .container {
            max-width: 1200px;
            margin: 0 auto;
        }
        
        .header {
            background: rgba(255, 255, 255, 0.95);
            backdrop-filter: blur(10px);
            border-radius: 20px;
            padding: 30px;
            margin-bottom: 20px;
            text-align: center;
            box-shadow: 0 8px 32px rgba(0, 0, 0, 0.1);
        }
        
        .header h1 {
            background: linear-gradient(135deg, var(--primary), var(--secondary));
            -webkit-background-clip: text;
            -webkit-text-fill-color: transparent;
            font-size: 2.5rem;
            margin-bottom: 10px;
        }
        
        .connection-info {
            background: var(--info);
            color: white;
            padding: 10px;
            border-radius: 10px;
            margin: 10px 0;
            font-size: 0.9rem;
        }
        
        .grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(300px, 1fr));
            gap: 20px;
            margin-bottom: 20px;
        }
        
        .card {
            background: rgba(255, 255, 255, 0.95);
            backdrop-filter: blur(10px);
            border-radius: 20px;
            padding: 25px;
            box-shadow: 0 8px 32px rgba(0, 0, 0, 0.1);
            transition: transform 0.3s ease, box-shadow 0.3s ease;
        }
        
        .card:hover {
            transform: translateY(-5px);
            box-shadow: 0 12px 40px rgba(0, 0, 0, 0.15);
        }
        
        .card-title {
            font-size: 1.3rem;
            font-weight: 600;
            margin-bottom: 15px;
            color: var(--dark);
            display: flex;
            align-items: center;
            gap: 10px;
        }
        
        .card-title i {
            color: var(--primary);
            font-style: normal;
            font-size: 1.5rem;
        }
        
        .sensor-value {
            font-size: 2.5rem;
            font-weight: 700;
            text-align: center;
            margin: 15px 0;
            background: linear-gradient(135deg, var(--primary), var(--secondary));
            -webkit-background-clip: text;
            -webkit-text-fill-color: transparent;
        }
        
        .control-group {
            margin: 20px 0;
        }
        
        .switch {
            position: relative;
            display: inline-block;
            width: 60px;
            height: 34px;
        }
        
        .switch input {
            opacity: 0;
            width: 0;
            height: 0;
        }
        
        .slider {
            position: absolute;
            cursor: pointer;
            top: 0;
            left: 0;
            right: 0;
            bottom: 0;
            background-color: #ccc;
            transition: .4s;
            border-radius: 34px;
        }
        
        .slider:before {
            position: absolute;
            content: "";
            height: 26px;
            width: 26px;
            left: 4px;
            bottom: 4px;
            background-color: white;
            transition: .4s;
            border-radius: 50%;
        }
        
        input:checked + .slider {
            background: linear-gradient(135deg, var(--primary), var(--secondary));
        }
        
        input:checked + .slider:before {
            transform: translateX(26px);
        }
        
        .slider-container {
            display: flex;
            align-items: center;
            gap: 15px;
            margin: 15px 0;
        }
        
        .slider-value {
            font-weight: 600;
            color: var(--primary);
            min-width: 40px;
        }
        
        input[type="range"] {
            flex: 1;
            height: 8px;
            border-radius: 4px;
            background: #e0e0e0;
            outline: none;
        }
        
        input[type="range"]::-webkit-slider-thumb {
            -webkit-appearance: none;
            width: 20px;
            height: 20px;
            border-radius: 50%;
            background: var(--primary);
            cursor: pointer;
        }
        
        .status-badge {
            display: inline-block;
            padding: 8px 16px;
            border-radius: 20px;
            font-weight: 600;
            margin: 5px;
        }
        
        .status-on {
            background: var(--success);
            color: white;
        }
        
        .status-off {
            background: var(--danger);
            color: white;
        }
        
        .status-auto {
            background: var(--info);
            color: white;
        }
        
        .status-manual {
            background: var(--warning);
            color: black;
        }
        
        .status-info {
            background: var(--info);
            color: white;
        }
        
        .status-warning {
            background: var(--warning);
            color: black;
        }
        
        .status-success {
            background: var(--success);
            color: white;
        }
        
        .status-danger {
            background: var(--danger);
            color: white;
        }
        
        .log-container {
            max-height: 300px;
            overflow-y: auto;
            background: var(--light);
            border-radius: 10px;
            padding: 15px;
            margin-top: 15px;
        }
        
        .log-entry {
            padding: 8px 12px;
            margin: 5px 0;
            background: white;
            border-radius: 8px;
            border-left: 4px solid var(--primary);
            font-size: 0.9rem;
        }
        
        .btn {
            padding: 12px 24px;
            border: none;
            border-radius: 10px;
            font-weight: 600;
            cursor: pointer;
            transition: all 0.3s ease;
            margin: 5px;
        }
        
        .btn-primary {
            background: linear-gradient(135deg, var(--primary), var(--secondary));
            color: white;
        }
        
        .btn:hover {
            transform: translateY(-2px);
            box-shadow: 0 4px 12px rgba(0, 0, 0, 0.2);
        }
        
        .current-time {
            font-size: 1.1rem;
            font-weight: 600;
            color: var(--dark);
            text-align: center;
            margin: 10px 0;
        }
        
        .wifi-status {
            background: var(--success);
            color: white;
            padding: 8px 16px;
            border-radius: 20px;
            font-size: 0.9rem;
            margin: 10px 0;
        }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>🌿 HỆ THỐNG TƯỚI CÂY THÔNG MINH</h1>
            <div class="wifi-status" id="wifiStatus">
                📶 WiFi: %WIFI_STATUS% | Mode: %WIFI_MODE%
            </div>
            <div class="current-time" id="currentTime">--:--:--</div>
            <div style="margin-top: 15px;">
                <button class="btn btn-primary" onclick="location.href='/wifi-config'">
                    🔧 Cấu Hình WiFi
                </button>
                <button class="btn btn-primary" onclick="location.href='/'">
                    🏠 Trang Chính
                </button>
            </div>
        </div>
        
        <div class="grid">
            <!-- Sensor Data Cards (giữ nguyên) -->
            <div class="card">
                <div class="card-title">
                    <i>💧</i>
                    ĐỘ ẨM ĐẤT
                </div>
                <div class="sensor-value" id="soilMoisture">0%</div>
                <div class="status-badge" id="soilStatus">Đang cập nhật...</div>
            </div>
            
            <div class="card">
                <div class="card-title">
                    <i>🌧️</i>
                    TRẠNG THÁI MƯA
                </div>
                <div class="sensor-value" id="rainStatus">--</div>
                <div class="status-badge" id="rainStatusText">Đang cập nhật...</div>
            </div>
            
            <div class="card">
                <div class="card-title">
                    <i>🚰</i>
                    MÁY BƠM
                </div>
                <div class="sensor-value" id="pumpStatus">TẮT</div>
                <div class="status-badge status-off" id="pumpStatusBadge">OFF</div>
            </div>
            
            <div class="card">
                <div class="card-title">
                    <i>⚙️</i>
                    CHẾ ĐỘ HOẠT ĐỘNG
                </div>
                <div class="control-group">
                    <div class="slider-container">
                        <span>Tự động</span>
                        <label class="switch">
                            <input type="checkbox" id="modeToggle">
                            <span class="slider"></span>
                        </label>
                        <span>Thủ công</span>
                    </div>
                    <div class="status-badge status-auto" id="modeStatus">CHẾ ĐỘ TỰ ĐỘNG</div>
                </div>
                
                <div id="manualControls" style="display: none;">
                    <div class="card-title">
                        <i>🎚️</i>
                        ĐIỀU KHIỂN THỦ CÔNG
                    </div>
                    <div class="slider-container">
                        <span>Tốc độ:</span>
                        <input type="range" min="0" max="100" value="50" id="pumpSpeedSlider">
                        <span class="slider-value" id="pumpSpeedValue">50%</span>
                    </div>
                    <button class="btn btn-primary" id="btnPumpOn">
                        ▶️ BẬT BƠM
                    </button>
                    <button class="btn btn-primary" id="btnPumpOff">
                        ⏹️ TẮT BƠM
                    </button>
                </div>
            </div>
        </div>
        
        <!-- Event Log -->
        <div class="card">
            <div class="card-title">
                <i>📋</i>
                NHẬT KÝ SỰ KIỆN
            </div>
            <div class="log-container" id="eventLog">
                <div class="log-entry">[Hệ thống] Đang khởi động...</div>
            </div>
            <button class="btn btn-primary" id="btnClearLog">
                🗑️ XÓA NHẬT KÝ
            </button>
        </div>
    </div>

    <script>
        // Update current time
        function updateTime() {
            const now = new Date();
            const timeString = now.toLocaleTimeString('vi-VN');
            document.getElementById('currentTime').textContent = timeString;
        }
        setInterval(updateTime, 1000);
        updateTime();
        
        // Toggle between auto and manual mode
        function toggleMode() {
            const isManual = document.getElementById('modeToggle').checked;
            const modeStatus = document.getElementById('modeStatus');
            const manualControls = document.getElementById('manualControls');
            
            if (isManual) {
                modeStatus.textContent = 'CHẾ ĐỘ THỦ CÔNG';
                modeStatus.className = 'status-badge status-manual';
                manualControls.style.display = 'block';
                addLog('Chuyển sang chế độ thủ công');
            } else {
                modeStatus.textContent = 'CHẾ ĐỘ TỰ ĐỘNG';
                modeStatus.className = 'status-badge status-auto';
                manualControls.style.display = 'none';
                addLog('Chuyển sang chế độ tự động');
            }
            
            fetch('/setMode?mode=' + (isManual ? 'manual' : 'auto'))
                .catch(error => console.error('Error setting mode:', error));
        }
        
        // Update pump speed
        function updatePumpSpeed(speed) {
            document.getElementById('pumpSpeedValue').textContent = speed + '%';
            fetch('/setSpeed?speed=' + speed)
                .catch(error => console.error('Error setting speed:', error));
            addLog('Điều chỉnh tốc độ bơm: ' + speed + '%');
        }
        
        // Control pump manually
        function controlPump(turnOn) {
            const endpoint = turnOn ? '/controlPump?state=on' : '/controlPump?state=off';
            fetch(endpoint)
                .then(response => {
                    if (!response.ok) {
                        throw new Error('Network response was not ok');
                    }
                    addLog(turnOn ? 'Bật máy bơm thủ công' : 'Tắt máy bơm thủ công');
                })
                .catch(error => {
                    console.error('Error controlling pump:', error);
                    addLog('Lỗi khi điều khiển máy bơm');
                });
        }
        
        // Add log entry
        function addLog(message) {
            const logContainer = document.getElementById('eventLog');
            const now = new Date();
            const timeString = now.toLocaleTimeString('vi-VN');
            const logEntry = document.createElement('div');
            logEntry.className = 'log-entry';
            logEntry.innerHTML = `<strong>[${timeString}]</strong> ${message}`;
            logContainer.appendChild(logEntry);
            logContainer.scrollTop = logContainer.scrollHeight;
        }
        
        // Clear log
        function clearLog() {
            document.getElementById('eventLog').innerHTML = '';
            addLog('Đã xóa nhật ký sự kiện');
        }
        
        // Update sensor data
        function updateSensorData(data) {
            console.log('Updating sensor data:', data);
            
            document.getElementById('soilMoisture').textContent = data.soil + '%';
            document.getElementById('rainStatus').textContent = data.rain ? 'CÓ MƯA' : 'KHÔNG MƯA';
            document.getElementById('rainStatusText').textContent = data.rain ? 'Đang mưa' : 'Không mưa';
            document.getElementById('rainStatusText').className = data.rain ? 'status-badge status-info' : 'status-badge status-warning';
            
            document.getElementById('pumpStatus').textContent = data.pump ? 'ĐANG BƠM' : 'ĐÃ TẮT';
            document.getElementById('pumpStatusBadge').textContent = data.pump ? 'ON' : 'OFF';
            document.getElementById('pumpStatusBadge').className = data.pump ? 'status-badge status-on' : 'status-badge status-off';
            
            // Update mode toggle
            document.getElementById('modeToggle').checked = !data.autoMode;
            toggleMode(); // Update UI
            
            const soilStatus = document.getElementById('soilStatus');
            if (data.soil < 30) {
                soilStatus.textContent = 'RẤT KHÔ';
                soilStatus.className = 'status-badge status-danger';
            } else if (data.soil < 60) {
                soilStatus.textContent = 'BÌNH THƯỜNG';
                soilStatus.className = 'status-badge status-warning';
            } else {
                soilStatus.textContent = 'ĐỦ ẨM';
                soilStatus.className = 'status-badge status-success';
            }
        }
        
        // Fetch data from ESP32
        function fetchData() {
            fetch('/getData')
                .then(response => {
                    if (!response.ok) {
                        throw new Error('Network response was not ok');
                    }
                    return response.json();
                })
                .then(data => updateSensorData(data))
                .catch(error => console.error('Error fetching data:', error));
        }
        
        // Initialize event listeners after page load
        document.addEventListener('DOMContentLoaded', function() {
            console.log('DOM loaded, initializing event listeners...');
            
            // Mode toggle
            document.getElementById('modeToggle').addEventListener('change', toggleMode);
            
            // Pump speed slider
            document.getElementById('pumpSpeedSlider').addEventListener('input', function() {
                updatePumpSpeed(this.value);
            });
            
            // Pump control buttons
            document.getElementById('btnPumpOn').addEventListener('click', function() {
                controlPump(true);
            });
            
            document.getElementById('btnPumpOff').addEventListener('click', function() {
                controlPump(false);
            });
            
            // Clear log button
            document.getElementById('btnClearLog').addEventListener('click', clearLog);
            
            // Initial setup
            setInterval(fetchData, 2000);
            fetchData();
            addLog('Hệ thống khởi động thành công');
            
            console.log('Event listeners initialized successfully');
        });
        
    </script>
</body>
</html>
)rawliteral";
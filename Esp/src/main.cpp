#include <ArduinoJson.h>
#include <EEPROM.h>
#include <PubSubClient.h>
#include <SPIFFS.h>
#include <WebServer.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <WiFiManager.h>
#include <time.h>

// HiveMQ Cloud Configuration
#include "hivemq_cert.h"
#include "hivemq_config.h"
#include "mqtt_debug.h" // MQTT Debug Tool

// ========== DEBUG MODE ==========
// Bỏ comment dòng sau để bật debug tự động khi khởi động
// #define DEBUG_MQTT_ON_STARTUP

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
WiFiClientSecure
    espClient; // WiFiClientSecure để kết nối TLS/SSL tới private broker
PubSubClient mqttClient(espClient);

// ========== GLOBAL VARIABLES ==========
const char *ntpServer = "pool.ntp.org";
const char *TZ_INFO = "Asia/Bangkok";
String eventLog = "";
String savedSSID = "";
String savedPassword = "";

// ========== PREVIOUS STATE TRACKING ==========
bool prevPumpStatus = false;
bool prevAutoMode = true;
int prevPumpSpeed = 50;

// ========== SENSOR DATA TRACKING (for change detection) ==========
int prevSoilMoisture = -1; // -1 = chưa có dữ liệu
int prevRainStatus = -1;
unsigned long lastForcedPublish = 0;
const unsigned long FORCED_PUBLISH_INTERVAL = 300000; // 5 phút heartbeat

// ========== SYSTEM STATUS TRACKING ==========
int prevWifiRSSI = 0;
unsigned long prevUptime = 0;

// ========== EVENT LOG DUPLICATE PREVENTION ==========
String lastLogMessage = "";

// ========== MEMORY PROTECTION ==========
const int MAX_LOG_LENGTH = 2000;   // Giảm từ 3000 để quản lý bộ nhớ tốt hơn
const int LOG_TRIM_LENGTH = 1500;  // Cắt bớt khi log vượt quá độ dài này
const int MIN_FREE_HEAP = 10000;   // Ngưỡng heap trống tối thiểu (10KB)
const int MAX_STRING_LENGTH = 256; // Độ dài chuỗi tối đa để đảm bảo an toàn

// ========== FUNCTION PROTOTYPES ==========
void scanWiFi();
void connectToWiFi(String ssid, String password);
void loadWiFiCreds();
void saveWiFiCreds(String ssid, String password);

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
void mqttCallback(char *topic, byte *payload, unsigned int length);
void reconnectMQTT();
void publishData();
void publishPumpStatus(String reason = "");
void publishSystemStatus();
void publishLog(String logMessage);
void loadSetupFlag();
void saveSetupFlag(bool completed);
void handleResetSetup();
void handleMemoryStats(); // 🛡️ Endpoint giám sát bộ nhớ

// ========== SETUP ==========
void setup() {
  Serial.begin(115200);
  UnoSerial.begin(9600, SERIAL_8N1, RXD2, TXD2);

  EEPROM.begin(EEPROM_SIZE);

  // Khởi tạo SPIFFS
  if (!SPIFFS.begin(true)) {
    Serial.println("❌ SPIFFS Mount Failed");
    return;
  }
  Serial.println("✅ SPIFFS Mounted Successfully");

  Serial.println("\n🚀 ESP32 Smart Irrigation System Starting...");

  Serial.printf("🆔 MQTT Client ID: %s\n", MQTT_CLIENT_ID);

  // Tải thông tin WiFi đã lưu vào flag setup
  loadWiFiCreds();
  loadSetupFlag();

  // WiFiManager tự động kết nối
  if (wm.autoConnect("SmartIrrigation_AP", "12345678")) {
    Serial.println("✅ Connected to WiFi using WiFiManager");
    Serial.println("🌐 Access web interface at: http://" +
                   WiFi.localIP().toString());
    addLog("STA Mode: Connected to " + WiFi.SSID());
    addLog("IP: " + WiFi.localIP().toString());
    wifiConfigured = true;

  } else {
    Serial.println("❌ Failed to connect to WiFi, starting AP mode");
    Serial.println("🌐 Access captive portal at: http://" +
                   WiFi.softAPIP().toString());
    addLog("AP Mode: SmartIrrigation_AP (12345678)");
  }

  // Khởi tạo thời gian
  configTzTime(TZ_INFO, ntpServer);

  Serial.print("⏳ Waiting for time sync");
  int retry = 0;
  while (time(nullptr) < 1600000000 &&
         retry < 20) { // Timestamp hợp lệ > năm 2020
    delay(500);
    Serial.print(".");
    retry++;
  }
  if (time(nullptr) > 1600000000) {
    Serial.println("\n✅ Time synced!");
    time_t now = time(nullptr);
    Serial.printf("🕒 Current time: %s", ctime(&now));
  } else {
    Serial.println("\n⚠️ Time sync failed, SSL may fail");
  }

  // Thiết lập web server
  setupWebServer();

  // Thiết lập MQTT
  setupMQTT();

  Serial.println("✅ System initialized successfully");
}

// ========== MAIN LOOP ==========
void loop() {
  // Lệnh Serial để debug MQTT
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    cmd.toLowerCase();

    if (cmd == "debug" || cmd == "mqtt" || cmd == "test") {
      Serial.println("\n╔════════════════════════════════════════╗");
      Serial.println("║  🔍 MQTT DEBUG REQUESTED               ║");
      Serial.println("╚════════════════════════════════════════╝");
      delay(1000);
      MQTTDebugger::debugMQTTConnection(espClient, mqttClient);
    } else if (cmd == "help") {
      Serial.println("\n📋 Available Commands:");
      Serial.println("   debug / mqtt / test  → Run MQTT diagnostic");
      Serial.println("   help                 → Show this help\n");
    }
  }

  server.handleClient();
  readUARTData();

  if (WiFi.status() == WL_CONNECTED) {
    if (!mqttClient.connected()) {
      reconnectMQTT();
    }
    mqttClient.loop();

    // Xuất bản dữ liệu cảm biến mỗi 5 giây
    static unsigned long lastPublish = 0;
    if (millis() - lastPublish > SENSOR_PUBLISH_INTERVAL) {
      publishData();
      lastPublish = millis();
    }

    // Xuất bản trạng thái hệ thống mỗi 30 giây
    static unsigned long lastStatusPublish = 0;
    if (millis() - lastStatusPublish > STATUS_PUBLISH_INTERVAL) {
      publishSystemStatus();
      lastStatusPublish = millis();
    }

    if (autoMode) {
      checkAutoWatering();
    }
  }

  // Đã xóa delay(100) để cập nhật theo thời gian thực
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
  // Xóa EEPROM trước
  for (int i = 0; i < 64; i++) {
    EEPROM.write(SSID_ADDR + i, 0);
    EEPROM.write(PASS_ADDR + i, 0);
  }

  // Lưu SSID
  for (int i = 0; i < ssid.length() && i < 32; i++) {
    EEPROM.write(SSID_ADDR + i, ssid[i]);
  }
  EEPROM.write(SSID_ADDR + ssid.length(), '\0');

  // Lưu Password
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

  // Tải SSID
  for (int i = 0; i < 32; i++) {
    char c = EEPROM.read(SSID_ADDR + i);
    if (c == '\0')
      break;
    if (c > 0)
      ssid += c;
  }

  // Tải Password
  for (int i = 0; i < 32; i++) {
    char c = EEPROM.read(PASS_ADDR + i);
    if (c == '\0')
      break;
    if (c > 0)
      password += c;
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
  server.on("/memory", HTTP_GET,
            handleMemoryStats); // 🛡️ Endpoint giám sát bộ nhớ
  server.onNotFound([]() { server.send(404, "text/plain", "404: Not Found"); });

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
  // Phục vụ trang cấu hình WiFi từ SPIFFS
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

  // 🛡️ CẤP PHÁT TRƯỚC: Dự trữ bộ đệm chuỗi để tránh cấp phát lại
  String json = "";
  json.reserve(600); // Dự trữ ~30 bytes mỗi mạng * 20 mạng
  json = "[";

  for (int i = 0; i < count && i < 20; i++) {
    if (i > 0)
      json += ",";

    // 🛡️ GIỚI HẠN ĐỘ DÀI SSID: Ngăn tên SSID quá dài
    String ssid = WiFi.SSID(i);
    if (ssid.length() > 32) {
      ssid = ssid.substring(0, 32); // SSID tối đa là 32 ký tự
    }

    json += "\"" + ssid + "\"";
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
      server.send(200, "application/json",
                  "{\"status\":\"success\",\"ip\":\"" +
                      WiFi.localIP().toString() + "\",\"ssid\":\"" + ssid +
                      "\"}");
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
  Serial.println("📡 Initializing MQTT with HiveMQ Private Cloud Broker...");

  // Cấu hình chứng chỉ TLS/SSL cho private broker
  espClient.setCACert(hivemq_root_ca);

  // Thiết lập MQTT server
  mqttClient.setServer(HIVEMQ_HOST, HIVEMQ_PORT);
  mqttClient.setCallback(mqttCallback);
  mqttClient.setBufferSize(MQTT_BUFFER_SIZE);
  mqttClient.setKeepAlive(MQTT_KEEPALIVE_INTERVAL);

  Serial.printf("🔐 MQTT Broker: %s:%d\n", HIVEMQ_HOST, HIVEMQ_PORT);
  Serial.printf("👤 Client ID: %s\n", MQTT_CLIENT_ID);
  Serial.printf("👤 Username: %s\n", MQTT_USERNAME);
  Serial.println("ℹ️ Private cloud broker - TLS enabled");
}

void mqttCallback(char *topic, byte *payload, unsigned int length) {
  // 🛡️ KIỂM TRA KÍCH THƯỚC PAYLOAD: Ngăn tràn ngăn xếp từ payload lớn
  const int MAX_PAYLOAD =
      MQTT_BUFFER_SIZE; // Sử dụng kích thước bộ đệm đã cấu hình (1024)
  if (length > MAX_PAYLOAD) {
    Serial.printf(
        "❌ MQTT payload too large: %u bytes (max %d). Rejecting message.\n",
        length, MAX_PAYLOAD);
    addLog("MQTT Error: Payload too large");
    return;
  }

  // Chuyển payload thành chuỗi với bộ đệm CỐ ĐỊNH (không dùng VLA)
  char message[MAX_PAYLOAD + 1];
  memcpy(message, payload, length);
  message[length] = '\0';

  String topicStr = String(topic);
  String messageStr = String(message);

// Ghi log chi tiết hơn
#if MQTT_DEBUG
  Serial.println("\n📥 ========== MQTT MESSAGE RECEIVED ==========");
  Serial.printf("   Topic: %s\n", topic);
  Serial.printf("   Length: %u bytes\n", length);
  Serial.printf("   Payload: %s\n", message);
  Serial.println("============================================\n");
#endif

  // Phân tích thông điệp JSON
  StaticJsonDocument<256> doc;
  DeserializationError error = deserializeJson(doc, message);

  if (error) {
    Serial.printf("❌ JSON parse error on topic [%s]: %s\n", topic,
                  error.c_str());
    Serial.printf("   Raw payload: %s\n", message);
    addLog("MQTT Parse Error: " + topicStr);
    return;
  }

  // Xử lý điều khiển bơm
  if (topicStr == TOPIC_PUMP_CONTROL) {
    String command = doc["command"] | "";

    Serial.printf("🎛️  Processing PUMP_CONTROL command: %s\n", command.c_str());

    if (command == "turn_on") {
      UnoSerial.println("PUMP_ON");
      pumpStatus = true;

      if (doc.containsKey("speed")) {
        pumpSpeed = doc["speed"];
        Serial.printf("   Speed set to: %d%%\n", pumpSpeed);
      }

      addLog("MQTT: Pump ON (Speed: " + String(pumpSpeed) + "%)");
      Serial.println("✅ Pump turned ON successfully");
    } else if (command == "turn_off") {
      UnoSerial.println("PUMP_OFF");
      pumpStatus = false;
      addLog("MQTT: Pump OFF");
      Serial.println("✅ Pump turned OFF successfully");
    } else {
      Serial.printf("⚠️  Unknown pump command: %s\n", command.c_str());
    }
  }

  // Xử lý điều khiển chế độ
  else if (topicStr == TOPIC_MODE_CONTROL) {
    String mode = doc["mode"] | "";

    Serial.printf("🎛️  Processing MODE_CONTROL: %s\n", mode.c_str());

    if (mode == "AUTO") {
      autoMode = true;
      addLog("MQTT: Mode changed to AUTO");
      Serial.println("✅ Mode changed to AUTO");
    } else if (mode == "MANUAL") {
      autoMode = false;
      if (doc.containsKey("speed")) {
        pumpSpeed = doc["speed"];
        Serial.printf("   Speed set to: %d%%\n", pumpSpeed);
      }
      addLog("MQTT: Mode changed to MANUAL");
      Serial.println("✅ Mode changed to MANUAL");
    } else {
      Serial.printf("⚠️  Unknown mode: %s\n", mode.c_str());
    }
  }

  // Xử lý cập nhật cấu hình
  else if (topicStr == TOPIC_CONFIG) {
    // Dành cho cập nhật cấu hình trong tương lai
    Serial.println("📝 Config update received");
  }

  // Topic không xác định
  else {
    Serial.printf("⚠️  Message received on UNKNOWN topic: %s\n", topic);
  }
}

void reconnectMQTT() {
  static unsigned long lastAttempt = 0;
  static int reconnectAttempts = 0;
  static unsigned long reconnectDelay = MQTT_RECONNECT_DELAY;

  // Không thử lại quá nhanh - dùng exponential backoff
  if (millis() - lastAttempt < reconnectDelay) {
    return;
  }

  lastAttempt = millis();

  if (!mqttClient.connected()) {
    reconnectAttempts++;
    Serial.printf("🔄 MQTT reconnect attempt #%d to %s...\n", reconnectAttempts,
                  HIVEMQ_HOST);
    Serial.printf("📋 Client ID: %s\n", MQTT_CLIENT_ID);
    Serial.printf("👤 Username: %s\n", MQTT_USERNAME);
    Serial.println("ℹ️ Private broker - Using credentials");

    // Thử kết nối với thông tin xác thực (private broker)
    if (mqttClient.connect(MQTT_CLIENT_ID, MQTT_USERNAME, MQTT_PASSWORD)) {
      Serial.println("✅ MQTT Connected!");
      mqttConnected = true;
      reconnectAttempts = 0;                 // Đặt lại bộ đếm khi thành công
      reconnectDelay = MQTT_RECONNECT_DELAY; // Đặt lại độ trễ
      addLog("MQTT: Connected to HiveMQ Private Cloud Broker");

      // Đăng ký các topic điều khiển với kiểm tra lỗi
      if (mqttClient.subscribe(TOPIC_PUMP_CONTROL, MQTT_QOS)) {
        Serial.printf("📤 Subscribed to: %s\n", TOPIC_PUMP_CONTROL);
      } else {
        Serial.printf("❌ Failed to subscribe to: %s\n", TOPIC_PUMP_CONTROL);
      }

      if (mqttClient.subscribe(TOPIC_MODE_CONTROL, MQTT_QOS)) {
        Serial.printf("📤 Subscribed to: %s\n", TOPIC_MODE_CONTROL);
      } else {
        Serial.printf("❌ Failed to subscribe to: %s\n", TOPIC_MODE_CONTROL);
      }

      if (mqttClient.subscribe(TOPIC_CONFIG, MQTT_QOS)) {
        Serial.printf("📤 Subscribed to: %s\n", TOPIC_CONFIG);
      } else {
        Serial.printf("❌ Failed to subscribe to: %s\n", TOPIC_CONFIG);
      }

      // Xuất bản trạng thái ban đầu
      publishSystemStatus();
    } else {
      mqttConnected = false;
      int state = mqttClient.state();
      Serial.printf("❌ MQTT Connection failed, rc=%d (attempt %d)\n", state,
                    reconnectAttempts);

      // Thông báo lỗi chi tiết
      switch (state) {
      case -4:
        Serial.println("   → MQTT_CONNECTION_TIMEOUT - Server didn't respond");
        break;
      case -3:
        Serial.println("   → MQTT_CONNECTION_LOST - Network connection lost");
        break;
      case -2:
        Serial.println("   → MQTT_CONNECT_FAILED - Network connection failed");
        Serial.println("   → Check: WiFi, Host, Port, or Certificate");
        break;
      case -1:
        Serial.println("   → MQTT_DISCONNECTED");
        break;
      case 1:
        Serial.println("   → MQTT_CONNECT_BAD_PROTOCOL - Check MQTT version");
        break;
      case 2:
        Serial.println("   → MQTT_CONNECT_BAD_CLIENT_ID - Invalid Client ID");
        break;
      case 3:
        Serial.println("   → MQTT_CONNECT_UNAVAILABLE - Broker unavailable");
        break;
      case 4:
        Serial.println(
            "   → MQTT_CONNECT_BAD_CREDENTIALS - Wrong username/password");
        Serial.printf("   → Verify credentials in hivemq_config.h\n");
        break;
      case 5:
        Serial.println("   → MQTT_CONNECT_UNAUTHORIZED - Not authorized");
        Serial.println("   → Check HiveMQ Cloud Access Management");
        Serial.println(
            "   → QUICK FIX: Disconnect duplicate Client ID in HiveMQ Console");
        Serial.printf("   → Or change Client ID in hivemq_config.h: %s\n",
                      MQTT_CLIENT_ID);
        break;
      default:
        Serial.printf("   → Unknown error code: %d\n", state);
      }

      // 🔍 TỰ ĐỘNG DEBUG: Chạy debug tool sau 3 lần thất bại
      if (reconnectAttempts == 3) {
        Serial.println("\n⚠️  Multiple connection failures detected!");
        Serial.println("🔍 Running MQTT Diagnostic Tool...\n");
        delay(1000);
        MQTTDebugger::debugMQTTConnection(espClient, mqttClient);
        Serial.println("\n💡 TIP: Type 'debug' in Serial Monitor to run "
                       "diagnostic again anytime\n");
      }

      // Exponential backoff: 5s, 10s, 20s, 30s (tối đa)
      reconnectDelay = min(reconnectDelay * 2, 30000UL);
      Serial.printf("⏱️  Next attempt in %lu seconds\n", reconnectDelay / 1000);
    }
  }
}

void publishData() {
  if (!mqttClient.connected()) {
    return;
  }

  // Kiểm tra xem có thay đổi đáng kể không
  bool hasSignificantChange = false;

  // Độ ẩm đất: thay đổi >= 2% mới gửi (tránh nhiễu)
  if (prevSoilMoisture == -1 || abs(soilMoisture - prevSoilMoisture) >= 2) {
    hasSignificantChange = true;
  }

  // Mưa: thay đổi ngay lập tức
  if (prevRainStatus == -1 || rainStatus != prevRainStatus) {
    hasSignificantChange = true;
  }

  // Pump: thay đổi ngay lập tức
  if (pumpStatus != prevPumpStatus) {
    hasSignificantChange = true;
  }

  // Auto mode: thay đổi ngay lập tức
  if (autoMode != prevAutoMode) {
    hasSignificantChange = true;
  }

  // Pump speed: thay đổi >= 5% mới gửi
  if (abs(pumpSpeed - prevPumpSpeed) >= 5) {
    hasSignificantChange = true;
  }

  // Heartbeat: Gửi bắt buộc mỗi 5 phút để xác nhận còn online
  bool shouldForcedPublish =
      (millis() - lastForcedPublish > FORCED_PUBLISH_INTERVAL);

  // Chỉ publish khi có thay đổi HOẶC đến lúc heartbeat
  if (!hasSignificantChange && !shouldForcedPublish) {
#if MQTT_DEBUG
    Serial.println("📊 No significant change, skipping sensor publish");
#endif
    return;
  }

  // Tạo tài liệu JSON
  StaticJsonDocument<256> doc;
  doc["timestamp"] = time(nullptr);
  doc["soil_moisture"] = soilMoisture;
  doc["rain_status"] = rainStatus;
  doc["pump_status"] = pumpStatus;
  doc["auto_mode"] = autoMode;
  doc["pump_speed"] = pumpSpeed;

  // Đánh dấu nếu là heartbeat
  if (shouldForcedPublish && !hasSignificantChange) {
    doc["heartbeat"] = true;
  }

  // Chuyển đổi thành chuỗi
  String payload;
  serializeJson(doc, payload);

  // Xuất bản lên HiveMQ Cloud
  bool published =
      mqttClient.publish(TOPIC_SENSOR_DATA, payload.c_str(), MQTT_RETAIN);

#if MQTT_DEBUG
  if (published) {
    if (shouldForcedPublish && !hasSignificantChange) {
      Serial.printf("💓 Heartbeat published: %s\n", payload.c_str());
    } else {
      Serial.printf("📊 Data changed, published: %s\n", payload.c_str());
    }
  } else {
    Serial.println("❌ Failed to publish sensor data");
  }
#endif

  // Cập nhật trạng thái trước đó
  if (published) {
    prevSoilMoisture = soilMoisture;
    prevRainStatus = rainStatus;
    prevPumpStatus = pumpStatus;
    prevAutoMode = autoMode;
    prevPumpSpeed = pumpSpeed;

    if (shouldForcedPublish) {
      lastForcedPublish = millis();
    }
  }
}

void publishPumpStatus(String reason) {
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

  mqttClient.publish(TOPIC_PUMP_STATUS, payload.c_str(),
                     true); // Retain = giữ lại

#if MQTT_DEBUG
  Serial.printf("📊 Published pump status: %s\n", payload.c_str());
#endif
}

void publishSystemStatus() {
  if (!mqttClient.connected()) {
    return;
  }

  // Kiểm tra thay đổi đáng kể
  int currentRSSI = WiFi.RSSI();
  unsigned long currentUptime = millis() / 1000;

  // Chỉ gửi khi:
  // 1. RSSI thay đổi > 5 dBm (chất lượng WiFi thay đổi đáng kể)
  // 2. Hoặc mỗi 10 phút (600 giây)
  bool rssiChanged = abs(currentRSSI - prevWifiRSSI) > 5;
  bool timeToPublish = (currentUptime - prevUptime) > 600; // 10 phút

  if (!rssiChanged && !timeToPublish) {
#if MQTT_DEBUG
    Serial.println("📊 System status unchanged, skipping publish");
#endif
    return;
  }

  StaticJsonDocument<512> doc;
  doc["timestamp"] = time(nullptr);
  doc["client_id"] = MQTT_CLIENT_ID;
  doc["uptime"] = currentUptime;
  doc["free_heap"] = ESP.getFreeHeap();
  doc["wifi_rssi"] = currentRSSI;
  doc["wifi_ssid"] = WiFi.SSID();
  doc["ip_address"] = WiFi.localIP().toString();

  String payload;
  serializeJson(doc, payload);

  bool published = mqttClient.publish(TOPIC_SYSTEM_STATUS, payload.c_str());

#if MQTT_DEBUG
  if (published) {
    Serial.printf("📊 System status published: %s\n", payload.c_str());
  } else {
    Serial.println("❌ Failed to publish system status");
  }
#endif

  if (published) {
    prevWifiRSSI = currentRSSI;
    prevUptime = currentUptime;
  }
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
  // 🛡️ BẢO VỆ BỘ NHỚ: Kiểm tra heap còn trống trước khi thêm log
  if (ESP.getFreeHeap() < MIN_FREE_HEAP) {
    Serial.println("⚠️ LOW MEMORY! Clearing old logs...");
    // Cắt bớt còn một nửa
    int trimPoint = eventLog.length() / 2;
    int lastBr = eventLog.lastIndexOf("<br>", trimPoint);
    if (lastBr != -1) {
      eventLog = eventLog.substring(0, lastBr);
    } else {
      eventLog = ""; // Xóa tất cả nếu không tìm thấy điểm ngắt
    }
    Serial.printf("✅ Logs trimmed. Free heap: %d bytes\n", ESP.getFreeHeap());
  }

  // Ngăn in trùng lặp liên tiếp
  if (message == lastLogMessage) {
    return; // Bỏ qua thêm thông điệp trùng lặp
  }
  lastLogMessage = message; // Cập nhật thông điệp cuối

  // 🛡️ GIỚI HẠN ĐỘ DÀI THÔNG ĐIỆP: Cắt bớt nếu quá dài
  if (message.length() > MAX_STRING_LENGTH) {
    message = message.substring(0, MAX_STRING_LENGTH - 3) + "...";
    Serial.println("⚠️ Message truncated (too long)");
  }

  time_t now = time(nullptr);
  struct tm timeinfo;

  // Dự trữ bộ nhớ cho eventLog để tránh phân mảnh
  if (eventLog.length() == 0) {
    eventLog.reserve(MAX_LOG_LENGTH + 100); // Dự trữ thêm không gian
  }

  // 🛡️ CẤP PHÁT TRƯỚC: Dự trữ không gian trước khi nối chuỗi
  // Note: capacity() là protected trong ESP32, không thể truy cập
  // Chỉ reserve khi cần (dựa vào length)
  if (eventLog.length() < MAX_LOG_LENGTH) {
    eventLog.reserve(eventLog.length() + message.length() + 100);
  }

  if (localtime_r(&now, &timeinfo)) {
    char timeStr[20];
    strftime(timeStr, sizeof(timeStr), "%H:%M:%S", &timeinfo);
    eventLog = "[" + String(timeStr) + "] " + message + "<br>" + eventLog;
  } else {
    eventLog = "[--:--:--] " + message + "<br>" + eventLog;
  }

  // Cắt bớt log tích cực hơn để bảo vệ bộ nhớ
  if (eventLog.length() > LOG_TRIM_LENGTH) {
    int lastIndex = eventLog.lastIndexOf("<br>", LOG_TRIM_LENGTH - 500);
    if (lastIndex != -1) {
      eventLog = eventLog.substring(0, lastIndex);
    }
  }

  // Giám sát sử dụng bộ nhớ
  Serial.printf("📝 %s | Log size: %d/%d bytes | Free heap: %d bytes\n",
                message.c_str(), eventLog.length(), MAX_LOG_LENGTH,
                ESP.getFreeHeap());
}

void readUARTData() {
  if (!UnoSerial.available()) {
    return;
  }

  // Đợi cho đến khi có newline character hoặc timeout
  unsigned long waitStart = millis();
  while (!UnoSerial.find('\n') && (millis() - waitStart < 200)) {
    // Chờ tối đa 200ms cho newline
    delay(10);
  }

  // Nếu timeout, flush buffer và return
  if (millis() - waitStart >= 200) {
    Serial.println("⚠️ UART: No newline found, flushing buffer");
    UnoSerial.flush();
    return;
  }

  // Đọc từ đầu buffer (sau khi đã tìm thấy newline)
  String data = UnoSerial.readStringUntil('\n');
  data.trim();

  // Kiểm tra dữ liệu có hợp lệ không
  if (data.length() == 0) {
    return;
  }

  // Kiểm tra kích thước
  if (data.length() >= MAX_STRING_LENGTH) {
    Serial.printf("⚠️ UART data too long (%d bytes), truncated\n",
                  data.length());
    data = data.substring(0, MAX_STRING_LENGTH - 1);
  }

  // Parse JSON
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
    } else {
      Serial.printf("⚠️ JSON parse error from UART: %s\n", error.c_str());
      Serial.printf("   Raw data: %s\n", data.c_str());
    }
  } else if (data == "PUMP_ON_ACK") {
    pumpStatus = true;
    addLog("Pump ON acknowledged");
  } else if (data == "PUMP_OFF_ACK") {
    pumpStatus = false;
    addLog("Pump OFF acknowledged");
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
  if (millis() - lastCheck < 30000)
    return;
  lastCheck = millis();

  int currentHour = getCurrentHour();

  if (currentHour == -1)
    return;

  // Logic tưới tự động
  if ((currentHour == 6 || currentHour == 17) && rainStatus == 0 &&
      soilMoisture < 40) {
    if (!pumpStatus) {
      UnoSerial.println("PUMP_ON");
      pumpStatus = true;
      String reason = "Auto: Hour " + String(currentHour) + ", Soil " +
                      String(soilMoisture) + "%";
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
  Serial.printf("📖 Loaded setup flag: %s\n",
                setupCompleted ? "COMPLETED" : "NOT COMPLETED");
}

void saveSetupFlag(bool completed) {
  EEPROM.write(SETUP_FLAG_ADDR, completed ? 1 : 0);
  EEPROM.commit();
  setupCompleted = completed;
  Serial.printf("💾 Setup flag saved: %s\n",
                completed ? "COMPLETED" : "NOT COMPLETED");
}

void handleResetSetup() {
  // Đặt lại thông tin WiFi
  for (int i = 0; i < 64; i++) {
    EEPROM.write(SSID_ADDR + i, 0);
    EEPROM.write(PASS_ADDR + i, 0);
  }
  EEPROM.write(SETUP_FLAG_ADDR, 0);
  EEPROM.commit();

  // Đặt lại các biến
  savedSSID = "";
  savedPassword = "";
  wifiConfigured = false;
  setupCompleted = false;

  Serial.println("🔄 Setup reset completed");
  addLog("Setup configuration reset");
}

// 🛡️ ========== MEMORY MONITORING ENDPOINT ==========
void handleMemoryStats() {
  StaticJsonDocument<512> doc;

  // Thông tin bộ nhớ
  doc["free_heap"] = ESP.getFreeHeap();
  doc["total_heap"] = ESP.getHeapSize();
  doc["min_free_heap"] = ESP.getMinFreeHeap();
  doc["max_alloc_heap"] = ESP.getMaxAllocHeap();

  // Sử dụng bộ nhớ log
  doc["log_size"] = eventLog.length();
  // Note: capacity() là protected trong ESP32, không thể truy cập
  doc["log_max"] = MAX_LOG_LENGTH;

  // Bộ đệm MQTT
  doc["mqtt_buffer_size"] = MQTT_BUFFER_SIZE;
  doc["mqtt_connected"] = mqttConnected;

  // Thông tin hệ thống
  doc["uptime_seconds"] = millis() / 1000;
  doc["wifi_rssi"] = WiFi.RSSI();
  doc["wifi_connected"] = (WiFi.status() == WL_CONNECTED);

  // Trạng thái sức khỏe bộ nhớ
  int freeHeap = ESP.getFreeHeap();
  String status;
  if (freeHeap > 100000) {
    status = "EXCELLENT";
  } else if (freeHeap > 50000) {
    status = "GOOD";
  } else if (freeHeap > MIN_FREE_HEAP) {
    status = "WARNING";
  } else {
    status = "CRITICAL";
  }
  doc["memory_status"] = status;

  String json;
  serializeJson(doc, json);
  server.send(200, "application/json", json);

  Serial.println("📊 Memory stats requested via API");
}
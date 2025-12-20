#include <Arduino.h>
#include <ArduinoJson.h>
#include <SoftwareSerial.h> // Thêm thư viện SoftwareSerial

// Cảm biến và relay
const int RAIN_SENSOR_PIN = A1; // Chuyển sang analog để đọc % khả năng mưa
const int SOIL_SENSOR_PIN = A0;
const int PUMP_RELAY_PIN = 8;

// SoftwareSerial với ESP32 (chân 4 = RX, chân 5 = TX)
SoftwareSerial ESP32Serial(4, 5); // RX, TX

// LED báo trạng thái (tùy chọn)
const int STATUS_LED = 13;

// Biến trạng thái
bool pumpState = false;
bool lastPumpState = false;
unsigned long lastSendTime = 0;
unsigned long lastConnectionCheck = 0;
const long SEND_INTERVAL = 2000;
const long CONNECTION_TIMEOUT = 10000; // 10 giây không có data = mất kết nối
bool espConnected = false;

// Calibration values - điều chỉnh theo cảm biến thực tế
// Độ ẩm đất - CALIBRATED for capacitive sensor
// HƯỚNG DẪN: Nếu cảm biến vẫn đọc sai, hãy:
// 1. Để cảm biến trong không khí và xem giá trị raw trong Serial Monitor
// 2. Nhúng cảm biến vào nước và xem giá trị raw
// 3. Cập nhật DRY_VALUE và WET_VALUE theo giá trị thực tế
const int DRY_VALUE = 700; // Khô (trong không khí) - giá trị analog khi khô
const int WET_VALUE =
    350; // Ướt (trong nước) - giá trị analog khi ướt hoàn toàn

// Cảm biến mưa (analog)
const int RAIN_DRY_VALUE =
    1000; // Giá trị khi khô (không mưa) - điều chỉnh sau khi test
const int RAIN_WET_VALUE =
    300; // Giá trị khi ướt (mưa nhiều) - điều chỉnh sau khi test

void setup() {
  Serial.begin(9600);      // For debugging
  ESP32Serial.begin(9600); // SoftwareSerial với ESP32

  // RAIN_SENSOR_PIN (A1) không cần pinMode vì là analog input
  pinMode(PUMP_RELAY_PIN, OUTPUT);
  pinMode(STATUS_LED, OUTPUT);
  digitalWrite(PUMP_RELAY_PIN, HIGH); // Tắt bơm

  Serial.println("🚀 Arduino Uno R3 - Smart Irrigation System");
  Serial.println("📤 Send interval: 2 seconds");
  Serial.println("📡 SoftwareSerial on pins: RX=4, TX=5");
  Serial.println("⏳ Waiting for ESP32 connection...");

  // Nhấp nháy LED 3 lần khi khởi động
  for (int i = 0; i < 3; i++) {
    digitalWrite(STATUS_LED, HIGH);
    delay(200);
    digitalWrite(STATUS_LED, LOW);
    delay(200);
  }
}

void loop() {
  // 1. Kiểm tra kết nối ESP32
  checkESPConnection();

  // 2. Điều khiển Relay theo lệnh từ ESP32
  controlPumpFromESP();

  // 3. Đọc cảm biến và gửi dữ liệu định kỳ
  if (millis() - lastSendTime >= SEND_INTERVAL) {
    sendSensorData();
    lastSendTime = millis();

    // Cập nhật thời gian kết nối
    if (espConnected) {
      lastConnectionCheck = millis();
    }
  }

  // 4. Debug status mỗi 10 giây
  debugStatus();

  // 5. Điều khiển LED trạng thái
  controlStatusLED();

  delay(10);
}

// Đọc cảm biến mưa - trả về % khả năng mưa (0-100%)
int readRainSensor() {
  // Đọc nhiều lần để lấy giá trị trung bình và ổn định
  int readings = 5;
  int total = 0;

  for (int i = 0; i < readings; i++) {
    total += analogRead(RAIN_SENSOR_PIN);
    delay(2);
  }

  int rawValue = total / readings;

  // Giới hạn giá trị trong khoảng calibration
  if (rawValue > RAIN_DRY_VALUE)
    rawValue = RAIN_DRY_VALUE;
  if (rawValue < RAIN_WET_VALUE)
    rawValue = RAIN_WET_VALUE;

  // Chuyển đổi thành phần trăm: giá trị thấp = ướt = % cao
  // Khi khô: raw = 1000 -> 0%
  // Khi ướt: raw = 300 -> 100%
  int percentage = map(rawValue, RAIN_DRY_VALUE, RAIN_WET_VALUE, 0, 100);
  return constrain(percentage, 0, 100);
}

// Đọc cảm biến độ ẩm đất (cải thiện độ ổn định)
int readSoilMoisture() {
  // Đọc nhiều lần để lấy giá trị trung bình
  int readings = 5;
  int total = 0;

  for (int i = 0; i < readings; i++) {
    total += analogRead(SOIL_SENSOR_PIN);
    delay(2);
  }

  int rawValue = total / readings;

  // Debug: In giá trị raw để hiệu chỉnh
  static unsigned long lastDebugPrint = 0;
  if (millis() - lastDebugPrint >= 5000) { // Mỗi 5 giây
    Serial.print("Soil Raw Value: ");
    Serial.print(rawValue);
    Serial.print(" (DRY=");
    Serial.print(DRY_VALUE);
    Serial.print(", WET=");
    Serial.print(WET_VALUE);
    Serial.println(")");
    lastDebugPrint = millis();
  }

  // Giới hạn giá trị
  if (rawValue > DRY_VALUE)
    rawValue = DRY_VALUE;
  if (rawValue < WET_VALUE)
    rawValue = WET_VALUE;

  // Chuyển đổi thành phần trăm (đảo ngược)
  int percentage = map(rawValue, DRY_VALUE, WET_VALUE, 0, 100);
  return constrain(percentage, 0, 100);
}

// Kiểm tra kết nối ESP32
void checkESPConnection() {
  static unsigned long lastDataReceived = 0;

  if (ESP32Serial.available()) {
    lastDataReceived = millis();
    if (!espConnected) {
      espConnected = true;
      Serial.println("✅ ESP32 Connected via SoftwareSerial!");
    }
  }

  // Nếu quá 10 giây không nhận được data
  if (espConnected && (millis() - lastDataReceived > CONNECTION_TIMEOUT)) {
    espConnected = false;
    Serial.println("⚠️ ESP32 Connection Lost!");

    // Tự động tắt bơm khi mất kết nối (an toàn)
    if (pumpState) {
      digitalWrite(PUMP_RELAY_PIN, HIGH);
      pumpState = false;
      Serial.println("🔴 Auto Pump OFF (safety)");
    }
  }
}

// Điều khiển bơm từ ESP32
void controlPumpFromESP() {
  if (ESP32Serial.available()) {
    String command = ESP32Serial.readStringUntil('\n');
    command.trim();

    // Ghi nhận có data từ ESP32
    lastConnectionCheck = millis();

    Serial.print("📨 From ESP32: ");
    Serial.println(command);

    if (command == "PUMP_ON") {
      digitalWrite(PUMP_RELAY_PIN, LOW); // Bật relay (LOW kích hoạt relay)
      pumpState = true;
      ESP32Serial.println("PUMP_ON_ACK");
      Serial.println("🔴 Pump ON");
    } else if (command == "PUMP_OFF") {
      digitalWrite(PUMP_RELAY_PIN, HIGH); // Tắt relay (HIGH tắt relay)
      pumpState = false;
      ESP32Serial.println("PUMP_OFF_ACK");
      Serial.println("🟢 Pump OFF");
    }
    // Xử lý các lệnh khác nếu có
    else if (command.startsWith("TEST")) {
      ESP32Serial.println("UNO_OK");
      Serial.println("✅ Test command received");
    } else if (command == "GET_STATUS") {
      sendSensorData(); // Gửi ngay dữ liệu
    }
  }
}

// Gửi dữ liệu cảm biến
void sendSensorData() {
  StaticJsonDocument<200> doc;

  doc["soil_moisture"] = readSoilMoisture();
  doc["rain"] = readRainSensor();
  doc["pump_status"] = pumpState;

  String jsonString;
  serializeJson(doc, jsonString);

  // Gửi đến ESP32 qua SoftwareSerial
  ESP32Serial.println(jsonString);

  // Debug
  Serial.print("📤 To ESP32: ");
  Serial.println(jsonString);
}

// Hàm debug trạng thái
void debugStatus() {
  static unsigned long lastDebug = 0;
  if (millis() - lastDebug >= 10000) {
    Serial.println("\n=== ARDUINO UNO STATUS ===");
    Serial.print("Soil Moisture: ");
    Serial.print(readSoilMoisture());
    Serial.println("%");
    Serial.print("Rain Probability: ");
    Serial.print(readRainSensor());
    Serial.println("%");
    Serial.print("Pump State: ");
    Serial.println(pumpState ? "ON" : "OFF");
    Serial.print("ESP32 Connection: ");
    Serial.println(espConnected ? "✅ CONNECTED" : "❌ DISCONNECTED");
    Serial.print("Uptime: ");
    Serial.print(millis() / 1000);
    Serial.println(" seconds");
    Serial.println("==========================\n");
    lastDebug = millis();
  }
}

// Điều khiển LED trạng thái
void controlStatusLED() {
  static unsigned long lastBlink = 0;
  static bool ledState = false;

  if (!espConnected) {
    // Nhấp nháy nhanh khi mất kết nối (200ms)
    if (millis() - lastBlink > 200) {
      digitalWrite(STATUS_LED, ledState);
      ledState = !ledState;
      lastBlink = millis();
    }
  } else if (pumpState) {
    // Bật sáng khi bơm đang chạy
    digitalWrite(STATUS_LED, HIGH);
  } else {
    // Nhấp nháy chậm khi kết nối bình thường (1000ms)
    if (millis() - lastBlink > 1000) {
      digitalWrite(STATUS_LED, ledState);
      ledState = !ledState;
      lastBlink = millis();
    }
  }
}

// Hàm khẩn cấp - tắt bơm ngay lập tức
void emergencyStop() {
  digitalWrite(PUMP_RELAY_PIN, HIGH);
  pumpState = false;
  ESP32Serial.println("EMERGENCY_STOP");
  Serial.println("🚨 EMERGENCY STOP - Pump OFF");
}

// Hàm test SoftwareSerial (có thể gọi từ Serial Monitor)
void testSerial() {
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();

    if (cmd == "test") {
      Serial.println("Testing SoftwareSerial...");
      ESP32Serial.println("HELLO_ESP32");
      Serial.println("Sent: HELLO_ESP32");
    } else if (cmd == "pumpon") {
      digitalWrite(PUMP_RELAY_PIN, LOW);
      pumpState = true;
      Serial.println("Manual Pump ON");
    } else if (cmd == "pumpoff") {
      digitalWrite(PUMP_RELAY_PIN, HIGH);
      pumpState = false;
      Serial.println("Manual Pump OFF");
    }
  }
}

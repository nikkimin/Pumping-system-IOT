#include <ArduinoJson.h>
#include <SoftwareSerial.h>

// Cảm biến và relay
const int RAIN_SENSOR_PIN = 7;
const int SOIL_SENSOR_PIN = A0;
const int PUMP_RELAY_PIN = 8;

// UART với ESP32 (Chân 4-RX, 5-TX)
SoftwareSerial ESP32Serial(4, 5); // RX, TX

// Biến định thời
const long SEND_INTERVAL = 5000;
long lastSendTime = 0;
StaticJsonDocument<256> doc;

// Biến trạng thái
bool pumpState = false;
unsigned long lastDataSend = 0;

void setup() {
  Serial.begin(9600);
  ESP32Serial.begin(9600); // Phải khớp với ESP32 (9600)
  
  pinMode(RAIN_SENSOR_PIN, INPUT);
  pinMode(PUMP_RELAY_PIN, OUTPUT);
  
  // Đảm bảo máy bơm TẮT khi khởi động
  digitalWrite(PUMP_RELAY_PIN, HIGH);
  pumpState = false;
  
  Serial.println("🚀 Arduino Uno Ready - UART on pins 2,3");
  Serial.println("📊 Baud rate: 9600");
  Serial.println("⏰ Send interval: 5 seconds");
}

void loop() {
  // 1. Điều khiển Relay theo lệnh từ ESP32
  controlPumpFromESP32();
  
  // 2. Đóng gói và gửi dữ liệu định kỳ
  if (millis() - lastSendTime >= SEND_INTERVAL) {
    sendDataToESP32();
    lastSendTime = millis();
  }
  
  // 3. Debug hiển thị trạng thái mỗi 10 giây
  if (millis() - lastDataSend >= 10000) {
    debugStatus();
    lastDataSend = millis();
  }
}

/**
 * Đọc cảm biến mưa (Digital).
 * Trả về 1 nếu trời đang mưa (có nước), 0 nếu trời khô.
 */
int readRainStatus() {
  int rainStatus = digitalRead(RAIN_SENSOR_PIN);
  
  // LOW = Mưa, HIGH = Khô (tùy thuộc vào cảm biến của bạn)
  if (rainStatus == LOW) { 
    return 1; // 1 = Đang Mưa
  } else {
    return 0; // 0 = Khô ráo
  }
}

/**
 * Đọc cảm biến độ ẩm đất (Analog) và chuyển thành giá trị phần trăm (0-100%).
 * Calibrate giá trị theo cảm biến thực tế của bạn
 */
int readSoilMoisture() {
  // Đọc giá trị thô từ chân Analog A0
  int rawValue = analogRead(SOIL_SENSOR_PIN);
  
  // Debug giá trị thô
  Serial.print("📈 Soil sensor raw: ");
  Serial.println(rawValue);
  
  // Calibrate các giá trị này theo cảm biến của bạn:
  // - Giá trị cao = khô (trong không khí)
  // - Giá trị thấp = ẩm (trong nước)
  int dryValue = 1000;   // Giá trị khi cảm biến khô (trong không khí)
  int wetValue = 300;    // Giá trị khi cảm biến ẩm (trong nước)
  
  // Đảm bảo giá trị trong khoảng hợp lệ
  if (rawValue > dryValue) rawValue = dryValue;
  if (rawValue < wetValue) rawValue = wetValue;
  
  // Chuyển đổi thành phần trăm (đảo ngược vì giá trị cao = khô)
  int percentageMoisture = map(rawValue, dryValue, wetValue, 0, 100);
  
  // Giới hạn giá trị trong khoảng 0-100
  percentageMoisture = constrain(percentageMoisture, 0, 100);
  
  return percentageMoisture;
}

/**
 * Điều khiển Relay máy bơm dựa trên lệnh nhận được từ ESP32.
 */
void controlPumpFromESP32() {
  // Kiểm tra dữ liệu đến trên SoftwareSerial
  if (ESP32Serial.available() > 0) {
    String command = ESP32Serial.readStringUntil('\n');
    command.trim();
    
    Serial.print("📨 Received from ESP32: ");
    Serial.println(command);
    
    if (command == "PUMP_ON") {
      digitalWrite(PUMP_RELAY_PIN, LOW); // BẬT bơm (LOW kích relay)
      pumpState = true;
      ESP32Serial.println("PUMP_ON_ACK"); // Phản hồi xác nhận
      Serial.println("🔴 PUMP ON - Relay activated");
    } 
    else if (command == "PUMP_OFF") {
      digitalWrite(PUMP_RELAY_PIN, HIGH); // TẮT bơm (HIGH tắt relay)
      pumpState = false;
      ESP32Serial.println("PUMP_OFF_ACK"); // Phản hồi xác nhận
      Serial.println("🟢 PUMP OFF - Relay deactivated");
    }
    else {
      Serial.print("❌ Unknown command: ");
      Serial.println(command);
    }
  }
}

/**
 * Gửi dữ liệu cảm biến đến ESP32
 */
void sendDataToESP32() {
  // 1. Đọc tất cả giá trị cảm biến
  int moistureValue = readSoilMoisture();
  int rainStatus = readRainStatus();
  
  // 2. Điền dữ liệu vào đối tượng JSON
  doc["time"] = millis(); 
  doc["soil_moisture"] = moistureValue;
  doc["rain"] = rainStatus;
  doc["pump_status"] = pumpState ? 1 : 0;
  
  // 3. Chuyển JSON thành chuỗi và gửi
  String jsonString;
  serializeJson(doc, jsonString);
  
  // Gửi chuỗi JSON đến ESP32 qua SoftwareSerial
  ESP32Serial.println(jsonString);
  
  // Debug trên Serial Monitor
  Serial.print("📤 Sent to ESP32: ");
  Serial.println(jsonString);
}

/**
 * Hiển thị trạng thái debug
 */
void debugStatus() {
  Serial.println("=== SYSTEM STATUS ===");
  Serial.print("Soil Moisture: ");
  Serial.print(readSoilMoisture());
  Serial.println("%");
  
  Serial.print("Rain Status: ");
  Serial.println(readRainStatus() ? "RAINING" : "DRY");
  
  Serial.print("Pump State: ");
  Serial.println(pumpState ? "ON" : "OFF");
  
  Serial.print("Relay Pin: ");
  Serial.println(digitalRead(PUMP_RELAY_PIN) == LOW ? "LOW (ON)" : "HIGH (OFF)");
  
  Serial.println("=====================");
}
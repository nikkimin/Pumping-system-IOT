# Pumping-system-IOT
Hệ thống máy bơm tự động điều khiển bằng IOT
hệ thống tưới cây tự động trong đó có UART cho arduino uno r3 với Esp32:
-Arduino Uno: dùng để kết nối với 2 cảm biến gồm relay cảm biến mưa và cảm biến độ ẩm đất và kết nối với mạch relay kết nối máy bơm.
 +) mạch cảm biến mưa: là một bảng 2 chân nối với một relay cảm biến, arduino uno sẽ đọc cảm biến từ relay đó.
  +) Cảm biến độ ẩm đất 3 chân.
  +) mạch relay máy bơm loại 5VDC
-Esp32: chịu trách nhiệm nhận tín hiệu từ Arduino và kết nối wifi với dashboard giao diện html bằng phương thức http, cùng với đó hiển thị thêm đồng hồ thời gian thực build-in esp trên để áp dụng điều kiện auto. Người dùng có thể chọn các chức năng sau: đổi chế độ auto/manual, khi manual sẽ hiển thị thêm thanh slider điều chỉnh tốc độ dòng chảy nước máy bơm. Còn khi auto sẽ theo thời gian thực 6h or 17h < cảm biến mưa không mưa < độ ẩm đất khô theo ưu tiên từ thấp đến cao. Khi có thay đổi bất kì sẽ lưu dữ liệu vào một list box trên giao diện.

Quy ước chung		
		
Arduino Uno – Cảm biến & Relay		
Chân kết nối:		
rainSensorPin = 7 → cảm biến mưa (digital relay sensor).		
soilSensorPin = A0 → cảm biến độ ẩm đất (analog).		
pumpRelayPin = 8 → relay máy bơm.		
EspSerial (SoftwareSerial RX=2, TX=3) → UART giao tiếp với ESP32.		
Biến:		
rainState (0 = không mưa, 1 = mưa).		
soilValue (0–1023).		
pumpState (true = ON, false = OFF).		
ESP32 – UART & Đồng hồ thời gian thực		
Chân UART2:		
RXD2 = 16, TXD2 = 17.		
UnoSerial (HardwareSerial) → giao tiếp với Uno.		
Biến trạng thái:		
rain (0/1).		
soil (0–1023). đất càng khô -> số càng nhỏ		
pump (true/false).		
modeAuto (true = Auto, false = Manual).		
manualFlowPercent (0–100%).		
autoHourSelected (6 hoặc 17).		
Đồng hồ NTP:		
getCurrentHour() → trả về giờ hiện tại (0–23).		
getCurrentTimeStr() → chuỗi thời gian hiển thị.		
Ngưỡng độ ẩm đất:		
soilDryThreshold = 700 (có thể chỉnh thực tế).		
		
Giao thức UART (JSON)		
Uno → ESP32 (telemetry):		
{"rain":0,"soil":512,"pump":1}		
ESP32 → Uno (command):		
{"cmd":"pump","state":1}		
		
Uno: đọc cảm biến, điều khiển relay, gửi dữ liệu JSON.		
ESP32: nhận dữ liệu, xử lý Auto/Manual, đồng bộ Blynk, hiển thị đồng hồ.		
HTML: Tạo trang web điều khiển hệ thống		
UART JSON: giao tiếp hai chiều, dễ debug và mở rộng.		


## MQTT Protocol - HiveMQ Cloud Integration		
		
### MQTT Broker Configuration		
**Broker:** HiveMQ Private Cloud		
**Host:** e947a9991cc442918fe1e94b5268b686.s1.eu.hivemq.cloud		
**Port:** 8883 (TLS/SSL cho ESP32), 8884 (WebSocket Secure cho Web)		
**Authentication:** Username/Password (pumpuser/pump123456A)		
**Security:** TLS/SSL + Certificate Authentication		
**QoS Level:** 0 (Fastest, no guarantee)		
**Keepalive:** 90 seconds (ESP32), 30 seconds (Web)		
		
### MQTT Topics Architecture		
		
#### 📤 PUBLISH Topics (ESP32 → Cloud)		
ESP32 gửi dữ liệu cảm biến và trạng thái hệ thống lên cloud:		
		
**1. Dữ liệu Cảm biến** → `smartirrigation/sensor/data`		
Interval: 10 giây		
Payload JSON:		
```json		
{		
  "timestamp": 1702644000,		
  "soil_moisture": 512,		
  "rain_status": 0,		
  "pump_status": true,		
  "auto_mode": true,		
  "pump_speed": 50		
}		
```		
		
**2. Trạng thái Máy bơm** → `smartirrigation/pump/status`		
Interval: Khi có thay đổi		
Retained: true (lưu trạng thái cuối)		
Payload JSON:		
```json		
{		
  "timestamp": 1702644000,		
  "status": "ON",		
  "speed": 75,		
  "mode": "MANUAL",		
  "reason": "Manual control: ON"		
}		
```		
		
**3. Trạng thái Hệ thống** → `smartirrigation/system/status`		
Interval: 60 giây		
Payload JSON:		
```json		
{		
  "timestamp": 1702644000,		
  "client_id": "ESP32_SmartIrrigation_003",		
  "uptime": 3600,		
  "free_heap": 150000,		
  "wifi_rssi": -45,		
  "wifi_ssid": "MyWiFi",		
  "ip_address": "192.168.1.100"		
}		
```		
		
**4. System Log** → `smartirrigation/system/log`		
Interval: Khi có sự kiện		
Payload JSON:		
```json		
{		
  "timestamp": 1702644000,		
  "message": "Pump turned ON - Auto watering",		
  "level": "INFO"		
}		
```		
		
#### 📥 SUBSCRIBE Topics (ESP32 ← Cloud)		
ESP32 nhận lệnh điều khiển từ web interface qua cloud:		
		
**1. Điều khiển Máy bơm** ← `smartirrigation/pump/control`		
Bật máy bơm:		
```json		
{		
  "command": "turn_on",		
  "speed": 75		
}		
```		
Tắt máy bơm:		
```json		
{		
  "command": "turn_off"		
}		
```		
		
**2. Chuyển chế độ** ← `smartirrigation/mode/control`		
Chế độ Auto:		
```json		
{		
  "mode": "AUTO"		
}		
```		
Chế độ Manual:		
```json		
{		
  "mode": "MANUAL",		
  "speed": 50		
}		
```		
		
**3. Cập nhật cấu hình** ← `smartirrigation/config/update`		
Dành cho các cập nhật cấu hình trong tương lai		
		
### Lệnh Test MQTT (Serial Monitor)		
Khi nạp code vào ESP32, có thể test MQTT bằng các lệnh:		
- `debug` hoặc `mqtt` hoặc `test` → Chạy công cụ chẩn đoán MQTT		
- `help` → Hiển thị danh sách lệnh		
		
### MQTT Client IDs		
- **ESP32:** `ESP32_SmartIrrigation_003` (cố định)		
- **Web Interface:** `WebClient_<random>` (tự động tạo mỗi lần load)		
⚠️ Lưu ý: Nếu dùng nhiều ESP32, cần đổi số cuối (_001, _002, _003...)		
		
### Debug & Troubleshooting		
- Auto debug: Tự động chạy sau 3 lần kết nối thất bại		
- Manual debug: Gõ `debug` trong Serial Monitor		
- Reconnect: Exponential backoff (5s → 10s → 20s → 30s max)		
- MQTT buffer size: 1024 bytes		
		
---		
		
upload: double click file upload_all.bat
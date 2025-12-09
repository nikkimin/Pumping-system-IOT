# Hướng Dẫn Build và Upload Firmware

## ✅ Lỗi đã được sửa

Tôi đã sửa 2 lỗi biên dịch:

### 1. Lỗi `mqttClientId` không tồn tại
**Đã sửa:** Thay `mqttClientId` bằng `MQTT_CLIENT_ID` trong `publishSystemStatus()`

### 2. Warning `MQTT_KEEPALIVE` redefined  
**Đã sửa:** Đổi tên thành `MQTT_KEEPALIVE_INTERVAL` để tránh xung đột với thư viện PubSubClient

---

## 🔧 Cách Build và Upload

### Option 1: Sử dụng VS Code + PlatformIO Extension

1. **Mở VS Code**
2. **Mở folder:** `d:\Pumping-system-IOT\Esp`
3. **Build:** Click nút ✔️ (Build) ở thanh dưới cùng
4. **Upload:** Click nút → (Upload) ở thanh dưới cùng
5. **Monitor:** Click nút 🔌 (Serial Monitor)

### Option 2: Sử dụng Command Line

Nếu PlatformIO CLI đã cài đặt:

```powershell
cd d:\Pumping-system-IOT\Esp
platformio run --target upload
platformio device monitor
```

### Option 3: Sử dụng Arduino IDE (Nếu không có PlatformIO)

1. Copy tất cả code từ `src/main.cpp`
2. Mở Arduino IDE
3. Tạo file `.ino` mới
4. Paste code vào
5. Copy `hivemq_config.h` và `hivemq_cert.h` vào cùng folder
6. Cài đặt các thư viện cần thiết:
   - WiFiManager
   - ArduinoJson
   - PubSubClient
7. Chọn board: ESP32 Dev Module
8. Upload

---

## 📋 Checklist Trước khi Upload

- [x] Đã sửa lỗi `mqttClientId`
- [x] Đã sửa lỗi `MQTT_KEEPALIVE` redefinition
- [ ] Đảm bảo thông tin trong `hivemq_config.h` đúng:
  - `HIVEMQ_HOST` = cluster URL của bạn
  - `MQTT_USERNAME` = username từ HiveMQ Console
  - `MQTT_PASSWORD` = password từ HiveMQ Console
- [ ] ESP32 đã kết nối qua USB
- [ ] Đã chọn đúng COM port

---

## ❓ Nếu Build Thất Bại

Hãy gửi lại **toàn bộ error message** để tôi hỗ trợ thêm!

---

**Code đã sẵn sàng để build! 🚀**

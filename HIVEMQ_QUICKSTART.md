# 🌐 Hướng Dẫn Nhanh: HiveMQ Cloud Integration

## 🚀 Quick Start

### Bước 1: Tạo HiveMQ Cloud Account
1. Truy cập: https://console.hivemq.cloud/
2. Đăng ký tài khoản miễn phí
3. Tạo cluster mới (chọn FREE plan)
4. Chọn region gần Việt Nam (Singapore/Tokyo)

### Bước 2: Lấy thông tin kết nối
Sau khi cluster **RUNNING**, lấy:
- **Host**: `xxxxxxxx.s1.eu.hivemq.cloud`
- **Port**: `8883`
- **Username** và **Password**: Tạo trong **Access Management**

### Bước 3: Cập nhật cấu hình
Mở file `Esp/src/hivemq_config.h` và thay đổi:

```cpp
const char* HIVEMQ_HOST = "your_cluster_url.s1.eu.hivemq.cloud";
const char* MQTT_USERNAME = "your_username";
const char* MQTT_PASSWORD = "your_password";
```

### Bước 4: Upload code
```bash
cd Esp
pio run --target upload
pio device monitor
```

**Kỳ vọng trong Serial Monitor:**
```
✅ Root CA certificate loaded (ISRG Root X1)
🔐 MQTT Broker: 10f287a7e9ba424b88c279464c967aa4.s1.eu.hivemq.cloud:8883
👤 Client ID: ESP32_SmartIrrigation_001
✅ MQTT Connected!
📤 Subscribed to: smartirrigation/pump/control
```

### Bước 5: Test kết nối
1. Dùng **MQTT Explorer** hoặc **HiveMQ Web Client**
2. Subscribe to: `smartirrigation/#`
3. Xem dữ liệu real-time

---

## 📊 MQTT Topics

### Publish (ESP32 → Cloud)
- `smartirrigation/sensor/data` - Dữ liệu cảm biến
- `smartirrigation/pump/status` - Trạng thái máy bơm
- `smartirrigation/system/status` - System status
- `smartirrigation/system/log` - Logs

### Subscribe (Cloud → ESP32)
- `smartirrigation/pump/control` - Điều khiển máy bơm
- `smartirrigation/mode/control` - Chuyển AUTO/MANUAL
- `smartirrigation/config/update` - Cập nhật config

---

## 🧪 Test Commands

### Bật máy bơm từ MQTT
```json
Topic: smartirrigation/pump/control
Payload: {"command":"turn_on","speed":80}
```

### Tắt máy bơm
```json
Topic: smartirrigation/pump/control
Payload: {"command":"turn_off"}
```

### Chuyển sang AUTO mode
```json
Topic: smartirrigation/mode/control
Payload: {"mode":"AUTO"}
```

### Chuyển sang MANUAL mode
```json
Topic: smartirrigation/mode/control
Payload: {"mode":"MANUAL","speed":60}
```

---

## 📖 Tài liệu đầy đủ
Xem file hướng dẫn chi tiết tại: `.gemini/antigravity/brain/.../HUONG_DAN_HIVEMQ_CLOUD.md`

## ❓ Troubleshooting

### ESP32 Errors
- **MQTT Connection Failed (rc=-2)**: 
  - ❌ Sai HIVEMQ_HOST hoặc HIVEMQ_PORT
  - ❌ WiFi không ổn định
  - ✅ Check `hivemq_config.h` → HIVEMQ_HOST
  
- **MQTT Connection Failed (rc=4)**: 
  - ❌ Sai MQTT_USERNAME hoặc MQTT_PASSWORD
  - ✅ Check credentials trong `hivemq_config.h`
  
- **MQTT Connection Failed (rc=5)**: 
  - ❌ Credentials OK nhưng không có quyền
  - ✅ Check HiveMQ Console → Access Management

### Web Interface Errors
- **WebSocket connection failed**:
  - ❌ Missing path `/mqtt`
  - ✅ Check browser console for detailed error
  
- **"Đã vượt quá số lần thử kết nối"**:
  - ❌ Cannot connect after 20 attempts
  - ✅ Reload page và check credentials

---

**Happy Coding! 🎉**

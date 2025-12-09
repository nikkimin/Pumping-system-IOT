# 🚀 Quick Testing Guide

## Upload và Test ESP32

### 1. Upload Firmware
```powershell
cd d:\Pumping-system-IOT\Esp
pio run --target upload
pio device monitor
```

### 2. Kỳ vọng trong Serial Monitor
```
✅ Root CA certificate loaded (ISRG Root X1)
✅ MQTT Connected!
📤 Subscribed to: smartirrigation/pump/control
```

**Nếu lỗi:**
- `rc=-2`: Check WiFi hoặc HIVEMQ_HOST
- `rc=4`: Check username/password trong hivemq_config.h
- `rc=5`: Check credentials trong HiveMQ Console

---

## Test Web Interface

### 1. Mở Web Interface
```
http://[ESP32_IP_ADDRESS]/
```

### 2. Mở Browser DevTools (F12) → Console
Kỳ vọng:
```javascript
Connecting to: wss://10f287a7e9ba424b88c279464c967aa4.s1.eu.hivemq.cloud:8884/mqtt
MQTT Connected
```

---

## Test từ HiveMQ Cloud Console

### Xem dữ liệu từ ESP32
1. Vào https://console.hivemq.cloud/
2. Click vào cluster → **Web Client**
3. Click **Connect**
4. Subscribe to: `smartirrigation/#`
5. Bạn sẽ thấy data mỗi 5 giây

### Gửi lệnh điều khiển
**Bật máy bơm:**
```
Topic: smartirrigation/pump/control
Payload: {"command":"turn_on","speed":80}
```

**Tắt máy bơm:**
```
Topic: smartirrigation/pump/control
Payload: {"command":"turn_off"}
```

**Chuyển sang AUTO mode:**
```
Topic: smartirrigation/mode/control
Payload: {"mode":"AUTO"}
```

---

## ✅ Success Checklist

- [ ] ESP32 kết nối MQTT với certificate validation
- [ ] Web interface kết nối qua WebSocket Secure
- [ ] Sensor data hiển thị trên web interface
- [ ] Điều khiển pump từ web interface hoạt động
- [ ] Điều khiển từ HiveMQ Console hoạt động
- [ ] Reconnection tự động khi mất kết nối

---

**Tất cả đã sẵn sàng! Chỉ cần upload và test! 🎉**

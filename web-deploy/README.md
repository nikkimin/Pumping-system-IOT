# Smart Irrigation Web Interface

Web interface để điều khiển hệ thống tưới cây thông minh qua MQTT Cloud.

## 📁 Cấu Trúc

```
web-deploy/
├── index.html          # Giao diện chính
├── script.js           # MQTT client và logic điều khiển  
├── style.css           # Styling
├── netlify.toml        # Config cho Netlify deployment
├── DEPLOY_GUIDE.md     # Hướng dẫn deploy chi tiết
└── README.md           # File này
```

## 🌐 Deploy lên Netlify

Xem hướng dẫn chi tiết tại: [DEPLOY_GUIDE.md](./DEPLOY_GUIDE.md)

**Quick Start:**
1. Push code lên GitHub
2. Kết nối Netlify với GitHub repo
3. Set base directory: `web-deploy`
4. Deploy!

## 🔧 Cấu Hình

### MQTT Broker

Trong file `script.js`, cập nhật thông tin HiveMQ Cloud:

```javascript
const MQTT_HOST = "YOUR_CLUSTER.s1.eu.hivemq.cloud";
const MQTT_USERNAME = "your_username";
const MQTT_PASSWORD = "your_password";
```

## ✨ Tính Năng

- 📊 Hiển thị sensor data real-time (độ ẩm đất, mưa)
- 🎛️ Điều khiển pump ON/OFF
- ⚙️ Chuyển đổi Auto/Manual mode
- 🎚️ Điều chỉnh tốc độ pump (Manual mode)
- 📋 Event log
- 🔄 Auto reconnect khi mất kết nối

## 🧪 Test Local

```bash
# Chạy web server đơn giản
cd web-deploy
python -m http.server 8000

# Mở browser: http://localhost:8000
```

## 📡 MQTT Topics

### Subscribe (Nhận từ ESP32)
- `smartirrigation/sensor/data` - Sensor data
- `smartirrigation/pump/status` - Pump status
- `smartirrigation/system/status` - System info

### Publish (Gửi đến ESP32)
- `smartirrigation/pump/control` - Control pump
- `smartirrigation/mode/control` - Change mode

## 🔒 Bảo Mật

- ✅ HTTPS tự động (Netlify)
- ✅ CSP headers (ngăn XSS)
- ✅ MQTT WebSocket Secure (WSS)
- ✅ Authentication credentials

## 📞 Hỗ Trợ

Gặp vấn đề? Xem [DEPLOY_GUIDE.md](./DEPLOY_GUIDE.md) phần Troubleshooting.

# 🌐 Smart Irrigation Web Interface

Web interface để điều khiển hệ thống tưới cây thông minh IoT qua HiveMQ Cloud.

## 📋 Thông tin

- **Mục đích**: Điều khiển ESP32 từ xa qua Internet
- **Công nghệ**: HTML5, CSS3, JavaScript, MQTT over WebSocket
- **Cloud**: HiveMQ Cloud (WebSocket Secure - port 8884)

## 🚀 Deploy nhanh

### Netlify (Khuyến nghị)
1. Vào https://app.netlify.com/drop
2. Kéo thả folder này vào
3. Nhận URL → Mở và sử dụng!

### Vercel
```bash
npm install -g vercel
vercel --prod
```

### GitHub Pages
```bash
git init
git add .
git commit -m "Deploy web interface"
git branch -M main
git remote add origin https://github.com/your-username/your-repo.git
git push -u origin main
# Bật GitHub Pages trong Settings
```

## 📂 File structure

```
web-deploy/
├── index.html       # Giao diện chính
├── style.css        # Styling
├── script.js        # MQTT logic
├── DEPLOY_GUIDE.md  # Hướng dẫn chi tiết
└── README.md        # File này
```

## 🔧 Cấu hình

Mở `script.js` và kiểm tra:

```javascript
const MQTT_HOST = "your-cluster.s1.eu.hivemq.cloud"; // HiveMQ Host
const MQTT_USERNAME = "your_username";               // MQTT Username
const MQTT_PASSWORD = "your_password";               // MQTT Password
```

## ✨ Tính năng

- ✅ Xem độ ẩm đất real-time
- ✅ Xem trạng thái mưa
- ✅ Điều khiển máy bơm (ON/OFF)
- ✅ Chuyển chế độ AUTO/MANUAL
- ✅ Điều chỉnh tốc độ bơm
- ✅ Nhật ký sự kiện

## 📖 Hướng dẫn chi tiết

Xem file [DEPLOY_GUIDE.md](./DEPLOY_GUIDE.md) để biết:
- Cách deploy lên Netlify/Vercel
- Troubleshooting
- Test kết nối
- Thêm vào Home Screen (mobile)

## 🔐 Bảo mật

⚠️ **Lưu ý**: MQTT credentials hiện đang public trong `script.js`. Để bảo mật hơn:
1. Tạo credentials riêng cho Web trong HiveMQ Console
2. Giới hạn quyền chỉ publish/subscribe topics cần thiết
3. Cân nhắc dùng backend proxy (NodeJS/Python) để ẩn credentials

## 🆘 Support

Nếu gặp vấn đề:
1. Kiểm tra Browser Console (F12) → xem error
2. Kiểm tra HiveMQ Cloud cluster status
3. Xem file DEPLOY_GUIDE.md → Troubleshooting

---

**Made with ❤️ for IoT Automation**

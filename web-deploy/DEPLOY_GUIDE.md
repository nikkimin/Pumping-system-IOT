# 🚀 Hướng Dẫn Deploy Web lên Netlify

## 📁 Chuẩn bị

Folder `web-deploy` đã chứa tất cả file cần thiết:
- ✅ `index.html` - Giao diện web (đã tối ưu cho cloud)
- ✅ `style.css` - CSS styling
- ✅ `script.js` - MQTT logic & control

## 🌐 Cách 1: Drag & Drop (Dễ nhất - 2 phút)

### Bước 1: Truy cập Netlify Drop
1. Mở trình duyệt và vào: https://app.netlify.com/drop
2. Nếu chưa có tài khoản → **Sign up** (miễn phí)
3. Nếu đã có → **Log in**

### Bước 2: Deploy
1. Kéo thả **toàn bộ folder `web-deploy`** vào vùng drop
2. Hoặc click **"browse to upload"** → chọn 3 file trong `web-deploy`
3. Chờ 5-10 giây → Netlify sẽ tự động deploy

### Bước 3: Lấy URL
Sau khi deploy thành công, bạn sẽ nhận được URL kiểu:
```
https://random-name-12345.netlify.app
```

**Lưu lại URL này!** Đây là địa chỉ để truy cập web interface từ bất kỳ đâu.

---

## 💻 Cách 2: Using Netlify CLI (Nâng cao)

### Cài đặt Netlify CLI
```powershell
npm install -g netlify-cli
```

### Login và Deploy
```powershell
# Login vào Netlify
netlify login

# Di chuyển vào folder web-deploy
cd d:\Pumping-system-IOT\web-deploy

# Deploy
netlify deploy --prod

# Chọn:
# - Create & configure a new site? → Yes
# - Publish directory? → . (dấu chấm)
```

### Kết quả
Netlify sẽ trả về:
```
✔ Deploy is live!
Website URL:  https://your-site-name.netlify.app
```

---

## 🧪 Test Kết Nối

### Bước 1: Mở web đã deploy
Truy cập URL vừa nhận được (ví dụ: `https://your-site.netlify.app`)

### Bước 2: Kiểm tra Console
1. Nhấn **F12** → Console tab
2. Kỳ vọng thấy:
```javascript
Connecting to: wss://10f287a7e9ba424b88c279464c967aa4.s1.eu.hivemq.cloud:8884/mqtt
Client ID: WebClient_xxxxxxxx
MQTT Connected
```

### Bước 3: Kiểm tra dữ liệu
- Độ ẩm đất, trạng thái mưa phải cập nhật **real-time**
- Thử **bật/tắt bơm** từ web interface
- Kiểm tra ESP32 Serial Monitor xem có nhận lệnh không

---

## ✅ Checklist Deploy Thành Công

- [ ] Web interface load thành công
- [ ] MQTT kết nối thành công (check Console)
- [ ] Sensor data hiển thị real-time
- [ ] Điều khiển pump từ web hoạt động
- [ ] ESP32 nhận được lệnh từ web

---

## 🔧 Troubleshooting

### ❌ "MQTT Connection Failed"
**Nguyên nhân**: Sai thông tin MQTT hoặc HiveMQ Cloud không chạy

**Giải pháp**:
1. Kiểm tra `script.js` → MQTT_HOST, MQTT_USERNAME, MQTT_PASSWORD
2. Vào HiveMQ Console → kiểm tra cluster status = **RUNNING**
3. Kiểm tra credentials trong **Access Management**

### ❌ "Cannot find 'script.js'"
**Nguyên nhân**: File chưa được upload

**Giải pháp**:
- Đảm bảo cả 3 file (HTML, CSS, JS) đều ở **cùng folder**
- Upload lại toàn bộ 3 file

### ❌ Web load nhưng không có data
**Nguyên nhân**: ESP32 chưa kết nối hoặc chưa publish data

**Giải pháp**:
1. Kiểm tra ESP32 đã kết nối WiFi chưa
2. Kiểm tra ESP32 Serial Monitor:
   ```
   ✅ MQTT Connected!
   📊 Published sensor data: {...}
   ```
3. Dùng HiveMQ Web Client subscribe `smartirrigation/#` để xem data

---

## 🎯 Tính năng sau khi Deploy

✅ **Điều khiển từ xa** - Không cần cùng mạng WiFi với ESP32  
✅ **Nhiều người dùng** - Nhiều người mở web cùng lúc  
✅ **Cross-platform** - Điện thoại, tablet, máy tính  
✅ **Real-time updates** - Dữ liệu cập nhật liên tục  
✅ **Secure** - HTTPS + WSS (WebSocket Secure)  

---

## 📱 Bonus: Thêm vào Home Screen (Mobile)

### Android (Chrome):
1. Mở web → Menu (⋮) → **Add to Home screen**
2. Đặt tên → **Add**
3. Icon sẽ xuất hiện trên màn hình chính

### iOS (Safari):
1. Mở web → Share icon (□↑)
2. Chọn **Add to Home Screen**
3. Đặt tên → **Add**

---

**Happy Deploying! 🎉**

> **Lưu ý**: Nếu thay đổi code, chỉ cần drag & drop lại folder vào Netlify để update!

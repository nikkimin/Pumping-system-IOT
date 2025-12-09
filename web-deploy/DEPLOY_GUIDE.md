# 🌐 Deploy Smart Irrigation Web Interface to Netlify

Hướng dẫn deploy web interface lên Netlify để điều khiển hệ thống tưới cây từ xa qua internet.

## 📋 Yêu Cầu

### 1. Tài khoản GitHub
- Tạo tài khoản tại: https://github.com
- Cài đặt Git trên máy

### 2. Tài khoản Netlify
- Tạo tài khoản miễn phí tại: https://www.netlify.com
- Đăng nhập bằng GitHub (recommended)

### 3. HiveMQ Cloud
- ESP32 đã kết nối thành công với HiveMQ Cloud
- Xác nhận credentials hoạt động

---

## 🚀 Cách Deploy

### **Phương Án 1: Deploy qua GitHub (Recommended)**

#### Bước 1: Push code lên GitHub

```bash
# Di chuyển vào thư mục dự án
cd d:\Pumping-system-IOT

# Khởi tạo Git (nếu chưa có)
git init

# Add remote repository (thay YOUR_USERNAME/YOUR_REPO)
git remote add origin https://github.com/YOUR_USERNAME/Pumping-system-IOT.git

# Add và commit code
git add .
git commit -m "Add web deployment files for Netlify"

# Push lên GitHub
git push -u origin main
```

#### Bước 2: Kết nối Netlify với GitHub

1. Đăng nhập Netlify: https://app.netlify.com
2. Click **"Add new site"** → **"Import an existing project"**
3. Chọn **"Deploy with GitHub"**
4. Authorize Netlify truy cập GitHub
5. Chọn repository: `Pumping-system-IOT`

#### Bước 3: Cấu hình Build Settings

```
Base directory: web-deploy
Build command: (để trống - không cần build)
Publish directory: . (hoặc để trống)
```

#### Bước 4: Deploy

1. Click **"Deploy site"**
2. Đợi build hoàn thành (~30 giây)
3. Nhận URL: `https://random-name-12345.netlify.app`

#### Bước 5: Custom Domain (Optional)

1. Trong Netlify Dashboard → **Site settings** → **Domain management**
2. Click **"Options"** → **"Edit site name"**
3. Đổi tên: `smart-irrigation-pumping` → `https://smart-irrigation-pumping.netlify.app`

---

### **Phương Án 2: Deploy thủ công (Drag & Drop)**

#### Bước 1: Chuẩn bị files

1. Mở folder: `d:\Pumping-system-IOT\web-deploy`
2. Đảm bảo có các files:
   - `index.html`
   - `script.js`
   - `style.css`
   - `netlify.toml`

#### Bước 2: Deploy

1. Truy cập: https://app.netlify.com
2. Click vào khu vực **"Drop your site folder here"**
3. Kéo thả folder `web-deploy` vào
4. Đợi upload và deploy
5. Nhận URL: `https://random-name-12345.netlify.app`

---

## ✅ Kiểm Tra Deployment

### 1. Mở trình duyệt

Truy cập URL Netlify của bạn (ví dụ: `https://smart-irrigation.netlify.app`)

### 2. Kiểm tra Console

Nhấn **F12** → Tab **Console**, kiểm tra:

```
Connecting to: wss://10f287a7e9ba424b88c279464c967aa4.s1.eu.hivemq.cloud:8884/mqtt
MQTT Connected
Subscribed to all topics
```

**Nếu thấy:**
- ✅ `MQTT Connected` → Kết nối thành công
- ❌ `MQTT Connection Failed` → Kiểm tra lại credentials

### 3. Test điều khiển

1. Chuyển sang **Manual mode**
2. Click **"BẬT BƠM"**
3. Kiểm tra Serial Monitor ESP32 - phải thấy:
   ```
   📥 MQTT Message received on [smartirrigation/pump/control]
   PUMP_ON
   ```

---

## 🔧 Troubleshooting

### Lỗi: "WebSocket connection failed"

**Nguyên nhân:** CSP (Content Security Policy) block WebSocket

**Giải pháp:** Kiểm tra file `netlify.toml`:
```toml
connect-src 'self' wss://10f287a7e9ba424b88c279464c967aa4.s1.eu.hivemq.cloud:8884;
```

### Lỗi: "MQTT Connection timeout"

**Nguyên nhân:** Sai MQTT credentials hoặc HiveMQ cluster offline

**Giải pháp:**
1. Kiểm tra `script.js` → dòng 2-7 (credentials)
2. Truy cập HiveMQ Console: https://console.hivemq.cloud
3. Xác nhận cluster **RUNNING**
4. Kiểm tra credentials trong **Access Management**

### Lỗi: "404 Not Found" khi reload page

**Nguyên nhân:** Netlify không biết SPA routing

**Giải pháp:** Đảm bảo có file `netlify.toml` với:
```toml
[[redirects]]
  from = "/*"
  to = "/index.html"
  status = 200
```

---

## 🎉 Hoàn Thành!

Web interface hiện đã deploy lên Netlify. Bạn có thể:
- ✅ Truy cập từ bất kỳ đâu qua internet
- ✅ Điều khiển ESP32 real-time qua MQTT
- ✅ Xem sensor data live
- ✅ Auto HTTPS (Netlify tự động thêm)

### Lưu ý quan trọng:

- 🔒 **HTTPS tự động:** Netlify tự động cấp SSL certificate
- 🆓 **Miễn phí:** 100GB bandwidth/tháng
- 🔄 **Auto deploy:** Mỗi lần push code lên GitHub, Netlify tự động deploy
- 📊 **Analytics:** Xem traffic trong Netlify Dashboard

---

## 📝 Thông Tin Deploy

**URL Production:** `https://YOUR-SITE-NAME.netlify.app`

**MQTT Broker:** `10f287a7e9ba424b88c279464c967aa4.s1.eu.hivemq.cloud:8884`

**Topics Subscribe:**
- `smartirrigation/sensor/data`
- `smartirrigation/pump/status`
- `smartirrigation/system/status`

**Topics Publish:**
- `smartirrigation/pump/control`
- `smartirrigation/mode/control`

---

## 🔄 Cập Nhật Code

Sau khi deploy lần đầu, để cập nhật:

```bash
# Sửa code trong web-deploy/
cd d:\Pumping-system-IOT

# Commit và push
git add web-deploy/
git commit -m "Update web interface"
git push

# Netlify tự động detect và deploy (30-60 giây)
```

---

**Happy Controlling! 🚀**

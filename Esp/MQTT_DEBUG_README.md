# 🔍 MQTT Connection Debug Tool

Tool này giúp bạn debug lỗi **MQTT_CONNECT_UNAUTHORIZED (rc=5)** một cách chi tiết và tự động.

## 📋 Các File Đã Tạo

1. **`mqtt_debug.h`** - Debug tool chính với class `MQTTDebugger`
2. **`debug_example.cpp`** - Ví dụ và hướng dẫn sử dụng
3. **`main.cpp`** (đã cập nhật) - Tích hợp debug tool vào code chính

---

## 🚀 Cách Sử Dụng

### **Cách 1: Debug Thủ Công (KHUYẾN NGHỊ)**

1. Upload firmware lên ESP32
2. Mở Serial Monitor (115200 baud)
3. Gõ command: **`debug`** hoặc **`mqtt`** hoặc **`test`**
4. Tool sẽ chạy và hiển thị kết quả chi tiết

```
📌 Khi nào dùng: Bất kỳ lúc nào bạn muốn kiểm tra MQTT connection
```

### **Cách 2: Debug Tự Động Khi Khởi Động**

1. Mở file `main.cpp`
2. Tìm dòng: `// #define DEBUG_MQTT_ON_STARTUP`
3. Bỏ comment (xóa `//`): `#define DEBUG_MQTT_ON_STARTUP`
4. Upload lại firmware
5. Debug tool sẽ tự động chạy ngay khi ESP32 khởi động

```
📌 Khi nào dùng: Khi bạn muốn debug ngay từ lần chạy đầu tiên
```

### **Cách 3: Debug Tự Động Sau 3 Lần Thất Bại** (ĐÃ ĐƯỢC TÍCH HỢP)

Tool sẽ **TỰ ĐỘNG** chạy diagnostic sau 3 lần kết nối MQTT thất bại.

```
📌 Khi nào xảy ra: Khi ESP32 không thể kết nối MQTT sau 3 lần thử
```

---

## 🔍 Debug Tool Kiểm Tra Gì?

Tool sẽ kiểm tra **7 BƯỚC** theo thứ tự:

### ✅ **STEP 1: WiFi Connection**
- Kiểm tra WiFi đã kết nối chưa
- Hiển thị SSID, IP, RSSI, Gateway, DNS

### ✅ **STEP 2: Time Synchronization (NTP)**
- Kiểm tra thời gian đã đồng bộ chưa
- **QUAN TRỌNG**: TLS/SSL cần thời gian chính xác!

### ✅ **STEP 3: DNS Resolution**
- Thử resolve hostname của HiveMQ
- Hiển thị IP address của broker

### ✅ **STEP 4: TCP Connection**
- Thử kết nối TCP đến HiveMQ port 8883
- Test với mode insecure để loại trừ lỗi certificate

### ✅ **STEP 5: TLS/SSL Handshake**
- Kiểm tra certificate validation
- Test secure connection với Root CA
- Nếu fail → retry với insecure mode để xác định nguyên nhân

### ✅ **STEP 6: MQTT Credentials**
- Kiểm tra Client ID, Username, Password không rỗng
- Phát hiện ký tự đặc biệt có thể gây lỗi
- Hiển thị credentials để verify với HiveMQ Console

### ✅ **STEP 7: MQTT Connection**
- Thử kết nối MQTT với credentials
- Hiển thị return code chi tiết
- Phân tích lỗi và đưa ra giải pháp

---

## ❌ Lỗi rc=5 (UNAUTHORIZED) - Nguyên Nhân & Giải Pháp

### 🔴 **1. DUPLICATE Client ID** (Phổ biến nhất!)

**Nguyên nhân:**
- Một ESP32 khác (hoặc web interface) đang dùng **CÙNG Client ID**
- HiveMQ chỉ cho phép **1 connection/Client ID**

**Giải pháp:**

```bash
# OPTION A: Disconnect duplicate connection
1. Vào HiveMQ Console → Clients
2. Tìm Client ID: ESP32_SmartIrrigation_001
3. Nếu thấy → Click "Disconnect"
4. Reset ESP32
```

```cpp
// OPTION B: Đổi Client ID (KHUYẾN NGHỊ)
// Mở hivemq_config.h dòng 21, đổi thành:
const char* MQTT_CLIENT_ID = "ESP32_SmartIrrigation_002";
// Hoặc dùng số random:
const char* MQTT_CLIENT_ID = "ESP32_SmartIrrigation_7834";
```

### 🔴 **2. Access Control List (ACL) Restriction**

**Nguyên nhân:**
- User không có quyền connect
- Permissions (ACL) chặn Client ID này

**Giải pháp:**

```bash
1. Vào HiveMQ Console → Access Management → Permissions
2. Chọn user 'pumpuser'
3. Đảm bảo có quyền:
   ✓ PUBLISH to: #
   ✓ SUBSCRIBE to: #
   ✓ CONNECT with any Client ID
```

### 🔴 **3. User Account Disabled/Deleted**

**Nguyên nhân:**
- User bị vô hiệu hóa hoặc xóa

**Giải pháp:**

```bash
1. Vào HiveMQ Console → Access Management → Users
2. Kiểm tra user 'pumpuser':
   - Status: ENABLED ✅
   - Nếu không tồn tại → Tạo lại
```

### 🔴 **4. IP Blocked / Rate Limiting**

**Nguyên nhân:**
- IP bị block do firewall
- Quá nhiều kết nối thất bại → bị rate limit

**Giải pháp:**

```bash
1. Vào HiveMQ Console → Security Settings
2. Kiểm tra IP Whitelist
3. Thêm IP của ESP32 (nếu có whitelist)
4. Hoặc tắt rate limiting tạm thời
```

---

## 📊 Ví Dụ Output Của Debug Tool

```
╔════════════════════════════════════════╗
║  🔍 MQTT CONNECTION DEBUG TOOL        ║
╚════════════════════════════════════════╝

📶 STEP 1: Checking WiFi Connection...
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
   ✅ WiFi: CONNECTED
   → SSID: MyWiFi
   → IP Address: 192.168.1.100
   → Signal Strength (RSSI): -45 dBm
   → Gateway: 192.168.1.1
   → DNS: 8.8.8.8

🕒 STEP 2: Checking Time Synchronization (NTP)...
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
   ✅ Time Sync: OK
   → Current Time: Mon Dec  9 14:50:40 2025
   → Timestamp: 1733736640
   → TLS/SSL certificate validation: ENABLED

🌐 STEP 3: Testing DNS Resolution...
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
   → Resolving: 10f287a7e9ba424b88c279464c967aa4.s1.eu.hivemq.cloud
   ✅ DNS Resolution: SUCCESS
   → Resolved IP: 18.195.123.45

🔌 STEP 4: Testing TCP Connection...
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
   → Connecting to: 10f287a7e9ba424b88c279464c967aa4.s1.eu.hivemq.cloud:8883
   ✅ TCP Connection: SUCCESS
   → Connection time: 234 ms
   → Socket is open

🔐 STEP 5: Testing TLS/SSL Handshake...
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
   → Attempting TLS connection to: ...
   → Certificate validation: ENABLED
   ✅ TLS Handshake: SUCCESS
   → TLS connection time: 567 ms
   → Certificate validation: PASSED
   → Secure channel established

🔑 STEP 6: Checking MQTT Credentials...
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
   → Client ID: 'ESP32_SmartIrrigation_001'
   → Username: 'pumpuser'
   → Password: 'pump123456A' (length: 11)
   ✅ Credentials format: OK
   → All fields are non-empty

   📝 IMPORTANT: Verify these credentials match HiveMQ Console:
      1. Go to HiveMQ Cloud Console
      2. Navigate to 'Access Management'
      3. Verify username and password match exactly

📡 STEP 7: Attempting MQTT Connection...
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
   → Broker: 10f287a7e9ba424b88c279464c967aa4.s1.eu.hivemq.cloud:8883
   → Client ID: ESP32_SmartIrrigation_001
   → Username: pumpuser
   → Connecting...

   ❌ MQTT Connection: FAILED!
   → Connection time: 123 ms
   → Return code: 5

   🔍 ERROR ANALYSIS:
   ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
   ❌ MQTT_CONNECT_UNAUTHORIZED (rc=5) ⚠️
   → Client is NOT AUTHORIZED to connect

   🔍 ROOT CAUSES:
   ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
   1. ❌ Client ID is BLOCKED or RESTRICTED
      → Your Client ID: 'ESP32_SmartIrrigation_001'
      → Check: HiveMQ Console → Access Management → Clients

   2. ❌ DUPLICATE Connection (Same Client ID)
      → Another device is using the SAME Client ID
      → HiveMQ allows ONLY ONE connection per Client ID
      → Check: HiveMQ Console → Clients → Active Connections

   [... và nhiều thông tin chi tiết khác ...]

╔════════════════════════════════════════╗
║  ✅ DEBUG COMPLETE                     ║
╚════════════════════════════════════════╝
```

---

## 🎯 Quick Fix Checklist

Khi gặp lỗi rc=5, làm theo checklist này:

- [ ] **Step 1**: Vào HiveMQ Console → Clients
- [ ] **Step 2**: Tìm Client ID `ESP32_SmartIrrigation_001`
- [ ] **Step 3**: Nếu thấy → Disconnect
- [ ] **Step 4**: Reset ESP32 và thử lại
- [ ] **Step 5**: Nếu vẫn lỗi → Đổi Client ID trong `hivemq_config.h`
- [ ] **Step 6**: Kiểm tra Access Management → Permissions
- [ ] **Step 7**: Đảm bảo user có quyền PUBLISH/SUBSCRIBE to `#`

---

## 📝 Serial Commands

Gõ các lệnh sau vào Serial Monitor (115200 baud):

| Command | Mô tả |
|---------|-------|
| `debug` | Chạy MQTT diagnostic tool |
| `mqtt`  | Chạy MQTT diagnostic tool |
| `test`  | Chạy MQTT diagnostic tool |
| `help`  | Hiển thị danh sách commands |

---

## 🔧 Troubleshooting

### Tool không chạy?

1. Kiểm tra đã include `mqtt_debug.h` trong `main.cpp`
2. Kiểm tra Serial Monitor đúng baud rate: **115200**
3. Upload lại firmware

### Tool chạy nhưng không có output?

1. Kiểm tra ESP32 đã kết nối WiFi chưa
2. Kiểm tra Serial cable kết nối đúng
3. Thử reset ESP32

---

## 📚 Tài Liệu Tham Khảo

- [HiveMQ Cloud Console](https://console.hivemq.cloud/)
- [MQTT Return Codes](http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.html#_Toc398718035)
- [PubSubClient Library](https://github.com/knolleary/pubsubclient)

---

## 💡 Tips

1. **Luôn kiểm tra Active Connections**: Trước khi connect, vào HiveMQ Console → Clients để xem có connection nào đang dùng Client ID của bạn không

2. **Dùng unique Client ID**: Thêm timestamp hoặc số random vào Client ID để tránh trùng lặp

3. **Enable debug logs**: Trong `hivemq_config.h`, đảm bảo `MQTT_DEBUG` = 1

4. **Check NTP time**: TLS/SSL cần thời gian chính xác, đảm bảo NTP sync thành công

---

Chúc bạn debug thành công! 🎉

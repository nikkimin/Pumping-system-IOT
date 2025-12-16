# NeonDB Setup Guide

Hướng dẫn từng bước để setup NeonDB (PostgreSQL) cho hệ thống IoT Pumping System.

## 📋 Tổng quan

NeonDB là serverless PostgreSQL database với:
- ✅ **Free tier**: 500 MB storage (đủ dùng > 50 năm cho project này)
- ✅ **Auto-pause**: Tự động ngủ khi không dùng → tiết kiệm tài nguyên
- ✅ **Serverless**: Không cần quản lý server
- ✅ **Fast queries**: Tối ưu cho thống kê và analytics

---

## 🚀 Bước 1: Tạo NeonDB Account

### 1.1 Đăng ký tài khoản

1. Truy cập: **https://neon.tech**
2. Click **"Sign Up"** ở góc trên phải
3. Chọn phương thức đăng ký:
   - **GitHub** (khuyến nghị - nhanh nhất)
   - **Google**
   - **Email**
4. Authorize và verify email (nếu dùng email)

### 1.2 Tạo Project mới

1. Sau khi đăng nhập, click **"Create a project"**
2. Điền thông tin:
   - **Project name**: `iot-pumping-system` (hoặc tên tùy chọn)
   - **Region**: Chọn **Singapore** (gần Việt Nam nhất)
   - **PostgreSQL version**: Để mặc định (latest)
3. Click **"Create Project"**
4. Đợi ~30 giây để NeonDB provision database

---

## 🔧 Bước 2: Lấy Connection String

### 2.1 Copy database URL

1. Trong dashboard project, tìm phần **"Connection Details"**
2. Tab **"Nodejs"** hoặc **"Connection string"**
3. Copy URL dạng:
   ```
   postgresql://username:password@ep-xxx.region.neon.tech/dbname?sslmode=require
   ```
4. **Lưu ý**: Đây là thông tin nhạy cảm, không share công khai!

### 2.2 Lưu trữ an toàn

**Tạm thời**: Lưu vào file text (sẽ xóa sau khi setup xong)

**Permanent**: Sẽ lưu vào Netlify Environment Variables (bước 4)

---

## 🗄️ Bước 3: Tạo Database Schema

### 3.1 Mở SQL Editor

1. Trong NeonDB dashboard, click **"SQL Editor"** ở sidebar
2. Hoặc truy cập: https://console.neon.tech/app/projects/[your-project-id]/sql-editor

### 3.2 Chạy Schema Script

1. Copy toàn bộ nội dung file [`schema.sql`](file:///d:/Pumping-system-IOT/database/schema.sql)
2. Paste vào SQL Editor
3. Click **"Run"** (hoặc Ctrl+Enter)
4. Đợi ~5-10 giây

### 3.3 Verify Schema

Chạy query sau để kiểm tra:

```sql
-- Kiểm tra tables đã tạo
SELECT table_name 
FROM information_schema.tables 
WHERE table_schema = 'public' 
ORDER BY table_name;
```

**Kết quả mong đợi:**
```
table_name
--------------
daily_stats
pump_events
sensor_logs
```

Chạy thêm để kiểm tra view:

```sql
SELECT * FROM v_weekly_stats;
```

**Kết quả mong đợi**: 3 rows (PUMP_ON, PUMP_OFF, MODE_CHANGE) với count = 0

---

## 🔌 Bước 4: Cấu hình Netlify

### 4.1 Thêm Environment Variable

1. Truy cập **Netlify Dashboard**: https://app.netlify.com
2. Chọn site **"pumping-system-iot"** (hoặc tên site của bạn)
3. Vào **Site settings** → **Environment variables**
4. Click **"Add a variable"**
5. Điền:
   - **Key**: `DATABASE_URL`
   - **Value**: Paste connection string từ bước 2.1
   - **Scopes**: Chọn cả **"Build"** và **"Functions"**
6. Click **"Create variable"**

### 4.2 Verify Variable

1. Vẫn trong **Environment variables** page
2. Kiểm tra `DATABASE_URL` đã xuất hiện
3. Click **"•••"** (3 dots) → chọn **"Show value"** để xác nhận

---

## ✅ Bước 5: Test Connection

### 5.1 Test từ NeonDB Console

Chạy query đơn giản:

```sql
-- Insert test event
INSERT INTO pump_events (event_type, old_value, new_value, triggered_by) 
VALUES ('PUMP_ON', 'OFF', 'ON', 'test');

-- Verify insert
SELECT * FROM pump_events ORDER BY timestamp DESC LIMIT 1;

-- Check daily_stats auto-update (via trigger)
SELECT * FROM daily_stats WHERE date = CURRENT_DATE;
```

**Kết quả mong đợi**:
- `pump_events`: 1 row mới
- `daily_stats`: 1 row với `pump_on_count = 1`

### 5.2 Cleanup Test Data

```sql
-- Xóa test data
DELETE FROM pump_events WHERE triggered_by = 'test';
DELETE FROM daily_stats WHERE date = CURRENT_DATE;
```

---

## 📊 Database Schema Overview

### Bảng `pump_events`
Lưu lịch sử mọi sự kiện bật/tắt bơm và đổi chế độ.

| Column | Type | Description |
|--------|------|-------------|
| id | SERIAL | Primary key |
| timestamp | TIMESTAMPTZ | Thời gian sự kiện |
| event_type | VARCHAR(20) | PUMP_ON / PUMP_OFF / MODE_CHANGE |
| old_value | VARCHAR(20) | Giá trị cũ |
| new_value | VARCHAR(20) | Giá trị mới |
| triggered_by | VARCHAR(20) | manual / auto / mqtt |
| metadata | JSONB | Dữ liệu bổ sung (soil_moisture, pump_speed, ...) |

### Bảng `sensor_logs`
Lưu dữ liệu cảm biến theo thời gian (mỗi 5 phút).

| Column | Type | Description |
|--------|------|-------------|
| id | SERIAL | Primary key |
| timestamp | TIMESTAMPTZ | Thời gian đo |
| soil_moisture | INT | Độ ẩm đất (0-100%) |
| rain_status | BOOLEAN | Có mưa không |
| pump_status | BOOLEAN | Trạng thái bơm |
| auto_mode | BOOLEAN | Chế độ auto/manual |
| pump_speed | INT | Tốc độ bơm (0-100%) |

### Bảng `daily_stats`
Thống kê tổng hợp theo ngày (tự động cập nhật bởi trigger).

| Column | Type | Description |
|--------|------|-------------|
| date | DATE | Ngày (primary key) |
| pump_on_count | INT | Số lần bật bơm |
| pump_off_count | INT | Số lần tắt bơm |
| mode_changes | INT | Số lần đổi chế độ |
| total_runtime_minutes | INT | Tổng thời gian chạy (phút) |
| avg_soil_moisture | DECIMAL | Độ ẩm trung bình |
| rain_hours | INT | Số giờ có mưa |

---

## 🔍 Useful Queries

### Thống kê 7 ngày gần nhất
```sql
SELECT * FROM v_weekly_stats;
```

### Lịch sử sự kiện hôm nay
```sql
SELECT 
    timestamp,
    event_type,
    old_value || ' → ' || new_value as change,
    triggered_by
FROM pump_events
WHERE timestamp >= CURRENT_DATE
ORDER BY timestamp DESC;
```

### Độ ẩm trung bình theo ngày (7 ngày)
```sql
SELECT 
    DATE(timestamp) as date,
    ROUND(AVG(soil_moisture), 2) as avg_moisture
FROM sensor_logs
WHERE timestamp >= NOW() - INTERVAL '7 days'
GROUP BY DATE(timestamp)
ORDER BY date DESC;
```

### Số lần bật bơm theo giờ (hôm nay)
```sql
SELECT 
    EXTRACT(HOUR FROM timestamp) as hour,
    COUNT(*) as count
FROM pump_events
WHERE event_type = 'PUMP_ON'
  AND timestamp >= CURRENT_DATE
GROUP BY EXTRACT(HOUR FROM timestamp)
ORDER BY hour;
```

---

## 🐛 Troubleshooting

### Lỗi: "relation already exists"
**Nguyên nhân**: Schema đã được tạo trước đó

**Giải pháp**:
```sql
-- Xóa toàn bộ schema (CẢNH BÁO: mất hết data)
DROP TRIGGER IF EXISTS trg_update_daily_stats ON pump_events;
DROP FUNCTION IF EXISTS update_daily_stats();
DROP VIEW IF EXISTS v_weekly_stats;
DROP TABLE IF EXISTS daily_stats CASCADE;
DROP TABLE IF EXISTS sensor_logs CASCADE;
DROP TABLE IF EXISTS pump_events CASCADE;

-- Sau đó chạy lại schema.sql
```

### Lỗi: "SSL connection required"
**Nguyên nhân**: Connection string thiếu `?sslmode=require`

**Giải pháp**: Thêm vào cuối connection string:
```
postgresql://user:pass@host/db?sslmode=require
```

### Lỗi: "password authentication failed"
**Nguyên nhân**: Sai password hoặc database đã reset

**Giải pháp**: 
1. Vào NeonDB dashboard
2. Settings → Reset password
3. Copy connection string mới
4. Update Netlify environment variable

---

## 📈 Data Retention Policy

**Khuyến nghị**: Xóa dữ liệu cũ sau 1 năm để tiết kiệm storage

### Auto-cleanup Script (Chạy hàng tháng)

```sql
-- Xóa sensor_logs cũ hơn 1 năm
DELETE FROM sensor_logs 
WHERE timestamp < NOW() - INTERVAL '1 year';

-- Xóa pump_events cũ hơn 1 năm
DELETE FROM pump_events 
WHERE timestamp < NOW() - INTERVAL '1 year';

-- Giữ daily_stats cho analysis lâu dài (không xóa)
```

Hoặc tạo function tự động:

```sql
CREATE OR REPLACE FUNCTION cleanup_old_data()
RETURNS void AS $$
BEGIN
    DELETE FROM sensor_logs WHERE timestamp < NOW() - INTERVAL '1 year';
    DELETE FROM pump_events WHERE timestamp < NOW() - INTERVAL '1 year';
    RAISE NOTICE 'Cleanup completed';
END;
$$ LANGUAGE plpgsql;

-- Chạy manual khi cần:
-- SELECT cleanup_old_data();
```

---

## 🎯 Next Steps

Sau khi hoàn thành setup database:

1. ✅ Tạo Netlify Functions (xem [`../web-deploy/netlify/functions/`](file:///d:/Pumping-system-IOT/web-deploy/netlify/functions))
2. ✅ Tích hợp frontend để hiển thị thống kê
3. ✅ Deploy lên Netlify
4. ✅ Test end-to-end flow

---

## 📞 Support

Nếu gặp vấn đề:
1. Check NeonDB status: https://neon.tech/status
2. Docs: https://neon.tech/docs
3. Community: https://neon.tech/discord

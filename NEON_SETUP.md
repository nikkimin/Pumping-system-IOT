# 🚀 Quick Start: NeonDB Setup

Quick reference guide để setup database cho IoT Pumping System.

## 📝 Checklist

- [ ] **Step 1**: Đăng ký NeonDB tại https://neon.tech
- [ ] **Step 2**: Tạo project mới (chọn region: Singapore)
- [ ] **Step 3**: Copy Connection String
- [ ] **Step 4**: Chạy [`database/schema.sql`](file:///d:/Pumping-system-IOT/database/schema.sql) trong NeonDB SQL Editor
- [ ] **Step 5**: Thêm `DATABASE_URL` vào Netlify Environment Variables
- [ ] **Step 6**: Verify bằng query test

## 📚 Chi tiết

Xem hướng dẫn đầy đủ tại: [`database/README.md`](file:///d:/Pumping-system-IOT/database/README.md)

## ⚡ Connection String Format

```
postgresql://username:password@ep-xxx.region.neon.tech/dbname?sslmode=require
```

**Lưu ý**: Keep this secret! Không commit vào Git.

## 🔧 Environment Variable

**Netlify Dashboard** → Site Settings → Environment variables → Add variable

```
Key: DATABASE_URL
Value: <your-connection-string>
Scopes: ✅ Build, ✅ Functions
```

## ✅ Verification

Sau khi setup, chạy trong NeonDB SQL Editor:

```sql
-- Check tables
SELECT table_name FROM information_schema.tables 
WHERE table_schema = 'public';

-- Kết quả mong đợi: daily_stats, pump_events, sensor_logs
```

## 📊 Tables Created

1. **`pump_events`** - Lịch sử bật/tắt bơm, đổi chế độ
2. **`sensor_logs`** - Dữ liệu cảm biến theo thời gian
3. **`daily_stats`** - Thống kê tổng hợp (auto-updated by trigger)

## 🎯 Next Steps

1. ✅ Complete database setup (this guide)
2. ⏭️ Deploy Netlify Functions (automatic with next deploy)
3. ⏭️ Test statistics display on website

---

**Estimated time**: 15-20 phút

**Cost**: $0 (Free tier - 500MB storage)

#ifndef HIVEMQ_CONFIG_H
#define HIVEMQ_CONFIG_H

// ========== HIVEMQ PRIVATE CLOUD BROKER CONFIGURATION ==========
// ⚠️ QUAN TRỌNG: Sử dụng HiveMQ Cloud Private Broker (cần authentication)

// HiveMQ Cloud Private Broker Information
const char *HIVEMQ_HOST =
    "e947a9991cc442918fe1e94b5268b686.s1.eu.hivemq.cloud"; // Private cloud
                                                           // broker
const int HIVEMQ_PORT = 8883;         // Port cho TLS/SSL (mã hóa)
const int HIVEMQ_WS_PORT = 8884;      // Port cho WebSocket Secure (wss://)
const char *HIVEMQ_WS_PATH = "/mqtt"; // WebSocket path

// MQTT Credentials (Private broker CẦN username/password)
const char *MQTT_USERNAME = "pumpuser";    // HiveMQ Cloud username
const char *MQTT_PASSWORD = "pump123456A"; // HiveMQ Cloud password

// Client ID (unique cho mỗi ESP32 device)
// ⚠️ QUAN TRỌNG: Nếu có nhiều ESP32, thay đổi số cuối: _001, _002, _003...
// Web interface sẽ tự động tạo Client ID riêng (WebClient_xxxxx)
const char *MQTT_CLIENT_ID = "ESP32_SmartIrrigation_003";

// ========== MQTT TOPICS ==========
// Publish Topics (ESP32 gửi dữ liệu lên cloud)
const char *TOPIC_SENSOR_DATA =
    "smartirrigation/sensor/data"; // Dữ liệu cảm biến (soil, rain)
const char *TOPIC_PUMP_STATUS =
    "smartirrigation/pump/status"; // Trạng thái máy bơm
const char *TOPIC_SYSTEM_STATUS =
    "smartirrigation/system/status"; // Trạng thái hệ thống
const char *TOPIC_SYSTEM_LOG = "smartirrigation/system/log"; // System logs

// Subscribe Topics (ESP32 nhận lệnh từ cloud)
const char *TOPIC_PUMP_CONTROL =
    "smartirrigation/pump/control"; // Điều khiển máy bơm ON/OFF
const char *TOPIC_MODE_CONTROL =
    "smartirrigation/mode/control"; // Chuyển AUTO/MANUAL
const char *TOPIC_CONFIG = "smartirrigation/config/update"; // Cập nhật cấu hình

// ========== MQTT SETTINGS (ĐÃ TỐI ƯU CHO PRIVATE BROKER) ==========
#define MQTT_KEEPALIVE_INTERVAL                                                \
  90               // Keepalive interval (giây) - tăng để giảm traffic
#define MQTT_QOS 1 // QoS Level: 1=At-least-once delivery (đảm bảo nhận message)
#define MQTT_RETAIN false // Retain messages (false = không lưu message cũ)
#define MQTT_RECONNECT_DELAY                                                   \
  3000 // Delay giữa các lần reconnect - giảm để kết nối nhanh hơn
#define MQTT_BUFFER_SIZE 1024 // Buffer size - tăng để xử lý message lớn

// ========== PUBLISH INTERVALS (CHECK INTERVALS) ==========
// Lưu ý: Đây là interval để KIỂM TRA thay đổi, không phải interval GỬI dữ liệu
// Logic mới chỉ GỬI khi có thay đổi + heartbeat mỗi 5 phút
#define SENSOR_PUBLISH_INTERVAL                                                \
  1000 // Kiểm tra mỗi 1 giây (giảm từ 5s → 1s để cập nhật nhanh hơn)
#define STATUS_PUBLISH_INTERVAL                                                \
  30000 // Kiểm tra mỗi 30 giây (nhưng chỉ gửi khi cần)

// ========== DEBUG MODE ==========
#define MQTT_DEBUG 1 // 1 = Bật debug, 0 = Tắt debug

// ========== TLS/SSL CONFIGURATION ==========
// ⚠️ Private Cloud broker YÊU CẦU TLS/SSL
// Port 8883 là kết nối mã hóa (TLS/SSL)
// Certificate đã được cấu hình trong hivemq_cert.h
#define USE_TLS 1 // 0 = Không dùng TLS (port 1883), 1 = Dùng TLS (port 8883)

// ========== NTP SERVERS ==========
// Multiple NTP servers for better time sync reliability
const char *NTP_SERVER_PRIMARY = "pool.ntp.org";
const char *NTP_SERVER_SECONDARY = "time.google.com";
const char *NTP_SERVER_TERTIARY = "time.cloudflare.com";

#endif

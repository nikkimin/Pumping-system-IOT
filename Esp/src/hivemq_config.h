#ifndef HIVEMQ_CONFIG_H
#define HIVEMQ_CONFIG_H

// ========== HIVEMQ PUBLIC BROKER CONFIGURATION ==========
// ⚠️ QUAN TRỌNG: Sử dụng HiveMQ Public Broker (miễn phí, không cần đăng ký)

// HiveMQ Public Broker Information
const char* HIVEMQ_HOST = "broker.hivemq.com";  // Public broker
const int HIVEMQ_PORT = 1883;  // Port cho TCP (không mã hóa)
const int HIVEMQ_WS_PORT = 8000;  // Port cho WebSocket (ws://)
const char* HIVEMQ_WS_PATH = "/mqtt";  // WebSocket path

// MQTT Credentials (Public broker KHÔNG cần username/password)
const char* MQTT_USERNAME = "";      // Để trống cho public broker
const char* MQTT_PASSWORD = "";      // Để trống cho public broker

// Client ID (unique cho mỗi ESP32 device)
// ⚠️ QUAN TRỌNG: Nếu có nhiều ESP32, thay đổi số cuối: _001, _002, _003...
// Web interface sẽ tự động tạo Client ID riêng (WebClient_xxxxx)
const char* MQTT_CLIENT_ID = "ESP32_SmartIrrigation_003";

// ========== MQTT TOPICS ==========
// Publish Topics (ESP32 gửi dữ liệu lên cloud)
const char* TOPIC_SENSOR_DATA = "smartirrigation/sensor/data";      // Dữ liệu cảm biến (soil, rain)
const char* TOPIC_PUMP_STATUS = "smartirrigation/pump/status";      // Trạng thái máy bơm
const char* TOPIC_SYSTEM_STATUS = "smartirrigation/system/status";  // Trạng thái hệ thống
const char* TOPIC_SYSTEM_LOG = "smartirrigation/system/log";        // System logs

// Subscribe Topics (ESP32 nhận lệnh từ cloud)
const char* TOPIC_PUMP_CONTROL = "smartirrigation/pump/control";    // Điều khiển máy bơm ON/OFF
const char* TOPIC_MODE_CONTROL = "smartirrigation/mode/control";    // Chuyển AUTO/MANUAL
const char* TOPIC_CONFIG = "smartirrigation/config/update";         // Cập nhật cấu hình

// ========== MQTT SETTINGS (ĐÃ TỐI ƯU CHO PUBLIC BROKER) ==========
#define MQTT_KEEPALIVE_INTERVAL 90   // Keepalive interval (giây) - tăng để giảm traffic
#define MQTT_QOS 0                   // QoS Level: 0=Nhanh nhất (phù hợp public broker)
#define MQTT_RETAIN false            // Retain messages (false = không lưu message cũ)
#define MQTT_RECONNECT_DELAY 3000    // Delay giữa các lần reconnect - giảm để kết nối nhanh hơn
#define MQTT_BUFFER_SIZE 1024        // Buffer size - tăng để xử lý message lớn

// ========== PUBLISH INTERVALS (ĐÃ TỐI ƯU) ==========
#define SENSOR_PUBLISH_INTERVAL 10000   // Publish sensor data mỗi 10 giây (giảm bandwidth)
#define STATUS_PUBLISH_INTERVAL 60000   // Publish system status mỗi 60 giây

// ========== DEBUG MODE ==========
#define MQTT_DEBUG 1  // 1 = Bật debug, 0 = Tắt debug

// ========== TLS/SSL CONFIGURATION ==========
// ⚠️ Public broker KHÔNG SỬ DỤNG TLS/SSL
// Port 1883 là kết nối không mã hóa (plain TCP)
// Nếu muốn dùng TLS: port 8883, cần certificate
#define USE_TLS 0  // 0 = Không dùng TLS (port 1883), 1 = Dùng TLS (port 8883)

// ========== NTP SERVERS ==========
// Multiple NTP servers for better time sync reliability
const char* NTP_SERVER_PRIMARY = "pool.ntp.org";
const char* NTP_SERVER_SECONDARY = "time.google.com";
const char* NTP_SERVER_TERTIARY = "time.cloudflare.com";

#endif

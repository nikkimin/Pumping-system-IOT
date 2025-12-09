#ifndef HIVEMQ_CONFIG_H
#define HIVEMQ_CONFIG_H

// ========== HIVEMQ CLOUD CONFIGURATION ==========
// ⚠️ QUAN TRỌNG: Thay đổi thông tin sau theo cluster HiveMQ của bạn!

// HiveMQ Cloud Cluster Information
// Ví dụ: a1b2c3d4e5f6.s1.eu.hivemq.cloud (lấy từ HiveMQ Console)
const char* HIVEMQ_HOST = "10f287a7e9ba424b88c279464c967aa4.s1.eu.hivemq.cloud";
const int HIVEMQ_PORT = 8883;  // Port cho TLS/SSL ESP32 (KHÔNG đổi)
const int HIVEMQ_WS_PORT = 8884;  // Port cho WebSocket Secure (wss://)
const char* HIVEMQ_WS_PATH = "/mqtt";  // WebSocket path (KHÔNG đổi)

// MQTT Credentials (tạo trong HiveMQ Console -> Access Management)
const char* MQTT_USERNAME = "pumpuser";      // Username mới (viết thường, không dấu)
const char* MQTT_PASSWORD = "pump123456A";    // Password mới (đơn giản, dễ nhớ)

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

// ========== MQTT SETTINGS ==========
#define MQTT_KEEPALIVE_INTERVAL 60  // Keepalive interval (giây)
#define MQTT_QOS 1                  // QoS Level: 0=At most once, 1=At least once, 2=Exactly once
#define MQTT_RETAIN false           // Retain messages (false = không lưu message cũ)
#define MQTT_RECONNECT_DELAY 5000   // Delay giữa các lần reconnect (ms)
#define MQTT_BUFFER_SIZE 512        // Buffer size cho MQTT messages

// ========== PUBLISH INTERVALS ==========
#define SENSOR_PUBLISH_INTERVAL 5000    // Publish sensor data mỗi 5 giây
#define STATUS_PUBLISH_INTERVAL 30000   // Publish system status mỗi 30 giây

// ========== DEBUG MODE ==========
#define MQTT_DEBUG 1  // 1 = Bật debug, 0 = Tắt debug

// ========== CERTIFICATE VALIDATION ==========
// ⚠️ CHỈ DÙNG ĐỂ DEBUG - BẬT CHẾ ĐỘ INSECURE
// Set = 1 để bỏ qua certificate validation (KHÔNG AN TOÀN, chỉ để test)
// Set = 0 để dùng certificate validation đầy đủ (RECOMMENDED)
#define INSECURE_MODE 0  // 0 = Secure (validate cert), 1 = Insecure (bypass cert)

// ========== NTP SERVERS ==========
// Multiple NTP servers for better time sync reliability
const char* NTP_SERVER_PRIMARY = "pool.ntp.org";
const char* NTP_SERVER_SECONDARY = "time.google.com";
const char* NTP_SERVER_TERTIARY = "time.cloudflare.com";

#endif

#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <time.h>

// WiFi Configuration
const char* WIFI_SSID = "Binh";
const char* WIFI_PASS = "20021997";

// Web Server
WebServer server(80);

// UART with Arduino Uno
HardwareSerial UnoSerial(2);
const int RXD2 = 16;
const int TXD2 = 17;

// System State
int soilMoisture = 0;
int rainStatus = 0;
bool pumpStatus = false;
bool autoMode = true;
int pumpSpeed = 50;

// NTP Time
const char* TZ_INFO = "Asia/Bangkok";
const char* ntpServer = "pool.ntp.org";

// Event Log
String eventLog = "";

// ========== KHAI BÁO HÀM TRƯỚC ==========
void addLog(String message);
void readUARTData();
void checkAutoWatering();
int getCurrentHour();
// ========================================

// HTML Interface với emoji thay vì FontAwesome
const char* htmlContent = R"rawliteral(
<!DOCTYPE html>
<html lang="vi">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Hệ Thống Tưới Cây Tự Động</title>
    <style>
        :root {
            --primary: #667eea;
            --secondary: #764ba2;
            --success: #28a745;
            --danger: #dc3545;
            --warning: #ffc107;
            --info: #17a2b8;
            --light: #f8f9fa;
            --dark: #343a40;
        }
        
        * {
            box-sizing: border-box;
            margin: 0;
            padding: 0;
        }
        
        body {
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            min-height: 100vh;
            padding: 20px;
        }
        
        .container {
            max-width: 1200px;
            margin: 0 auto;
        }
        
        .header {
            background: rgba(255, 255, 255, 0.95);
            backdrop-filter: blur(10px);
            border-radius: 20px;
            padding: 30px;
            margin-bottom: 20px;
            text-align: center;
            box-shadow: 0 8px 32px rgba(0, 0, 0, 0.1);
        }
        
        .header h1 {
            background: linear-gradient(135deg, var(--primary), var(--secondary));
            -webkit-background-clip: text;
            -webkit-text-fill-color: transparent;
            font-size: 2.5rem;
            margin-bottom: 10px;
        }
        
        .connection-info {
            background: var(--info);
            color: white;
            padding: 10px;
            border-radius: 10px;
            margin: 10px 0;
            font-size: 0.9rem;
        }
        
        .grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(300px, 1fr));
            gap: 20px;
            margin-bottom: 20px;
        }
        
        .card {
            background: rgba(255, 255, 255, 0.95);
            backdrop-filter: blur(10px);
            border-radius: 20px;
            padding: 25px;
            box-shadow: 0 8px 32px rgba(0, 0, 0, 0.1);
            transition: transform 0.3s ease, box-shadow 0.3s ease;
        }
        
        .card:hover {
            transform: translateY(-5px);
            box-shadow: 0 12px 40px rgba(0, 0, 0, 0.15);
        }
        
        .card-title {
            font-size: 1.3rem;
            font-weight: 600;
            margin-bottom: 15px;
            color: var(--dark);
            display: flex;
            align-items: center;
            gap: 10px;
        }
        
        .card-title i {
            color: var(--primary);
            font-style: normal;
            font-size: 1.5rem;
        }
        
        .sensor-value {
            font-size: 2.5rem;
            font-weight: 700;
            text-align: center;
            margin: 15px 0;
            background: linear-gradient(135deg, var(--primary), var(--secondary));
            -webkit-background-clip: text;
            -webkit-text-fill-color: transparent;
        }
        
        .control-group {
            margin: 20px 0;
        }
        
        .switch {
            position: relative;
            display: inline-block;
            width: 60px;
            height: 34px;
        }
        
        .switch input {
            opacity: 0;
            width: 0;
            height: 0;
        }
        
        .slider {
            position: absolute;
            cursor: pointer;
            top: 0;
            left: 0;
            right: 0;
            bottom: 0;
            background-color: #ccc;
            transition: .4s;
            border-radius: 34px;
        }
        
        .slider:before {
            position: absolute;
            content: "";
            height: 26px;
            width: 26px;
            left: 4px;
            bottom: 4px;
            background-color: white;
            transition: .4s;
            border-radius: 50%;
        }
        
        input:checked + .slider {
            background: linear-gradient(135deg, var(--primary), var(--secondary));
        }
        
        input:checked + .slider:before {
            transform: translateX(26px);
        }
        
        .slider-container {
            display: flex;
            align-items: center;
            gap: 15px;
            margin: 15px 0;
        }
        
        .slider-value {
            font-weight: 600;
            color: var(--primary);
            min-width: 40px;
        }
        
        input[type="range"] {
            flex: 1;
            height: 8px;
            border-radius: 4px;
            background: #e0e0e0;
            outline: none;
        }
        
        input[type="range"]::-webkit-slider-thumb {
            -webkit-appearance: none;
            width: 20px;
            height: 20px;
            border-radius: 50%;
            background: var(--primary);
            cursor: pointer;
        }
        
        .status-badge {
            display: inline-block;
            padding: 8px 16px;
            border-radius: 20px;
            font-weight: 600;
            margin: 5px;
        }
        
        .status-on {
            background: var(--success);
            color: white;
        }
        
        .status-off {
            background: var(--danger);
            color: white;
        }
        
        .status-auto {
            background: var(--info);
            color: white;
        }
        
        .status-manual {
            background: var(--warning);
            color: black;
        }
        
        .status-info {
            background: var(--info);
            color: white;
        }
        
        .status-warning {
            background: var(--warning);
            color: black;
        }
        
        .status-success {
            background: var(--success);
            color: white;
        }
        
        .status-danger {
            background: var(--danger);
            color: white;
        }
        
        .log-container {
            max-height: 300px;
            overflow-y: auto;
            background: var(--light);
            border-radius: 10px;
            padding: 15px;
            margin-top: 15px;
        }
        
        .log-entry {
            padding: 8px 12px;
            margin: 5px 0;
            background: white;
            border-radius: 8px;
            border-left: 4px solid var(--primary);
            font-size: 0.9rem;
        }
        
        .btn {
            padding: 12px 24px;
            border: none;
            border-radius: 10px;
            font-weight: 600;
            cursor: pointer;
            transition: all 0.3s ease;
            margin: 5px;
        }
        
        .btn-primary {
            background: linear-gradient(135deg, var(--primary), var(--secondary));
            color: white;
        }
        
        .btn:hover {
            transform: translateY(-2px);
            box-shadow: 0 4px 12px rgba(0, 0, 0, 0.2);
        }
        
        .current-time {
            font-size: 1.1rem;
            font-weight: 600;
            color: var(--dark);
            text-align: center;
            margin: 10px 0;
        }
        
        .wifi-status {
            background: var(--success);
            color: white;
            padding: 8px 16px;
            border-radius: 20px;
            font-size: 0.9rem;
            margin: 10px 0;
        }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>🌿 HỆ THỐNG TƯỚI CÂY TỰ ĐỘNG</h1>
            <div class="wifi-status">
                📶 Đã kết nối WiFi | Chế độ: Station
            </div>
            <div class="current-time" id="currentTime">--:--:--</div>
        </div>
        
        <div class="grid">
            <!-- Sensor Data -->
            <div class="card">
                <div class="card-title">
                    <i>💧</i>
                    ĐỘ ẨM ĐẤT
                </div>
                <div class="sensor-value" id="soilMoisture">0%</div>
                <div class="status-badge" id="soilStatus">Đang cập nhật...</div>
            </div>
            
            <div class="card">
                <div class="card-title">
                    <i>🌧️</i>
                    TRẠNG THÁI MƯA
                </div>
                <div class="sensor-value" id="rainStatus">--</div>
                <div class="status-badge" id="rainStatusText">Đang cập nhật...</div>
            </div>
            
            <div class="card">
                <div class="card-title">
                    <i>🚰</i>
                    MÁY BƠM
                </div>
                <div class="sensor-value" id="pumpStatus">TẮT</div>
                <div class="status-badge status-off" id="pumpStatusBadge">OFF</div>
            </div>
            
            <div class="card">
                <div class="card-title">
                    <i>⚙️</i>
                    CHẾ ĐỘ HOẠT ĐỘNG
                </div>
                <div class="control-group">
                    <div class="slider-container">
                        <span>Tự động</span>
                        <label class="switch">
                            <input type="checkbox" id="modeToggle">
                            <span class="slider"></span>
                        </label>
                        <span>Thủ công</span>
                    </div>
                    <div class="status-badge status-auto" id="modeStatus">CHẾ ĐỘ TỰ ĐỘNG</div>
                </div>
                
                <div id="manualControls" style="display: none;">
                    <div class="card-title">
                        <i>🎚️</i>
                        ĐIỀU KHIỂN THỦ CÔNG
                    </div>
                    <div class="slider-container">
                        <span>Tốc độ:</span>
                        <input type="range" min="0" max="100" value="50" id="pumpSpeedSlider">
                        <span class="slider-value" id="pumpSpeedValue">50%</span>
                    </div>
                    <button class="btn btn-primary" id="btnPumpOn">
                        ▶️ BẬT BƠM
                    </button>
                    <button class="btn btn-primary" id="btnPumpOff">
                        ⏹️ TẮT BƠM
                    </button>
                </div>
            </div>
        </div>
        
        <!-- Event Log -->
        <div class="card">
            <div class="card-title">
                <i>📋</i>
                NHẬT KÝ SỰ KIỆN
            </div>
            <div class="log-container" id="eventLog">
                <div class="log-entry">[Hệ thống] Đang khởi động...</div>
            </div>
            <button class="btn btn-primary" id="btnClearLog">
                🗑️ XÓA NHẬT KÝ
            </button>
        </div>
    </div>

    <script>
        // Update current time
        function updateTime() {
            const now = new Date();
            const timeString = now.toLocaleTimeString('vi-VN');
            document.getElementById('currentTime').textContent = timeString;
        }
        setInterval(updateTime, 1000);
        updateTime();
        
        // Toggle between auto and manual mode
        function toggleMode() {
            const isManual = document.getElementById('modeToggle').checked;
            const modeStatus = document.getElementById('modeStatus');
            const manualControls = document.getElementById('manualControls');
            
            if (isManual) {
                modeStatus.textContent = 'CHẾ ĐỘ THỦ CÔNG';
                modeStatus.className = 'status-badge status-manual';
                manualControls.style.display = 'block';
                addLog('Chuyển sang chế độ thủ công');
            } else {
                modeStatus.textContent = 'CHẾ ĐỘ TỰ ĐỘNG';
                modeStatus.className = 'status-badge status-auto';
                manualControls.style.display = 'none';
                addLog('Chuyển sang chế độ tự động');
            }
            
            fetch('/setMode?mode=' + (isManual ? 'manual' : 'auto'))
                .catch(error => console.error('Error setting mode:', error));
        }
        
        // Update pump speed
        function updatePumpSpeed(speed) {
            document.getElementById('pumpSpeedValue').textContent = speed + '%';
            fetch('/setSpeed?speed=' + speed)
                .catch(error => console.error('Error setting speed:', error));
            addLog('Điều chỉnh tốc độ bơm: ' + speed + '%');
        }
        
        // Control pump manually
        function controlPump(turnOn) {
            const endpoint = turnOn ? '/controlPump?state=on' : '/controlPump?state=off';
            fetch(endpoint)
                .then(response => {
                    if (!response.ok) {
                        throw new Error('Network response was not ok');
                    }
                    addLog(turnOn ? 'Bật máy bơm thủ công' : 'Tắt máy bơm thủ công');
                })
                .catch(error => {
                    console.error('Error controlling pump:', error);
                    addLog('Lỗi khi điều khiển máy bơm');
                });
        }
        
        // Add log entry
        function addLog(message) {
            const logContainer = document.getElementById('eventLog');
            const now = new Date();
            const timeString = now.toLocaleTimeString('vi-VN');
            const logEntry = document.createElement('div');
            logEntry.className = 'log-entry';
            logEntry.innerHTML = `<strong>[${timeString}]</strong> ${message}`;
            logContainer.appendChild(logEntry);
            logContainer.scrollTop = logContainer.scrollHeight;
        }
        
        // Clear log
        function clearLog() {
            document.getElementById('eventLog').innerHTML = '';
            addLog('Đã xóa nhật ký sự kiện');
        }
        
        // Update sensor data
        function updateSensorData(data) {
            console.log('Updating sensor data:', data);
            
            document.getElementById('soilMoisture').textContent = data.soil + '%';
            document.getElementById('rainStatus').textContent = data.rain ? 'CÓ MƯA' : 'KHÔNG MƯA';
            document.getElementById('rainStatusText').textContent = data.rain ? 'Đang mưa' : 'Không mưa';
            document.getElementById('rainStatusText').className = data.rain ? 'status-badge status-info' : 'status-badge status-warning';
            
            document.getElementById('pumpStatus').textContent = data.pump ? 'ĐANG BƠM' : 'ĐÃ TẮT';
            document.getElementById('pumpStatusBadge').textContent = data.pump ? 'ON' : 'OFF';
            document.getElementById('pumpStatusBadge').className = data.pump ? 'status-badge status-on' : 'status-badge status-off';
            
            const soilStatus = document.getElementById('soilStatus');
            if (data.soil < 30) {
                soilStatus.textContent = 'RẤT KHÔ';
                soilStatus.className = 'status-badge status-danger';
            } else if (data.soil < 60) {
                soilStatus.textContent = 'BÌNH THƯỜNG';
                soilStatus.className = 'status-badge status-warning';
            } else {
                soilStatus.textContent = 'ĐỦ ẨM';
                soilStatus.className = 'status-badge status-success';
            }
        }
        
        // Fetch data from ESP32
        function fetchData() {
            fetch('/getData')
                .then(response => {
                    if (!response.ok) {
                        throw new Error('Network response was not ok');
                    }
                    return response.json();
                })
                .then(data => updateSensorData(data))
                .catch(error => console.error('Error fetching data:', error));
        }
        
        // Initialize event listeners after page load
        document.addEventListener('DOMContentLoaded', function() {
            console.log('DOM loaded, initializing event listeners...');
            
            // Mode toggle
            document.getElementById('modeToggle').addEventListener('change', toggleMode);
            
            // Pump speed slider
            document.getElementById('pumpSpeedSlider').addEventListener('input', function() {
                updatePumpSpeed(this.value);
            });
            
            // Pump control buttons
            document.getElementById('btnPumpOn').addEventListener('click', function() {
                controlPump(true);
            });
            
            document.getElementById('btnPumpOff').addEventListener('click', function() {
                controlPump(false);
            });
            
            // Clear log button
            document.getElementById('btnClearLog').addEventListener('click', clearLog);
            
            // Initial setup
            setInterval(fetchData, 2000);
            fetchData();
            addLog('Hệ thống khởi động thành công');
            
            console.log('Event listeners initialized successfully');
        });
        
    </script>
</body>
</html>
)rawliteral";

void setup() {
    Serial.begin(9600);
    UnoSerial.begin(9600, SERIAL_8N1, RXD2, TXD2);
    
    Serial.println("🚀 Starting ESP32 Garden System...");
    
    // Connect to WiFi
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    Serial.print("📶 Connecting to WiFi");
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(1000);
        Serial.print(".");
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\n✅ Connected! IP: " + WiFi.localIP().toString());
    } else {
        Serial.println("\n❌ Failed to connect to WiFi");
    }
    
    // Initialize NTP
    configTzTime(TZ_INFO, ntpServer);
    Serial.println("⏰ NTP time configured");
    
    // Setup web server routes với debug chi tiết
    server.on("/", HTTP_GET, []() {
        Serial.println("🌐 GET / - Serving HTML interface");
        server.send(200, "text/html", htmlContent);
    });
    
    server.on("/getData", HTTP_GET, []() {
        Serial.println("📊 GET /getData - Sending sensor data");
        String json = "{\"soil\":" + String(soilMoisture) + 
                     ",\"rain\":" + String(rainStatus) + 
                     ",\"pump\":" + String(pumpStatus) + "}";
        server.send(200, "application/json", json);
    });
    
    server.on("/controlPump", HTTP_GET, []() {
        Serial.println("🎛️ GET /controlPump");
        if (server.hasArg("state")) {
            String state = server.arg("state");
            Serial.println("💧 Pump control received: " + state);
            if (state == "on") {
                UnoSerial.println("PUMP_ON");
                addLog("Manual: Bật máy bơm");
                Serial.println("🔴 Sending PUMP_ON to Arduino");
            } else {
                UnoSerial.println("PUMP_OFF");
                addLog("Manual: Tắt máy bơm");
                Serial.println("🟢 Sending PUMP_OFF to Arduino");
            }
            server.send(200, "text/plain", "OK");
        } else {
            server.send(400, "text/plain", "Missing state parameter");
        }
    });
    
    server.on("/setMode", HTTP_GET, []() {
        Serial.println("🔄 GET /setMode");
        if (server.hasArg("mode")) {
            String mode = server.arg("mode");
            autoMode = (mode == "auto");
            Serial.println("📝 Mode set to: " + String(autoMode ? "AUTO" : "MANUAL"));
            addLog("Chế độ: " + String(autoMode ? "Tự động" : "Thủ công"));
            server.send(200, "text/plain", "OK");
        } else {
            server.send(400, "text/plain", "Missing mode parameter");
        }
    });
    
    server.on("/setSpeed", HTTP_GET, []() {
        Serial.println("🎚️ GET /setSpeed");
        if (server.hasArg("speed")) {
            pumpSpeed = server.arg("speed").toInt();
            Serial.println("📊 Pump speed set to: " + String(pumpSpeed) + "%");
            server.send(200, "text/plain", "OK");
        } else {
            server.send(400, "text/plain", "Missing speed parameter");
        }
    });
    
    // Handle 404 errors
    server.onNotFound([]() {
        Serial.println("❌ 404 - Path not found: " + server.uri());
        server.send(404, "text/plain", "Path not found");
    });
    
    server.begin();
    Serial.println("✅ Web server started on port 80");
    addLog("Web server khởi động thành công");
    addLog("IP: " + WiFi.localIP().toString());
}

void loop() {
    server.handleClient();
    readUARTData();
    
    if (autoMode) {
        checkAutoWatering();
    }
    
    delay(100);
}

// ========== ĐỊNH NGHĨA CÁC HÀM ==========

void addLog(String message) {
    time_t now = time(nullptr);
    struct tm timeinfo;
    
    if (localtime_r(&now, &timeinfo)) {
        char timeStr[20];
        strftime(timeStr, sizeof(timeStr), "%H:%M:%S", &timeinfo);
        eventLog = "[" + String(timeStr) + "] " + message + "<br>" + eventLog;
    } else {
        eventLog = "[--:--:--] " + message + "<br>" + eventLog;
    }
    
    // Giới hạn độ dài log
    if (eventLog.length() > 3000) {
        int lastIndex = eventLog.lastIndexOf("<br>", 2000);
        if (lastIndex != -1) {
            eventLog = eventLog.substring(0, lastIndex);
        }
    }
    
    Serial.println("📝 LOG: " + message);
}

void readUARTData() {
    while (UnoSerial.available()) {
        String data = UnoSerial.readStringUntil('\n');
        data.trim();
        
        if (data.length() > 0) {
            Serial.println("📨 UART Received: " + data);
        }
        
        if (data.startsWith("{")) {
            // JSON data from Arduino
            StaticJsonDocument<256> doc;
            DeserializationError error = deserializeJson(doc, data);
            
            if (!error) {
                int newSoil = doc["soil_moisture"];
                int newRain = doc["rain"];
                bool newPump = doc["pump_status"];
                
                // Only update if values changed
                if (newSoil != soilMoisture || newRain != rainStatus || newPump != pumpStatus) {
                    soilMoisture = newSoil;
                    rainStatus = newRain;
                    pumpStatus = newPump;
                    
                    Serial.println("📊 Data updated - Soil: " + String(soilMoisture) + 
                                 "%, Rain: " + String(rainStatus) + 
                                 ", Pump: " + String(pumpStatus));
                }
            } else {
                Serial.println("❌ JSON Parse Error: " + String(error.c_str()));
            }
        } else if (data == "PUMP_ON_ACK") {
            pumpStatus = true;
            addLog("Máy bơm đã BẬT");
            Serial.println("🔴 Pump ON acknowledged");
        } else if (data == "PUMP_OFF_ACK") {
            pumpStatus = false;
            addLog("Máy bơm đã TẮT");
            Serial.println("🟢 Pump OFF acknowledged");
        } else if (data.length() > 0) {
            Serial.println("📨 UART Message: " + data);
        }
    }
}

int getCurrentHour() {
    time_t now = time(nullptr);
    struct tm timeinfo;
    
    if (localtime_r(&now, &timeinfo)) {
        return timeinfo.tm_hour;
    }
    return -1; // Return -1 if time is not available
}

void checkAutoWatering() {
    static unsigned long lastCheck = 0;
    if (millis() - lastCheck < 30000) return; // Check every 30 seconds
    lastCheck = millis();
    
    int currentHour = getCurrentHour();
    
    if (currentHour == -1) {
        Serial.println("⏰ Cannot get current time, skipping auto check");
        return;
    }
    
    Serial.println("🤖 Auto check - Hour: " + String(currentHour) + 
                  ", Rain: " + String(rainStatus) + 
                  ", Soil: " + String(soilMoisture) + "%");
    
    // Auto watering logic: 6h or 17h, no rain, soil dry
    if ((currentHour == 6 || currentHour == 17) && rainStatus == 0 && soilMoisture < 40) {
        if (!pumpStatus) {
            UnoSerial.println("PUMP_ON");
            addLog("Auto: Bật bơm (Thời gian: " + String(currentHour) + 
                  "h, Độ ẩm: " + String(soilMoisture) + "%)");
            Serial.println("🤖 Auto: Starting pump");
        }
    } else {
        if (pumpStatus) {
            UnoSerial.println("PUMP_OFF");
            addLog("Auto: Tắt bơm (Điều kiện không thỏa mãn)");
            Serial.println("🤖 Auto: Stopping pump");
        }
    }
}
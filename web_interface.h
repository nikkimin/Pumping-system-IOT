#ifndef WEB_INTERFACE_H
#define WEB_INTERFACE_H

// Main HTML interface for Smart Irrigation System
const char* htmlContent = R"rawliteral(
<!DOCTYPE html>
<html lang="vi">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Hệ Thống Tưới Cây Thông Minh</title>
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
            <h1>🌿 HỆ THỐNG TƯỚI CÂY THÔNG MINH</h1>
            <div class="wifi-status" id="wifiStatus">
                📶 WiFi: %WIFI_STATUS% | Mode: %WIFI_MODE%
            </div>
            <div class="current-time" id="currentTime">--:--:--</div>
            <div style="margin-top: 15px;">
                <button class="btn btn-primary" onclick="location.href='/wifi-config'">
                    🔧 Cấu Hình WiFi
                </button>
                <button class="btn btn-primary" onclick="resetSetup()">
                    🔄 Reset Setup
                </button>
                <button class="btn btn-primary" onclick="location.href='/'">
                    🏠 Trang Chính
                </button>
            </div>
        </div>
        
        <div class="grid">
            <!-- Sensor Data Cards -->
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
        
        // Reset setup
        function resetSetup() {
            if (confirm('Bạn có chắc muốn reset cấu hình hệ thống?')) {
                fetch('/reset-setup')
                    .then(() => {
                        addLog('Đã reset cấu hình hệ thống');
                        setTimeout(() => location.reload(), 2000);
                    })
                    .catch(error => console.error('Error resetting setup:', error));
            }
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
            
            // Update mode toggle
            document.getElementById('modeToggle').checked = !data.autoMode;
            toggleMode(); // Update UI
            
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

#endif // WEB_INTERFACE_H

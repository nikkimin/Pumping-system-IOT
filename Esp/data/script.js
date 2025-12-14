// MQTT Configuration - HiveMQ Public Broker
const MQTT_HOST = "broker.hivemq.com";  // Public broker
const MQTT_PORT = 8000; // WebSocket Port (không mã hóa)
const MQTT_PATH = "/mqtt"; // WebSocket path
const MQTT_CLIENT_ID = "ESP32_SmartIrrigation_003";
const MQTT_USERNAME = "";  // Public broker không cần username
const MQTT_PASSWORD = "";  // Public broker không cần password

// Topics
const TOPIC_SENSOR_DATA = "smartirrigation/sensor/data";
const TOPIC_PUMP_STATUS = "smartirrigation/pump/status";
const TOPIC_SYSTEM_STATUS = "smartirrigation/system/status";
const TOPIC_PUMP_CONTROL = "smartirrigation/pump/control";
const TOPIC_MODE_CONTROL = "smartirrigation/mode/control";

let client;
let mqttConnected = false;
let reconnectAttempts = 0;
let reconnectTimeout = null;
const MAX_RECONNECT_ATTEMPTS = 20; // Maximum retry attempts
const MAX_RECONNECT_DELAY = 30000; // Max 30 seconds
const INITIAL_RECONNECT_DELAY = 2000; // Start with 2 seconds

// Update current time
function updateTime() {
    const now = new Date();
    const timeString = now.toLocaleTimeString('vi-VN');
    document.getElementById('currentTime').textContent = timeString;
}
setInterval(updateTime, 1000);
updateTime();

// Initialize MQTT Connection
function initMQTT() {
    addLog("Đang kết nối tới MQTT Broker...");
    console.log(`Connecting to: wss://${MQTT_HOST}:${MQTT_PORT}${MQTT_PATH}`);
    console.log(`Client ID: ${MQTT_CLIENT_ID}`);

    // Create client with proper WebSocket path
    client = new Paho.MQTT.Client(MQTT_HOST, MQTT_PORT, MQTT_PATH, MQTT_CLIENT_ID);

    client.onConnectionLost = onConnectionLost;
    client.onMessageArrived = onMessageArrived;

    const options = {
        useSSL: false,  // Public broker sử dụng WS (không mã hóa) thay vì WSS
        onSuccess: onConnect,
        onFailure: onFailure,
        keepAliveInterval: 30,
        cleanSession: true,
        timeout: 10
        // Public broker không cần username/password
    };

    client.connect(options);
}

function onConnect() {
    console.log("MQTT Connected");
    mqttConnected = true;
    reconnectAttempts = 0; // Reset reconnect counter on successful connection
    addLog("✅ Đã kết nối tới HiveMQ Public Broker");
    updateWifiStatus("Đã kết nối Cloud");

    // Subscribe to topics
    client.subscribe(TOPIC_SENSOR_DATA);
    client.subscribe(TOPIC_PUMP_STATUS);
    client.subscribe(TOPIC_SYSTEM_STATUS);

    console.log("Subscribed to all topics");
}

function onFailure(responseObject) {
    console.log("MQTT Connection Failed: " + responseObject.errorMessage);
    mqttConnected = false;
    addLog("❌ Lỗi kết nối MQTT: " + responseObject.errorMessage);
    updateWifiStatus("Mất kết nối");

    // Automatic reconnection with exponential backoff
    attemptReconnect();
}

function onConnectionLost(responseObject) {
    if (responseObject.errorCode !== 0) {
        console.log("Connection Lost: " + responseObject.errorMessage);
        mqttConnected = false;
        addLog("⚠️ Mất kết nối MQTT: " + responseObject.errorMessage);
        updateWifiStatus("Mất kết nối");

        // Automatic reconnection with exponential backoff
        attemptReconnect();
    }
}

// Automatic reconnection logic with exponential backoff
function attemptReconnect() {
    // Clear any existing reconnect timeout
    if (reconnectTimeout) {
        clearTimeout(reconnectTimeout);
    }

    reconnectAttempts++;

    // Check if max attempts reached
    if (reconnectAttempts > MAX_RECONNECT_ATTEMPTS) {
        addLog(`❌ Đã vượt quá số lần thử kết nối tối đa (${MAX_RECONNECT_ATTEMPTS}). Vui lòng tải lại trang.`);
        updateWifiStatus(`Kết nối thất bại`);
        return;
    }

    // Calculate delay using exponential backoff: 2s, 4s, 8s, 16s, 30s (max)
    const delay = Math.min(
        INITIAL_RECONNECT_DELAY * Math.pow(2, reconnectAttempts - 1),
        MAX_RECONNECT_DELAY
    );

    addLog(`🔄 Sẽ thử kết nối lại sau ${delay / 1000} giây... (Lần thử: ${reconnectAttempts}/${MAX_RECONNECT_ATTEMPTS})`);
    updateWifiStatus(`Kết nối lại sau ${delay / 1000}s`);

    reconnectTimeout = setTimeout(() => {
        if (!mqttConnected) {
            addLog("🔄 Đang thử kết nối lại...");
            initMQTT();
        }
    }, delay);
}

function onMessageArrived(message) {
    console.log("Message Arrived: " + message.destinationName + " - " + message.payloadString);
    const topic = message.destinationName;
    let data;

    try {
        data = JSON.parse(message.payloadString);
    } catch (e) {
        console.error("JSON Parse Error", e);
        return;
    }

    if (topic === TOPIC_SENSOR_DATA) {
        updateSensorUI(data);
    } else if (topic === TOPIC_PUMP_STATUS) {
        updatePumpUI(data);
    } else if (topic === TOPIC_SYSTEM_STATUS) {
        // Optional: Update system info if needed
    }
}

function publishMessage(topic, payloadObj) {
    if (!mqttConnected) {
        addLog("Chưa kết nối MQTT, không thể gửi lệnh");
        return;
    }
    const message = new Paho.MQTT.Message(JSON.stringify(payloadObj));
    message.destinationName = topic;
    client.send(message);
}

// UI Update Functions
function updateSensorUI(data) {
    document.getElementById('soilMoisture').textContent = data.soil_moisture + '%';
    document.getElementById('rainStatus').textContent = data.rain_status ? 'CÓ MƯA' : 'KHÔNG MƯA';
    document.getElementById('rainStatusText').textContent = data.rain_status ? 'Đang mưa' : 'Không mưa';
    document.getElementById('rainStatusText').className = data.rain_status ? 'status-badge status-info' : 'status-badge status-warning';

    // Update Soil Status
    const soilStatus = document.getElementById('soilStatus');
    if (data.soil_moisture < 30) {
        soilStatus.textContent = 'RẤT KHÔ';
        soilStatus.className = 'status-badge status-danger';
    } else if (data.soil_moisture < 60) {
        soilStatus.textContent = 'BÌNH THƯỜNG';
        soilStatus.className = 'status-badge status-warning';
    } else {
        soilStatus.textContent = 'ĐỦ ẨM';
        soilStatus.className = 'status-badge status-success';
    }
}

function updatePumpUI(data) {
    // data.status is "ON" or "OFF"
    const isPumpOn = (data.status === "ON");
    document.getElementById('pumpStatus').textContent = isPumpOn ? 'ĐANG BƠM' : 'ĐÃ TẮT';
    document.getElementById('pumpStatusBadge').textContent = isPumpOn ? 'ON' : 'OFF';
    document.getElementById('pumpStatusBadge').className = isPumpOn ? 'status-badge status-on' : 'status-badge status-off';

    // data.mode is "AUTO" or "MANUAL"
    const isAuto = (data.mode === "AUTO");
    document.getElementById('modeToggle').checked = !isAuto;

    const modeStatus = document.getElementById('modeStatus');
    const manualControls = document.getElementById('manualControls');

    if (!isAuto) {
        modeStatus.textContent = 'CHẾ ĐỘ THỦ CÔNG';
        modeStatus.className = 'status-badge status-manual';
        manualControls.style.display = 'block';
    } else {
        modeStatus.textContent = 'CHẾ ĐỘ TỰ ĐỘNG';
        modeStatus.className = 'status-badge status-auto';
        manualControls.style.display = 'none';
    }

    if (data.speed) {
        document.getElementById('pumpSpeedValue').textContent = data.speed + '%';
        document.getElementById('pumpSpeedSlider').value = data.speed;
    }
}

function updateWifiStatus(status) {
    document.getElementById('wifiStatus').innerHTML = '📶 ' + status;
}

// User Actions
function toggleMode() {
    const isManual = document.getElementById('modeToggle').checked;
    // Optimistic UI update
    const modeStatus = document.getElementById('modeStatus');
    const manualControls = document.getElementById('manualControls');

    if (isManual) {
        modeStatus.textContent = 'CHẾ ĐỘ THỦ CÔNG';
        modeStatus.className = 'status-badge status-manual';
        manualControls.style.display = 'block';
        addLog('Đang chuyển sang chế độ thủ công...');
        publishMessage(TOPIC_MODE_CONTROL, { mode: "MANUAL" });
    } else {
        modeStatus.textContent = 'CHẾ ĐỘ TỰ ĐỘNG';
        modeStatus.className = 'status-badge status-auto';
        manualControls.style.display = 'none';
        addLog('Đang chuyển sang chế độ tự động...');
        publishMessage(TOPIC_MODE_CONTROL, { mode: "AUTO" });
    }
}

function updatePumpSpeed(speed) {
    document.getElementById('pumpSpeedValue').textContent = speed + '%';
    // Debounce or just send? For MQTT maybe send only on change end? 
    // For now, let's send it. The Slider 'input' event fires rapidly.
    // 'change' event fires on release. We used 'input' before.
    // To avoid flood, we only send if manual mode is active, which is implied by visibility.
    // Ideally we should use 'change' for MQTT to save bandwidth, but 'input' feels more responsive.
    // Let's stick to the previous behavior but maybe user should be careful.
    // Actually, let's use the pump control topic to set speed too if needed, or just mode logic?
    // main.cpp: TOPIC_PUMP_CONTROL handles speed if doc contains "speed".
    // main.cpp: TOPIC_MODE_CONTROL handles speed if doc contains "speed".

    // We'll update the speed by sending a MANUAL mode command with speed, 
    // or if pump is ON, send PUMP status?
    // Best way is to send a mode update or pump control update.
    // Let's assume we are in Manual mode if this is visible.
    publishMessage(TOPIC_MODE_CONTROL, { mode: "MANUAL", speed: parseInt(speed) });
}

function controlPump(turnOn) {
    addLog(turnOn ? 'Đang gửi lệnh bật bơm...' : 'Đang gửi lệnh tắt bơm...');
    const command = turnOn ? "turn_on" : "turn_off";
    const speed = parseInt(document.getElementById('pumpSpeedSlider').value);

    publishMessage(TOPIC_PUMP_CONTROL, {
        command: command,
        speed: speed
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

// Reset setup - NOT AVAILABLE IN CLOUD MODE
function resetSetup() {
    alert("Tính năng Reset Setup chỉ khả dụng khi kết nối trực tiếp với Wifi của thiết bị (Mạng LAN).");
}

// Initialize event listeners after page load
document.addEventListener('DOMContentLoaded', function () {
    console.log('DOM loaded, initializing event listeners...');

    // Mode toggle
    document.getElementById('modeToggle').addEventListener('change', toggleMode);

    // Pump speed slider - Change to 'change' event to reduce MQTT spam
    document.getElementById('pumpSpeedSlider').addEventListener('change', function () {
        updatePumpSpeed(this.value);
    });
    // Update label on input for visual feedback
    document.getElementById('pumpSpeedSlider').addEventListener('input', function () {
        document.getElementById('pumpSpeedValue').textContent = this.value + '%';
    });

    // Pump control buttons
    document.getElementById('btnPumpOn').addEventListener('click', function () {
        controlPump(true);
    });

    document.getElementById('btnPumpOff').addEventListener('click', function () {
        controlPump(false);
    });

    // Clear log button
    document.getElementById('btnClearLog').addEventListener('click', clearLog);

    // Initial setup
    initMQTT();
    addLog('Giao diện đang khởi động...');
});

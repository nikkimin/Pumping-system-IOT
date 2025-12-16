// MQTT Configuration - HiveMQ Cloud (Private)
const MQTT_HOST = "e947a9991cc442918fe1e94b5268b686.s1.eu.hivemq.cloud";  // HiveMQ Cloud riêng
const MQTT_PORT = 8884; // WebSocket Secure Port (WSS - cho HTTPS/Netlify)
const MQTT_PATH = "/mqtt"; // WebSocket path
const MQTT_CLIENT_ID = "WebClient_" + Math.random().toString(16).substring(2, 10);  // Random Client ID
const MQTT_USERNAME = "pumpuser";  // HiveMQ Cloud username
const MQTT_PASSWORD = "pump123456A";  // HiveMQ Cloud password

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
        useSSL: true,  // WSS (mã hóa) - BẮT BUỘC cho HTTPS/Netlify
        onSuccess: onConnect,
        onFailure: onFailure,
        keepAliveInterval: 30,
        cleanSession: true,
        timeout: 10,
        userName: MQTT_USERNAME,  // HiveMQ Cloud authentication
        password: MQTT_PASSWORD   // HiveMQ Cloud authentication
    };

    client.connect(options);
}

function onConnect() {
    console.log("MQTT Connected");
    mqttConnected = true;
    reconnectAttempts = 0; // Reset reconnect counter on successful connection
    addLog("✅ Đã kết nối tới HiveMQ Public Broker");
    updateWifiStatus("Đã kết nối Cloud");

    // Subscribe to topics with QoS 1 and callbacks
    client.subscribe(TOPIC_SENSOR_DATA, {
        qos: 1,
        onSuccess: function () {
            console.log("✅ Subscribed to:", TOPIC_SENSOR_DATA, "with QoS 1");
            addLog("Subscribe: " + TOPIC_SENSOR_DATA);
        },
        onFailure: function (err) {
            console.error("❌ Failed to subscribe to:", TOPIC_SENSOR_DATA, err);
            addLog("⚠️ Lỗi subscribe: " + TOPIC_SENSOR_DATA);
        }
    });

    client.subscribe(TOPIC_PUMP_STATUS, {
        qos: 1,
        onSuccess: function () {
            console.log("✅ Subscribed to:", TOPIC_PUMP_STATUS, "with QoS 1");
            addLog("Subscribe: " + TOPIC_PUMP_STATUS);
        },
        onFailure: function (err) {
            console.error("❌ Failed to subscribe to:", TOPIC_PUMP_STATUS, err);
            addLog("⚠️ Lỗi subscribe: " + TOPIC_PUMP_STATUS);
        }
    });

    client.subscribe(TOPIC_SYSTEM_STATUS, {
        qos: 1,
        onSuccess: function () {
            console.log("✅ Subscribed to:", TOPIC_SYSTEM_STATUS, "with QoS 1");
            addLog("Subscribe: " + TOPIC_SYSTEM_STATUS);
        },
        onFailure: function (err) {
            console.error("❌ Failed to subscribe to:", TOPIC_SYSTEM_STATUS, err);
            addLog("⚠️ Lỗi subscribe: " + TOPIC_SYSTEM_STATUS);
        }
    });
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
    const topic = message.destinationName;
    const payload = message.payloadString;

    // Enhanced logging
    console.log("📩 Message Arrived:");
    console.log("  Topic:", topic);
    console.log("  Payload:", payload);
    console.log("  QoS:", message.qos);
    console.log("  Retained:", message.retained);

    let data;
    try {
        data = JSON.parse(payload);
    } catch (e) {
        console.error("❌ JSON Parse Error on topic", topic, ":", e);
        console.error("  Raw payload:", payload);
        addLog("⚠️ Lỗi parse JSON từ: " + topic);
        return;
    }

    if (topic === TOPIC_SENSOR_DATA) {
        console.log("📊 Processing sensor data:", data);
        updateSensorUI(data);
        addLog("Nhận dữ liệu cảm biến: Độ ẩm " + data.soil_moisture + "%");
    } else if (topic === TOPIC_PUMP_STATUS) {
        console.log("🔄 Processing pump status:", data);
        updatePumpUI(data);
        addLog("Cập nhật trạng thái bơm: " + data.status);
    } else if (topic === TOPIC_SYSTEM_STATUS) {
        console.log("ℹ️ System status received:", data);
        // Optional: Update system info if needed
    } else {
        console.warn("⚠️ Unknown topic:", topic);
    }
}

function publishMessage(topic, payloadObj) {
    if (!mqttConnected) {
        addLog("Chưa kết nối MQTT, không thể gửi lệnh");
        return;
    }
    const message = new Paho.MQTT.Message(JSON.stringify(payloadObj));
    message.destinationName = topic;
    message.qos = 1;  // QoS 1 for at-least-once delivery
    message.retained = false;  // Don't retain control messages

    console.log("📤 Publishing message:");
    console.log("  Topic:", topic);
    console.log("  Payload:", JSON.stringify(payloadObj));
    console.log("  QoS:", message.qos);

    client.send(message);
}

// ===================================================================
// DATABASE INTEGRATION FUNCTIONS
// ===================================================================

/**
 * Log event to database via Netlify Function
 * @param {string} eventType - "PUMP_ON" | "PUMP_OFF" | "MODE_CHANGE"
 * @param {string} oldValue - Previous state
 * @param {string} newValue - New state
 * @param {object} metadata - Additional data (soil_moisture, pump_speed, etc.)
 */
async function logEventToDB(eventType, oldValue, newValue, metadata = {}) {
    try {
        const response = await fetch('/.netlify/functions/log-event', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json'
            },
            body: JSON.stringify({
                event_type: eventType,
                old_value: oldValue,
                new_value: newValue,
                triggered_by: 'mqtt',
                metadata: metadata
            })
        });

        const result = await response.json();

        if (result.success) {
            console.log('✅ Event logged to database:', eventType);
        } else {
            console.error('❌ Failed to log event:', result.error);
        }
    } catch (error) {
        console.error('❌ Database logging error:', error);
        // Don't throw - fail silently to not break UI
    }
}

/**
 * Log sensor data to database (called every 5 minutes)
 */
async function logSensorToDB() {
    try {
        const response = await fetch('/.netlify/functions/log-sensor', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json'
            },
            body: JSON.stringify({
                soil_moisture: soilMoisture,
                rain_status: rainStatus === 1,
                pump_status: pumpStatus,
                auto_mode: autoMode,
                pump_speed: pumpSpeed
            })
        });

        const result = await response.json();

        if (result.success) {
            console.log('📊 Sensor data logged to database');
        }
    } catch (error) {
        console.error('❌ Sensor logging error:', error);
    }
}

/**
 * Load statistics from database and update UI
 * @param {string} period - "today" | "7days" | "30days"
 */
async function loadStats(period = '7days') {
    try {
        const response = await fetch(`/.netlify/functions/get-stats?period=${period}`);
        const stats = await response.json();

        if (stats.success) {
            // Update statistics UI
            document.getElementById('pumpOnCount').textContent = stats.pump_on_count;
            document.getElementById('modeChangeCount').textContent = stats.mode_changes;
            document.getElementById('totalRuntime').textContent = stats.total_runtime_minutes + ' phút';
            document.getElementById('avgMoisture').textContent = stats.avg_soil_moisture.toFixed(1) + '%';

            // Update last update timestamp
            const updateTime = new Date(stats.last_updated).toLocaleTimeString('vi-VN');
            document.getElementById('statsLastUpdate').textContent = `Cập nhật lúc ${updateTime}`;

            console.log('📊 Statistics loaded successfully');
        } else {
            console.error('❌ Failed to load stats:', stats.error);
            document.getElementById('statsLastUpdate').textContent = 'Lỗi tải dữ liệu';
        }
    } catch (error) {
        console.error('❌ Stats loading error:', error);
        document.getElementById('statsLastUpdate').textContent = 'Chưa kết nối database';
    }
}

// Global variables to track previous states for change detection
let prevPumpStatusForDB = null;
let prevAutoModeForDB = null;
let lastSensorLogTime = 0;
const SENSOR_LOG_INTERVAL = 5 * 60 * 1000; // 5 minutes

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

    // 🔹 DATABASE INTEGRATION: Log pump state changes
    if (prevPumpStatusForDB !== null && prevPumpStatusForDB !== isPumpOn) {
        const eventType = isPumpOn ? 'PUMP_ON' : 'PUMP_OFF';
        const oldVal = prevPumpStatusForDB ? 'ON' : 'OFF';
        const newVal = isPumpOn ? 'ON' : 'OFF';
        logEventToDB(eventType, oldVal, newVal, {
            soil_moisture: soilMoisture,
            pump_speed: pumpSpeed
        });
    }
    prevPumpStatusForDB = isPumpOn;

    document.getElementById('pumpStatus').textContent = isPumpOn ? 'ĐANG BƠM' : 'ĐÃ TẮT';
    document.getElementById('pumpStatusBadge').textContent = isPumpOn ? 'ON' : 'OFF';
    document.getElementById('pumpStatusBadge').className = isPumpOn ? 'status-badge status-on' : 'status-badge status-off';

    // data.mode is "AUTO" or "MANUAL"
    const isAuto = (data.mode === "AUTO");

    // 🔹 DATABASE INTEGRATION: Log mode changes
    if (prevAutoModeForDB !== null && prevAutoModeForDB !== isAuto) {
        const oldMode = prevAutoModeForDB ? 'AUTO' : 'MANUAL';
        const newMode = isAuto ? 'AUTO' : 'MANUAL';
        logEventToDB('MODE_CHANGE', oldMode, newMode, {
            soil_moisture: soilMoisture
        });
    }
    prevAutoModeForDB = isAuto;

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
    const wasAuto = !isManual;

    // Optimistic UI update
    const modeStatus = document.getElementById('modeStatus');
    const manualControls = document.getElementById('manualControls');

    if (isManual) {
        modeStatus.textContent = 'CHẾ ĐỘ THỦ CÔNG';
        modeStatus.className = 'status-badge status-manual';
        manualControls.style.display = 'block';
        addLog('Đang chuyển sang chế độ thủ công...');
        publishMessage(TOPIC_MODE_CONTROL, { mode: "MANUAL" });

        // 🔹 DATABASE INTEGRATION: Log mode change
        logEventToDB('MODE_CHANGE', 'AUTO', 'MANUAL', { soil_moisture: soilMoisture });
    } else {
        modeStatus.textContent = 'CHẾ ĐỘ TỰ ĐỘNG';
        modeStatus.className = 'status-badge status-auto';
        manualControls.style.display = 'none';
        addLog('Đang chuyển sang chế độ tự động...');
        publishMessage(TOPIC_MODE_CONTROL, { mode: "AUTO" });

        // 🔹 DATABASE INTEGRATION: Log mode change
        logEventToDB('MODE_CHANGE', 'MANUAL', 'AUTO', { soil_moisture: soilMoisture });
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

    // 🔹 DATABASE INTEGRATION: Load statistics on page load
    setTimeout(() => {
        loadStats('7days'); // Load after 3 seconds to ensure MQTT connected
    }, 3000);

    // 🔹 DATABASE INTEGRATION: Auto-refresh statistics every 30 seconds
    setInterval(() => {
        loadStats('7days');
    }, 30000);

    // 🔹 DATABASE INTEGRATION: Log sensor data every 5 minutes
    setInterval(() => {
        if (mqttConnected) {
            logSensorToDB();
        }
    }, SENSOR_LOG_INTERVAL);
});

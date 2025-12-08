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
    if (confirm('Bạn có chắc chắn muốn reset thiết lập?')) {
        fetch('/reset-setup')
            .then(response => {
                if (response.ok) {
                    alert('Đã reset thiết lập thành công!');
                    setTimeout(() => {
                        window.location.reload();
                    }, 1000);
                }
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
document.addEventListener('DOMContentLoaded', function () {
    console.log('DOM loaded, initializing event listeners...');

    // Mode toggle
    document.getElementById('modeToggle').addEventListener('change', toggleMode);

    // Pump speed slider
    document.getElementById('pumpSpeedSlider').addEventListener('input', function () {
        updatePumpSpeed(this.value);
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
    setInterval(fetchData, 2000);
    fetchData();
    addLog('Hệ thống khởi động thành công');

    console.log('Event listeners initialized successfully');
});

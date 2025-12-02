#ifndef WIFI_CONFIG_H
#define WIFI_CONFIG_H

// WiFi Configuration HTML interface
const char* wifiConfigHTML = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>WiFi Configuration</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            padding: 20px;
            min-height: 100vh;
        }
        .container {
            max-width: 500px;
            margin: 0 auto;
            background: white;
            padding: 30px;
            border-radius: 20px;
            box-shadow: 0 8px 32px rgba(0,0,0,0.1);
        }
        h1 {
            color: #333;
            text-align: center;
            margin-bottom: 30px;
        }
        .form-group {
            margin-bottom: 20px;
        }
        label {
            display: block;
            margin-bottom: 5px;
            font-weight: bold;
            color: #555;
        }
        select, input {
            width: 100%;
            padding: 12px;
            border: 2px solid #ddd;
            border-radius: 10px;
            font-size: 16px;
            transition: border 0.3s;
        }
        select:focus, input:focus {
            border-color: #667eea;
            outline: none;
        }
        .btn {
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
            border: none;
            padding: 15px;
            border-radius: 10px;
            font-size: 16px;
            font-weight: bold;
            cursor: pointer;
            width: 100%;
            transition: transform 0.3s;
        }
        .btn:hover {
            transform: translateY(-2px);
        }
        .btn-secondary {
            background: #6c757d;
            margin-top: 10px;
        }
        .status {
            padding: 10px;
            border-radius: 5px;
            margin: 10px 0;
            text-align: center;
            font-weight: bold;
        }
        .success {
            background: #d4edda;
            color: #155724;
        }
        .error {
            background: #f8d7da;
            color: #721c24;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>🔧 WiFi Configuration</h1>
        
        <div class="form-group">
            <label for="wifiSelect">Select WiFi Network:</label>
            <select id="wifiSelect">
                <option value="">-- Select Network --</option>
            </select>
        </div>
        
        <div class="form-group">
            <label for="ssid">Or Enter SSID:</label>
            <input type="text" id="ssid" placeholder="WiFi SSID">
        </div>
        
        <div class="form-group">
            <label for="password">Password:</label>
            <input type="password" id="password" placeholder="WiFi Password">
        </div>
        
        <div id="status"></div>
        
        <button class="btn" onclick="connectWiFi()">🔗 Connect to WiFi</button>
        <button class="btn btn-secondary" onclick="scanWiFi()">🔍 Scan Networks</button>
        <button class="btn btn-secondary" onclick="goToMain()">🏠 Back to Main</button>
        
        <div style="margin-top: 20px; text-align: center;">
            <small>Current IP: %ESP_IP%</small><br>
            <small>AP SSID: SmartIrrigation_AP</small><br>
            <small>AP Password: 12345678</small>
        </div>
    </div>
    
    <script>
        function scanWiFi() {
            fetch('/wifi-scan')
                .then(response => response.json())
                .then(networks => {
                    const select = document.getElementById('wifiSelect');
                    select.innerHTML = '<option value="">-- Select Network --</option>';
                    networks.forEach(network => {
                        const option = document.createElement('option');
                        option.value = network;
                        option.textContent = network;
                        select.appendChild(option);
                    });
                    showStatus('Found ' + networks.length + ' networks', 'success');
                })
                .catch(error => {
                    showStatus('Scan failed: ' + error, 'error');
                });
        }
        
        function connectWiFi() {
            const ssid = document.getElementById('ssid').value || 
                        document.getElementById('wifiSelect').value;
            const password = document.getElementById('password').value;
            
            if (!ssid || !password) {
                showStatus('Please enter SSID and password', 'error');
                return;
            }
            
            const formData = new FormData();
            formData.append('ssid', ssid);
            formData.append('password', password);
            
            showStatus('Connecting...', 'success');
            
            fetch('/wifi-connect', {
                method: 'POST',
                body: formData
            })
            .then(response => response.json())
            .then(data => {
                if (data.status === 'success') {
                    showStatus('Connected! IP: ' + data.ip, 'success');
                    setTimeout(() => {
                        window.location.href = '/';
                    }, 3000);
                } else {
                    showStatus('Connection failed', 'error');
                }
            })
            .catch(error => {
                showStatus('Error: ' + error, 'error');
            });
        }
        
        function goToMain() {
            window.location.href = '/';
        }
        
        function showStatus(message, type) {
            const statusDiv = document.getElementById('status');
            statusDiv.textContent = message;
            statusDiv.className = 'status ' + type;
        }
        
        // Auto scan on load
        window.onload = scanWiFi;
    </script>
</body>
</html>
)rawliteral";

#endif // WIFI_CONFIG_H

# Test ESP32 Connectivity for OTA
# Run this before attempting OTA upload

Write-Host "`n╔════════════════════════════════════════╗" -ForegroundColor Cyan
Write-Host "║  ESP32 OTA Connectivity Test          ║" -ForegroundColor Cyan
Write-Host "╚════════════════════════════════════════╝`n" -ForegroundColor Cyan

$ESP_IP = "192.168.0.107"

# Test 1: Ping
Write-Host "[1/3] Testing Ping..." -ForegroundColor Yellow
$pingResult = Test-Connection -ComputerName $ESP_IP -Count 2 -Quiet
if ($pingResult) {
    Write-Host "  ✅ Ping OK - ESP32 is online`n" -ForegroundColor Green
} else {
    Write-Host "  ❌ Ping FAILED - ESP32 is offline or wrong IP`n" -ForegroundColor Red
    exit 1
}

# Test 2: Web Server (Port 80)
Write-Host "[2/3] Testing Web Server (Port 80)..." -ForegroundColor Yellow
try {
    $response = Invoke-WebRequest -Uri "http://$ESP_IP" -TimeoutSec 5 -UseBasicParsing -ErrorAction Stop
    Write-Host "  ✅ Web Server OK - Status Code: $($response.StatusCode)`n" -ForegroundColor Green
} catch {
    Write-Host "  ❌ Web Server NOT responding`n" -ForegroundColor Red
}

# Test 3: OTA Port (3232)
Write-Host "[3/3] Testing OTA Port (3232)..." -ForegroundColor Yellow
$tcpClient = New-Object System.Net.Sockets.TcpClient
try {
    $tcpClient.Connect($ESP_IP, 3232)
    if ($tcpClient.Connected) {
        Write-Host "  ✅ OTA Port OPEN - Ready for upload`n" -ForegroundColor Green
        $tcpClient.Close()
    }
} catch {
    Write-Host "  ⚠️  OTA Port NOT responding (this is normal until upload starts)`n" -ForegroundColor Yellow
}

# Summary
Write-Host "`n╔════════════════════════════════════════╗" -ForegroundColor Cyan
Write-Host "║  Summary                               ║" -ForegroundColor Cyan
Write-Host "╚════════════════════════════════════════╝" -ForegroundColor Cyan

if ($pingResult) {
    Write-Host "ESP32 IP: $ESP_IP" -ForegroundColor White
    Write-Host "Network: ✅ ONLINE" -ForegroundColor Green
    Write-Host "`nRecommendation:" -ForegroundColor Yellow
    Write-Host "1. If web server OK → ESP32 is running, try OTA upload" -ForegroundColor White
    Write-Host "2. If OTA fails → Press RESET button on ESP32 and wait 30 seconds" -ForegroundColor White
    Write-Host "3. Then retry: pio run --target upload`n" -ForegroundColor White
} else {
    Write-Host "❌ ESP32 is OFFLINE - Check:" -ForegroundColor Red
    Write-Host "   - Power supply" -ForegroundColor White
    Write-Host "   - WiFi connection" -ForegroundColor White
    Write-Host "   - IP address (check Serial Monitor)`n" -ForegroundColor White
}

# Pumping-system-IOT
Hệ thống máy bơm tự động điều khiển bằng IOT
hệ thống tưới cây tự động trong đó có UART cho arduino uno r3 với Esp32:
-Arduino Uno: dùng để kết nối với 2 cảm biến gồm relay cảm biến mưa và cảm biến độ ẩm đất và kết nối với mạch relay kết nối máy bơm.
 +) mạch cảm biến mưa: là một bảng 2 chân nối với một relay cảm biến, arduino uno sẽ đọc cảm biến từ relay đó.
  +) Cảm biến độ ẩm đất 3 chân.
  +) mạch relay máy bơm loại 5VDC
-Esp32: chịu trách nhiệm nhận tín hiệu từ Arduino và kết nối wifi với dashboard giao diện html bằng phương thức http, cùng với đó hiển thị thêm đồng hồ thời gian thực build-in esp trên để áp dụng điều kiện auto. Người dùng có thể chọn các chức năng sau: đổi chế độ auto/manual, khi manual sẽ hiển thị thêm thanh slider điều chỉnh tốc độ dòng chảy nước máy bơm. Còn khi auto sẽ theo thời gian thực 6h or 17h < cảm biến mưa không mưa < độ ẩm đất khô theo ưu tiên từ thấp đến cao. Khi có thay đổi bất kì sẽ lưu dữ liệu vào một list box trên giao diện.

Quy ước chung		
		
Arduino Uno – Cảm biến & Relay		
Chân kết nối:		
rainSensorPin = 7 → cảm biến mưa (digital relay sensor).		
soilSensorPin = A0 → cảm biến độ ẩm đất (analog).		
pumpRelayPin = 8 → relay máy bơm.		
EspSerial (SoftwareSerial RX=2, TX=3) → UART giao tiếp với ESP32.		
Biến:		
rainState (0 = không mưa, 1 = mưa).		
soilValue (0–1023).		
pumpState (true = ON, false = OFF).		
ESP32 – UART & Đồng hồ thời gian thực		
Chân UART2:		
RXD2 = 16, TXD2 = 17.		
UnoSerial (HardwareSerial) → giao tiếp với Uno.		
Biến trạng thái:		
rain (0/1).		
soil (0–1023). đất càng khô -> số càng nhỏ		
pump (true/false).		
modeAuto (true = Auto, false = Manual).		
manualFlowPercent (0–100%).		
autoHourSelected (6 hoặc 17).		
Đồng hồ NTP:		
getCurrentHour() → trả về giờ hiện tại (0–23).		
getCurrentTimeStr() → chuỗi thời gian hiển thị.		
Ngưỡng độ ẩm đất:		
soilDryThreshold = 700 (có thể chỉnh thực tế).		
		
Giao thức UART (JSON)		
Uno → ESP32 (telemetry):		
{"rain":0,"soil":512,"pump":1}		
ESP32 → Uno (command):		
{"cmd":"pump","state":1}		
		
Uno: đọc cảm biến, điều khiển relay, gửi dữ liệu JSON.		
ESP32: nhận dữ liệu, xử lý Auto/Manual, đồng bộ Blynk, hiển thị đồng hồ.		
HTML: Tạo trang web điều khiển hệ thống		
UART JSON: giao tiếp hai chiều, dễ debug và mở rộng.		


upload: double click file upload_all.bat
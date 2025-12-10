## Hệ Thống Kiểm Soát Ra Vào Thông Minh AIoT

Hệ thống quản lý bãi đỗ xe thông minh (AIoT Parking Access Control - PAC)

Dự án PAC sử dụng ESP32-CAM tích hợp AI (Edge Impulse) để phát hiện người, tự động chụp ảnh gửi lên Cloud (Firebase), và giao tiếp với Server xử lý trung tâm để điều khiển đóng/mở cổng hoặc gửi cảnh báo qua Email.
* Khi vào gửi xe, hệ thống sẽ lấy ảnh khuôn mặt và biển số để lưu thông tin.
* Khi xe đi ra, hệ thống sẽ lấy cơ sở dữ liệu đã lưu trước đó để kiểm tra có đúng thông tin người gửi.

### Tính Năng Chính
-   Nhận diện tại biên (Edge AI): Sử dụng mô hình Edge Impulse chạy trực tiếp trên ESP32 để phát hiện khuôn mặt người theo thời gian thực.
-   Lưu trữ đám mây: Tự động chụp và tải hình ảnh lên Firebase Storage.
-   Server tải ảnh về và xử lý Nhận diện Khuôn mặt + Biển số xe.
-   Điều khiển thời gian thực:
    - Hợp lệ (PASS): Mở cổng tự động.
    - Vi phạm (FAIL): Bật đèn báo động và gửi Email cảnh báo.
-   Mọi quá trình hoạt động của hệ thống sẽ được trực quan hóa bằng đèn LED.

### Yêu Cầu Phần Cứng
- AI Thinker ESP32-CAM
- Module nạp FTDI FT232RL
- Servo SG90 hoặc MG996R
- LED: Vàng – Đỏ – Xanh lá
- Jumper, Breadboard, nguồn 5V (>1A)

### Sơ Đồ Nối Dây

| Thiết bị | GPIO | Chức năng |
| :--- | :--- | :--- |
| LED Vàng | 12 | Báo đang chụp / chờ Server |
| LED Đỏ | 14 | Báo lỗi (FAIL_IN, FAIL_OUT, Timeout hoặc lỗi kết nối Server) |
| LED Xanh | 15 | Báo hợp lệ |
| Servo | 13 | PWM |

Lưu ý: Khi nạp code → nối IO0 với GND. Khi chạy → tháo IO0 và Reset.

### Yêu Cầu Phần Mềm & Thư Viện
**1. Arduino IDE (ESP32)** 

Cài các thư viện:
- FirebaseClient (mobizt)
- ReadyMail (mobizt)
- ESP32Servo (Kevin Harrington)
- Edge Impulse SDK (xuất từ [ei-detection-face-arduino-1.0.2.zip](https://drive.google.com/file/d/1XcXOS-NsnK1AXM9lq0sCVTFBR-Q4y79S/view?usp=drive_link))

**2. Server** 
* Cài đặt trên môi trường python 3.11
* Cài đặt yolov5.
* Cài đặt các thư viện trong requirements.txt.
* Tải file trọng số yolov5su.pt : https://www.bing.com/search?qs=SC&pq=yolov5su.pt+d&sk=CSYN1&sc=3-13&q=yolov5s.pt+download&cvid=59aab2c1fe9c49c1a213f540989ba30f&gs_lcrp=EgRlZGdlKgYIARAAGEAyBggAEEUYOTIGCAEQABhAMgYIAhAAGEAyCAgDEOkHGPxV0gEIMTMyNWowajSoAgiwAgE&FORM=ANAB01&PC=LCTS
* Tải các file trọng số :
  - Phát hiện biển số : [https://drive.google.com/drive/u/0/folders/18UhhcOFV1Ew-98G3BAfpE8v2D9B-qmJh](https://drive.google.com/drive/u/0/folders/18UhhcOFV1Ew-98G3BAfpE8v2D9B-qmJh)
  - OCR biển số : [https://drive.google.com/drive/u/0/folders/11hXqQFjcrLv5xR5ePFQD8uAIo9oBaplR](https://drive.google.com/drive/u/0/folders/11hXqQFjcrLv5xR5ePFQD8uAIo9oBaplR)
  - Sau khi tải về local đọc file License_Plate/plate_ocr.py để truyền trọng số.
* Tạo các database và storage trên firebase và lấy các file .json của database và storage.
* Đọc comment trong file Data_Processing/basic_processing.py để biết cách truyền đường dẫn file .json.

### Cấu Trúc Thư Mục Code Arduino IDE (ESP32)

    /PAC-ESP32
    │── PAC-ESP32.ino   # File chính
    │── Config.h                   # Thông tin WiFi, Firebase, Email
    │── Network.h                  # WiFi + Upload ảnh + API Server
    │── Hardware.h                 # LED + Servo
    │── Camera.h                   # Camera


### Hướng Dẫn Cài Đặt & Chạy
#### Cấu hình
**Bước 1 — Cấu hình Server**
Mở CMD → gõ ipconfig để lấy IPv4 (vd: 192.168.1.10).

**Bước 2 — Cấu hình ESP32**
- Mở Face_Detection_Final.ino trong Arduino IDE --> Mở file Config.h.
- Cập nhật: 
  - WIFI_SSID / WIFI_PASSWORD
  - serverBaseUrl = "http://<IPv4>:5000" (Lấy từ Bước 1)
  - Thông tin Firebase và Email App Password
- Nạp code vào ESP32-CAM.

#### Thực thi luồng vào
```bash
python server_in.py

```

#### Thực thi luồng ra
```bash
python server_out.py

```
**Vận hành** 

- Đèn vàng nhấp nháy → phát hiện người, đang tải ảnh
- Đèn vàng sáng → chờ Server xử lý
- Đèn xanh → PASS → mở cổng
- Đèn đỏ chớp nháy 1 lần → Lấy dữ liệu lỗi → Chụp lại hình
- Đèn đỏ → FAIL → gửi Email cảnh báo

### Khắc Phục Lỗi Thường Gặp

| Hiện tượng | Nguyên nhân | Cách khắc phục |
| :--- | :--- | :--- |
| Lỗi Connection Refused (-1) | ESP32 không thấy Server. | 1. Kiểm tra lại IP trong `Config.h`.<br>2. Tắt Windows Firewall.<br>3. Đảm bảo máy tính và ESP32 cùng WiFi. |
| Lỗi HTTP 404 | Sai đường dẫn API. | Server đang chạy code cũ? Hãy đảm bảo Server có các hàm `/trigger-process`. |
| Lỗi Camera Init Failed | Lỏng dây hoặc thiếu nguồn. | Kiểm tra cáp camera, đảm bảo nguồn cấp đủ 5V/2A. |
| Brownout Detector was triggered | Nguồn yếu. | Thay dây cáp USB mới hoặc nguồn tốt hơn. |

### Luồng Hoạt Động (Flowchart)


    Start((Bắt đầu)) --> Detect{AI Phát hiện người?}
    Detect -- Có --> Yellow[Đèn Vàng: ON]
    Yellow --> Upload[Gửi ảnh lên Firebase 20s]
    Upload --> Trigger[Gửi lệnh Trigger tới Server]
    Trigger --> Polling{Hỏi thăm Server...} 
    Polling -- Đang xử lý --> Wait[Đợi 10s] --> Polling
    Polling -- Xong --> Result{Kết quả?}
    
    Result -- PASS --> Green[Đèn Xanh: ON] --> OpenGate[Mở Cổng 10s] --> CloseGate[Đóng Cổng]
    Result -- FAIL --> Red[Đèn Đỏ: ON] --> Email[Gửi Email Cảnh Báo]
    
    CloseGate --> End((Kết thúc))
    Email --> End

Developed by vinhquyen-lee, nguyenducmanh-itus

#



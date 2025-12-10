#ifndef CONFIG_H
#define CONFIG_H

// ======================= CẤU HÌNH WIFI =======================
#define WIFI_SSID "your-wifi-ssid"
#define WIFI_PASSWORD "wifi-password"

// ===================== CẤU HÌNH SERVER =====================
const char* serverBaseUrl = "http://192.168.137.1:5000";

// ==================== CẤU HÌNH FIREBASE ====================
#define API_KEY "AIzaSyB-aroEbjjPH83grcwXkax6d-JTXz8mQJc"
#define USER_EMAIL "your-email"
#define USER_PASSWORD "password"
#define STORAGE_BUCKET_ID "storage-bucket-id" //Firebase Storage Bucket ID

// ====================== CẤU HÌNH SMTP ======================
#define SMTP_HOST "smtp.gmail.com"
#define SMTP_PORT 465
#define AUTHOR_EMAIL "your-email"
#define AUTHOR_PASSWORD "app-password" // App Password
#define RECIPIENT_EMAIL "recipient-email"

// =================== CẤU HÌNH CHÂN GPIO ===================
#define PIN_LED_YELLOW  12  // Đèn Vàng
#define PIN_LED_RED     14  // Đèn Đỏ
#define PIN_LED_GREEN   15  // Đèn Xanh
#define PIN_SERVO       13  // Servo

// ==================== CẤU HÌNH CAMERA ====================
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

#define EI_CAMERA_RAW_FRAME_BUFFER_COLS     320
#define EI_CAMERA_RAW_FRAME_BUFFER_ROWS     240
#define EI_CAMERA_FRAME_BYTE_SIZE           3

#endif
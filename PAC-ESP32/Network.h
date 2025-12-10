#ifndef NETWORK_H
#define NETWORK_H

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

#define ENABLE_USER_AUTH
#define ENABLE_STORAGE
#define ENABLE_SMTP 

#include <FirebaseClient.h>
#include <ReadyMail.h>
#include <HTTPClient.h>

#include "Config.h"
#include "Hardware.h"

extern bool taskComplete;
extern int photoCounter;
extern unsigned long lastShotTime;

// Các đối tượng mạng
UserAuth user_auth(API_KEY, USER_EMAIL, USER_PASSWORD, 3000);
FirebaseApp app;
WiFiClientSecure ssl_client; 
using AsyncClient = AsyncClientClass;
AsyncClient aClient(ssl_client);
Storage storage;
WiFiClientSecure smtp_ssl_client; 
SMTPClient smtp(smtp_ssl_client);

// --- HÀM GỬI MAIL ---
void smtpCb(SMTPStatus status) {
    if (status.progress.available)
        ReadyMail.printf("ReadyMail[smtp][%d] %d%% completed\n", status.state, status.progress.value);
}

void sendWarningMail() {
    Serial.println("Gửi mail cảnh báo...");
    smtp.stop(); 
    if (!smtp.connect(SMTP_HOST, SMTP_PORT, smtpCb)) { 
      Serial.println("Kết nối SMTP thất bại"); 
      return; 
    }

    if (!smtp.authenticate(AUTHOR_EMAIL, AUTHOR_PASSWORD, readymail_auth_password)) { 
      Serial.println("Đăng nhập SMTP thất bại"); 
      smtp.stop(); 
      return; 
    }
    
    SMTPMessage msg;
    msg.headers.add(rfc822_subject, "CANH BAO: Phat hien xe sai pham!");
    msg.headers.add(rfc822_from, "ESP32 Camera <" + String(AUTHOR_EMAIL) + ">");
    msg.headers.add(rfc822_to, "Admin <" + String(RECIPIENT_EMAIL) + ">");
    msg.text.body("HE THONG AI DA XAC NHAN: CO XE VI PHAM!\n");

    if (!smtp.send(msg)) {
      Serial.println("Gửi mail lỗi!");
    } else {
      Serial.println("Đã gửi mail thành công!");
    }
    smtp.stop();
}

// --- HÀM FIREBASE ---
void processData(AsyncResult &aResult) {
    if (aResult.isError()) {
        Firebase.printf("Error: %s\n", aResult.error().message().c_str());
        if (String(aResult.uid()).indexOf("upload") >= 0) { taskComplete = true; lastShotTime = millis(); }
    }
    if (aResult.uploadProgress()) {
        if (aResult.uploadInfo().total == aResult.uploadInfo().uploaded) {
            Serial.printf("Upload OK! Task: %s\n", aResult.uid().c_str());
            taskComplete = true; lastShotTime = millis();
        }
    }
}

void sendPhotoToFirebase() {
    camera_fb_t *fb = esp_camera_fb_get(); 
    if (!fb || fb->format != PIXFORMAT_JPEG) {
        if (fb) esp_camera_fb_return(fb);
        taskComplete = true; 
        return;
    }
    
    std::string photoPath = "/data/photo_" + std::to_string(photoCounter) + ".jpg ";
    Serial.printf("Uploading %d bytes -> %s\n", fb->len, photoPath.c_str());
    
    BlobConfig upload_image(fb->buf, fb->len);
    storage.upload(
        aClient,
        FirebaseStorage::Parent(STORAGE_BUCKET_ID, photoPath.c_str()),
        getBlob(upload_image),
        "image/jpg",
        processData,
        "uploadTask"
    ); 

    esp_camera_fb_return(fb); 
    photoCounter++; 
}

// --- HÀM GIAO TIẾP SERVER (POLLING) ---
void sendSignalToPC() {
    if(WiFi.status() == WL_CONNECTED){
        HTTPClient http;
        WiFiClient client;
        
        Serial.println("[HTTP] Bắt đầu quy trình xử lý (Polling)...");
        digitalWrite(PIN_LED_YELLOW, HIGH); // Đèn vàng sáng

        // 1. TRIGGER
        String triggerUrl = String(serverBaseUrl) + "/trigger-process";
        http.begin(client, triggerUrl);
        int httpCode = http.GET();
        http.end(); 

        if (httpCode != 200) {
            digitalWrite(PIN_LED_YELLOW, LOW);
              Serial.printf("Lỗi kích hoạt Server (Code: %d)\n", httpCode);
              if (httpCode == 404) Serial.println("   -> Lỗi: Server không có hàm /trigger-process (Code Server cũ?)");
              if (httpCode == -1) Serial.println("   -> Lỗi: Không kết nối được (Sai IP hoặc Firewall chặn)");
            
            blinkRedLed(3);
            return;
        }
        
        Serial.println("✅ Trigger OK. Đang chờ Server xử lý...");

        // 2. POLLING
        int maxRetries = 100; 
        while (maxRetries > 0) {
            maxRetries--;
            
            // Hỏi 10s/lần
            delay(10000); 

            String checkUrl = String(serverBaseUrl) + "/check-status";
            http.begin(client, checkUrl);
            int checkCode = http.GET();
            
            if (checkCode == 200) {
                String status = http.getString();
                Serial.println("   -> Status: " + status);

                // --- KẾT QUẢ ---
                if (status == "PASS") {
                    Serial.println("PASS -> Mở Cổng");
                    digitalWrite(PIN_LED_YELLOW, LOW);
                    digitalWrite(PIN_LED_GREEN, HIGH);
                    openGate();
                    delay(10000);
                    closeGate();
                    digitalWrite(PIN_LED_GREEN, LOW);
                    http.end(); 
                    return;
                }
                else if (status == "FAIL_OUT") {
                    Serial.println("FAIL_OUT -> Cảnh báo");
                    digitalWrite(PIN_LED_YELLOW, LOW);
                    digitalWrite(PIN_LED_RED, HIGH);
                    sendWarningMail();
                    delay(5000);
                    digitalWrite(PIN_LED_RED, LOW);
                    http.end(); return;
                }
                else if (status == "FAIL_IN") {
                    Serial.println("FAIL_IN -> Lỗi vào");
                    digitalWrite(PIN_LED_YELLOW, LOW);
                    blinkRedLed(1);
                    http.end(); return;
                }
                else if (status == "BUSY") {
                     Serial.println("Server đang bận, thử lại sau...");
                }
                // Nếu là PROCESSING hoặc IDLE thì vòng lặp tiếp tục chạy (Đèn vàng vẫn sáng)
            } else {
                Serial.printf("Lỗi kiểm tra trạng thái (Code: %d)\n", checkCode);
            }
            http.end();
        }
        
        digitalWrite(PIN_LED_YELLOW, LOW);
        Serial.println("❌ Timeout Polling!");
        blinkRedLed(5);

    } else { 
        Serial.println("Mất kết nối WiFi!"); 
    }
}

void initNetwork() {
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
    Serial.println("\nWiFi OK: " + WiFi.localIP().toString());
    WiFi.setSleep(false); 

    ssl_client.setInsecure();
    ssl_client.setHandshakeTimeout(15000); 
    initializeApp(aClient, app, getAuth(user_auth), processData, "authTask");
    app.getApp<Storage>(storage);
    
    smtp_ssl_client.setInsecure();
    if (smtp.connect(SMTP_HOST, SMTP_PORT, smtpCb)) { smtp.stop(); Serial.println("SMTP Check: OK"); } 
}

#endif
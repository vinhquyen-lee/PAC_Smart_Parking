#include <Arduino.h>

// Thư viện AI (Edge Impulse)
#include <Detection_Face_inferencing.h>
#include "edge-impulse-sdk/dsp/image/image.hpp"
#include "esp_heap_caps.h"
#include "Config.h"

// Các biến toàn cục
// Biến trạng thái
bool sendingMode = false;
bool taskComplete = true; 
unsigned long sendingStartTime = 0;
unsigned long lastShotTime = 0;
int photoCounter = 0;

// Biến LED
unsigned long lastBlinkTime = 0;
bool yellowLedState = false;

// Biến bộ đệm ảnh cho AI
uint8_t *snapshot_buf = nullptr;

#include "Hardware.h" // LED, Servo
#include "Camera.h"   // Camera
#include "Network.h"  // WiFi, Firebase, Mail, Server

static bool debug_nn = false;

// --- HÀM CALLBACK CHO EDGE IMPULSE ---
static int ei_camera_get_data(size_t offset, size_t length, float *out_ptr) {
    size_t pixel_ix = offset * 3;
    size_t pixels_left = length;
    size_t out_ptr_ix = 0;
    
    while (pixels_left != 0) {
        // Chuyển đổi RGB888 sang float input cho model
        out_ptr[out_ptr_ix] = (snapshot_buf[pixel_ix + 2] << 16) + (snapshot_buf[pixel_ix + 1] << 8) + snapshot_buf[pixel_ix];
        out_ptr_ix++;
        pixel_ix += 3;
        pixels_left--;
    }
    return 0;
}

// ======================= SETUP =======================
void setup() {
    Serial.begin(115200);
    Serial.println("\n=== AIoT SYSTEM STARTING ===");

    // 1. Khởi tạo LED, Servo
    initHardware(); 
    
    // 2. Khởi tạo Camera
    if (!ei_camera_init()) {
        Serial.println("Camera Init Failed!");
        while (1) delay(1000);
    }
    
    // 3. Cấp phát bộ nhớ cho AI (PSRAM)
    snapshot_buf = (uint8_t*) heap_caps_malloc(
        EI_CAMERA_RAW_FRAME_BUFFER_COLS * EI_CAMERA_RAW_FRAME_BUFFER_ROWS * EI_CAMERA_FRAME_BYTE_SIZE,
        MALLOC_CAP_SPIRAM
    );
    
    if (!snapshot_buf) {
        Serial.println("PSRAM Malloc Failed!");
        while(1) delay(1000);
    }

    // 4. Khởi tạo WiFi, Firebase, Mail
    initNetwork();

    Serial.println("System Ready. Starting detection...");
    taskComplete = true; 
}

// ======================= LOOP =======================
void loop() {
    unsigned long now = millis();
    
    // Duy trì kết nối Firebase 
    app.loop();            

    // --- GỬI ẢNH & CHỜ XỬ LÝ (20s) ---
    if (sendingMode) {
        
        // Nhấp nháy đèn vàng
        if (now - lastBlinkTime > 500) { 
            lastBlinkTime = now;
            yellowLedState = !yellowLedState; 
            digitalWrite(PIN_LED_YELLOW, yellowLedState);
        }

        // Hết 20 giây -> Gọi Server xử lý
        if (now - sendingStartTime > 20000) { 
            sendingMode = false;
            
            digitalWrite(PIN_LED_YELLOW, LOW); 
            
            Serial.println(">>> Đã gửi ảnh xong 20s. Gọi Server xử lý...");
            
            sendSignalToPC(); 
            
            Serial.println(">>> Hoàn tất quy trình. Tiếp tục giám sát.");
            return;
        }

        // Gửi ảnh lên Firebase 
        if (now - lastShotTime >= 2000 && taskComplete && app.ready()) {
            lastShotTime = now; 
            taskComplete = false; 
            sendPhotoToFirebase(); 
        }
        return; 
    }

    // --- CHẾ ĐỘ GIÁM SÁT (AI) ---
    if (ei_sleep(5) != EI_IMPULSE_OK) return;

    ei::signal_t signal;
    signal.total_length = EI_CLASSIFIER_INPUT_WIDTH * EI_CLASSIFIER_INPUT_HEIGHT;
    signal.get_data = &ei_camera_get_data;

    if (!ei_camera_capture((size_t)EI_CLASSIFIER_INPUT_WIDTH, (size_t)EI_CLASSIFIER_INPUT_HEIGHT, snapshot_buf)) return;

    ei_impulse_result_t result = { 0 };
    EI_IMPULSE_ERROR err = run_classifier(&signal, &result, debug_nn);
    if (err != EI_IMPULSE_OK) return;

    #if EI_CLASSIFIER_OBJECT_DETECTION == 1
        bool personDetected = false;
        for (uint32_t i = 0; i < result.bounding_boxes_count; i++) {
            ei_impulse_result_bounding_box_t bb = result.bounding_boxes[i];
            if (bb.value == 0) continue;
            
            if ((strcmp(bb.label, "person") == 0 || strcmp(bb.label, "0") == 0) && bb.value >= 0.5) {
                personDetected = true;
                break;
            }
        }

        // Nếu phát hiện người -> Chuyển sang chế độ gửi ảnh
        if (personDetected) {
            sendingMode = true;
            sendingStartTime = millis();
            lastShotTime = millis() - 2000; 
            taskComplete = true; 
            photoCounter = 0; 
            
            Serial.println("!!! PHÁT HIỆN NGƯỜI -> Bắt đầu gửi ảnh...");
        }
    #endif
}

// Kiểm tra model AI
#if !defined(EI_CLASSIFIER_SENSOR) || EI_CLASSIFIER_SENSOR != EI_CLASSIFIER_SENSOR_CAMERA
#error "Invalid model for current sensor"
#endif
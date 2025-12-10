#ifndef HARDWARE_H
#define HARDWARE_H

#include <Arduino.h>
#include <ESP32Servo.h>
#include "Config.h"

Servo myGateServo;

void initHardware() {
    // Cấu hình LED
    pinMode(PIN_LED_YELLOW, OUTPUT);
    digitalWrite(PIN_LED_YELLOW, LOW);
    
    pinMode(PIN_LED_RED, OUTPUT);
    digitalWrite(PIN_LED_RED, LOW);
    
    pinMode(PIN_LED_GREEN, OUTPUT);
    digitalWrite(PIN_LED_GREEN, LOW);
    
    // Cấu hình Servo
    myGateServo.setPeriodHertz(50);
    myGateServo.attach(PIN_SERVO, 1000, 2000); 
    myGateServo.write(0); // Đóng cổng
}

void openGate() {
    Serial.println("Opening Gate...");
    myGateServo.write(180);
}

void closeGate() {
    Serial.println("Closing Gate...");
    myGateServo.write(0);
}

// Nháy đèn đỏ báo lỗi
void blinkRedLed(int times) {
    for(int i=0; i<times; i++){
        digitalWrite(PIN_LED_RED, HIGH); delay(200);
        digitalWrite(PIN_LED_RED, LOW); delay(200);
    }
}

#endif
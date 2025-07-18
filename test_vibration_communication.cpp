/*
 * 震动传感器通信测试程序
 * 测试震动传感器触发后的ESP-NOW通信和计时功能
 */

#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <FastLED.h>
#include "config.h"

// 全局变量
DeviceRole deviceRole = ROLE_UNDEFINED;
uint8_t peerAddress[6];
CRGB leds[LED_COUNT];
unsigned long trainingStartTime = 0;
unsigned long lastVibrationTime = 0;
bool trainingActive = false;
bool vibrationDetected = false;

// 前向声明
void onDataReceived(const esp_now_recv_info* recv_info, const uint8_t* data, int len);
void onDataSent(const uint8_t* mac, esp_now_send_status_t status);
void initESPNow();
void initHardware();
void determineDeviceRole();
void updateVibrationSensor();
void updateLEDs();
void sendTrainingStart();
void sendTrainingComplete(unsigned long duration);
void displayMessage(const char* message);

void setup() {
    Serial.begin(115200);
    Serial.println("震动传感器通信测试程序");
    Serial.println("============================");
    
    // 初始化硬件
    initHardware();
    
    // 初始化WiFi
    WiFi.mode(WIFI_STA);
    
    // 确定设备角色
    determineDeviceRole();
    
    // 初始化ESP-NOW
    initESPNow();
    
    Serial.println("系统准备就绪");
    Serial.println("主设备: 按按钮开始训练");
    Serial.println("从设备: 等待开始信号");
    
    // 设置LED为就绪状态
    fill_solid(leds, LED_COUNT, CRGB::Green);
    FastLED.show();
}

void loop() {
    updateVibrationSensor();
    updateLEDs();
    
    // 主设备：检查按钮按下开始训练
    if (deviceRole == ROLE_MASTER && digitalRead(BUTTON_PIN) == LOW) {
        if (!trainingActive) {
            sendTrainingStart();
            trainingActive = true;
            trainingStartTime = millis();
            displayMessage("训练开始");
            Serial.println("主设备: 训练开始");
            
            // LED变为蓝色表示训练中
            fill_solid(leds, LED_COUNT, CRGB::Blue);
            FastLED.show();
        }
        delay(200); // 防抖
    }
    
    // 检查训练超时
    if (trainingActive && (millis() - trainingStartTime > 30000)) {
        trainingActive = false;
        Serial.println("训练超时");
        displayMessage("训练超时");
        
        // LED变为红色表示超时
        fill_solid(leds, LED_COUNT, CRGB::Red);
        FastLED.show();
        
        delay(2000);
        fill_solid(leds, LED_COUNT, CRGB::Green);
        FastLED.show();
    }
    
    delay(10);
}

void initHardware() {
    // 初始化LED (GPIO1)
    FastLED.addLeds<NEOPIXEL, LED_PIN>(leds, LED_COUNT);
    FastLED.setBrightness(LED_BRIGHTNESS);
    
    // 初始化按钮 (GPIO0)
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    
    // 初始化蜂鸣器 (GPIO4)
    pinMode(BUZZER_PIN, OUTPUT);
    
    // 初始化震动传感器 (GPIO2)
    pinMode(VIBRATION_SENSOR_PIN, INPUT);
    
    Serial.println("硬件初始化完成");
    Serial.printf("LED引脚: GPIO%d\n", LED_PIN);
    Serial.printf("按钮引脚: GPIO%d\n", BUTTON_PIN);
    Serial.printf("蜂鸣器引脚: GPIO%d\n", BUZZER_PIN);
    Serial.printf("震动传感器引脚: GPIO%d\n", VIBRATION_SENSOR_PIN);
    Serial.printf("OLED SDA: GPIO%d, SCL: GPIO%d\n", OLED_SDA_PIN, OLED_SCL_PIN);
}

void determineDeviceRole() {
    uint8_t mac[6];
    WiFi.macAddress(mac);
    
    uint8_t deviceA[] = DEVICE_A_MAC;
    uint8_t deviceB[] = DEVICE_B_MAC;
    
    #ifdef FORCE_MASTER_ROLE
        deviceRole = ROLE_MASTER;
        memcpy(peerAddress, deviceB, 6);
        Serial.println("设备角色: 主设备 (Master) - 强制设置");
        return;
    #endif
    
    #ifdef FORCE_SLAVE_ROLE
        deviceRole = ROLE_SLAVE;
        memcpy(peerAddress, deviceA, 6);
        Serial.println("设备角色: 从设备 (Slave) - 强制设置");
        return;
    #endif
    
    // 根据MAC地址识别
    if (memcmp(mac, deviceA, 6) == 0) {
        deviceRole = ROLE_MASTER;
        memcpy(peerAddress, deviceB, 6);
        Serial.println("设备角色: 主设备 (Master) - 自动识别");
    } else if (memcmp(mac, deviceB, 6) == 0) {
        deviceRole = ROLE_SLAVE;
        memcpy(peerAddress, deviceA, 6);
        Serial.println("设备角色: 从设备 (Slave) - 自动识别");
    } else {
        deviceRole = ROLE_UNDEFINED;
        Serial.println("设备角色: 未定义");
    }
    
    Serial.print("本机MAC: ");
    for (int i = 0; i < 6; i++) {
        Serial.printf("%02X", mac[i]);
        if (i < 5) Serial.print(":");
    }
    Serial.println();
}

void initESPNow() {
    if (esp_now_init() != ESP_OK) {
        Serial.println("ESP-NOW 初始化失败");
        return;
    }
    
    esp_now_register_recv_cb(onDataReceived);
    esp_now_register_send_cb(onDataSent);
    
    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, peerAddress, 6);
    peerInfo.channel = ESPNOW_CHANNEL;
    peerInfo.encrypt = ESPNOW_ENCRYPT;
    
    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
        Serial.println("添加对等设备失败");
        return;
    }
    
    Serial.println("ESP-NOW 初始化成功");
}

void updateVibrationSensor() {
    int vibrationStrength = analogRead(VIBRATION_SENSOR_PIN);
    
    if (vibrationStrength > VIBRATION_THRESHOLD && 
        (millis() - lastVibrationTime > VIBRATION_DEBOUNCE_MS)) {
        
        lastVibrationTime = millis();
        vibrationDetected = true;
        
        Serial.printf("检测到震动: 强度=%d\\n", vibrationStrength);
        
        // 蜂鸣器提示
        tone(BUZZER_PIN, 2000, 200);
        
        if (trainingActive) {
            unsigned long duration = millis() - trainingStartTime;
            sendTrainingComplete(duration);
            trainingActive = false;
            
            Serial.printf("训练完成: 用时=%lu毫秒\\n", duration);
            displayMessage("训练完成");
            
            // LED变为黄色表示完成
            fill_solid(leds, LED_COUNT, CRGB::Yellow);
            FastLED.show();
            
            delay(2000);
            fill_solid(leds, LED_COUNT, CRGB::Green);
            FastLED.show();
        }
    }
}

void updateLEDs() {
    // LED呼吸效果或其他视觉反馈
    static unsigned long lastUpdate = 0;
    static int brightness = 0;
    static int direction = 1;
    
    if (millis() - lastUpdate > 50) {
        brightness += direction * 5;
        if (brightness >= 255 || brightness <= 0) {
            direction *= -1;
        }
        FastLED.setBrightness(brightness);
        FastLED.show();
        lastUpdate = millis();
    }
}

void onDataReceived(const esp_now_recv_info* recv_info, const uint8_t* data, int len) {
    message_t message;
    memcpy(&message, data, sizeof(message_t));
    
    Serial.printf("接收消息: 命令=0x%02X, 数据=%d\\n", message.command, message.data);
    
    switch (message.command) {
        case CMD_START_TASK:
            if (deviceRole == ROLE_SLAVE) {
                trainingActive = true;
                trainingStartTime = millis();
                Serial.println("从设备: 收到训练开始信号");
                displayMessage("训练开始");
                
                // LED变为蓝色
                fill_solid(leds, LED_COUNT, CRGB::Blue);
                FastLED.show();
                
                // 蜂鸣器提示
                tone(BUZZER_PIN, 1000, 200);
            }
            break;
            
        case CMD_TASK_COMPLETE:
            if (deviceRole == ROLE_MASTER) {
                Serial.printf("主设备: 收到完成信号, 用时=%d毫秒\\n", message.data);
                displayMessage("任务完成");
                
                // LED变为绿色
                fill_solid(leds, LED_COUNT, CRGB::Green);
                FastLED.show();
                
                // 蜂鸣器提示
                tone(BUZZER_PIN, 1500, 300);
            }
            break;
    }
}

void onDataSent(const uint8_t* mac, esp_now_send_status_t status) {
    Serial.printf("发送状态: %s\\n", (status == ESP_NOW_SEND_SUCCESS) ? "成功" : "失败");
}

void sendTrainingStart() {
    message_t message;
    message.command = CMD_START_TASK;
    message.target_id = 1;
    message.source_id = 0;
    message.timestamp = millis();
    message.data = 0;
    
    esp_err_t result = esp_now_send(peerAddress, (uint8_t*)&message, sizeof(message));
    if (result == ESP_OK) {
        Serial.println("发送训练开始信号");
    } else {
        Serial.printf("发送失败: %d\\n", result);
    }
}

void sendTrainingComplete(unsigned long duration) {
    message_t message;
    message.command = CMD_TASK_COMPLETE;
    message.target_id = 0;
    message.source_id = 1;
    message.timestamp = millis();
    message.data = duration;
    
    esp_err_t result = esp_now_send(peerAddress, (uint8_t*)&message, sizeof(message));
    if (result == ESP_OK) {
        Serial.println("发送训练完成信号");
    } else {
        Serial.printf("发送失败: %d\\n", result);
    }
}

void displayMessage(const char* message) {
    #ifdef FORCE_MASTER_ROLE
    // 主设备有显示屏时的处理
    Serial.printf("显示: %s\\n", message);
    #else
    // 从设备只通过串口输出
    Serial.printf("状态: %s\\n", message);
    #endif
}

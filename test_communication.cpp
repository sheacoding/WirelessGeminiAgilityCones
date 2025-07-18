/*
 * ESP-NOW 通信测试程序
 * 用于验证主机与从机之间的通信功能
 */

#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include "config.h"

// 全局变量
DeviceRole deviceRole = ROLE_UNDEFINED;
uint8_t peerAddress[6];
unsigned long lastHeartbeat = 0;
unsigned long testStartTime = 0;
bool communicationTest = false;

// 前向声明
void onDataReceived(const esp_now_recv_info* recv_info, const uint8_t* data, int len);
void onDataSent(const uint8_t* mac, esp_now_send_status_t status);
void initESPNow();
void determineDeviceRole();
void sendTestMessage();
void sendHeartbeat();

void setup() {
    Serial.begin(115200);
    Serial.println("ESP-NOW 通信测试程序");
    Serial.println("========================");
    
    // 初始化WiFi
    WiFi.mode(WIFI_STA);
    
    // 确定设备角色
    determineDeviceRole();
    
    // 初始化ESP-NOW
    initESPNow();
    
    // 开始测试
    testStartTime = millis();
    Serial.println("通信测试开始...");
}

void loop() {
    unsigned long currentTime = millis();
    
    // 主设备每5秒发送心跳包
    if (deviceRole == ROLE_MASTER && currentTime - lastHeartbeat > 5000) {
        sendHeartbeat();
        lastHeartbeat = currentTime;
    }
    
    // 主设备每10秒发送测试消息
    if (deviceRole == ROLE_MASTER && currentTime - testStartTime > 10000) {
        sendTestMessage();
        testStartTime = currentTime;
    }
    
    delay(100);
}

void determineDeviceRole() {
    uint8_t mac[6];
    WiFi.macAddress(mac);
    
    // 预定义的设备A和设备B的MAC地址
    uint8_t deviceA[] = DEVICE_A_MAC;
    uint8_t deviceB[] = DEVICE_B_MAC;
    
    // 检查是否强制设置角色
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
    
    // 根据MAC地址自动识别角色
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
        Serial.print("当前MAC地址: ");
        for (int i = 0; i < 6; i++) {
            Serial.printf("%02X", mac[i]);
            if (i < 5) Serial.print(":");
        }
        Serial.println();
    }
    
    Serial.print("对端MAC地址: ");
    for (int i = 0; i < 6; i++) {
        Serial.printf("%02X", peerAddress[i]);
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
    
    // 添加对等设备
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

void onDataReceived(const esp_now_recv_info* recv_info, const uint8_t* data, int len) {
    message_t message;
    memcpy(&message, data, sizeof(message_t));
    
    Serial.printf("接收到消息: 命令=0x%02X, 源ID=%d, 数据=%d, 时间戳=%lu\n", 
                  message.command, message.source_id, message.data, message.timestamp);
    
    switch (message.command) {
        case CMD_HEARTBEAT:
            Serial.println("收到心跳包");
            if (deviceRole == ROLE_SLAVE) {
                // 从设备回复心跳响应
                message_t response;
                response.command = CMD_HEARTBEAT;
                response.target_id = message.source_id;
                response.source_id = 1;
                response.timestamp = millis();
                response.data = 0xAA;
                
                esp_now_send(peerAddress, (uint8_t*)&response, sizeof(response));
            }
            break;
            
        case CMD_START_TASK:
            Serial.println("收到开始任务命令");
            if (deviceRole == ROLE_SLAVE) {
                // 从设备开始计时
                Serial.println("从设备开始计时...");
                
                // 模拟一段时间后发送完成消息
                delay(2000);
                
                message_t complete;
                complete.command = CMD_TASK_COMPLETE;
                complete.target_id = message.source_id;
                complete.source_id = 1;
                complete.timestamp = millis();
                complete.data = 2000; // 模拟2秒完成时间
                
                esp_now_send(peerAddress, (uint8_t*)&complete, sizeof(complete));
            }
            break;
            
        case CMD_TASK_COMPLETE:
            Serial.printf("收到任务完成: 用时=%dms\n", message.data);
            break;
            
        default:
            Serial.printf("未知命令: 0x%02X\n", message.command);
            break;
    }
}

void onDataSent(const uint8_t* mac, esp_now_send_status_t status) {
    Serial.printf("发送状态: %s\n", (status == ESP_NOW_SEND_SUCCESS) ? "成功" : "失败");
}

void sendHeartbeat() {
    message_t message;
    message.command = CMD_HEARTBEAT;
    message.target_id = 1;
    message.source_id = 0;
    message.timestamp = millis();
    message.data = 0x55;
    
    esp_err_t result = esp_now_send(peerAddress, (uint8_t*)&message, sizeof(message));
    if (result == ESP_OK) {
        Serial.println("发送心跳包");
    } else {
        Serial.printf("发送心跳包失败: %d\n", result);
    }
}

void sendTestMessage() {
    message_t message;
    message.command = CMD_START_TASK;
    message.target_id = 1;
    message.source_id = 0;
    message.timestamp = millis();
    message.data = 0;
    
    esp_err_t result = esp_now_send(peerAddress, (uint8_t*)&message, sizeof(message));
    if (result == ESP_OK) {
        Serial.println("发送测试任务开始命令");
    } else {
        Serial.printf("发送测试命令失败: %d\n", result);
    }
}

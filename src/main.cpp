#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include "config.h"
#include "hardware.h"
#include "menu.h"
#include "vibration_training.h"
#include "ButtonManager.h"

// 全局变量
SystemState currentState = STATE_INIT;
DeviceRole deviceRole = ROLE_UNDEFINED;
uint8_t peerAddress[6];
bool systemInitialized = false;

// 按键管理器实例
ButtonManager buttonManager(BUTTON_PIN);

// 前向声明
void onDataReceived(const esp_now_recv_info* recv_info, const uint8_t* data, int len);
void onDataSent(const uint8_t* mac, esp_now_send_status_t status);
void initESPNow();
void determineDeviceRole();
void startTraining();
void returnToMenu();
void handleVibrationTraining();
void handleDualTraining();
void updateSystem();

// 按键事件回调函数
void onSingleClick();
void onDoubleClick();
void onLongPress();

void setup() {
    Serial.begin(115200);
    Serial.println("ESP-NOW 双子星敏捷锥启动中...");
    
    // 初始化硬件
    if (!hardware.init()) {
        Serial.println("硬件初始化失败!");
        currentState = STATE_ERROR;
        return;
    }
    
    // 初始化WiFi
    WiFi.mode(WIFI_STA);
    
    // 确定设备角色
    determineDeviceRole();
    
    // 初始化ESP-NOW
    initESPNow();
    
    // 初始化菜单
    menu.init();
    
    // 初始化按键管理器
    buttonManager.init();
    buttonManager.attachSingleClick(onSingleClick);
    buttonManager.attachDoubleClick(onDoubleClick);
    buttonManager.attachLongPress(onLongPress);
    Serial.println("按键管理器初始化完成");
    
    // 设置状态
    currentState = STATE_MENU;
    systemInitialized = true;
    
    hardware.displayStatus("系统已就绪");
    hardware.playStartSound();
    
    Serial.println("系统初始化完成");
}

void loop() {
    if (!systemInitialized) {
        return;
    }
    
    hardware.update();
    
    // 更新按键管理器
    buttonManager.tick();
    
    updateSystem();
    
    delay(10); // 短暂延迟以避免过度占用CPU
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
    
    Serial.printf("接收到消息: 命令=%d, 数据=%d\n", message.command, message.data);
    
    switch (message.command) {
        case CMD_START_TASK:
            if (currentState == STATE_READY) {
                currentState = STATE_TIMING;
                vibrationTraining.start();
            }
            break;
            
        case CMD_TASK_COMPLETE:
            if (currentState == STATE_TIMING) {
                currentState = STATE_COMPLETE;
                hardware.displayResult(message.data, "训练完成");
                hardware.playCompleteSound();
            }
            break;
            
        case CMD_RESET:
            currentState = STATE_MENU;
            menu.init();
            break;
    }
}

void onDataSent(const uint8_t* mac, esp_now_send_status_t status) {
    Serial.printf("发送状态: %s\n", (status == ESP_NOW_SEND_SUCCESS) ? "成功" : "失败");
}

void handleVibrationTraining() {
    switch (menu.getCurrentMode()) {
        case MODE_SINGLE_TIMER:
            // 单次计时模式
            if (currentState == STATE_TIMING) {
                vibrationTraining.update();
                if (vibrationTraining.isCompleted()) {
                    currentState = STATE_COMPLETE;
                    hardware.displayResult(vibrationTraining.getElapsedTime(), "计时完成");
                }
            }
            break;
            
        case MODE_VIBRATION_TRAINING:
            // 震动训练模式
            if (currentState == STATE_TIMING) {
                vibrationTraining.update();
                if (vibrationTraining.isCompleted()) {
                    currentState = STATE_COMPLETE;
                }
            }
            break;
            
        case MODE_DUAL_TRAINING:
            // 双设备训练模式
            handleDualTraining();
            break;
    }
}

void handleDualTraining() {
    // 双设备训练逻辑
    if (deviceRole == ROLE_MASTER && currentState == STATE_READY) {
        // 主设备发送开始信号
        message_t message;
        message.command = CMD_START_TASK;
        message.target_id = 1;
        message.source_id = 0;
        message.timestamp = millis();
        message.data = 0;
        
        esp_err_t result = esp_now_send(peerAddress, (uint8_t*)&message, sizeof(message));
        if (result == ESP_OK) {
            Serial.println("发送开始信号成功");
            currentState = STATE_TIMING;
            vibrationTraining.start();
        }
    }
}

void updateSystem() {
    switch (currentState) {
        case STATE_INIT:
            // 初始化状态
            break;
            
        case STATE_MENU:
            menu.update();
            // 当菜单选择“开始训练”后，自动切换到准备状态
            if (!menu.isMenuActive()) {
                currentState = STATE_READY;
                Serial.println("切换到准备状态");
                hardware.displayStatus("准备就绪！按键开始训练");
                hardware.setAllLEDs(COLOR_GREEN);
                hardware.showLEDs();
            }
            break;
            
        case STATE_READY:
            hardware.displayStatus("按按钮开始");
            break;
            
        case STATE_TIMING:
            handleVibrationTraining();
            break;
            
        case STATE_COMPLETE:
            hardware.displayStatus("按按钮继续");
            break;
            
        case STATE_ERROR:
            hardware.displayStatus("系统错误");
            hardware.setAllLEDs(COLOR_RED);
            hardware.showLEDs();
            break;
    }
}

void startTraining() {
    Serial.println("开始训练...");
    
    // 检查当前状态是否允许开始训练
    if (currentState == STATE_READY) {
        // 切换到计时状态
        currentState = STATE_TIMING;
        
        // 播放开始音效
        if (hardware.getSettings()->soundEnabled) {
            hardware.playStartSound();
        }
        
        // 设置LED为训练颜色
        hardware.setAllLEDs(hardware.getLedColorValue(hardware.getSettings()->ledColor));
        hardware.showLEDs();
        
        // 显示训练开始界面
        hardware.displayStatus("训练开始！");
        
        // 启动震动训练
        vibrationTraining.start();
        
        Serial.println("训练状态切换完成");
    } else {
        Serial.println("当前状态不允许开始训练");
    }
}

void returnToMenu() {
    Serial.println("返回主菜单...");
    
    // 保存当前训练数据（如果有）
    if (currentState == STATE_TIMING || currentState == STATE_COMPLETE) {
        // 这里可以添加保存训练数据的逻辑
        // saveTrainingData();
    }
    
    // 切换到菜单状态
    currentState = STATE_MENU;
    
    // 重置硬件状态
    hardware.clearLEDs();
    hardware.showLEDs();
    
    // 停止任何正在播放的音效
    noTone(BUZZER_PIN);
    
    // 重新初始化菜单
    menu.init();
    
    Serial.println("已返回主菜单");
}

// 按键事件回调函数实现
void onSingleClick() {
    Serial.printf("[%lu] 单击按键事件 - 当前状态: %d\n", millis(), currentState);
    
    switch (currentState) {
        case STATE_MENU:
            if (menu.isMenuActive()) {
                Serial.println("  -> 菜单下一项");
                menu.selectNext();
            }
            break;
        case STATE_READY:
            // 开始训练
            Serial.println("  -> 开始训练");
            startTraining();
            break;
        case STATE_COMPLETE:
            // 返回菜单
            Serial.println("  -> 返回菜单");
            returnToMenu();
            break;
        default:
            Serial.printf("  -> 当前状态不处理单击事件: %d\n", currentState);
            break;
    }
}

void onDoubleClick() {
    Serial.printf("[%lu] ★★★ 双击按键事件触发 ★★★ - 当前状态: %d\n", millis(), currentState);
    
    switch (currentState) {
        case STATE_MENU:
            if (menu.isMenuActive()) {
                Serial.println("  -> 菜单上一项");
                menu.selectPrevious();
            } else {
                Serial.println("  -> 菜单未激活");
            }
            break;
        default:
            Serial.printf("  -> 当前状态不处理双击事件: %d\n", currentState);
            break;
    }
}

void onLongPress() {
    Serial.printf("[%lu] 长按按键事件 - 当前状态: %d\n", millis(), currentState);
    
    switch (currentState) {
        case STATE_MENU:
            if (menu.isMenuActive()) {
                Serial.println("  -> 菜单确认");
                menu.confirm();
            }
            break;
        case STATE_READY:
        case STATE_TIMING:
        case STATE_COMPLETE:
            // 返回菜单
            Serial.println("  -> 返回菜单");
            returnToMenu();
            break;
        default:
            Serial.printf("  -> 当前状态不处理长按事件: %d\n", currentState);
            break;
    }
}

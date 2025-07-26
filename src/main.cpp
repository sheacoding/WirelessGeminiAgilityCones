#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include "config.h"
#include "hardware.h"
#include "menu.h"
#include "vibration_training.h"
#include "ButtonManager.h"
#include "time_manager.h"

// 全局变量
SystemState currentState = STATE_INIT;
DeviceRole deviceRole = ROLE_UNDEFINED;
uint8_t peerAddress[6];
bool systemInitialized = false;

// 连接状态监控变量
ConnectionStatus connectionStatus = CONN_DISCONNECTED;
unsigned long lastHeartbeatSent = 0;
unsigned long lastHeartbeatReceived = 0;
unsigned long lastConnectionCheck = 0;
uint8_t connectionRetryCount = 0;
bool waitingForHeartbeatAck = false;

// 设备配对变量
PairingStatus pairingStatus = PAIRING_IDLE;
DiscoveredDevice discoveredDevices[MAX_DISCOVERED_DEVICES];
uint8_t discoveredDeviceCount = 0;
uint8_t selectedDeviceIndex = 0;
unsigned long pairingStartTime = 0;
bool pairingModeActive = false;
unsigned long lastPairingDisplayUpdate = 0;
PairingStatus lastDisplayedPairingStatus = PAIRING_IDLE;

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

// 连接状态监控函数
void updateConnectionStatus();
void sendHeartbeat();
void handleHeartbeat(const message_t& message);
void checkConnectionTimeout();
void setConnectionStatus(ConnectionStatus status);
const char* getConnectionStatusString(ConnectionStatus status);

// 设备配对函数
void startDevicePairing();
void stopDevicePairing();
void updatePairingProcess();
void sendPairingRequest(const uint8_t* targetMac);
void handlePairingMessage(const message_t& message, const uint8_t* senderMac);
void addDiscoveredDevice(const uint8_t* mac, int8_t rssi);
void displayPairingStatus();
void updatePairingDisplay(bool checkUpdateNeeded = true);
void clearPairedDevice();
const char* getPairingStatusString(PairingStatus status);

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
    
// 初始化时间管理器
    if (!timeManager.init()) {
        Serial.println("时间管理器初始化失败!");
    } else {
        Serial.println("时间管理器初始化成功!");
        timeManager.printTimeInfo();
    }

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
    
    // 更新连接状态监控
    updateConnectionStatus();
    
    // 更新配对流程
    if (pairingModeActive) {
        updatePairingProcess();
    }
    
    updateSystem();
    
    delay(10); // 短暂延迟以避免过度占用CPU
}

void determineDeviceRole() {
    uint8_t mac[6];
    WiFi.macAddress(mac);
    
    // 输出实际MAC地址用于调试
    Serial.printf("实际设备MAC地址: %02X:%02X:%02X:%02X:%02X:%02X\n", 
                  mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    
    // 预定义的设备A和设备B的MAC地址
    uint8_t deviceA[] = DEVICE_A_MAC;
    uint8_t deviceB[] = DEVICE_B_MAC;
    
    Serial.printf("配置的设备A MAC: %02X:%02X:%02X:%02X:%02X:%02X\n", 
                  deviceA[0], deviceA[1], deviceA[2], deviceA[3], deviceA[4], deviceA[5]);
    Serial.printf("配置的设备B MAC: %02X:%02X:%02X:%02X:%02X:%02X\n", 
                  deviceB[0], deviceB[1], deviceB[2], deviceB[3], deviceB[4], deviceB[5]);
    
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
        // 检查是否有保存的配对设备
        SystemSettings* settings = hardware.getSettings();
        if (settings->hasPairedDevice) {
            memcpy(peerAddress, settings->pairedDeviceMac, 6);
            // 根据当前MAC确定角色（这里简化处理，实际可能需要更复杂的逻辑）
            deviceRole = (mac[5] < settings->pairedDeviceMac[5]) ? ROLE_MASTER : ROLE_SLAVE;
            Serial.printf("设备角色: %s - 使用已保存的配对信息\n", 
                         (deviceRole == ROLE_MASTER) ? "主设备 (Master)" : "从设备 (Slave)");
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
    
    // 设置连接状态为连接中
    setConnectionStatus(CONN_CONNECTING);
    lastConnectionCheck = millis();
}

void onDataReceived(const esp_now_recv_info* recv_info, const uint8_t* data, int len) {
    message_t message;
    memcpy(&message, data, sizeof(message_t));
    
    Serial.printf("接收到消息: 命令=%d, 数据=%d\n", message.command, message.data);
    
    // 更新最后收到消息的时间
    lastHeartbeatReceived = millis();
    
    switch (message.command) {
        case CMD_HEARTBEAT:
            handleHeartbeat(message);
            break;
            
        case CMD_HEARTBEAT_ACK:
            waitingForHeartbeatAck = false;
            setConnectionStatus(CONN_CONNECTED);
            Serial.println("收到心跳应答，连接正常");
            break;
            
        case CMD_PAIRING_REQUEST:
        case CMD_PAIRING_RESPONSE:
        case CMD_PAIRING_CONFIRM:
        case CMD_DEVICE_INFO:
            if (pairingModeActive) {
                handlePairingMessage(message, recv_info->src_addr);
            }
            break;
            
        case CMD_START_TASK:
            if (currentState == STATE_READY && connectionStatus == CONN_CONNECTED) {
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
            
        case CMD_VT_START_ROUND:
            // 主机收到从机的开始信号
            if (deviceRole == ROLE_MASTER) {
                vibrationTraining.handleSlaveComplete(message.data);
                Serial.println("主机收到从机开始计时信号");
            }
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
    if (deviceRole == ROLE_MASTER && currentState == STATE_READY && connectionStatus == CONN_CONNECTED) {
        // 主设备发送开始信号
        message_t message;
        message.command = CMD_START_TASK;
        message.target_id = 1;
        message.source_id = 0;
        message.timestamp = millis();
        message.data = 0;
        message.checksum = 0; // TODO: 实现校验和计算
        
        esp_err_t result = esp_now_send(peerAddress, (uint8_t*)&message, sizeof(message));
        if (result == ESP_OK) {
            Serial.println("发送开始信号成功");
            currentState = STATE_TIMING;
            vibrationTraining.start();
        } else {
            Serial.println("发送开始信号失败，检查连接状态");
            hardware.displayStatus("连接错误");
        }
    } else if (currentState == STATE_READY && connectionStatus != CONN_CONNECTED) {
        hardware.displayStatus("等待连接...");
    }
}

void updateSystem() {
    switch (currentState) {
        case STATE_INIT:
            // 初始化状态
            break;
            
        case STATE_MENU:
            // 如果配对模式激活，暂停菜单更新避免显示冲突
            if (!pairingModeActive) {
                menu.update();
                // 当菜单选择"开始训练"后，自动切换到准备状态
                if (!menu.isMenuActive()) {
                    currentState = STATE_READY;
                    Serial.println("切换到准备状态");
                    hardware.displayStatus("准备就绪！按键开始训练");
                    hardware.setAllLEDs(COLOR_GREEN);
                    hardware.showLEDs();
                }
            }
            break;
            
        case STATE_READY:
            if (connectionStatus == CONN_CONNECTED) {
                hardware.displayStatus("按按钮开始");
            } else {
                hardware.displayStatus(getConnectionStatusString(connectionStatus));
            }
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
    
    // 清理并重新初始化显示器
    hardware.displayClear();
    
    // 重新初始化菜单
    menu.init();
    
    Serial.println("已返回主菜单");
}

// 按键事件回调函数实现
void onSingleClick() {
    Serial.printf("[%lu] 单击按键事件 - 当前状态: %d\n", millis(), currentState);
    
    // 如果在配对模式中，处理设备选择
    if (pairingModeActive && pairingStatus == PAIRING_FOUND_DEVICE) {
        selectedDeviceIndex = (selectedDeviceIndex + 1) % discoveredDeviceCount;
        displayPairingStatus();
        Serial.printf("  -> 选择设备: %d\n", selectedDeviceIndex);
        return;
    }
    
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
    
    // 如果在配对模式中，处理设备确认选择或退出
    if (pairingModeActive) {
        if (pairingStatus == PAIRING_FOUND_DEVICE && discoveredDeviceCount > 0) {
            // 确认选择当前设备进行配对
            Serial.printf("  -> 确认配对设备: %s\n", discoveredDevices[selectedDeviceIndex].name);
            sendPairingRequest(discoveredDevices[selectedDeviceIndex].mac);
            return;
        } else {
            // 退出配对模式
            Serial.println("  -> 退出配对模式");
            stopDevicePairing();
            return;
        }
    }
    
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

// 连接状态监控函数实现
void updateConnectionStatus() {
    unsigned long currentTime = millis();
    
    // 定期检查连接状态
    if (currentTime - lastConnectionCheck >= CONNECTION_CHECK_INTERVAL) {
        lastConnectionCheck = currentTime;
        checkConnectionTimeout();
        
        // 发送心跳包
        if (currentTime - lastHeartbeatSent >= HEARTBEAT_INTERVAL_MS) {
            sendHeartbeat();
            lastHeartbeatSent = currentTime;
        }
    }
}

void sendHeartbeat() {
    message_t message;
    message.command = CMD_HEARTBEAT;
    message.target_id = (deviceRole == ROLE_MASTER) ? 1 : 0;
    message.source_id = (deviceRole == ROLE_MASTER) ? 0 : 1;
    message.timestamp = millis();
    message.data = connectionRetryCount;
    message.checksum = 0; // TODO: 实现校验和计算
    
    esp_err_t result = esp_now_send(peerAddress, (uint8_t*)&message, sizeof(message));
    if (result == ESP_OK) {
        waitingForHeartbeatAck = true;
        Serial.printf("发送心跳包 (重试次数: %d)\n", connectionRetryCount);
    } else {
        Serial.printf("心跳包发送失败: %d\n", result);
        connectionRetryCount++;
        if (connectionRetryCount >= CONNECTION_RETRY_COUNT) {
            setConnectionStatus(CONN_ERROR);
        }
    }
}

void handleHeartbeat(const message_t& message) {
    Serial.printf("收到心跳包，源ID: %d\n", message.source_id);
    
    // 发送心跳应答
    message_t ackMessage;
    ackMessage.command = CMD_HEARTBEAT_ACK;
    ackMessage.target_id = message.source_id;
    ackMessage.source_id = (deviceRole == ROLE_MASTER) ? 0 : 1;
    ackMessage.timestamp = millis();
    ackMessage.data = 0;
    ackMessage.checksum = 0; // TODO: 实现校验和计算
    
    esp_err_t result = esp_now_send(peerAddress, (uint8_t*)&ackMessage, sizeof(ackMessage));
    if (result == ESP_OK) {
        setConnectionStatus(CONN_CONNECTED);
        Serial.println("发送心跳应答");
    } else {
        Serial.printf("心跳应答发送失败: %d\n", result);
    }
}

void checkConnectionTimeout() {
    unsigned long currentTime = millis();
    
    // 检查是否超时
    if (connectionStatus == CONN_CONNECTED || connectionStatus == CONN_CONNECTING) {
        if (currentTime - lastHeartbeatReceived > HEARTBEAT_TIMEOUT_MS) {
            Serial.println("连接超时，尝试重连...");
            setConnectionStatus(CONN_TIMEOUT);
            connectionRetryCount++;
            
            if (connectionRetryCount >= CONNECTION_RETRY_COUNT) {
                setConnectionStatus(CONN_ERROR);
                Serial.println("连接失败，已达到最大重试次数");
            } else {
                setConnectionStatus(CONN_CONNECTING);
                Serial.printf("重试连接 (%d/%d)\n", connectionRetryCount, CONNECTION_RETRY_COUNT);
            }
        }
    }
    
    // 检查心跳应答超时
    if (waitingForHeartbeatAck && (currentTime - lastHeartbeatSent > HEARTBEAT_TIMEOUT_MS / 2)) {
        Serial.println("心跳应答超时");
        waitingForHeartbeatAck = false;
        connectionRetryCount++;
        
        if (connectionRetryCount >= CONNECTION_RETRY_COUNT) {
            setConnectionStatus(CONN_ERROR);
        } else {
            setConnectionStatus(CONN_CONNECTING);
        }
    }
}

void setConnectionStatus(ConnectionStatus status) {
    if (connectionStatus != status) {
        ConnectionStatus oldStatus = connectionStatus;
        connectionStatus = status;
        
        Serial.printf("连接状态变更: %s -> %s\n", 
                     getConnectionStatusString(oldStatus),
                     getConnectionStatusString(status));
        
        // 根据连接状态更新硬件指示
        switch (status) {
            case CONN_CONNECTED:
                connectionRetryCount = 0;
                hardware.setAllLEDs(COLOR_GREEN);
                break;
            case CONN_CONNECTING:
                hardware.setAllLEDs(COLOR_YELLOW);
                break;
            case CONN_DISCONNECTED:
            case CONN_TIMEOUT:
                hardware.setAllLEDs(COLOR_ORANGE);
                break;
            case CONN_ERROR:
                hardware.setAllLEDs(COLOR_RED);
                break;
        }
        hardware.showLEDs();
    }
}

const char* getConnectionStatusString(ConnectionStatus status) {
    switch (status) {
        case CONN_DISCONNECTED: return "未连接";
        case CONN_CONNECTING: return "连接中";
        case CONN_CONNECTED: return "已连接";
        case CONN_ERROR: return "连接错误";
        case CONN_TIMEOUT: return "连接超时";
        default: return "未知状态";
    }
}

// 设备配对功能实现
void startDevicePairing() {
    if (pairingModeActive) {
        Serial.println("配对模式已在运行中");
        return;
    }
    
    pairingModeActive = true;
    pairingStatus = PAIRING_SCANNING;
    pairingStartTime = millis();
    discoveredDeviceCount = 0;
    selectedDeviceIndex = 0;
    
    // 清空发现的设备列表
    memset(discoveredDevices, 0, sizeof(discoveredDevices));
    
    // 开始广播配对请求
    message_t pairingMsg;
    pairingMsg.command = CMD_PAIRING_REQUEST;
    pairingMsg.target_id = 0xFF; // 广播
    pairingMsg.source_id = (deviceRole == ROLE_MASTER) ? 0 : 1;
    pairingMsg.timestamp = millis();
    pairingMsg.data = 0;
    pairingMsg.checksum = 0;
    
    // 添加广播地址作为对等设备（如果尚未添加）
    uint8_t broadcastAddr[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    
    // 检查是否已存在广播对等设备
    if (!esp_now_is_peer_exist(broadcastAddr)) {
        esp_now_peer_info_t broadcastPeer = {};
        memcpy(broadcastPeer.peer_addr, broadcastAddr, 6);
        broadcastPeer.channel = ESPNOW_CHANNEL;
        broadcastPeer.encrypt = false; // 广播通常不加密
        
        esp_err_t addResult = esp_now_add_peer(&broadcastPeer);
        if (addResult != ESP_OK) {
            Serial.printf("添加广播对等设备失败: %d\n", addResult);
        } else {
            Serial.println("成功添加广播对等设备");
        }
    }
    
    // 广播配对请求
    esp_err_t broadcastResult = esp_now_send(broadcastAddr, (uint8_t*)&pairingMsg, sizeof(pairingMsg));
    if (broadcastResult == ESP_OK) {
        Serial.println("广播配对请求发送成功");
    } else {
        Serial.printf("广播配对请求发送失败: %d\n", broadcastResult);
    }
    
    Serial.println("开始设备配对扫描...");
    displayPairingStatus();
}

void stopDevicePairing() {
    pairingModeActive = false;
    pairingStatus = PAIRING_IDLE;
    
    // 重置显示状态变量
    lastDisplayedPairingStatus = PAIRING_IDLE;
    lastPairingDisplayUpdate = 0;
    
    // 恢复菜单显示
    if (currentState == STATE_MENU) {
        menu.update(); // 强制刷新菜单显示
    }
    
    Serial.println("停止设备配对，恢复菜单显示");
}

void updatePairingProcess() {
    unsigned long currentTime = millis();
    
    // 检查配对超时
    if (currentTime - pairingStartTime > PAIRING_TIMEOUT_MS) {
        pairingStatus = PAIRING_TIMEOUT;
        Serial.println("配对超时");
        displayPairingStatus();
        delay(2000);
        stopDevicePairing();
        return;
    }
    
    switch (pairingStatus) {
        case PAIRING_SCANNING:
            // 扫描阶段 - 定期发送配对请求
            if (currentTime - pairingStartTime > PAIRING_SCAN_DURATION_MS) {
                if (discoveredDeviceCount > 0) {
                    pairingStatus = PAIRING_FOUND_DEVICE;
                    Serial.printf("发现 %d 个设备\n", discoveredDeviceCount);
                } else {
                    pairingStatus = PAIRING_FAILED;
                    Serial.println("未发现兼容设备");
                }
                displayPairingStatus();
            }
            break;
            
        case PAIRING_FOUND_DEVICE:
            // 等待用户选择设备 - 使用智能显示更新
            updatePairingDisplay(true); // 只在需要时更新
            break;
            
        case PAIRING_CONNECTING:
            // 连接阶段超时检查
            if (currentTime - pairingStartTime > PAIRING_TIMEOUT_MS - 2000) {
                pairingStatus = PAIRING_FAILED;
                displayPairingStatus();
            }
            break;
            
        case PAIRING_SUCCESS:
        case PAIRING_FAILED:
        case PAIRING_TIMEOUT:
            // 显示结果后自动退出
            if (currentTime - pairingStartTime > PAIRING_TIMEOUT_MS + 2000) {
                stopDevicePairing();
            }
            break;
    }
}

void sendPairingRequest(const uint8_t* targetMac) {
    message_t pairingMsg;
    pairingMsg.command = CMD_PAIRING_CONFIRM;
    pairingMsg.target_id = 1;
    pairingMsg.source_id = (deviceRole == ROLE_MASTER) ? 0 : 1;
    pairingMsg.timestamp = millis();
    pairingMsg.data = 0;
    pairingMsg.checksum = 0;
    
    esp_err_t result = esp_now_send(targetMac, (uint8_t*)&pairingMsg, sizeof(pairingMsg));
    if (result == ESP_OK) {
        pairingStatus = PAIRING_CONNECTING;
        Serial.println("发送配对确认");
    } else {
        pairingStatus = PAIRING_FAILED;
        Serial.printf("配对确认发送失败: %d\n", result);
    }
}

void handlePairingMessage(const message_t& message, const uint8_t* senderMac) {
    switch (message.command) {
        case CMD_PAIRING_REQUEST:
            Serial.println("收到配对请求");
            // 先确保发送者已被添加为对等设备
            if (!esp_now_is_peer_exist(senderMac)) {
                esp_now_peer_info_t senderPeer = {};
                memcpy(senderPeer.peer_addr, senderMac, 6);
                senderPeer.channel = ESPNOW_CHANNEL;
                senderPeer.encrypt = false; // 配对期间不加密
                
                esp_err_t addResult = esp_now_add_peer(&senderPeer);
                if (addResult != ESP_OK) {
                    Serial.printf("添加配对请求发送者失败: %d\n", addResult);
                    break;
                }
            }
            
            // 发送设备信息回应
            {
                message_t response;
                response.command = CMD_DEVICE_INFO;
                response.target_id = message.source_id;
                response.source_id = (deviceRole == ROLE_MASTER) ? 0 : 1;
                response.timestamp = millis();
                response.data = deviceRole; // 发送角色信息
                response.checksum = 0;
                
                esp_err_t sendResult = esp_now_send(senderMac, (uint8_t*)&response, sizeof(response));
                if (sendResult == ESP_OK) {
                    Serial.println("发送设备信息回应成功");
                } else {
                    Serial.printf("发送设备信息回应失败: %d\n", sendResult);
                }
            }
            break;
            
        case CMD_DEVICE_INFO:
            Serial.printf("收到设备信息，角色: %d\n", message.data);
            
            // 确保发送者已被添加为对等设备
            if (!esp_now_is_peer_exist(senderMac)) {
                esp_now_peer_info_t senderPeer = {};
                memcpy(senderPeer.peer_addr, senderMac, 6);
                senderPeer.channel = ESPNOW_CHANNEL;
                senderPeer.encrypt = false; // 配对期间不加密
                
                esp_err_t addResult = esp_now_add_peer(&senderPeer);
                if (addResult != ESP_OK) {
                    Serial.printf("添加设备信息发送者失败: %d\n", addResult);
                } else {
                    Serial.println("成功添加设备信息发送者为对等设备");
                }
            }
            
            addDiscoveredDevice(senderMac, -50); // 假设信号强度
            break;
            
        case CMD_PAIRING_CONFIRM:
            Serial.println("收到配对确认");
            // 更新对端地址并设置连接状态
            memcpy(peerAddress, senderMac, 6);
            
            // 保存配对设备信息到设置中
            SystemSettings* settings = hardware.getSettings();
            memcpy(settings->pairedDeviceMac, senderMac, 6);
            settings->hasPairedDevice = true;
            hardware.saveSettings();
            
            pairingStatus = PAIRING_SUCCESS;
            setConnectionStatus(CONN_CONNECTED);
            Serial.println("配对成功！设备信息已保存");
            displayPairingStatus();
            break;
    }
}

void addDiscoveredDevice(const uint8_t* mac, int8_t rssi) {
    if (discoveredDeviceCount >= MAX_DISCOVERED_DEVICES) {
        return; // 设备列表已满
    }
    
    // 检查是否已存在
    for (int i = 0; i < discoveredDeviceCount; i++) {
        if (memcmp(discoveredDevices[i].mac, mac, 6) == 0) {
            discoveredDevices[i].rssi = rssi;
            discoveredDevices[i].lastSeen = millis();
            return; // 更新现有设备
        }
    }
    
    // 添加新设备
    DiscoveredDevice* device = &discoveredDevices[discoveredDeviceCount];
    memcpy(device->mac, mac, 6);
    device->rssi = rssi;
    device->lastSeen = millis();
    device->isCompatible = true; // 假设兼容
    snprintf(device->name, sizeof(device->name), "设备%02X%02X", mac[4], mac[5]);
    
    discoveredDeviceCount++;
    Serial.printf("发现新设备: %s (信号: %ddBm)\n", device->name, rssi);
    
    // 发现新设备时强制更新显示
    if (pairingStatus == PAIRING_SCANNING || pairingStatus == PAIRING_FOUND_DEVICE) {
        displayPairingStatus();
    }
}

void displayPairingStatus() {
    updatePairingDisplay(false); // 强制更新
}

void updatePairingDisplay(bool checkUpdateNeeded) {
    unsigned long currentTime = millis();
    
    // 检查是否需要更新显示（避免过度刷新）
    if (checkUpdateNeeded) {
        // 只有状态改变或者超过500ms才更新显示
        if (pairingStatus == lastDisplayedPairingStatus && 
            (currentTime - lastPairingDisplayUpdate < 500)) {
            return;
        }
    }
    
    lastPairingDisplayUpdate = currentTime;
    lastDisplayedPairingStatus = pairingStatus;
    
    hardware.displayDevicePairing(pairingStatus, discoveredDevices, discoveredDeviceCount, selectedDeviceIndex);
    
    // LED指示配对状态
    switch (pairingStatus) {
        case PAIRING_SCANNING:
            hardware.setAllLEDs(COLOR_BLUE); // 蓝色表示扫描中
            break;
        case PAIRING_FOUND_DEVICE:
            hardware.setAllLEDs(COLOR_YELLOW); // 黄色表示发现设备
            break;
        case PAIRING_CONNECTING:
            hardware.setAllLEDs(COLOR_ORANGE); // 橙色表示连接中
            break;
        case PAIRING_SUCCESS:
            hardware.setAllLEDs(COLOR_GREEN); // 绿色表示成功
            break;
        case PAIRING_FAILED:
        case PAIRING_TIMEOUT:
            hardware.setAllLEDs(COLOR_RED); // 红色表示失败
            break;
        default:
            hardware.clearLEDs();
            break;
    }
    hardware.showLEDs();
}

void clearPairedDevice() {
    SystemSettings* settings = hardware.getSettings();
    memset(settings->pairedDeviceMac, 0, 6);
    settings->hasPairedDevice = false;
    hardware.saveSettings();
    
    // 重置设备角色和对端地址
    deviceRole = ROLE_UNDEFINED;
    memset(peerAddress, 0, 6);
    setConnectionStatus(CONN_DISCONNECTED);
    
    Serial.println("已清除配对设备信息");
}

const char* getPairingStatusString(PairingStatus status) {
    switch (status) {
        case PAIRING_IDLE: return "空闲";
        case PAIRING_SCANNING: return "扫描中";
        case PAIRING_FOUND_DEVICE: return "发现设备";
        case PAIRING_CONNECTING: return "连接中";
        case PAIRING_SUCCESS: return "配对成功";
        case PAIRING_FAILED: return "配对失败";
        case PAIRING_TIMEOUT: return "配对超时";
        default: return "未知状态";
    }
}

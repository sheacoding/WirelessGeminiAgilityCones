#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include "config.h"
#include "hardware.h"

// 全局变量
SlaveState currentState = SLAVE_INIT;
DeviceRole deviceRole = ROLE_SLAVE;
uint8_t peerAddress[6];
bool systemInitialized = false;

// 连接状态监控变量
ConnectionStatus connectionStatus = CONN_DISCONNECTED;
unsigned long lastHeartbeatSent = 0;
unsigned long lastHeartbeatReceived = 0;
unsigned long lastConnectionCheck = 0;
uint8_t connectionRetryCount = 0;
bool waitingForHeartbeatAck = false;

// 训练相关变量
unsigned long trainingStartTime = 0;
bool trainingActive = false;

// 设备配对变量
PairingStatus pairingStatus = PAIRING_IDLE;
DiscoveredDevice discoveredDevices[MAX_DISCOVERED_DEVICES];
uint8_t discoveredDeviceCount = 0;
unsigned long pairingStartTime = 0;
bool pairingModeActive = false;

// 前向声明
void onDataReceived(const esp_now_recv_info* recv_info, const uint8_t* data, int len);
void onDataSent(const uint8_t* mac, esp_now_send_status_t status);
void initESPNow();
void determineDeviceRole();
void updateSystem();

// 连接状态监控函数
void updateConnectionStatus();
void sendHeartbeat();
void handleHeartbeat(const message_t& message);
void checkConnectionTimeout();
void setConnectionStatus(ConnectionStatus status);
const char* getConnectionStatusString(ConnectionStatus status);

// 训练处理函数
void handleTrainingStart();
void handleTrainingComplete();
void sendTrainingResult(unsigned long duration);
void sendStartTrainingSignal();

// 设备配对函数
void handlePairingMessage(const message_t& message, const uint8_t* senderMac);
void respondToPairingRequest(const uint8_t* senderMac);

void setup() {
    Serial.begin(115200);
    Serial.println("ESP-NOW 从机设备启动中...");
    
    // 初始化硬件
    if (!slaveHardware.init()) {
        Serial.println("硬件初始化失败!");
        currentState = SLAVE_ERROR;
        return;
    }
    
    // 初始化WiFi
    WiFi.mode(WIFI_STA);
    
    // 确定设备角色
    determineDeviceRole();
    
    // 初始化ESP-NOW
    initESPNow();
    
    // 设置状态 - 强制重置到IDLE状态
    currentState = SLAVE_IDLE;
    systemInitialized = true;
    
    Serial.printf("从机状态设置为: %d (SLAVE_IDLE)\n", currentState);
    slaveHardware.indicateTrainingState(currentState);
    slaveHardware.playStartSound();
    
    Serial.println("从机系统初始化完成");
}

void loop() {
    if (!systemInitialized) {
        return;
    }
    
    slaveHardware.update();
    
    // 更新连接状态监控
    updateConnectionStatus();
    
    updateSystem();
    
    delay(10); // 短暂延迟以避免过度占用CPU
}

void determineDeviceRole() {
    uint8_t mac[6];
    WiFi.macAddress(mac);
    
    // 预定义的设备A和设备B的MAC地址
    uint8_t deviceA[] = DEVICE_A_MAC;
    uint8_t deviceB[] = DEVICE_B_MAC;
    
    // 从机设备强制设置为SLAVE角色
    deviceRole = ROLE_SLAVE;
    memcpy(peerAddress, deviceA, 6); // 连接到主设备
    Serial.println("设备角色: 从设备 (Slave)");
    
    Serial.print("本机MAC: ");
    for (int i = 0; i < 6; i++) {
        Serial.printf("%02X", mac[i]);
        if (i < 5) Serial.print(":");
    }
    Serial.println();
    
    Serial.print("主设备MAC: ");
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
    
    // 添加对等设备（主设备）
    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, peerAddress, 6);
    peerInfo.channel = ESPNOW_CHANNEL;
    peerInfo.encrypt = ESPNOW_ENCRYPT;
    
    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
        Serial.println("添加对等设备失败");
        return;
    }
    
    // 添加广播地址对等设备以接收配对请求
    uint8_t broadcastAddr[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    esp_now_peer_info_t broadcastPeer = {};
    memcpy(broadcastPeer.peer_addr, broadcastAddr, 6);
    broadcastPeer.channel = ESPNOW_CHANNEL;
    broadcastPeer.encrypt = false; // 广播不加密
    
    if (esp_now_add_peer(&broadcastPeer) != ESP_OK) {
        Serial.println("添加广播对等设备失败");
    } else {
        Serial.println("成功添加广播对等设备");
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
            
        case CMD_START_TASK:
            handleTrainingStart();
            break;
            
        case CMD_RESET:
            currentState = SLAVE_IDLE;
            trainingActive = false;
            slaveHardware.indicateTrainingState(currentState);
            Serial.println("收到重置命令");
            break;
            
        case CMD_VT_ROUND_COMPLETE:
            // 主机发送的完成信号，重置从机状态
            Serial.printf("收到主机完成信号，用时: %lu ms\n", message.data);
            currentState = SLAVE_IDLE;
            trainingActive = false;
            slaveHardware.indicateTrainingState(currentState);
            slaveHardware.playCompleteSound();
            break;
            
        case CMD_PAIRING_REQUEST:
            // 从机设备应该始终响应配对请求，无需pairingModeActive检查
            Serial.println("收到广播配对请求，准备响应");
            handlePairingMessage(message, recv_info->src_addr);
            break;
    }
}

void onDataSent(const uint8_t* mac, esp_now_send_status_t status) {
    Serial.printf("发送状态: %s\n", (status == ESP_NOW_SEND_SUCCESS) ? "成功" : "失败");
}

void updateSystem() {
    // 每5秒输出当前状态用于调试
    static unsigned long lastStatusDebug = 0;
    if (millis() - lastStatusDebug > 5000) {
        Serial.printf("从机当前状态: %d (IDLE=%d, READY=%d, TRAINING=%d)\n", 
                     currentState, SLAVE_IDLE, SLAVE_READY, SLAVE_TRAINING);
        lastStatusDebug = millis();
    }
    
    // 如果从机长时间不在IDLE状态，自动重置
    static unsigned long lastIdleTime = millis();
    if (currentState == SLAVE_IDLE) {
        lastIdleTime = millis();
    } else if (millis() - lastIdleTime > 30000) { // 30秒超时
        Serial.println("从机状态超时，重置到IDLE状态");
        currentState = SLAVE_IDLE;
        lastIdleTime = millis();
    }
    
    switch (currentState) {
        case SLAVE_INIT:
            // 初始化状态
            Serial.println("从机处于INIT状态");
            break;
            
        case SLAVE_IDLE:
            // 空闲状态，等待震动触发或主设备命令
            slaveHardware.indicateConnectionStatus(connectionStatus);
            
            // 在空闲状态检测震动，发送开始信号给主机
            Serial.println("从机在IDLE状态，检测震动中...");
            if (slaveHardware.isVibrationDetected()) {
                Serial.println("从机检测到震动，发送开始信号给主机");
                sendStartTrainingSignal();
                currentState = SLAVE_READY;
                slaveHardware.indicateTrainingState(currentState);
            }
            break;
            
        case SLAVE_READY:
            // 准备状态 - 也检测震动，发送开始信号给主机
            slaveHardware.indicateTrainingState(currentState);
            
            Serial.println("从机在READY状态，检测震动中...");
            if (slaveHardware.isVibrationDetected()) {
                Serial.println("从机检测到震动，发送开始信号给主机");
                sendStartTrainingSignal();
                // 保持READY状态，等待主机完成信号
            }
            break;
            
        case SLAVE_TRAINING:
            // 训练中，监控震动传感器
            if (slaveHardware.isVibrationDetected()) {
                handleTrainingComplete();
            }
            
            // 检查训练超时
            if (millis() - trainingStartTime > TIMING_TIMEOUT_MS) {
                Serial.println("训练超时");
                currentState = SLAVE_ERROR;
                slaveHardware.indicateTrainingState(currentState);
            }
            break;
            
        case SLAVE_COMPLETE:
            // 完成状态，等待一段时间后回到空闲
            if (millis() - trainingStartTime > 5000) {
                currentState = SLAVE_IDLE;
                slaveHardware.indicateTrainingState(currentState);
            }
            break;
            
        case SLAVE_ERROR:
            // 错误状态
            slaveHardware.indicateTrainingState(currentState);
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
    message.target_id = 0; // 发送给主设备
    message.source_id = 1; // 从设备ID
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
    ackMessage.source_id = 1; // 从设备ID
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
        slaveHardware.indicateConnectionStatus(status);
        
        if (status == CONN_CONNECTED) {
            connectionRetryCount = 0;
            slaveHardware.playConnectedSound();
        }
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

// 训练处理函数实现
void handleTrainingStart() {
    if (connectionStatus == CONN_CONNECTED) {
        currentState = SLAVE_READY;
        trainingStartTime = millis();
        trainingActive = true;
        
        Serial.println("收到训练开始命令");
        slaveHardware.indicateTrainingState(currentState);
        slaveHardware.playStartSound();
        
        // 短暂延迟后进入训练状态
        delay(TIMING_READY_DELAY_MS);
        currentState = SLAVE_TRAINING;
        slaveHardware.indicateTrainingState(currentState);
        Serial.println("进入训练状态，等待震动触发");
    } else {
        Serial.println("连接未建立，无法开始训练");
    }
}

void handleTrainingComplete() {
    if (trainingActive) {
        unsigned long duration = millis() - trainingStartTime;
        trainingActive = false;
        currentState = SLAVE_COMPLETE;
        
        Serial.printf("训练完成，用时: %lu毫秒\n", duration);
        
        slaveHardware.indicateVibrationDetected();
        slaveHardware.indicateTrainingState(currentState);
        
        // 发送训练结果给主设备
        sendTrainingResult(duration);
    }
}

void sendTrainingResult(unsigned long duration) {
    message_t message;
    message.command = CMD_TASK_COMPLETE;
    message.target_id = 0; // 发送给主设备
    message.source_id = 1; // 从设备ID
    message.timestamp = millis();
    message.data = duration;
    message.checksum = 0; // TODO: 实现校验和计算
    
    esp_err_t result = esp_now_send(peerAddress, (uint8_t*)&message, sizeof(message));
    if (result == ESP_OK) {
        Serial.println("训练结果发送成功");
    } else {
        Serial.printf("训练结果发送失败: %d\n", result);
    }
}

void sendStartTrainingSignal() {
    message_t message;
    message.command = CMD_VT_START_ROUND;
    message.target_id = 0; // 发送给主设备
    message.source_id = 1; // 从设备ID
    message.timestamp = millis();
    message.data = 0;
    message.checksum = 0; // TODO: 实现校验和计算
    
    Serial.println("=== 从机发送开始训练信号 ===");
    Serial.printf("目标地址: %02X:%02X:%02X:%02X:%02X:%02X\n", 
                  peerAddress[0], peerAddress[1], peerAddress[2], 
                  peerAddress[3], peerAddress[4], peerAddress[5]);
    
    esp_err_t result = esp_now_send(peerAddress, (uint8_t*)&message, sizeof(message));
    if (result == ESP_OK) {
        Serial.println("从机开始训练信号发送成功");
    } else {
        Serial.printf("从机开始训练信号发送失败: %d\n", result);
    }
}

// 设备配对函数实现
void handlePairingMessage(const message_t& message, const uint8_t* senderMac) {
    switch (message.command) {
        case CMD_PAIRING_REQUEST:
            Serial.println("收到配对请求");
            respondToPairingRequest(senderMac);
            break;
    }
}

void respondToPairingRequest(const uint8_t* senderMac) {
    // 先检查发送者是否已经是对等设备，如果不是则添加
    if (!esp_now_is_peer_exist(senderMac)) {
        esp_now_peer_info_t senderPeer = {};
        memcpy(senderPeer.peer_addr, senderMac, 6);
        senderPeer.channel = ESPNOW_CHANNEL;
        senderPeer.encrypt = false; // 配对期间不加密
        
        esp_err_t addResult = esp_now_add_peer(&senderPeer);
        if (addResult != ESP_OK) {
            Serial.printf("添加发送者对等设备失败: %d\n", addResult);
            return;
        } else {
            Serial.println("成功添加发送者对等设备");
        }
    }
    
    // 发送设备信息回应
    message_t response;
    response.command = CMD_DEVICE_INFO;
    response.target_id = 0; // 发送给主设备
    response.source_id = 1; // 从设备ID
    response.timestamp = millis();
    response.data = deviceRole; // 发送角色信息
    response.checksum = 0;
    
    esp_err_t result = esp_now_send(senderMac, (uint8_t*)&response, sizeof(response));
    if (result == ESP_OK) {
        Serial.println("发送设备信息回应成功");
        
        // 打印发送者MAC地址用于调试
        Serial.print("回应发送至MAC: ");
        for (int i = 0; i < 6; i++) {
            Serial.printf("%02X", senderMac[i]);
            if (i < 5) Serial.print(":");
        }
        Serial.println();
    } else {
        Serial.printf("设备信息回应发送失败: %d\n", result);
    }
}
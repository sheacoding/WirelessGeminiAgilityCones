#include "vibration_training.h"
#include <esp_now.h>

extern DeviceRole deviceRole;

VibrationTrainingManager vibrationTraining;

VibrationTrainingManager::VibrationTrainingManager() 
    : running(false), completed(false), state(VT_STATE_IDLE),
      singleStartTime(0), singleElapsedTime(0), trainingStartTime(0),
      totalTrainingTime(0), elapsedTime(0), sessionCount(0), 
      lastSessionTime(0), lastAlertTime(0), alertInterval(30000) {}

void VibrationTrainingManager::init() {
    reset();
    showReadyCountdown();
}

void VibrationTrainingManager::update() {
    if (running) {
        updateTimer();
        checkAlerts();
        hardware.update();
        
        // 根据设备角色和状态处理震动检测
        if (deviceRole == ROLE_MASTER) {
            // 主机：在TIMING状态下检测震动，结束单次计时
            if (state == VT_STATE_TIMING) {
                Serial.println("主机在TIMING状态，正在检测震动...");
                if (hardware.isVibrationDetected()) {
                    Serial.println("主机在TIMING状态检测到震动，调用handleMasterVibration");
                    handleMasterVibration();
                }
            } else {
                // 每10秒输出一次非TIMING状态信息
                static unsigned long lastNonTimingDebug = 0;
                if (millis() - lastNonTimingDebug > 10000) {
                    Serial.printf("主机不在TIMING状态，当前状态: %d (VT_STATE_TIMING=%d)\n", state, VT_STATE_TIMING);
                    lastNonTimingDebug = millis();
                }
            }
        } else if (deviceRole == ROLE_SLAVE) {
            // 从机：在WAITING状态下检测震动，发送开始信号
            if (state == VT_STATE_WAITING && hardware.isVibrationDetected()) {
                Serial.println("从机在WAITING状态检测到震动，调用handleSlaveVibration");
                handleSlaveVibration();
            }
        } else {
            // 单设备模式：直接在WAITING状态检测震动开始计时
            if (state == VT_STATE_WAITING && hardware.isVibrationDetected()) {
                Serial.println("单设备模式开始计时");
                state = VT_STATE_TIMING;
                singleStartTime = millis();
                hardware.displayStatus("计时中...");
            } else if (state == VT_STATE_TIMING && hardware.isVibrationDetected()) {
                Serial.println("单设备模式结束计时");
                handleMasterVibration();
            }
        }
        
        // 每5秒输出当前状态用于调试
        static unsigned long lastDebugOutput = 0;
        if (millis() - lastDebugOutput > 5000) {
            Serial.printf("震动训练状态: running=%s, state=%d, deviceRole=%d\n", 
                         running ? "true" : "false", state, deviceRole);
            lastDebugOutput = millis();
        }
        
        checkTimeout();
    }
}

void VibrationTrainingManager::start() {
    running = true;
    completed = false;
    state = VT_STATE_WAITING;
    trainingStartTime = millis();
    elapsedTime = 0;
    sessionCount = 0;
    totalTrainingTime = 0;
    lastAlertTime = 0;
    
    hardware.playStartSound();
    hardware.setAllLEDs(COLOR_GREEN);
    hardware.showLEDs();
    
    if (deviceRole == ROLE_MASTER) {
        hardware.displayStatus("等待从机触发...");
        Serial.println("主机训练开始，等待从机发送开始信号");
    } else if (deviceRole == ROLE_SLAVE) {
        hardware.displayStatus("触摸此设备开始");
        Serial.println("从机训练开始，等待震动触发");
    } else {
        hardware.displayStatus("单设备训练开始");
        Serial.println("单设备训练模式");
    }
}

void VibrationTrainingManager::stop() {
    running = false;
    hardware.displayStatus("Training Stopped");
    hardware.playCompleteSound();
}

void VibrationTrainingManager::reset() {
    running = false;
    completed = false;
    state = VT_STATE_IDLE;
    singleStartTime = 0;
    singleElapsedTime = 0;
    elapsedTime = 0;
    hardware.displayClear();
}

// 主机检测到震动，结束单次计时
void VibrationTrainingManager::handleMasterVibration() {
    Serial.printf("主机handleMasterVibration被调用，当前状态: %d\n", state);
    
    if (state == VT_STATE_TIMING) {
        singleElapsedTime = millis() - singleStartTime;
        state = VT_STATE_COMPLETED;
        
        // 更新统计数据
        sessionCount++;
        lastSessionTime = singleElapsedTime;
        totalTrainingTime += singleElapsedTime;
        
        // 显示结果
        char resultText[50];
        sprintf(resultText, "第%d次: %.3f秒", sessionCount, singleElapsedTime / 1000.0);
        hardware.displayResult(singleElapsedTime, resultText);
        
        // 视觉和音效反馈
        hardware.setAllLEDs(COLOR_BLUE);
        hardware.showLEDs();
        hardware.playCompleteSound();
        
        // 记录训练数据
        hardware.addTrainingRecord(singleElapsedTime, MODE_VIBRATION_TRAINING, true);
        
        Serial.printf("主机完成第%d次，用时: %.3f秒\n", sessionCount, singleElapsedTime / 1000.0);
        
        // 发送完成信号给从机，通知重置
        sendCompleteMessage();
        
        // 等待2秒后重置到等待状态
        delay(2000);
        state = VT_STATE_WAITING;
        hardware.setAllLEDs(COLOR_GREEN);
        hardware.showLEDs();
        hardware.displayStatus("等待从机触发...");
    } else {
        Serial.printf("主机状态不是TIMING，当前状态: %d\n", state);
    }
}

// 从机开始信号处理（收到从机发来的开始计时信号）
void VibrationTrainingManager::handleSlaveComplete(unsigned long roundTime) {
    Serial.printf("handleSlaveComplete 被调用，当前状态: %d, 数据: %lu\n", state, roundTime);
    
    if (state == VT_STATE_WAITING) {
        // 主机收到从机的开始信号，开始计时
        Serial.printf("主机状态从 %d (WAITING) 切换到 %d (TIMING)\n", state, VT_STATE_TIMING);
        state = VT_STATE_TIMING;
        singleStartTime = millis();
        
        hardware.playStartSound();
        hardware.setAllLEDs(COLOR_YELLOW);
        hardware.showLEDs();
        hardware.displayStatus("计时中...触摸主机结束");
        Serial.printf("主机开始计时，开始时间: %lu\n", singleStartTime);
        Serial.printf("主机现在应该在TIMING状态，state=%d\n", state);
    } else {
        Serial.printf("主机状态不是WAITING，当前状态: %d\n", state);
    }
}

// 从机检测到震动，发送开始信号
void VibrationTrainingManager::handleSlaveVibration() {
    Serial.printf("从机handleSlaveVibration被调用，当前状态: %d\n", state);
    
    if (state == VT_STATE_WAITING) {
        state = VT_STATE_TIMING;  // 从机进入等待主机完成状态
        
        hardware.playStartSound();
        hardware.setAllLEDs(COLOR_ORANGE);
        hardware.showLEDs();
        hardware.displayStatus("信号已发送，等待主机...");
        
        // 发送开始信号给主机
        sendStartMessage();
        Serial.println("从机检测到震动，发送开始信号给主机");
        
        // 延迟后更新显示
        delay(1000);
        hardware.displayStatus("等待主机完成计时...");
    } else {
        Serial.printf("从机状态不是WAITING，当前状态: %d\n", state);
    }
}

void VibrationTrainingManager::updateTimer() {
    if (running) {
        elapsedTime = millis() - trainingStartTime;
        
        // 根据状态显示不同信息
        if (state == VT_STATE_TIMING) {
            singleElapsedTime = millis() - singleStartTime;
            // 实时显示计时状态
            char timingText[50];
            sprintf(timingText, "计时中: %.3f秒", singleElapsedTime / 1000.0);
            hardware.displayStatus(timingText);
            Serial.printf("单次计时中: %.3f秒\n", singleElapsedTime / 1000.0);
        } else if (state == VT_STATE_WAITING) {
            // 等待状态显示
            if (deviceRole == ROLE_MASTER) {
                hardware.displayStatus("等待从机触发...");
            } else if (deviceRole == ROLE_SLAVE) {
                hardware.displayStatus("触摸此设备开始");
            }
        }
        
        // 每5秒切换到详细状态显示
        static unsigned long lastDetailedDisplay = 0;
        if (millis() - lastDetailedDisplay > 5000) {
            float currentTimeSeconds = (totalTrainingTime + elapsedTime) / 1000.0;
            bool isConnected = true; // TODO: 从实际连接状态获取
            int batteryLevel = 80;   // TODO: 从实际电池状态获取
            int signalStrength = 75; // TODO: 从实际信号强度获取
            bool isMaster = (deviceRole == ROLE_MASTER);
            
            hardware.displayTrainingDetailedStatus(currentTimeSeconds, sessionCount, 
                                                 isConnected, batteryLevel, signalStrength, isMaster);
            lastDetailedDisplay = millis();
        }
        
        updateVisualFeedback();
    }
}

void VibrationTrainingManager::checkTimeout() {
    // 单次计时超时检查
    if (state == VT_STATE_TIMING && singleElapsedTime >= TIMING_TIMEOUT_MS) {
        state = VT_STATE_WAITING;
        hardware.displayStatus("Timeout - Try again");
        hardware.playErrorSound();
        
        // 记录超时的训练数据  
        hardware.addTrainingRecord(singleElapsedTime, MODE_SINGLE_TIMER, false);
        
        delay(2000);
        if (deviceRole == ROLE_MASTER) {
            hardware.displayStatus("Touch device to start");
        } else {
            hardware.displayStatus("Waiting for start...");
        }
    }
}

void VibrationTrainingManager::showReadyCountdown() {
    hardware.displayStatus("Get Ready");
    for (int i = TIMING_READY_DELAY_MS / 1000; i > 0; --i) {
        char countStr[4];
        sprintf(countStr, "%d", i);
        hardware.displayTextCentered(countStr, 40);
        delay(1000);
    }
}

void VibrationTrainingManager::updateVisualFeedback() {
    int progress = (elapsedTime * 100) / TIMING_TIMEOUT_MS;
    hardware.ledProgressBar(progress, COLOR_GREEN);
}

void VibrationTrainingManager::checkAlerts() {
    // 总运动时长达标提醒
    unsigned long currentTotalTime = totalTrainingTime + elapsedTime;
    uint32_t alertIntervalMs = hardware.getSettings()->alertDuration * 1000;
    
    if (currentTotalTime - lastAlertTime >= alertIntervalMs) {
        lastAlertTime = currentTotalTime;
        hardware.playAlertSound();
        hardware.displayStatus("Time Goal Reached!");
        delay(1000);
    }
}

// 退出训练并显示当天运动情况
void VibrationTrainingManager::exitTraining() {
    running = false;
    state = VT_STATE_IDLE;
    
    displayDailyStats();
    hardware.playCompleteSound();
}

// 发送开始计时消息给主机（从机发送）
void VibrationTrainingManager::sendStartMessage() {
    extern uint8_t peerAddress[6];
    
    Serial.println("=== sendStartMessage() 被调用 ===");
    Serial.printf("peerAddress: %02X:%02X:%02X:%02X:%02X:%02X\n", 
                  peerAddress[0], peerAddress[1], peerAddress[2], 
                  peerAddress[3], peerAddress[4], peerAddress[5]);
    
    message_t msg;
    msg.command = CMD_VT_START_ROUND;
    msg.target_id = 0;  // 发送给主机
    msg.source_id = 1;  // 从机发送
    msg.timestamp = millis();
    msg.data = 0;
    msg.checksum = 0; // TODO: 计算校验和
    
    Serial.printf("发送消息: command=%d, target_id=%d, source_id=%d\n", 
                  msg.command, msg.target_id, msg.source_id);
    
    esp_err_t result = esp_now_send(peerAddress, (uint8_t*)&msg, sizeof(msg));
    if (result == ESP_OK) {
        Serial.println("从机发送开始计时信号成功");
    } else {
        Serial.printf("从机发送开始计时信号失败: %d\n", result);
    }
}

// 从机收到主机完成信号，重置状态
void VibrationTrainingManager::handleRoundComplete(unsigned long roundTime) {
    if (state == VT_STATE_TIMING) {
        state = VT_STATE_WAITING;  // 重置到等待状态
        
        // 显示主机完成的用时
        char resultText[50];
        sprintf(resultText, "主机用时: %.3f秒", roundTime / 1000.0);
        hardware.displayResult(roundTime, resultText);
        
        // 视觉反馈
        hardware.setAllLEDs(COLOR_GREEN);
        hardware.showLEDs();
        hardware.playCompleteSound();
        
        Serial.printf("从机收到主机完成信号，用时: %.3f秒\n", roundTime / 1000.0);
        
        // 等待2秒后重置显示
        delay(2000);
        hardware.displayStatus("触摸此设备开始");
    }
}

// 发送完成信号给从机（主机发送）
void VibrationTrainingManager::sendCompleteMessage() {
    extern uint8_t peerAddress[6];
    
    message_t msg;
    msg.command = CMD_VT_ROUND_COMPLETE;
    msg.target_id = 1;  // 发送给从机
    msg.source_id = 0;  // 主机发送
    msg.timestamp = millis();
    msg.data = singleElapsedTime;  // 发送用时
    msg.checksum = 0; // TODO: 计算校验和
    
    esp_err_t result = esp_now_send(peerAddress, (uint8_t*)&msg, sizeof(msg));
    if (result == ESP_OK) {
        Serial.println("主机发送完成信号成功");
    } else {
        Serial.printf("主机发送完成信号失败: %d\n", result);
    }
}

// 显示当天运动情况
void VibrationTrainingManager::displayDailyStats() {
    hardware.displayClear();
    
    char statsText[100];
    sprintf(statsText, "Today's Training\nSessions: %d\nTotal Time: %.1fs", 
            sessionCount, (totalTrainingTime + elapsedTime) / 1000.0);
    
    hardware.displayTextCentered(statsText, 30);
    delay(3000);
}

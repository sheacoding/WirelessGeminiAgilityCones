// 主循环中检测和处理按键事件标志的示例代码
// 这个示例展示如何在main.cpp的loop()函数中检测hardware中的按键事件标志

#include "hardware.h"
#include "menu.h"

// 全局变量声明
extern HardwareManager hardware;
extern MenuManager menu;
extern SystemState currentState;

void loop() {
    // 更新硬件状态（包括按键事件处理）
    hardware.update();
    
    // 检测按键事件标志并处理
    handleButtonFlags();
    
    // 其他主循环逻辑...
    switch (currentState) {
        case STATE_MENU:
            // 菜单状态处理
            menu.update();
            break;
            
        case STATE_READY:
            // 准备状态处理
            // 显示准备界面，等待开始训练
            break;
            
        case STATE_TIMING:
            // 计时状态处理
            // 处理训练逻辑
            break;
            
        case STATE_COMPLETE:
            // 完成状态处理
            // 显示结果，等待用户操作
            break;
    }
    
    // 其他系统更新...
    delay(10); // 避免过度占用CPU
}

void handleButtonFlags() {
    // 检查开始训练标志
    if (hardware.getStartTrainingFlag()) {
        Serial.println("检测到开始训练标志");
        
        // 清除标志
        hardware.clearStartTrainingFlag();
        
        // 执行开始训练的逻辑
        startTraining();
    }
    
    // 检查返回菜单标志
    if (hardware.getReturnToMenuFlag()) {
        Serial.println("检测到返回菜单标志");
        
        // 清除标志
        hardware.clearReturnToMenuFlag();
        
        // 执行返回菜单的逻辑
        returnToMenu();
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
        
        // 初始化训练计时器等
        // initTrainingTimer();
        
        Serial.println("训练状态切换完成");
    } else {
        Serial.println("当前状态不允许开始训练");
    }
}

void returnToMenu() {
    Serial.println("返回主菜单...");
    
    // 保存当前训练数据（如果有）
    if (currentState == STATE_TIMING || currentState == STATE_COMPLETE) {
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
    menu.displayMainMenu();
    
    Serial.println("已返回主菜单");
}

// 辅助函数：检查训练是否应该自动完成
void checkTrainingCompletion() {
    if (currentState == STATE_TIMING) {
        // 检查完成条件（例如震动传感器触发）
        if (hardware.isVibrationDetected()) {
            completeTraining();
        }
    }
}

void completeTraining() {
    Serial.println("训练完成！");
    
    // 切换到完成状态
    currentState = STATE_COMPLETE;
    
    // 播放完成音效
    if (hardware.getSettings()->soundEnabled) {
        hardware.playCompleteSound();
    }
    
    // 显示完成效果
    hardware.setAllLEDs(COLOR_GREEN);
    hardware.showLEDs();
    
    // 显示结果
    // unsigned long trainingTime = getTrainingTime();
    // hardware.displayResult(trainingTime, "训练完成！");
    
    Serial.println("训练完成处理完毕");
}

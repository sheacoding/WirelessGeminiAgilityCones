#include <Arduino.h>
#include "config.h"
#include "hardware.h"
#include "menu.h"

// 全局变量
SystemState currentState = STATE_MENU;
DeviceRole deviceRole = ROLE_MASTER;
bool systemInitialized = false;

void setup() {
    Serial.begin(115200);
    Serial.println("菜单功能测试程序启动...");
    
    // 初始化硬件
    if (!hardware.init()) {
        Serial.println("硬件初始化失败!");
        return;
    }
    
    // 初始化菜单
    menu.init();
    
    systemInitialized = true;
    Serial.println("系统初始化完成，开始菜单测试");
    Serial.println("操作说明：");
    Serial.println("- 短按按钮：切换菜单选项");
    Serial.println("- 长按按钮：确认选择");
}

void loop() {
    if (!systemInitialized) {
        return;
    }
    
    hardware.update();
    
    static bool lastButtonState = false;
    static bool lastLongPressState = false;
    bool currentButtonState = hardware.isButtonPressed();
    bool currentLongPressState = hardware.isButtonLongPressed();
    
    // 输出原始按钮状态用于调试
    static unsigned long lastDebugTime = 0;
    if (millis() - lastDebugTime > 2000) {
        int rawButton = digitalRead(BUTTON_PIN);
        Serial.printf("调试信息: 原始按钮=%d, 处理后=%d, 长按=%d, 菜单活动=%d\n", 
                     rawButton, currentButtonState, currentLongPressState, menu.isMenuActive());
        lastDebugTime = millis();
    }
    
    // 检测按钮按下边沿
    if (currentButtonState && !lastButtonState) {
        Serial.println("检测到按钮按下边沿");
        if (menu.isMenuActive()) {
            menu.selectNext();
            Serial.printf("当前菜单项: %d\n", menu.getCurrentMenuItem());
        } else {
            Serial.println("菜单不活动，无法切换");
        }
    }
    
    // 检测长按
    if (currentLongPressState && !lastLongPressState) {
        Serial.println("检测到长按");
        if (menu.isMenuActive()) {
            menu.confirm();
            Serial.printf("选择了模式: %d\n", menu.getCurrentMode());
            // 重新激活菜单用于测试
            delay(1000);
            menu.init();
        } else {
            Serial.println("菜单不活动，无法确认");
        }
    }
    
    lastButtonState = currentButtonState;
    lastLongPressState = currentLongPressState;
    
    delay(50); // 短暂延迟
}

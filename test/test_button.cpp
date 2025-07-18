#include <Arduino.h>
#include "config.h"

void setup() {
    Serial.begin(115200);
    Serial.println("按钮测试程序启动...");
    
    // 初始化按钮引脚
    pinMode(BUTTON_PIN, INPUT);
    Serial.printf("按钮引脚 GPIO%d 初始化完成\n", BUTTON_PIN);
    
    Serial.println("开始按钮测试：");
    Serial.println("- 按下按钮应该看到HIGH状态");
    Serial.println("- 松开按钮应该看到LOW状态");
}

void loop() {
    static bool lastButtonState = false;
    static unsigned long lastPrintTime = 0;
    
    // 读取按钮状态
    bool currentButtonState = (digitalRead(BUTTON_PIN) == HIGH);
    
    // 检测按钮状态变化
    if (currentButtonState != lastButtonState) {
        Serial.printf("按钮状态变化: %s\n", currentButtonState ? "按下 (HIGH)" : "释放 (LOW)");
        lastButtonState = currentButtonState;
    }
    
    // 每秒输出一次当前状态
    if (millis() - lastPrintTime > 1000) {
        Serial.printf("当前按钮状态: %s (原始值: %d)\n", 
                     currentButtonState ? "按下" : "释放", 
                     digitalRead(BUTTON_PIN));
        lastPrintTime = millis();
    }
    
    delay(50); // 短暂延迟
}

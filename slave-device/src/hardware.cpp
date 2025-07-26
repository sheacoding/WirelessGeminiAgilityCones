#include "hardware.h"

// 全局从机硬件管理类对象
SlaveHardwareManager slaveHardware;

SlaveHardwareManager::SlaveHardwareManager() 
    : lastVibrationTime(0), lastLEDUpdate(0) {}

bool SlaveHardwareManager::init() {
    Serial.println("从机硬件初始化开始...");
    
    // 初始化震动传感器引脚 (常闭开关量传感器，使用内部上拉电阻)
    pinMode(VIBRATION_SENSOR_PIN, INPUT_PULLUP);
    Serial.println("震动传感器引脚初始化完成（常闭开关量传感器）");
    
    // 初始化蜂鸣器引脚
    pinMode(BUZZER_PIN, OUTPUT);
    Serial.println("蜂鸣器引脚初始化完成");
    
    // 初始化LED
    FastLED.addLeds<NEOPIXEL, LED_PIN>(leds, LED_COUNT);
    FastLED.setBrightness(LED_BRIGHTNESS);
    setAllLEDs(COLOR_BLACK);
    showLEDs();
    Serial.println("LED灯带初始化完成");
    
    // 初始化完成指示
    playStartSound();
    setAllLEDs(COLOR_BLUE);
    showLEDs();
    delay(500);
    clearLEDs();
    showLEDs();
    
    Serial.println("从机硬件初始化完成");
    return true;
}

void SlaveHardwareManager::update() {
    updateVibration();
    updateLEDEffects();
}

// LED控制函数
void SlaveHardwareManager::setLED(int index, uint32_t color) {
    if (index >= 0 && index < LED_COUNT) {
        leds[index] = CRGB(color);
    }
}

void SlaveHardwareManager::setAllLEDs(uint32_t color) {
    for (int i = 0; i < LED_COUNT; i++) {
        leds[i] = CRGB(color);
    }
}

void SlaveHardwareManager::clearLEDs() {
    setAllLEDs(COLOR_BLACK);
}

void SlaveHardwareManager::showLEDs() {
    FastLED.show();
}

void SlaveHardwareManager::ledBreathingEffect(uint32_t color) {
    static unsigned long lastUpdate = 0;
    static int brightness = 0;
    static int direction = 1;
    
    if (millis() - lastUpdate > 50) {
        brightness += direction * 5;
        if (brightness >= 255 || brightness <= 0) {
            direction *= -1;
        }
        
        // 应用呼吸效果
        uint8_t r = (color >> 16) & 0xFF;
        uint8_t g = (color >> 8) & 0xFF;
        uint8_t b = color & 0xFF;
        
        r = (r * brightness) / 255;
        g = (g * brightness) / 255;
        b = (b * brightness) / 255;
        
        uint32_t breathColor = (r << 16) | (g << 8) | b;
        setAllLEDs(breathColor);
        showLEDs();
        
        lastUpdate = millis();
    }
}

void SlaveHardwareManager::ledProgressBar(int progress, uint32_t color) {
    clearLEDs();
    int ledsToLight = (progress * LED_COUNT) / 100;
    for (int i = 0; i < ledsToLight && i < LED_COUNT; i++) {
        setLED(i, color);
    }
    showLEDs();
}

void SlaveHardwareManager::ledAlertEffect() {
    for (int i = 0; i < 3; i++) {
        setAllLEDs(COLOR_RED);
        showLEDs();
        delay(200);
        clearLEDs();
        showLEDs();
        delay(200);
    }
}

// 震动传感器函数
bool SlaveHardwareManager::isVibrationDetected() {
    static bool lastState = HIGH;  // 常闭传感器正常状态为HIGH
    static unsigned long lastTriggerTime = 0;
    
    // 读取数字量状态
    bool currentState = digitalRead(VIBRATION_SENSOR_PIN);
    
    // 每次都输出当前状态用于调试
    static unsigned long lastDebugTime = 0;
    if (millis() - lastDebugTime > 100) {  // 每100ms输出一次状态变化
        Serial.printf("震动检测调试: lastState=%s, currentState=%s\n", 
                     lastState ? "HIGH" : "LOW", currentState ? "HIGH" : "LOW");
        lastDebugTime = millis();
    }
    
    // 检测从HIGH到LOW的下降沿（震动触发）
    if (lastState == HIGH && currentState == LOW) {
        Serial.printf("检测到边沿变化: HIGH->LOW, 防抖检查中...\n");
        
        // 防抖检查
        if (millis() - lastTriggerTime > VIBRATION_DEBOUNCE_MS) {
            lastTriggerTime = millis();
            lastState = currentState;
            
            Serial.printf("*** 从机震动检测到! 传感器状态: HIGH->LOW ***\n");
            
            // 震动检测视觉反馈
            indicateVibrationDetected();
            
            return true;
        } else {
            Serial.printf("防抖未通过: 距离上次触发仅 %lu ms\n", millis() - lastTriggerTime);
        }
    }
    
    lastState = currentState;
    return false;
}

int SlaveHardwareManager::getVibrationStrength() {
    // 对于开关量传感器，返回数字状态 (HIGH=1, LOW=0)
    return digitalRead(VIBRATION_SENSOR_PIN) ? 1 : 0;
}

// 蜂鸣器函数
void SlaveHardwareManager::beep(int frequency, int duration) {
    tone(BUZZER_PIN, frequency, duration);
}

void SlaveHardwareManager::playStartSound() {
    beep(1000, 200);
    delay(250);
    beep(1200, 200);
}

void SlaveHardwareManager::playCompleteSound() {
    beep(1500, 300);
    delay(100);
    beep(1800, 200);
    delay(100);
    beep(2000, 300);
}

void SlaveHardwareManager::playErrorSound() {
    beep(500, 500);
    delay(100);
    beep(400, 500);
}

void SlaveHardwareManager::playConnectedSound() {
    beep(800, 100);
    delay(150);
    beep(1000, 100);
    delay(150);
    beep(1200, 200);
}

// 状态指示函数
void SlaveHardwareManager::indicateConnectionStatus(ConnectionStatus status) {
    switch (status) {
        case CONN_CONNECTED:
            setAllLEDs(COLOR_GREEN);
            break;
        case CONN_CONNECTING:
            ledBreathingEffect(COLOR_YELLOW);
            return; // 不要调用showLEDs，因为呼吸效果会自己处理
        case CONN_DISCONNECTED:
        case CONN_TIMEOUT:
            setAllLEDs(COLOR_ORANGE);
            break;
        case CONN_ERROR:
            setAllLEDs(COLOR_RED);
            break;
    }
    showLEDs();
}

void SlaveHardwareManager::indicateTrainingState(SlaveState state) {
    switch (state) {
        case SLAVE_IDLE:
            setAllLEDs(COLOR_BLUE);
            break;
        case SLAVE_READY:
            ledBreathingEffect(COLOR_GREEN);
            return; // 不要调用showLEDs
        case SLAVE_TRAINING:
            setAllLEDs(COLOR_PURPLE);
            break;
        case SLAVE_COMPLETE:
            setAllLEDs(COLOR_CYAN);
            playCompleteSound();
            break;
        case SLAVE_ERROR:
            ledAlertEffect();
            return; // 不要调用showLEDs
        default:
            clearLEDs();
            break;
    }
    showLEDs();
}

void SlaveHardwareManager::indicateVibrationDetected() {
    Serial.println("震动检测指示开始");
    
    // 快速闪烁指示震动检测 - 更明显的视觉反馈
    for (int i = 0; i < 3; i++) {
        setAllLEDs(COLOR_GREEN);  // 使用绿色表示成功检测
        showLEDs();
        delay(100);
        clearLEDs();
        showLEDs();
        delay(50);
    }
    
    // 最后显示成功状态
    setAllLEDs(COLOR_BLUE);
    showLEDs();
    
    // 震动检测成功音效
    beep(2000, 150);  // 更长的提示音
    delay(100);
    beep(2500, 100);  // 双音提示
    
    Serial.println("震动检测指示完成");
}

// 私有函数实现
void SlaveHardwareManager::updateVibration() {
    // 开关量震动传感器监控逻辑
    static unsigned long lastStatusCheck = 0;
    
    // 每500ms输出一次传感器状态用于调试
    if (millis() - lastStatusCheck >= 500) {
        lastStatusCheck = millis();
        
        bool sensorState = digitalRead(VIBRATION_SENSOR_PIN);
        Serial.printf("震动传感器状态: %s (常闭传感器: HIGH=正常, LOW=震动)\n", 
                     sensorState ? "HIGH" : "LOW");
    }
}

void SlaveHardwareManager::updateLEDEffects() {
    // 这里可以添加LED效果的更新逻辑
    // 例如呼吸效果、流水灯效果等
}
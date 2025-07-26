#ifndef HARDWARE_H
#define HARDWARE_H

#include <Arduino.h>
#include <FastLED.h>
#include "config.h"

// 从机硬件管理类 - 仅包含必要的硬件组件
class SlaveHardwareManager {
public:
    SlaveHardwareManager();
    bool init();
    void update();
    
    // LED控制
    void setLED(int index, uint32_t color);
    void setAllLEDs(uint32_t color);
    void clearLEDs();
    void showLEDs();
    void ledBreathingEffect(uint32_t color);
    void ledProgressBar(int progress, uint32_t color);
    void ledAlertEffect();
    
    // 震动传感器
    bool isVibrationDetected();
    int getVibrationStrength();
    
    // 蜂鸣器
    void beep(int frequency = BEEP_FREQUENCY, int duration = BEEP_DURATION);
    void playStartSound();
    void playCompleteSound();
    void playErrorSound();
    void playConnectedSound();
    
    // 状态指示
    void indicateConnectionStatus(ConnectionStatus status);
    void indicateTrainingState(SlaveState state);
    void indicateVibrationDetected();
    
private:
    CRGB leds[LED_COUNT];
    unsigned long lastVibrationTime;
    unsigned long lastLEDUpdate;
    
    void updateVibration();
    void updateLEDEffects();
};

extern SlaveHardwareManager slaveHardware;

#endif // HARDWARE_H
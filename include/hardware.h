#ifndef HARDWARE_H
#define HARDWARE_H

#include <Arduino.h>
#include <FastLED.h>
#include <U8g2lib.h>
#include <Wire.h>
#include <WiFi.h>
// 移除Bounce2库，使用新的按键管理器
#include "config.h"

// 包含中文字体支持
#include "u8g2_wqy.h"

// 硬件管理类
class HardwareManager {
public:
    HardwareManager();
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
    
    // 传统按钮接口（兼容性保留）
    bool isButtonPressed();
    bool isButtonLongPressed();
    
    // 蜂鸣器
    void beep(int frequency = BEEP_FREQUENCY, int duration = BEEP_DURATION);
    void playStartSound();
    void playCompleteSound();
    void playErrorSound();
    void playAlertSound();
    
    // OLED显示
    void displayInit();
    void displayClear();
    void displayText(const char* text, int x = 0, int y = 0, int size = 1);
    void displayTextCentered(const char* text, int y = 35);  // 居中显示文本的便捷函数
    void displayMainMenu(const char* items[], int selectedIndex, int itemCount);
    void displayMenu(const char* items[], int selectedIndex, int itemCount);
    void displayTimer(unsigned long time);
    void displayStatus(const char* status);
    void displayResult(unsigned long time, const char* result);
    void displayTrainingStatus(unsigned long totalTime, unsigned long lastTime);
    void displayTrainingDetailedStatus(float currentTime, int sessionCount, bool isConnected, int batteryLevel, int signalStrength, bool isMaster);
    void displayHistoryData();
    
    // 训练数据管理
    void addTrainingRecord(uint32_t duration, uint8_t mode, bool completed);
    void calculateTrainingStats();
    TrainingStats* getTrainingStats();
    void initializeTrainingData();
    void createWeeklyTrainingData();
    void displaySystemSettings();
    void displaySystemSettingsMenu(int selectedIndex);
    void displaySystemSettingsDetail(SettingsItems item, int value = 0);
    void displayLedColorSelection(LedColorOption selectedColor);
    void displayBrightnessAdjustment(int brightness);
    void displayDateTimeAdjustment(int year, int month, int day, int hour, int minute);
    void displayAlertDurationAdjustment(int duration);
    void displayDevicePairing(PairingStatus status, DiscoveredDevice* devices, int deviceCount, int selectedIndex);
    
    // 系统设置管理
    void initializeSettings();
    void saveSettings();
    void loadSettings();
    SystemSettings* getSettings();
    void resetSettings();
    uint32_t getLedColorValue(LedColorOption colorOption);
    const char* getLedColorName(LedColorOption colorOption);
    
private:
    CRGB leds[LED_COUNT];
    U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2;
    
    unsigned long lastVibrationTime;
    
    void updateVibration();
    unsigned long formatTime(unsigned long ms);
    void drawTrendGraph(); // 绘制趋势图表
};

extern HardwareManager hardware;

#endif // HARDWARE_H

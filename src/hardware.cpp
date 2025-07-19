#include "hardware.h"
#include "menu.h"
#include "time_manager.h"

// 全局硬件管理类对象
HardwareManager hardware;

// 系统设置全局变量
SystemSettings systemSettings = {
    .soundEnabled = DEFAULT_SOUND_ENABLED,
    .ledColor = DEFAULT_LED_COLOR,
    .ledBrightness = DEFAULT_LED_BRIGHTNESS,
    .year = 2024,
    .month = 7,
    .day = 16,
    .hour = 12,
    .minute = 0,
    .alertDuration = DEFAULT_ALERT_DURATION
};

HardwareManager::HardwareManager() 
    : 
#ifdef FORCE_MASTER_ROLE
      u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE, /* clock=*/ OLED_SCL_PIN, /* data=*/ OLED_SDA_PIN),
#endif
      lastVibrationTime(0) {}

bool HardwareManager::init() {
    // 初始化按钮引脚 - GPIO5高电平触发按钮，使用内部下拉电阻
    pinMode(BUTTON_PIN, INPUT_PULLDOWN);
    Serial.println("按钮引脚初始化完成（高电平触发）");
    
    // 初始化震动传感器引脚
    pinMode(VIBRATION_SENSOR_PIN, INPUT);
    Serial.println("震动传感器引脚初始化完成");
    
    // 初始化蜂鸣器引脚
    pinMode(BUZZER_PIN, OUTPUT);
    Serial.println("蜂鸣器引脚初始化完成");
    
    // 初始化LED
    FastLED.addLeds<NEOPIXEL, LED_PIN>(leds, LED_COUNT);
    FastLED.setBrightness(LED_BRIGHTNESS);
    setAllLEDs(COLOR_BLACK);
    showLEDs();
    Serial.println("LED初始化完成");

#ifdef FORCE_MASTER_ROLE
    // 仅在主设备上初始化显示屏
    u8g2.begin();
    u8g2.enableUTF8Print();
    displayInit();
    Serial.println("主设备显示屏初始化完成");
#else
    Serial.println("从设备模式 - 无显示屏");
#endif
    return true;
}

void HardwareManager::update() {
    updateVibration();
}

void HardwareManager::setLED(int index, uint32_t color) {
    if (index >= 0 && index < LED_COUNT) {
        leds[index] = color;
    }
}

void HardwareManager::setAllLEDs(uint32_t color) {
    for (int i = 0; i < LED_COUNT; ++i) {
        setLED(i, color);
    }
}

void HardwareManager::clearLEDs() {
    setAllLEDs(COLOR_BLACK);
}

void HardwareManager::showLEDs() {
    FastLED.show();
}

void HardwareManager::ledProgressBar(int progress, uint32_t color) {
    int filled = (LED_COUNT * progress) / 100;
    setAllLEDs(COLOR_BLACK);
    for (int i = 0; i < filled; ++i) {
        setLED(i, color);
    }
    showLEDs();
}

void HardwareManager::ledBreathingEffect(uint32_t color) {
    // 呼吸灯效果
}

void HardwareManager::ledAlertEffect() {
    setAllLEDs(COLOR_RED);
    showLEDs();
    delay(100);
    clearLEDs();
    showLEDs();
    delay(100);
}

bool HardwareManager::isVibrationDetected() {
    int strength = getVibrationStrength();
    if (strength > VIBRATION_THRESHOLD && millis() - lastVibrationTime > VIBRATION_DEBOUNCE_MS) {
        lastVibrationTime = millis();
        return true;
    }
    return false;
}

int HardwareManager::getVibrationStrength() {
    return analogRead(VIBRATION_SENSOR_PIN);
}


void HardwareManager::beep(int frequency, int duration) {
    tone(BUZZER_PIN, frequency, duration);
}

void HardwareManager::playStartSound() {
    beep(1000, 200);
}

void HardwareManager::playCompleteSound() {
    beep(2000, 200);
}

void HardwareManager::playErrorSound() {
    beep(400, 500);
}

void HardwareManager::playAlertSound() {
    beep(1500, 100);
}

void HardwareManager::displayInit() {
#ifdef FORCE_MASTER_ROLE
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_wqy12_t_gb2312a);
    u8g2.setFontDirection(0);
#endif
}

void HardwareManager::displayClear() {
#ifdef FORCE_MASTER_ROLE
    u8g2.clearBuffer();
#endif
}

void HardwareManager::displayText(const char* text, int x, int y, int size) {
#ifdef FORCE_MASTER_ROLE
    displayClear();
    u8g2.setCursor(x, y);
    u8g2.print(text);
    u8g2.sendBuffer();
#else
    // 从设备通过串口输出显示信息
    Serial.printf("显示: %s (位置: %d,%d, 大小: %d)\n", text, x, y, size);
#endif
}

void HardwareManager::displayMainMenu(const char* items[], int selectedIndex, int itemCount) {
#ifdef FORCE_MASTER_ROLE
    displayClear();
    
    // 显示主标题 - 居中显示
    const char* title = "智能训练锥";
    int titleWidth = u8g2.getUTF8Width(title);
    int titleX = (128 - titleWidth) / 2;  // 128是OLED屏幕宽度
    u8g2.setCursor(titleX, 12);
    u8g2.print(title);
    
    // 显示菜单选项，居中显示
    for (int i = 0; i < itemCount; ++i) {
        int y = 30 + i * 15;
        
        // 创建完整的菜单项文本
        char menuText[64];
        sprintf(menuText, "%d. %s", i + 1, items[i]);
        
        // 计算文本宽度用于居中
        int textWidth = u8g2.getUTF8Width(menuText);
        int textX = (128 - textWidth) / 2;
        
        if (i == selectedIndex) {
            // 选中项的特殊显示效果
            u8g2.drawRBox(0, y - 12, 127, 14, 2);
            u8g2.setDrawColor(0); // 设置为黑色文字（反色显示）
            u8g2.setCursor(textX, y);
            u8g2.print(menuText);
            u8g2.setDrawColor(1); // 恢复为白色文字
        } else {
            // 未选中项的普通显示
            u8g2.setCursor(textX, y);
            u8g2.print(menuText);
        }
    }
    u8g2.sendBuffer();
#else
    // 从设备通过串口输出菜单信息
    Serial.println("智能训练锥:");
    for (int i = 0; i < itemCount; ++i) {
        Serial.printf("%s%d. %s\n", (i == selectedIndex) ? "> " : "  ", i + 1, items[i]);
    }
#endif
}

void HardwareManager::displayMenu(const char* items[], int selectedIndex, int itemCount) {
#ifdef FORCE_MASTER_ROLE
    displayClear();
    
    // 简单的菜单显示（用于子菜单）
    for (int i = 0; i < itemCount; ++i) {
        int y = 16 + i * 15;
        
        if (i == selectedIndex) {
            u8g2.drawRBox(0, y - 12, 127, 14, 2);
            u8g2.setDrawColor(0);
            u8g2.setCursor(5, y);
            u8g2.print(items[i]);
            u8g2.setDrawColor(1);
        } else {
            u8g2.setCursor(5, y);
            u8g2.print(items[i]);
        }
    }
    u8g2.sendBuffer();
#else
    Serial.println("菜单:");
    for (int i = 0; i < itemCount; ++i) {
        Serial.printf("%s%s\n", (i == selectedIndex) ? "> " : "  ", items[i]);
    }
#endif
}

void HardwareManager::displayTimer(unsigned long time) {
    char buf[16];
    sprintf(buf, "时间: %lu", time / 1000);
#ifdef FORCE_MASTER_ROLE
    displayText(buf, 0, 15);
#else
    Serial.println(buf);
#endif
}

void HardwareManager::displayStatus(const char* status) {
#ifdef FORCE_MASTER_ROLE
    displayText(status, 0, 30);
#else
    Serial.printf("状态: %s\n", status);
#endif
}

void HardwareManager::displayResult(unsigned long time, const char* result) {
#ifdef FORCE_MASTER_ROLE
    displayClear();
    
    // 显示时间
    char buf[16];
    sprintf(buf, "时间: %lu", time / 1000);
    u8g2.setCursor(0, 15);
    u8g2.print(buf);
    
    // 显示结果
    u8g2.setCursor(0, 30);
    u8g2.print(result);
    
    u8g2.sendBuffer();
#else
    Serial.printf("结果: %s, 时间: %lu秒\n", result, time / 1000);
#endif
}

void HardwareManager::updateVibration() {
    // 震动传感器更新逻辑
}


void HardwareManager::displayTrainingStatus(unsigned long totalTime, unsigned long lastTime) {
#ifdef FORCE_MASTER_ROLE
    displayClear();
    
    // 显示标题
    u8g2.setCursor(40, 12);
    u8g2.print("训练中...");
    
    // 显示总时长
    char totalBuf[20];
    unsigned long totalSeconds = totalTime / 1000;
    sprintf(totalBuf, "总时长: %02lu:%02lu:%02lu", totalSeconds / 3600, (totalSeconds % 3600) / 60, totalSeconds % 60);
    u8g2.setCursor(5, 30);
    u8g2.print(totalBuf);
    
    // 上次用时显示
    char lastBuf[20];
    sprintf(lastBuf, "上次用时: %.3f 秒", lastTime / 1000.0);
    u8g2.setCursor(5, 45);
    u8g2.print(lastBuf);
    
    // 简单的进度条效果
    u8g2.drawFrame(5, 55, 118, 6);
    int progress = (millis() / 100) % 116;
    u8g2.drawBox(6, 56, progress, 4);
    
    u8g2.sendBuffer();
#else
    Serial.printf("训练中 - 总时长: %lu秒, 上次用时: %lu秒\n", totalTime / 1000, lastTime / 1000);
#endif
}

void HardwareManager::displayHistoryData() {
#ifdef FORCE_MASTER_ROLE
    displayClear();
    
    // 显示标题
    u8g2.setCursor(40, 12);
    u8g2.print("训练统计");
    
    // 显示当前时间
    String currentTime = timeManager.formatTime("%H:%M:%S");
    u8g2.setCursor(5, 25);
    u8g2.printf("当前时间: %s", currentTime.c_str());
    
    // 显示今日日期
    String currentDate = timeManager.formatTime("%Y-%m-%d");
    u8g2.setCursor(5, 40);
    u8g2.printf("今日日期: %s", currentDate.c_str());
    
    // 显示系统运行时间
    unsigned long uptime = millis() / 1000;
    u8g2.setCursor(5, 55);
    u8g2.printf("运行时间: %02lu:%02lu:%02lu", uptime / 3600, (uptime % 3600) / 60, uptime % 60);
    
    u8g2.sendBuffer();
#else
    Serial.printf("训练统计 - 当前时间: %s\n", timeManager.formatTime("%Y-%m-%d %H:%M:%S").c_str());
#endif
}

void HardwareManager::displaySystemSettings() {
#ifdef FORCE_MASTER_ROLE
    displayClear();
    
    // 显示标题
    u8g2.setCursor(40, 12);
    u8g2.print("系统设置");
    
    // 显示MAC地址
    uint8_t mac[6];
    WiFi.macAddress(mac);
    u8g2.setCursor(5, 30);
    u8g2.printf("MAC:  %02X : %02X : %02X : %02X : %02X : %02X ", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    
    // 显示声音设置
    u8g2.setCursor(5, 45);
    u8g2.print("声音: [ 开启 ] [ 关闭 ]");
    
    // 显示灯环颜色
    u8g2.setCursor(5, 60);
    u8g2.print("灯环: [ 红 ] [ 绿 ] [ 蓝 ] [ 黄 ]");
    
    u8g2.sendBuffer();
#else
    Serial.println("系统设置页面");
#endif
}


// ==================== 系统设置功能实现 ====================

void HardwareManager::displaySystemSettingsMenu(int selectedIndex) {
#ifdef FORCE_MASTER_ROLE
    displayClear();
    
    // 显示标题
    const char* title = "系统设置";
    int titleWidth = u8g2.getUTF8Width(title);
    int titleX = (128 - titleWidth) / 2;
    u8g2.setCursor(titleX, 12);
    u8g2.print(title);
    
    // 显示MAC地址 - 缩短格式以节省空间
    uint8_t mac[6];
    WiFi.macAddress(mac);
    u8g2.setCursor(5, 23);
    u8g2.printf("MAC:  %02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    
    // 设置选项
    const char* settingsItems[] = {
        "声音设置",
        "LED颜色", 
        "LED亮度",
        "日期时间",
        "达标提醒",
        "返回"
    };
    
    // 计算滚动显示参数
    const int maxVisibleItems = 3;  // 最多显示3个选项
    const int itemHeight = 12;      // 每个选项的高度
    const int startY = 35;          // 第一个选项的起始Y坐标
    
    // 计算滚动偏移
    int scrollOffset = 0;
    if (selectedIndex >= maxVisibleItems) {
        scrollOffset = selectedIndex - maxVisibleItems + 1;
    }
    
    // 显示可见的设置选项
    for (int i = 0; i < maxVisibleItems && (i + scrollOffset) < SETTING_ITEM_COUNT; ++i) {
        int itemIndex = i + scrollOffset;
        int y = startY + i * itemHeight;
        
        // 计算文本宽度用于居中
        int textWidth = u8g2.getUTF8Width(settingsItems[itemIndex]);
        int textX = (128 - textWidth) / 2;
        
        if (itemIndex == selectedIndex) {
            // 选中项背景 - 根据文本宽度调整背景框
            int bgX = textX - 4;
            int bgWidth = textWidth + 8;
            u8g2.drawRBox(bgX, y - 9, bgWidth, 11, 2);
            u8g2.setDrawColor(0);
            u8g2.setCursor(textX, y);
            u8g2.print(settingsItems[itemIndex]);
            
            // 添加选中指示器 - 放在文本右侧
            u8g2.setCursor(textX + textWidth + 6, y);
            u8g2.print("◄");
            
            u8g2.setDrawColor(1);
        } else {
            // 未选中项 - 居中显示
            u8g2.setCursor(textX, y);
            u8g2.print(settingsItems[itemIndex]);
        }
    }
    
    // 显示滚动指示器
    if (SETTING_ITEM_COUNT > maxVisibleItems) {
        // 计算滚动条位置
        int scrollBarHeight = 25;
        int scrollBarY = startY - 7;
        int scrollBarX = 125;
        
        // 滚动条背景
        u8g2.drawFrame(scrollBarX, scrollBarY, 2, scrollBarHeight);
        
        // 滚动条滑块
        int sliderHeight = (scrollBarHeight * maxVisibleItems) / SETTING_ITEM_COUNT;
        int sliderY = scrollBarY + (scrollBarHeight - sliderHeight) * selectedIndex / (SETTING_ITEM_COUNT - 1);
        u8g2.drawBox(scrollBarX, sliderY, 2, sliderHeight);
        
        // 显示当前位置信息
        u8g2.setCursor(5, 63);
        u8g2.printf("%d/%d", selectedIndex + 1, SETTING_ITEM_COUNT);
    }
    
    u8g2.sendBuffer();
#else
    Serial.println("系统设置菜单");
#endif
}

void HardwareManager::displaySystemSettingsDetail(SettingsItems item, int value) {
#ifdef FORCE_MASTER_ROLE
    displayClear();
    
    switch (item) {
        case SETTING_SOUND_TOGGLE:
            {
                // 标题居中
                const char* title = "声音设置";
                int titleWidth = u8g2.getUTF8Width(title);
                int titleX = (128 - titleWidth) / 2;
                u8g2.setCursor(titleX, 12);
                u8g2.print(title);
                
                // 当前状态显示
                u8g2.setCursor(5, 30);
                u8g2.printf("当前状态: %s", systemSettings.soundEnabled ? "开启" : "关闭");
                
                // 选项显示
                u8g2.setCursor(5, 45);
                u8g2.printf("切换到: %s", systemSettings.soundEnabled ? "关闭" : "开启");
                
            }
            break;
            
        case SETTING_LED_COLOR:
            displayLedColorSelection(systemSettings.ledColor);
            return;
            
        case SETTING_LED_BRIGHTNESS:
            displayBrightnessAdjustment(systemSettings.ledBrightness);
            return;
            
        case SETTING_DATE_TIME:
            displayDateTimeAdjustment(systemSettings.year, systemSettings.month, 
                                    systemSettings.day, systemSettings.hour, 
                                    systemSettings.minute);
            return;
            
        case SETTING_ALERT_DURATION:
            displayAlertDurationAdjustment(systemSettings.alertDuration);
            return;
            
        default:
            break;
    }
    
    u8g2.sendBuffer();
#else
    Serial.println("设置详情页面");
#endif
}

void HardwareManager::displayLedColorSelection(LedColorOption selectedColor) {
#ifdef FORCE_MASTER_ROLE
    displayClear();
    
    // 标题居中
    const char* title = "LED颜色";
    int titleWidth = u8g2.getUTF8Width(title);
    int titleX = (128 - titleWidth) / 2;
    u8g2.setCursor(titleX, 12);
    u8g2.print(title);
    
    // 颜色选项和名称
    const char* colorNames[] = {"红", "绿", "蓝", "黄", "紫", "青"};
    
    // 横向分布显示颜色选项
    int startX = 5;
    int itemWidth = 20;
    int y = 35;
    
    for (int i = 0; i < LED_COLOR_COUNT; ++i) {
        int x = startX + i * itemWidth;
        
        if (i == selectedColor) {
            // 选中项：绘制方框背景
            u8g2.drawRBox(x - 2, y - 10, itemWidth - 2, 16, 2);
            u8g2.setDrawColor(0); // 反色显示
            u8g2.setCursor(x, y);
            u8g2.printf("%s", colorNames[i]);
            u8g2.setDrawColor(1); // 恢复正常颜色
        } else {
            // 未选中项：正常显示
            u8g2.setCursor(x, y);
            u8g2.printf("%s", colorNames[i]);
        }
    }
    
    // 当前选择显示在底部
    u8g2.setCursor(5, 63);
    u8g2.printf("当前: %s", getLedColorName(selectedColor));
    
    u8g2.sendBuffer();
#else
    Serial.println("LED颜色选择页面");
#endif
}

void HardwareManager::displayBrightnessAdjustment(int brightness) {
#ifdef FORCE_MASTER_ROLE
    displayClear();
    
    // 标题
    u8g2.setCursor(45, 12);
    u8g2.print("LED亮度");
    
    // 亮度条
    u8g2.setCursor(5, 30);
    u8g2.print("亮度:");
    
    u8g2.drawFrame(40, 25, 70, 8);
    int barWidth = (brightness * 68) / 100;
    u8g2.drawBox(41, 26, barWidth, 6);
    
    // 百分比显示
    u8g2.setCursor(115, 30);
    u8g2.printf("%d%%", brightness);
    
    
    u8g2.sendBuffer();
#else
    Serial.printf("LED亮度调节: %d%%\n", brightness);
#endif
}

void HardwareManager::displayDateTimeAdjustment(int year, int month, int day, int hour, int minute) {
#ifdef FORCE_MASTER_ROLE
    displayClear();
    
    // 标题
    u8g2.setCursor(40, 12);
    u8g2.print("日期时间");
    
    // 显示实时时间
    String currentTime = timeManager.formatTime("%Y-%m-%d %H:%M:%S");
    u8g2.setCursor(5, 25);
    u8g2.printf("当前: %s", currentTime.c_str());
    
    // 时区信息
    u8g2.setCursor(5, 40);
    u8g2.printf("时区: UTC+8 (北京时间)");
    
    // NTP同步状态
    u8g2.setCursor(5, 55);
    u8g2.printf("NTP: %s", timeManager.isTimeValid() ? "已同步" : "未同步");
    
    u8g2.sendBuffer();
#else
    Serial.printf("当前时间: %s\n", timeManager.formatTime("%Y-%m-%d %H:%M:%S").c_str());
#endif
}

void HardwareManager::displayAlertDurationAdjustment(int duration) {
#ifdef FORCE_MASTER_ROLE
    displayClear();
    
    // 标题
    u8g2.setCursor(35, 12);
    u8g2.print("达标提醒");
    
    // 时长显示
    u8g2.setCursor(5, 35);
    u8g2.printf("提醒时长: %d 分钟", duration);
    
    // 说明
    u8g2.setCursor(5, 50);
    u8g2.print("连续训练达标时长");
    
    
    u8g2.sendBuffer();
#else
    Serial.printf("达标提醒时长: %d 分钟\n", duration);
#endif
}

void HardwareManager::initializeSettings() {
    // 初始化系统设置为默认值
    systemSettings.soundEnabled = DEFAULT_SOUND_ENABLED;
    systemSettings.ledColor = DEFAULT_LED_COLOR;
    systemSettings.ledBrightness = DEFAULT_LED_BRIGHTNESS;
    
    // 使用时间管理器获取当前时间
    if (timeManager.isTimeValid()) {
        timeManager.updateSystemSettings(&systemSettings);
    } else {
        // 如果时间无效，使用默认值
        systemSettings.year = 2024;
        systemSettings.month = 7;
        systemSettings.day = 18;
        systemSettings.hour = 12;
        systemSettings.minute = 0;
    }
    
    systemSettings.alertDuration = DEFAULT_ALERT_DURATION;
    
    Serial.println("系统设置初始化完成");
    Serial.printf("当前时间: %04d-%02d-%02d %02d:%02d\n", 
                  systemSettings.year, systemSettings.month, systemSettings.day, 
                  systemSettings.hour, systemSettings.minute);
}

void HardwareManager::saveSettings() {
    // 这里可以添加保存到EEPROM或文件系统的代码
    Serial.println("系统设置已保存");
}

void HardwareManager::loadSettings() {
    // 这里可以添加从EEPROM或文件系统加载的代码
    Serial.println("系统设置已加载");
}

SystemSettings* HardwareManager::getSettings() {
    return &systemSettings;
}

void HardwareManager::resetSettings() {
    initializeSettings();
    saveSettings();
    Serial.println("系统设置已重置");
}

uint32_t HardwareManager::getLedColorValue(LedColorOption colorOption) {
    switch (colorOption) {
        case LED_COLOR_RED:    return COLOR_RED;
        case LED_COLOR_GREEN:  return COLOR_GREEN;
        case LED_COLOR_BLUE:   return COLOR_BLUE;
        case LED_COLOR_YELLOW: return COLOR_YELLOW;
        case LED_COLOR_PURPLE: return COLOR_PURPLE;
        case LED_COLOR_CYAN:   return COLOR_CYAN;
        default:               return COLOR_GREEN;
    }
}

const char* HardwareManager::getLedColorName(LedColorOption colorOption) {
    switch (colorOption) {
        case LED_COLOR_RED:    return "红";
        case LED_COLOR_GREEN:  return "绿";
        case LED_COLOR_BLUE:   return "蓝";
        case LED_COLOR_YELLOW: return "黄";
        case LED_COLOR_PURPLE: return "紫";
        case LED_COLOR_CYAN:   return "青";
        default:               return "绿";
    }
}

// 传统按钮接口实现（兼容性保留）
bool HardwareManager::isButtonPressed() {
    // GPIO5高电平触发按钮 - 按下时为HIGH
    return digitalRead(BUTTON_PIN) == HIGH; // 高电平触发
}

bool HardwareManager::isButtonLongPressed() {
    // 这里可以实现长按检测逻辑
    // 为了简化，暂时返回false，因为现在使用ButtonManager来处理
    return false;
}



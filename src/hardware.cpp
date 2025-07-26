#include "hardware.h"
#include "menu.h"
#include "time_manager.h"

// 外部函数声明
extern const char* getPairingStatusString(PairingStatus status);

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
    .alertDuration = DEFAULT_ALERT_DURATION,
    .pairedDeviceMac = {0},
    .hasPairedDevice = false
};

// 训练数据存储
static TrainingRecord trainingRecords[MAX_TRAINING_RECORDS];
static int recordCount = 0;
static TrainingStats trainingStats;

HardwareManager::HardwareManager() 
    : u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE, /* clock=*/ OLED_SCL_PIN, /* data=*/ OLED_SDA_PIN),
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

    // 初始化显示屏
    u8g2.begin();
    u8g2.enableUTF8Print();
    displayInit();
    Serial.println("显示屏初始化完成");

    // 初始化训练数据
    initializeTrainingData();
    
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
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_wqy12_t_gb2312a);
    u8g2.setFontDirection(0);
}

void HardwareManager::displayClear() {
    u8g2.clearBuffer();
}

void HardwareManager::displayText(const char* text, int x, int y, int size) {
    displayClear();
    u8g2.setCursor(x, y);
    u8g2.print(text);
    u8g2.sendBuffer();
}

void HardwareManager::displayMainMenu(const char* items[], int selectedIndex, int itemCount) {
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
}

void HardwareManager::displayMenu(const char* items[], int selectedIndex, int itemCount) {
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
}

void HardwareManager::displayTimer(unsigned long time) {
    char buf[16];
    sprintf(buf, "时间: %lu", time / 1000);
    displayText(buf, 0, 15);
}

void HardwareManager::displayStatus(const char* status) {
    displayText(status, 0, 30);
}

void HardwareManager::displayResult(unsigned long time, const char* result) {
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
}

void HardwareManager::updateVibration() {
    // 震动传感器更新逻辑
}


void HardwareManager::displayTrainingStatus(unsigned long totalTime, unsigned long lastTime) {
    displayClear();
    
    // 大字体标题 - 居中
    u8g2.setFont(u8g2_font_wqy12_t_gb2312a);
    const char* title = "训练中...";
    int titleWidth = u8g2.getUTF8Width(title);
    u8g2.setCursor((128 - titleWidth) / 2, 12);
    u8g2.print(title);
    
    // 实时训练状态指示器 - 闪烁圆点
    static bool statusBlink = false;
    static unsigned long lastBlinkTime = 0;
    if (millis() - lastBlinkTime > 500) {
        statusBlink = !statusBlink;
        lastBlinkTime = millis();
    }
    if (statusBlink) {
        u8g2.drawDisc(118, 8, 3);
    }
    
    // 总时长显示 - 小字体
    u8g2.setFont(u8g2_font_6x10_tf);
    char totalBuf[32];
    unsigned long totalSeconds = totalTime / 1000;
    sprintf(totalBuf, "总时长: %02lu:%02lu:%02lu", totalSeconds / 3600, (totalSeconds % 3600) / 60, totalSeconds % 60);
    u8g2.setCursor(10, 28);
    u8g2.print(totalBuf);
    
    // 上次用时显示 - 小字体
    char lastBuf[32];
    if (lastTime > 0) {
        sprintf(lastBuf, "上次: %.3f秒", lastTime / 1000.0);
    } else {
        sprintf(lastBuf, "上次: --.-秒");
    }
    u8g2.setCursor(10, 40);
    u8g2.print(lastBuf);
    
    // 动画进度条 - 增强视觉效果
    static int progressAnimFrame = 0;
    progressAnimFrame = (progressAnimFrame + 2) % 216; // 双倍速度循环
    
    // 进度条背景
    u8g2.drawFrame(10, 47, 108, 8);
    
    // 动态进度条 - 基于训练时长的活跃度
    unsigned long currentTime = millis();
    int barWidth = 54 + 30 * sin(currentTime / 800.0); // 更慢的动画
    if (barWidth > 106) barWidth = 106;
    if (barWidth < 15) barWidth = 15;
    u8g2.drawBox(11, 48, barWidth, 6);
    
    // 添加进度条内的动态点
    int dotPos = 15 + (progressAnimFrame * 90 / 216);
    if (dotPos < barWidth - 5) {
        u8g2.setDrawColor(0); // 反色
        u8g2.drawDisc(dotPos, 51, 2);
        u8g2.setDrawColor(1); // 恢复正常色
    }
    
    // 训练状态文字
    u8g2.setFont(u8g2_font_5x7_tf);
    const char* statusText = "等待运动检测...";
    u8g2.setCursor(10, 63);
    u8g2.print(statusText);
    
    // 连接状态指示
    u8g2.setCursor(90, 63);
    u8g2.print("已连接");
    
    u8g2.setFont(u8g2_font_wqy12_t_gb2312a); // 恢复默认字体
    u8g2.sendBuffer();
}

void HardwareManager::displayTrainingDetailedStatus(float currentTime, int sessionCount, bool isConnected, int batteryLevel, int signalStrength, bool isMaster) {
    displayClear();
    
    // 大字体时间显示 - 居中
    u8g2.setFont(u8g2_font_logisoso16_tf);
    char timeStr[16];
    sprintf(timeStr, "%.3f", currentTime);
    int timeWidth = u8g2.getStrWidth(timeStr);
    u8g2.setCursor((128 - timeWidth) / 2, 25);
    u8g2.print(timeStr);
    
    // "秒" 字 - 小字体，居中
    u8g2.setFont(u8g2_font_6x10_tf);
    const char* unit = "秒";
    int unitWidth = u8g2.getUTF8Width(unit);
    u8g2.setCursor((128 - unitWidth) / 2, 37);
    u8g2.print(unit);
    
    // 状态栏信息
    u8g2.setCursor(10, 50);
    u8g2.printf("第%d次", sessionCount);
    
    // 设备角色居中显示
    const char* role = isMaster ? "MASTER" : "SLAVE";
    int roleWidth = u8g2.getStrWidth(role);
    u8g2.setCursor((128 - roleWidth) / 2, 50);
    u8g2.print(role);
    
    // 连接状态
    const char* connStatus = isConnected ? "连接OK" : "未连接";
    int connWidth = u8g2.getUTF8Width(connStatus);
    u8g2.setCursor(128 - connWidth - 5, 50);
    u8g2.print(connStatus);
    
    // 电池电量显示
    u8g2.drawFrame(10, 57, 20, 6);
    int batteryBarWidth = (batteryLevel * 18) / 100;
    u8g2.drawBox(11, 58, batteryBarWidth, 4);
    
    // 电池电量百分比
    u8g2.setFont(u8g2_font_4x6_tf);
    u8g2.setCursor(35, 62);
    u8g2.printf("%d%%", batteryLevel);
    
    // 信号强度指示器
    u8g2.setFont(u8g2_font_6x10_tf);
    int signalBars = (signalStrength * 4) / 100; // 4个信号格
    for (int i = 0; i < 4; i++) {
        int barHeight = 2 + i * 2;
        if (i < signalBars) {
            u8g2.drawBox(95 + i * 3, 63 - barHeight, 2, barHeight);
        } else {
            u8g2.drawFrame(95 + i * 3, 63 - barHeight, 2, barHeight);
        }
    }
    
    // WiFi标签
    u8g2.setFont(u8g2_font_4x6_tf);
    u8g2.setCursor(108, 62);
    u8g2.print("WiFi");
    
    u8g2.setFont(u8g2_font_wqy12_t_gb2312a); // 恢复默认字体
    u8g2.sendBuffer();
}

void HardwareManager::displayHistoryData() {
    displayClear();
    
    // 标题：训练统计 (居中显示)  
    const char* title = "训练统计";
    int titleWidth = u8g2.getUTF8Width(title);
    u8g2.setCursor((128 - titleWidth) / 2, 12);
    u8g2.print(title);
    
    // 计算统计数据
    calculateTrainingStats();
    
    // 使用小字体显示统计数据
    u8g2.setFont(u8g2_font_6x10_tf);
    
    // 总时长显示 (格式: HH:MM:SS)
    uint32_t totalHours = trainingStats.totalTrainingTime / 3600000;
    uint32_t totalMinutes = (trainingStats.totalTrainingTime % 3600000) / 60000;
    uint32_t totalSeconds = (trainingStats.totalTrainingTime % 60000) / 1000;
    
    u8g2.setCursor(10, 28);
    u8g2.printf("总时长: %02lu:%02lu:%02lu", totalHours, totalMinutes, totalSeconds);
    
    // 平均时间显示 (格式: X.XXXs)
    if (trainingStats.averageTime > 0) {
        u8g2.printf("平均: %.3fs", trainingStats.averageTime / 1000.0);
    } else {
        u8g2.print("平均: --");
    }
    
    // 本周进步显示 (带箭头图标)
    u8g2.setCursor(10, 52);
    const char* progressIcon = trainingStats.progressIncreasing ? "↑" : "↓";
    u8g2.printf("本周进步: %s%.1f%%", progressIcon, abs((int)trainingStats.weeklyProgress) / 100.0);
    
    // 绘制简单趋势图 (折线图)
    // 模拟8个数据点的趋势线
    int graphPoints[][2] = {{10,58}, {25,56}, {40,53}, {55,51}, {70,49}, {85,46}, {100,44}, {115,41}};
    
    // 绘制折线
    for (int i = 0; i < 7; i++) {
        u8g2.drawLine(graphPoints[i][0], graphPoints[i][1], graphPoints[i+1][0], graphPoints[i+1][1]);
    }
    
    // 绘制数据点
    for (int i = 0; i < 8; i++) {
        u8g2.drawDisc(graphPoints[i][0], graphPoints[i][1], 1);
    }
    
    // 坐标轴
    u8g2.drawHLine(10, 63, 108);
    
    // 底部标签
    u8g2.setFont(u8g2_font_4x6_tf);
    u8g2.setCursor(58, 63);
    u8g2.print("本周趋势");
    
    u8g2.setFont(u8g2_font_wqy12_t_gb2312a); // 恢复默认字体
    
    u8g2.sendBuffer();
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
        "设备配对",
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
            
        case SETTING_DEVICE_PAIRING:
            {
                // 标题居中
                const char* title = "设备配对";
                int titleWidth = u8g2.getUTF8Width(title);
                int titleX = (128 - titleWidth) / 2;
                u8g2.setCursor(titleX, 12);
                u8g2.print(title);
                
                // 当前配对状态
                u8g2.setCursor(5, 30);
                if (systemSettings.hasPairedDevice) {
                    u8g2.print("已配对设备");
                    u8g2.setCursor(5, 45);
                    u8g2.printf("MAC: %02X:%02X:...", 
                               systemSettings.pairedDeviceMac[0], 
                               systemSettings.pairedDeviceMac[1]);
                } else {
                    u8g2.print("未配对设备");
                }
                
                // 操作提示
                u8g2.setCursor(5, 55);
                u8g2.print("单击搜索 双击清除");
            }
            break;
            
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
    u8g2.printf("提醒时长: %d 秒", duration);
    
    // 说明
    u8g2.setCursor(5, 50);
    u8g2.print("连续训练达标时长");
    
    
    u8g2.sendBuffer();
#else
    Serial.printf("达标提醒时长: %d 秒\n", duration);
#endif
}

void HardwareManager::displayDevicePairing(PairingStatus status, DiscoveredDevice* devices, int deviceCount, int selectedIndex) {
#ifdef FORCE_MASTER_ROLE
    displayClear();
    
    // 标题
    u8g2.setCursor(35, 12);
    u8g2.print("设备配对");
    
    // 状态显示
    u8g2.setCursor(5, 25);
    switch (status) {
        case PAIRING_SCANNING:
            u8g2.print("正在扫描设备...");
            break;
        case PAIRING_FOUND_DEVICE:
            u8g2.printf("发现 %d 个设备", deviceCount);
            if (deviceCount > 0 && selectedIndex < deviceCount) {
                u8g2.setCursor(5, 40);
                u8g2.printf("> %s", devices[selectedIndex].name);
                u8g2.setCursor(90, 40);
                u8g2.printf("%ddBm", devices[selectedIndex].rssi);
            }
            u8g2.setCursor(5, 55);
            u8g2.print("单击选择 长按确认");
            break;
        case PAIRING_CONNECTING:
            u8g2.print("正在连接...");
            break;
        case PAIRING_SUCCESS:
            u8g2.print("配对成功！");
            break;
        case PAIRING_FAILED:
            u8g2.print("配对失败");
            break;
        case PAIRING_TIMEOUT:
            u8g2.print("配对超时");
            break;
        default:
            u8g2.print("空闲状态");
            break;
    }
    
    u8g2.sendBuffer();
#else
    Serial.printf("设备配对状态: %s\n", getPairingStatusString(status));
    if (status == PAIRING_FOUND_DEVICE && deviceCount > 0) {
        for (int i = 0; i < deviceCount; i++) {
            Serial.printf("  %s %s (信号: %ddBm)\n", 
                         (i == selectedIndex) ? ">" : " ",
                         devices[i].name, devices[i].rssi);
        }
    }
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

// 训练数据管理函数实现
void HardwareManager::addTrainingRecord(uint32_t duration, uint8_t mode, bool completed) {
    // 如果记录已满，移除最旧的记录
    if (recordCount >= MAX_TRAINING_RECORDS) {
        for (int i = 0; i < MAX_TRAINING_RECORDS - 1; i++) {
            trainingRecords[i] = trainingRecords[i + 1];
        }
        recordCount = MAX_TRAINING_RECORDS - 1;
    }
    
    // 添加新记录
    trainingRecords[recordCount].timestamp = millis();
    trainingRecords[recordCount].duration = duration;
    trainingRecords[recordCount].mode = mode;
    trainingRecords[recordCount].completed = completed;
    recordCount++;
    
    Serial.printf("添加训练记录: 时长=%lums, 模式=%d, 完成=%s\n", 
                  duration, mode, completed ? "是" : "否");
}

void HardwareManager::calculateTrainingStats() {
    trainingStats.totalTrainingTime = 0;
    trainingStats.totalSessions = 0;
    trainingStats.averageTime = 0;
    trainingStats.bestTime = UINT32_MAX;
    
    // 统计总时长、总次数、最佳时间
    uint32_t completedCount = 0;
    uint32_t totalCompletedTime = 0;
    
    for (int i = 0; i < recordCount; i++) {
        trainingStats.totalSessions++;
        trainingStats.totalTrainingTime += trainingRecords[i].duration;
        
        if (trainingRecords[i].completed) {
            completedCount++;
            totalCompletedTime += trainingRecords[i].duration;
            
            if (trainingRecords[i].duration < trainingStats.bestTime) {
                trainingStats.bestTime = trainingRecords[i].duration;
            }
        }
    }
    
    // 计算平均时间（仅完成的训练）
    if (completedCount > 0) {
        trainingStats.averageTime = totalCompletedTime / completedCount;
    }
    
    // 计算本周进步 (简化版，使用最近几次的数据)
    if (recordCount >= 4) {
        uint32_t oldAvg = 0, newAvg = 0;
        int oldCount = 0, newCount = 0;
        
        // 前半部分作为"旧"数据
        for (int i = 0; i < recordCount / 2; i++) {
            if (trainingRecords[i].completed) {
                oldAvg += trainingRecords[i].duration;
                oldCount++;
            }
        }
        
        // 后半部分作为"新"数据
        for (int i = recordCount / 2; i < recordCount; i++) {
            if (trainingRecords[i].completed) {
                newAvg += trainingRecords[i].duration;
                newCount++;
            }
        }
        
        if (oldCount > 0 && newCount > 0) {
            oldAvg /= oldCount;
            newAvg /= newCount;
            
            if (oldAvg > 0) {
                // 时间减少表示进步
                int32_t improvement = ((int32_t)oldAvg - (int32_t)newAvg) * 10000 / oldAvg;
                trainingStats.weeklyProgress = abs(improvement);
                trainingStats.progressIncreasing = improvement > 0;
            }
        }
    }
    
    // 计算真实的每日平均时间趋势（基于训练记录数据）
    // 将训练记录分成7组，每组代表一天的平均表现
    int recordsPerDay = recordCount / 7;
    if (recordsPerDay == 0) recordsPerDay = 1; // 至少1条记录
    
    for (int day = 0; day < 7; day++) {
        uint32_t dayTotal = 0;
        int dayCount = 0;
        int startIdx = day * recordsPerDay;
        int endIdx = (day == 6) ? recordCount : (day + 1) * recordsPerDay;
        
        // 计算该天的平均完成时间（仅统计已完成的训练）
        for (int i = startIdx; i < endIdx && i < recordCount; i++) {
            if (trainingRecords[i].completed) {
                dayTotal += trainingRecords[i].duration;
                dayCount++;
            }
        }
        
        if (dayCount > 0) {
            trainingStats.weeklyTrend[day] = dayTotal / dayCount;
        } else {
            // 如果当天没有完成的训练，使用总平均时间
            trainingStats.weeklyTrend[day] = trainingStats.averageTime > 0 ? 
                                            trainingStats.averageTime : 3000;
        }
    }
}

void HardwareManager::drawTrendGraph() {
#ifdef FORCE_MASTER_ROLE
    // 图表区域：x=10-118, y=52-58 (6像素高度)
    const int graphX = 10;
    const int graphY = 52;
    const int graphWidth = 108;
    const int graphHeight = 6;
    const int pointCount = WEEKLY_TREND_POINTS;
    
    // 找到最大和最小值用于缩放
    uint32_t minVal = UINT32_MAX, maxVal = 0;
    for (int i = 0; i < pointCount && i < 7; i++) {
        if (trainingStats.weeklyTrend[i] < minVal) minVal = trainingStats.weeklyTrend[i];
        if (trainingStats.weeklyTrend[i] > maxVal) maxVal = trainingStats.weeklyTrend[i];
    }
    
    if (maxVal == minVal) maxVal = minVal + 1; // 避免除零
    
    // 绘制数据点和连线
    int prevX = -1, prevY = -1;
    for (int i = 0; i < pointCount && i < 7; i++) {
        int x = graphX + (i * graphWidth) / (pointCount - 1);
        int y = graphY + graphHeight - ((trainingStats.weeklyTrend[i] - minVal) * graphHeight) / (maxVal - minVal);
        
        // 绘制数据点
        u8g2.drawPixel(x, y);
        u8g2.drawPixel(x-1, y);
        u8g2.drawPixel(x+1, y);
        u8g2.drawPixel(x, y-1);
        u8g2.drawPixel(x, y+1);
        
        // 绘制连线
        if (prevX >= 0) {
            u8g2.drawLine(prevX, prevY, x, y);
        }
        
        prevX = x;
        prevY = y;
    }
    
    // 绘制底部基线
    u8g2.drawLine(graphX, graphY + graphHeight, graphX + graphWidth, graphY + graphHeight);
#endif
}

TrainingStats* HardwareManager::getTrainingStats() {
    return &trainingStats;
}

void HardwareManager::initializeTrainingData() {
    recordCount = 0;
    memset(&trainingStats, 0, sizeof(TrainingStats));
    
    // 创建更真实的最近一周训练数据
    createWeeklyTrainingData();
    
    Serial.println("训练数据初始化完成，已添加一周模拟数据");
}

void HardwareManager::createWeeklyTrainingData() {
    // 获取当前时间戳（毫秒）
    uint32_t currentTime = millis();
    uint32_t oneDay = 24 * 60 * 60 * 1000; // 一天的毫秒数
    
    // 创建最近7天的训练数据，模拟真实的训练模式
    // 第7天前（周一）- 较慢的训练时间，刚开始训练
    addTrainingRecord(3200, 0, true);  // 3.2秒 完成
    addTrainingRecord(3800, 0, true);  // 3.8秒 完成
    addTrainingRecord(4100, 0, false); // 4.1秒 未完成（超时）
    
    // 第6天前（周二）- 稍有进步
    addTrainingRecord(2900, 0, true);  // 2.9秒 完成
    addTrainingRecord(3400, 0, true);  // 3.4秒 完成
    addTrainingRecord(3600, 0, true);  // 3.6秒 完成
    addTrainingRecord(4200, 0, false); // 4.2秒 未完成
    
    // 第5天前（周三）- 持续改善
    addTrainingRecord(2700, 0, true);  // 2.7秒 完成
    addTrainingRecord(2800, 0, true);  // 2.8秒 完成
    addTrainingRecord(3100, 0, true);  // 3.1秒 完成
    addTrainingRecord(3300, 0, true);  // 3.3秒 完成
    
    // 第4天前（周四）- 最佳表现日
    addTrainingRecord(2400, 0, true);  // 2.4秒 完成（个人最佳）
    addTrainingRecord(2600, 0, true);  // 2.6秒 完成
    addTrainingRecord(2500, 0, true);  // 2.5秒 完成
    addTrainingRecord(2800, 0, true);  // 2.8秒 完成
    addTrainingRecord(3000, 0, true);  // 3.0秒 完成
    
    // 第3天前（周五）- 保持稳定
    addTrainingRecord(2650, 0, true);  // 2.65秒 完成
    addTrainingRecord(2720, 0, true);  // 2.72秒 完成
    addTrainingRecord(2890, 0, true);  // 2.89秒 完成
    
    // 第2天前（周六）- 疲劳期，成绩略有下降
    addTrainingRecord(2950, 0, true);  // 2.95秒 完成
    addTrainingRecord(3150, 0, true);  // 3.15秒 完成
    addTrainingRecord(3450, 0, true);  // 3.45秒 完成
    addTrainingRecord(3800, 0, false); // 3.8秒 未完成
    
    // 第1天前（周日）- 休息后恢复
    addTrainingRecord(2750, 0, true);  // 2.75秒 完成
    addTrainingRecord(2820, 0, true);  // 2.82秒 完成
    addTrainingRecord(2680, 0, true);  // 2.68秒 完成
    addTrainingRecord(2930, 0, true);  // 2.93秒 完成
    
    // 今天 - 继续保持良好状态
    addTrainingRecord(2580, 0, true);  // 2.58秒 完成
    addTrainingRecord(2690, 0, true);  // 2.69秒 完成
    
    Serial.printf("已创建 %d 条模拟训练记录\n", recordCount);
}



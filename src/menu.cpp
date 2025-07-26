#include "menu.h"

// 外部变量声明
extern DeviceRole deviceRole;

MenuManager menu;

const char* MenuManager::menuItems[] = {
    "开始训练",
    "历史数据",
    "系统设置"
};

const int MenuManager::menuItemCount = MENU_ITEM_COUNT;

MenuManager::MenuManager() 
    : menuActive(true), currentMenuItem(0), currentMode(MODE_SINGLE_TIMER),
      currentMenuState(MENU_STATE_MAIN), currentSettingsItem(0),
      currentSettingsDetail(SETTING_SOUND_TOGGLE), adjustmentValue(0) {}

void MenuManager::init() {
    menuActive = true;
    currentMenuItem = 0;
    currentMenuState = MENU_STATE_MAIN;
    hardware.initializeSettings();
    show();
}

void MenuManager::update() {
    if (menuActive) {
        show();
    }
}

void MenuManager::show() {
    if (!menuActive) return;
    
    switch (currentMenuState) {
        case MENU_STATE_MAIN:
            showMainMenu();
            break;
        case MENU_STATE_SETTINGS:
            showSettingsMenu();
            break;
        case MENU_STATE_SETTINGS_DETAIL:
            showSettingsDetail();
            break;
    }
}

void MenuManager::selectNext() {
    if (!menuActive) return;
    
    switch (currentMenuState) {
        case MENU_STATE_MAIN:
            currentMenuItem = (currentMenuItem + 1) % menuItemCount;
            break;
        case MENU_STATE_SETTINGS:
            currentSettingsItem = (currentSettingsItem + 1) % SETTING_ITEM_COUNT;
            break;
        case MENU_STATE_SETTINGS_DETAIL:
            handleSettingsAdjustment(true);
            return; // 不播放导航音效
    }
    
    show();
    if (hardware.getSettings()->soundEnabled) {
        hardware.beep(1000, 50);
    }
}

void MenuManager::selectPrevious() {
    if (!menuActive) return;
    
    switch (currentMenuState) {
        case MENU_STATE_MAIN:
            currentMenuItem = (currentMenuItem - 1 + menuItemCount) % menuItemCount;
            break;
        case MENU_STATE_SETTINGS:
            currentSettingsItem = (currentSettingsItem - 1 + SETTING_ITEM_COUNT) % SETTING_ITEM_COUNT;
            break;
        case MENU_STATE_SETTINGS_DETAIL:
            handleSettingsAdjustment(false);
            return; // 不播放导航音效
    }
    
    show();
    if (hardware.getSettings()->soundEnabled) {
        hardware.beep(1000, 50);
    }
}

void MenuManager::confirm() {
    if (!menuActive) return;
    
    switch (currentMenuState) {
        case MENU_STATE_MAIN:
            handleMenuSelection();
            break;
        case MENU_STATE_SETTINGS:
            handleSettingsSelection();
            break;
        case MENU_STATE_SETTINGS_DETAIL:
            handleSettingsDetailSelection();
            break;
    }
    
    if (hardware.getSettings()->soundEnabled) {
        hardware.beep(1200, 100);
    }
}

void MenuManager::back() {
    switch (currentMenuState) {
        case MENU_STATE_MAIN:
            if (!menuActive) {
                menuActive = true;
                show();
            }
            break;
        case MENU_STATE_SETTINGS:
            currentMenuState = MENU_STATE_MAIN;
            show();
            break;
        case MENU_STATE_SETTINGS_DETAIL:
            currentMenuState = MENU_STATE_SETTINGS;
            show();
            break;
    }
    
    if (hardware.getSettings()->soundEnabled) {
        hardware.beep(800, 100);
    }
}

void MenuManager::showMainMenu() {
    hardware.displayMainMenu(menuItems, currentMenuItem, menuItemCount);
    
    // LED指示 - 使用设置中的颜色
    hardware.clearLEDs();
    uint32_t selectedColor = hardware.getLedColorValue(hardware.getSettings()->ledColor);
    
    // 为当前选中项设置特殊颜色
    switch (currentMenuItem) {
        case MENU_START_TRAINING:
            hardware.setLED(currentMenuItem, COLOR_GREEN);
            break;
        case MENU_HISTORY_DATA:
            hardware.setLED(currentMenuItem, COLOR_YELLOW);
            break;
        case MENU_SYSTEM_SETTINGS:
            hardware.setLED(currentMenuItem, selectedColor);
            break;
    }
    
    // 其他LED显示暗淡的颜色
    for (int i = 0; i < menuItemCount; i++) {
        if (i != currentMenuItem) {
            hardware.setLED(i, 0x000020); // 暗蓝色
        }
    }
    
    hardware.showLEDs();
}

void MenuManager::showSettingsMenu() {
    hardware.displaySystemSettingsMenu(currentSettingsItem);
    
    // LED指示当前设置项
    hardware.clearLEDs();
    uint32_t selectedColor = hardware.getLedColorValue(hardware.getSettings()->ledColor);
    
    for (int i = 0; i < SETTING_ITEM_COUNT && i < LED_COUNT; i++) {
        if (i == currentSettingsItem) {
            hardware.setLED(i, selectedColor);
        } else {
            hardware.setLED(i, 0x000010);
        }
    }
    
    hardware.showLEDs();
}

void MenuManager::showSettingsDetail() {
    hardware.displaySystemSettingsDetail(currentSettingsDetail, adjustmentValue);
    
    // LED显示调整进度
    hardware.clearLEDs();
    uint32_t selectedColor = hardware.getLedColorValue(hardware.getSettings()->ledColor);
    
    // 根据调整值显示LED进度
    int progressLeds = 0;
    switch (currentSettingsDetail) {
        case SETTING_LED_BRIGHTNESS:
            progressLeds = (hardware.getSettings()->ledBrightness * LED_COUNT) / 100;
            break;
        case SETTING_ALERT_DURATION:
            progressLeds = (hardware.getSettings()->alertDuration * LED_COUNT) / 300; // 假设最大300秒（5分钟）
            break;
        default:
            progressLeds = LED_COUNT / 2; // 默认一半
            break;
    }
    
    for (int i = 0; i < progressLeds && i < LED_COUNT; i++) {
        hardware.setLED(i, selectedColor);
    }
    
    hardware.showLEDs();
}

void MenuManager::handleMenuSelection() {
    switch (currentMenuItem) {
        case MENU_START_TRAINING:
            // 根据设备配置选择合适的训练模式
            if (deviceRole == ROLE_MASTER || deviceRole == ROLE_SLAVE) {
                currentMode = MODE_VIBRATION_TRAINING;  // 主从设备配合的震动训练
                Serial.println("选择了震动训练模式");
            } else {
                currentMode = MODE_SINGLE_TIMER;  // 单设备计时模式
                Serial.println("选择了单设备计时模式");
            }
            menuActive = false;  // 隐藏菜单，但状态切换由主循环控制
            break;
            
        case MENU_HISTORY_DATA:
            currentMode = MODE_VIBRATION_TRAINING;
            Serial.println("选择了历史数据");
            hardware.displayHistoryData();
            delay(3000); // 显示3秒
            show(); // 返回主菜单
            break;
            
        case MENU_SYSTEM_SETTINGS:
            currentMode = MODE_SETTINGS;
            Serial.println("选择了系统设置");
            enterSettingsMenu();
            break;
    }
}

void MenuManager::handleSettingsSelection() {
    currentSettingsDetail = (SettingsItems)currentSettingsItem;
    
    if (currentSettingsDetail == SETTING_BACK) {
        currentMenuState = MENU_STATE_MAIN;
        show();
        return;
    }
    
    currentMenuState = MENU_STATE_SETTINGS_DETAIL;
    
    // 初始化调整值
    switch (currentSettingsDetail) {
        case SETTING_LED_BRIGHTNESS:
            adjustmentValue = hardware.getSettings()->ledBrightness;
            break;
        case SETTING_ALERT_DURATION:
            adjustmentValue = hardware.getSettings()->alertDuration;
            break;
        case SETTING_DEVICE_PAIRING:
            adjustmentValue = 0;
            break;
        default:
            adjustmentValue = 0;
            break;
    }
    
    show();
}

void MenuManager::handleSettingsDetailSelection() {
    // 应用设置更改
    applySettingsChange();
    
    // 返回设置菜单
    currentMenuState = MENU_STATE_SETTINGS;
    show();
}

void MenuManager::handleSettingsAdjustment(bool increase) {
    SystemSettings* settings = hardware.getSettings();
    
    Serial.printf("设置调整: %s\n", increase ? "增加" : "减少");
    
    switch (currentSettingsDetail) {
        case SETTING_SOUND_TOGGLE:
            settings->soundEnabled = !settings->soundEnabled;
            Serial.printf("声音开关: %s\n", settings->soundEnabled ? "开启" : "关闭");
            break;
            
        case SETTING_LED_COLOR:
            if (increase) {
                settings->ledColor = (LedColorOption)((settings->ledColor + 1) % LED_COLOR_COUNT);
            } else {
                settings->ledColor = (LedColorOption)((settings->ledColor - 1 + LED_COLOR_COUNT) % LED_COLOR_COUNT);
            }
            Serial.printf("LED颜色: %d\n", settings->ledColor);
            break;
            
        case SETTING_LED_BRIGHTNESS:
            if (increase && settings->ledBrightness < 100) {
                settings->ledBrightness += 5;
            } else if (!increase && settings->ledBrightness > 0) {
                settings->ledBrightness -= 5;
            }
            adjustmentValue = settings->ledBrightness;
            Serial.printf("LED亮度: %d\n", settings->ledBrightness);
            break;
            
        case SETTING_ALERT_DURATION:
            if (increase && settings->alertDuration < 300) {
                settings->alertDuration += 30;
            } else if (!increase && settings->alertDuration > 30) {
                settings->alertDuration -= 30;
            }
            adjustmentValue = settings->alertDuration;
            Serial.printf("提醒时长: %d 秒\n", settings->alertDuration);
            break;
            
        case SETTING_DEVICE_PAIRING:
            if (increase) {
                // 开始设备配对
                extern void startDevicePairing();
                startDevicePairing();
                Serial.println("开始设备配对...");
            } else {
                // 清除配对设备
                extern void clearPairedDevice();
                clearPairedDevice();
                Serial.println("清除配对设备...");
            }
            break;
            
        default:
            break;
    }
    
    show();
    updateSystemSettings();
}

void MenuManager::applySettingsChange() {
    hardware.saveSettings();
    updateSystemSettings();
    Serial.println("设置已应用");
}

void MenuManager::updateSystemSettings() {
    SystemSettings* settings = hardware.getSettings();
    
    // 更新LED亮度
    int ledBrightness = (settings->ledBrightness * 255) / 100;
    FastLED.setBrightness(ledBrightness);
    
    // 立即显示LED更改
    show();
}

void MenuManager::enterSettingsMenu() {
    currentMenuState = MENU_STATE_SETTINGS;
    currentSettingsItem = 0;
    show();
}

void MenuManager::exitSettingsMenu() {
    currentMenuState = MENU_STATE_MAIN;
    show();
}

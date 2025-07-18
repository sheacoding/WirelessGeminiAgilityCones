#ifndef MENU_H
#define MENU_H

#include "config.h"
#include "hardware.h"

// 菜单状态枚举
enum MenuState {
    MENU_STATE_MAIN,
    MENU_STATE_SETTINGS,
    MENU_STATE_SETTINGS_DETAIL
};

class MenuManager {
public:
    MenuManager();
    void init();
    void update();
    void show();
    void selectNext();
    void selectPrevious();
    void confirm();
    void back();
    
    TrainingMode getCurrentMode() const { return currentMode; }
    int getCurrentMenuItem() const { return currentMenuItem; }
    bool isMenuActive() const { return menuActive; }
    
    // 系统设置相关
    void enterSettingsMenu();
    void exitSettingsMenu();
    void handleSettingsNavigation();
    void handleSettingsAdjustment(bool increase);
    
private:
    bool menuActive;
    int currentMenuItem;
    TrainingMode currentMode;
    
    // 菜单状态管理
    MenuState currentMenuState;
    int currentSettingsItem;
    SettingsItems currentSettingsDetail;
    int adjustmentValue;
    
    static const char* menuItems[];
    static const int menuItemCount;
    
    void showMainMenu();
    void showSettingsMenu();
    void showSettingsDetail();
    void handleMenuSelection();
    void handleSettingsSelection();
    void handleSettingsDetailSelection();
    void applySettingsChange();
    void updateSystemSettings();
};

extern MenuManager menu;

#endif // MENU_H

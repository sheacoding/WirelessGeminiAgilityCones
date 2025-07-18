#include "system_state_manager.h"
#include "hardware.h"
#include "menu.h"
#include "vibration_training.h"

// 全局系统状态管理器实例
SystemStateManager stateManager;

SystemStateManager::SystemStateManager() 
    : currentState(STATE_INIT),
      previousState(STATE_INIT),
      stateChangeTime(0),
      stateChanged(false) {
}

bool SystemStateManager::init() {
    Serial.println("初始化系统状态管理器...");
    
    currentState = STATE_INIT;
    previousState = STATE_INIT;
    stateChangeTime = millis();
    stateChanged = false;
    
    Serial.println("系统状态管理器初始化完成");
    return true;
}

void SystemStateManager::update() {
    // 检查是否有状态变化需要处理
    if (stateChanged) {
        stateChanged = false;
        onStateChange(previousState, currentState);
    }
    
    // TODO: 重新实现按键事件处理逻辑
    /*
    ButtonEvent event = buttonManager.getEvent();
    if (event != BUTTON_NONE) {
        handleButtonEvent(event);
        buttonManager.clearEvent();
    }
    */
}

SystemState SystemStateManager::getCurrentState() {
    return currentState;
}

void SystemStateManager::setState(SystemState newState) {
    if (newState != currentState) {
        transitionTo(newState);
    }
}

bool SystemStateManager::canTransitionTo(SystemState newState) {
    return isValidTransition(currentState, newState);
}

void SystemStateManager::transitionTo(SystemState newState) {
    if (!canTransitionTo(newState)) {
        Serial.printf("无效的状态转换: %s -> %s\n", 
                     getStateString(currentState), 
                     getStateString(newState));
        return;
    }
    
    SystemState oldState = currentState;
    
    // 退出当前状态
    onExitState(currentState);
    
    // 更新状态
    previousState = currentState;
    currentState = newState;
    stateChangeTime = millis();
    stateChanged = true;
    
    // 进入新状态
    onEnterState(currentState);
    
    // 记录状态变化
    logStateChange(oldState, newState);
}

/*
void SystemStateManager::handleButtonEvent(ButtonEvent event) {
    // 暂时禁用按键事件处理
    // TODO: 重新实现按键事件处理逻辑
}
*/

const char* SystemStateManager::getStateString() {
    return getStateString(currentState);
}

const char* SystemStateManager::getStateString(SystemState state) {
    switch (state) {
        case STATE_INIT:     return "INIT";
        case STATE_MENU:     return "MENU";
        case STATE_READY:    return "READY";
        case STATE_TIMING:   return "TIMING";
        case STATE_COMPLETE: return "COMPLETE";
        case STATE_ERROR:    return "ERROR";
        default:             return "UNKNOWN";
    }
}

void SystemStateManager::onStateChange(SystemState oldState, SystemState newState) {
    Serial.printf("状态变化: %s -> %s\n", getStateString(oldState), getStateString(newState));
    
    // 这里可以添加全局状态变化处理逻辑
    // 例如：更新显示、播放声音、控制LED等
}

// 菜单状态下的按键处理
void SystemStateManager::handleClickInMenu() {
    extern MenuManager menu;
    if (menu.isMenuActive()) {
        menu.selectNext();
    }
}

void SystemStateManager::handleDoubleClickInMenu() {
    extern MenuManager menu;
    if (menu.isMenuActive()) {
        menu.selectPrevious();
    }
}

void SystemStateManager::handleLongPressInMenu() {
    extern MenuManager menu;
    if (menu.isMenuActive()) {
        menu.confirm();
        
        // 检查是否选择了开始训练
        if (!menu.isMenuActive()) {
            transitionTo(STATE_READY);
        }
    }
}

// 准备状态下的按键处理
void SystemStateManager::handleClickInReady() {
    // 开始训练
    extern VibrationTrainingManager vibrationTraining;
    vibrationTraining.start();
    transitionTo(STATE_TIMING);
}

void SystemStateManager::handleLongPressInReady() {
    // 返回菜单
    extern MenuManager menu;
    menu.init();
    transitionTo(STATE_MENU);
}

// 完成状态下的按键处理
void SystemStateManager::handleClickInComplete() {
    // 返回菜单
    extern MenuManager menu;
    menu.init();
    transitionTo(STATE_MENU);
}

void SystemStateManager::handleLongPressInComplete() {
    // 返回菜单
    extern MenuManager menu;
    menu.init();
    transitionTo(STATE_MENU);
}

// 计时状态下的按键处理
void SystemStateManager::handleLongPressInTiming() {
    // 停止训练并返回菜单
    extern VibrationTrainingManager vibrationTraining;
    vibrationTraining.stop();
    
    extern MenuManager menu;
    menu.init();
    transitionTo(STATE_MENU);
}

// 状态转换验证
bool SystemStateManager::isValidTransition(SystemState from, SystemState to) {
    // 定义允许的状态转换
    switch (from) {
        case STATE_INIT:
            return (to == STATE_MENU || to == STATE_ERROR);
            
        case STATE_MENU:
            return (to == STATE_READY || to == STATE_ERROR);
            
        case STATE_READY:
            return (to == STATE_TIMING || to == STATE_MENU || to == STATE_ERROR);
            
        case STATE_TIMING:
            return (to == STATE_COMPLETE || to == STATE_MENU || to == STATE_ERROR);
            
        case STATE_COMPLETE:
            return (to == STATE_MENU || to == STATE_READY || to == STATE_ERROR);
            
        case STATE_ERROR:
            return (to == STATE_MENU || to == STATE_INIT);
            
        default:
            return false;
    }
}

// 状态进入处理
void SystemStateManager::onEnterState(SystemState state) {
    extern HardwareManager hardware;
    
    switch (state) {
        case STATE_INIT:
            Serial.println("进入初始化状态");
            hardware.clearLEDs();
            hardware.showLEDs();
            break;
            
        case STATE_MENU:
            Serial.println("进入菜单状态");
            hardware.clearLEDs();
            hardware.showLEDs();
            break;
            
        case STATE_READY:
            Serial.println("进入准备状态");
            hardware.displayStatus("准备就绪！按键开始训练");
            hardware.setAllLEDs(COLOR_GREEN);
            hardware.showLEDs();
            break;
            
        case STATE_TIMING:
            Serial.println("进入计时状态");
            hardware.displayStatus("训练中...");
            hardware.setAllLEDs(COLOR_BLUE);
            hardware.showLEDs();
            if (hardware.getSettings()->soundEnabled) {
                hardware.playStartSound();
            }
            break;
            
        case STATE_COMPLETE:
            Serial.println("进入完成状态");
            hardware.displayStatus("训练完成！按键继续");
            hardware.setAllLEDs(COLOR_YELLOW);
            hardware.showLEDs();
            if (hardware.getSettings()->soundEnabled) {
                hardware.playCompleteSound();
            }
            break;
            
        case STATE_ERROR:
            Serial.println("进入错误状态");
            hardware.displayStatus("系统错误");
            hardware.setAllLEDs(COLOR_RED);
            hardware.showLEDs();
            if (hardware.getSettings()->soundEnabled) {
                hardware.playErrorSound();
            }
            break;
    }
}

// 状态退出处理
void SystemStateManager::onExitState(SystemState state) {
    switch (state) {
        case STATE_TIMING:
            Serial.println("退出计时状态");
            // 停止计时相关的处理
            break;
            
        default:
            break;
    }
}

// 记录状态变化
void SystemStateManager::logStateChange(SystemState oldState, SystemState newState) {
    Serial.printf("[状态管理器] %s -> %s (时间: %lu)\n", 
                  getStateString(oldState), 
                  getStateString(newState), 
                  millis());
}

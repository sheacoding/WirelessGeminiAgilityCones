#ifndef SYSTEM_STATE_MANAGER_H
#define SYSTEM_STATE_MANAGER_H

#include <Arduino.h>
#include "config.h"
#include "ButtonManager.h"

// 系统状态管理器类
class SystemStateManager {
public:
    SystemStateManager();
    
    // 初始化状态管理器
    bool init();
    
    // 更新状态管理器
    void update();
    
    // 获取当前状态
    SystemState getCurrentState();
    
    // 设置状态
    void setState(SystemState newState);
    
    // 状态转换
    bool canTransitionTo(SystemState newState);
    void transitionTo(SystemState newState);
    
    // 处理按键事件
    // void handleButtonEvent(ButtonEvent event); // 暂时禁用
    
    // 获取状态字符串
    const char* getStateString();
    const char* getStateString(SystemState state);
    
    // 状态监听
    void onStateChange(SystemState oldState, SystemState newState);
    
private:
    SystemState currentState;
    SystemState previousState;
    unsigned long stateChangeTime;
    bool stateChanged;
    
    // 按键事件处理
    void handleClickInMenu();
    void handleDoubleClickInMenu();
    void handleLongPressInMenu();
    
    void handleClickInReady();
    void handleLongPressInReady();
    
    void handleClickInComplete();
    void handleLongPressInComplete();
    
    void handleLongPressInTiming();
    
    // 状态转换验证
    bool isValidTransition(SystemState from, SystemState to);
    
    // 状态进入/退出处理
    void onEnterState(SystemState state);
    void onExitState(SystemState state);
    
    // 调试和日志
    void logStateChange(SystemState oldState, SystemState newState);
};

extern SystemStateManager stateManager;

#endif // SYSTEM_STATE_MANAGER_H

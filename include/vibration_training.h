#ifndef VIBRATION_TRAINING_H
#define VIBRATION_TRAINING_H

#include "config.h"
#include "hardware.h"

// 震动训练状态
enum VibrationTrainingState {
    VT_STATE_IDLE,           // 空闲状态
    VT_STATE_WAITING,        // 等待主机震动
    VT_STATE_TIMING,         // 单次计时中
    VT_STATE_COMPLETED       // 单次完成
};

class VibrationTrainingManager {
public:
    VibrationTrainingManager();
    void init();
    void update();
    void start();
    void stop();
    void reset();
    void exitTraining();  // 退出训练并显示当天运动情况
    
    // 主机逻辑
    void handleMasterVibration();  // 主机检测到震动，结束单次计时
    void handleSlaveComplete(unsigned long roundTime);  // 收到从机开始信号
    
    // 从机逻辑  
    void handleSlaveVibration();   // 从机检测到震动，发送开始信号
    void handleRoundComplete(unsigned long roundTime); // 从机收到主机完成信号
    
    bool isRunning() const { return running; }
    bool isCompleted() const { return completed; }
    unsigned long getElapsedTime() const { return elapsedTime; }
    unsigned long getTotalTrainingTime() const { return totalTrainingTime; }
    int getSessionCount() const { return sessionCount; }
    
private:
    bool running;                    // 训练是否运行中
    bool completed;                  // 整个训练是否完成
    VibrationTrainingState state;    // 当前震动训练状态
    
    // 单次计时相关
    unsigned long singleStartTime;   // 单次开始时间
    unsigned long singleElapsedTime; // 单次用时
    
    // 总体统计
    unsigned long trainingStartTime; // 训练开始时间
    unsigned long totalTrainingTime; // 总运动时长
    unsigned long elapsedTime;       // 当前训练总时长
    int sessionCount;                // 完成次数
    unsigned long lastSessionTime;   // 上次单次用时
    
    // 提醒相关
    unsigned long lastAlertTime;     // 上次提醒时间
    unsigned long alertInterval;     // 提醒间隔
    
    void updateTimer();
    void checkTimeout();                 // 检查超时
    void checkAlerts();                  // 检查达标提醒
    void showReadyCountdown();
    void updateVisualFeedback();
    void sendStartMessage();             // 从机发送开始信号给主机
    void sendCompleteMessage();          // 主机发送完成信号给从机
    void displayDailyStats();            // 显示当天运动情况
};

extern VibrationTrainingManager vibrationTraining;

#endif // VIBRATION_TRAINING_H

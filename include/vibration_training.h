#ifndef VIBRATION_TRAINING_H
#define VIBRATION_TRAINING_H

#include "config.h"
#include "hardware.h"

class VibrationTrainingManager {
public:
    VibrationTrainingManager();
    void init();
    void update();
    void start();
    void stop();
    void reset();
    
    bool isRunning() const { return running; }
    bool isCompleted() const { return completed; }
    unsigned long getElapsedTime() const { return elapsedTime; }
    
private:
    bool running;
    bool completed;
    bool waitingForVibration;
    unsigned long startTime;
    unsigned long elapsedTime;
    unsigned long lastAlertTime;
    
    void handleVibrationDetected();
    void updateTimer();
    void checkTimeout();
    void showReadyCountdown();
    void updateVisualFeedback();
    void playTimerAlerts();
};

extern VibrationTrainingManager vibrationTraining;

#endif // VIBRATION_TRAINING_H

#include "vibration_training.h"

VibrationTrainingManager vibrationTraining;

VibrationTrainingManager::VibrationTrainingManager() 
    : running(false), completed(false), waitingForVibration(false),
      startTime(0), elapsedTime(0), lastAlertTime(0) {}

void VibrationTrainingManager::init() {
    reset();
    showReadyCountdown();
}

void VibrationTrainingManager::update() {
    if (running) {
        updateTimer();
        playTimerAlerts();
        hardware.update();
        if (hardware.isVibrationDetected() && waitingForVibration) {
            handleVibrationDetected();
        }
        checkTimeout();
    }
}

void VibrationTrainingManager::start() {
    running = true;
    completed = false;
    waitingForVibration = true;
    startTime = millis();
    hardware.playStartSound();
    hardware.displayStatus("Training Started");
}

void VibrationTrainingManager::stop() {
    running = false;
    hardware.displayStatus("Training Stopped");
    hardware.playCompleteSound();
}

void VibrationTrainingManager::reset() {
    running = false;
    completed = false;
    waitingForVibration = false;
    elapsedTime = 0;
    hardware.displayClear();
}

void VibrationTrainingManager::handleVibrationDetected() {
    if (running) {
        waitingForVibration = false;
        elapsedTime = millis() - startTime;
        completed = true;
        stop();
        hardware.displayResult(elapsedTime, "Completed");
        hardware.ledAlertEffect();
    }
}

void VibrationTrainingManager::updateTimer() {
    if (running) {
        elapsedTime = millis() - startTime;
        hardware.displayTimer(elapsedTime);
        updateVisualFeedback();
    }
}

void VibrationTrainingManager::checkTimeout() {
    if (running && elapsedTime >= TIMING_TIMEOUT_MS) {
        stop();
        completed = false;
        hardware.displayStatus("Timeout");
        hardware.playErrorSound();
    }
}

void VibrationTrainingManager::showReadyCountdown() {
    hardware.displayStatus("Get Ready");
    for (int i = TIMING_READY_DELAY_MS / 1000; i > 0; --i) {
        char countStr[4];
        sprintf(countStr, "%d", i);
        hardware.displayText(countStr, 60, 40, 2);
        delay(1000);
    }
}

void VibrationTrainingManager::updateVisualFeedback() {
    int progress = (elapsedTime * 100) / TIMING_TIMEOUT_MS;
    hardware.ledProgressBar(progress, COLOR_GREEN);
}

void VibrationTrainingManager::playTimerAlerts() {
    if (elapsedTime - lastAlertTime >= TIMING_ALERT_INTERVAL) {
        lastAlertTime = elapsedTime;
        hardware.playAlertSound();
    }
}

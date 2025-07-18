#include "ButtonManager.h"
#include "config.h"

ButtonManager::ButtonManager(uint8_t buttonPin) 
    : pin(buttonPin), button(), enabled(true),
      debounceTicks(BUTTON_DEBOUNCE_MS), clickTicks(BUTTON_CLICK_MS), pressTicks(BUTTON_LONG_PRESS_MS) {}

void ButtonManager::init() {
    // GPIO5高电平触发按钮 - 按下时为HIGH，未按下时为LOW
    // 使用内部下拉电阻，确保未按下时为LOW
    button.setup(pin, INPUT_PULLDOWN, false); // false表示activeHigh（高电平有效）
    
    // 设置各种时间参数
    button.setDebounceMs(debounceTicks);
    button.setClickMs(clickTicks);
    button.setPressMs(pressTicks);
    
    // OneButton库默认支持双击，只要设置了doubleClick回调函数即可
    Serial.printf("  双击检测已默认启用\n");
    Serial.printf("  双击时间间隔: %d ms\n", BUTTON_DOUBLE_CLICK_MS);
    
    Serial.printf("按钮管理器初始化完成（高电平触发）\n");
    Serial.printf("  引脚: GPIO%d\n", pin);
    Serial.printf("  防抖时间: %lu ms\n", debounceTicks);
    Serial.printf("  点击间隔: %lu ms\n", clickTicks);
    Serial.printf("  长按时间: %lu ms\n", pressTicks);
    Serial.printf("  按钮类型: 高电平触发\n");
}

void ButtonManager::tick() {
    if (enabled) {
        button.tick();
    }
}

void ButtonManager::enable() {
    enabled = true;
    Serial.println("按钮管理器已启用");
}

void ButtonManager::disable() {
    enabled = false;
    Serial.println("按钮管理器已禁用");
}

bool ButtonManager::isEnabled() const {
    return enabled;
}

void ButtonManager::setDebounceTicks(unsigned long ticks) {
    debounceTicks = ticks;
    button.setDebounceMs(ticks);
}

void ButtonManager::setClickTicks(unsigned long ticks) {
    clickTicks = ticks;
    button.setClickMs(ticks);
}

void ButtonManager::setPressTicks(unsigned long ticks) {
    pressTicks = ticks;
    button.setPressMs(ticks);
}

void ButtonManager::attachSingleClick(void (*callback)()) {
    button.attachClick(callback);
}

void ButtonManager::attachDoubleClick(void (*callback)()) {
    button.attachDoubleClick(callback);
    Serial.println("双击回调函数已注册");
}

void ButtonManager::attachLongPress(void (*callback)()) {
    button.attachLongPressStart(callback);
}

void ButtonManager::attachMultiClick(void (*callback)(int)) {
    // OneButton库中的attachMultiClick不支持参数
    // 这里可以留空或者实现其他逻辑
    Serial.println("警告：MultiClick不支持参数");
}

void ButtonManager::attachDuringLongPress(void (*callback)()) {
    button.attachDuringLongPress(callback);
}

void ButtonManager::attachLongPressStop(void (*callback)()) {
    button.attachLongPressStop(callback);
}

bool ButtonManager::isPressed() const {
    // GPIO5高电平触发按钮 - 按下时为HIGH
    return digitalRead(pin) == HIGH; // 高电平触发
}

bool ButtonManager::isLongPressed() const {
    return button.isLongPressed();
}

unsigned long ButtonManager::getDebounceTicks() const {
    return debounceTicks;
}

unsigned long ButtonManager::getClickTicks() const {
    return clickTicks;
}

unsigned long ButtonManager::getPressTicks() const {
    return pressTicks;
}

void ButtonManager::reset() {
    button.reset();
    Serial.println("按钮状态已重置");
}

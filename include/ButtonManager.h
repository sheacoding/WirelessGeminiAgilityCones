#ifndef BUTTON_MANAGER_H
#define BUTTON_MANAGER_H

#include <Arduino.h>
#include <OneButton.h>

class ButtonManager {
public:
    ButtonManager(uint8_t buttonPin);
    void init();
    void tick();
    void enable();
    void disable();
    bool isEnabled() const;
    void setDebounceTicks(unsigned long ticks);
    void setClickTicks(unsigned long ticks);
    void setPressTicks(unsigned long ticks);
    void attachSingleClick(void (*callback)());
    void attachDoubleClick(void (*callback)());
    void attachLongPress(void (*callback)());
    void attachMultiClick(void (*callback)(int));
    void attachDuringLongPress(void (*callback)());
    void attachLongPressStop(void (*callback)());
    bool isPressed() const;
    bool isLongPressed() const;
    unsigned long getDebounceTicks() const;
    unsigned long getClickTicks() const;
    unsigned long getPressTicks() const;
    void reset();

private:
    uint8_t pin;
    OneButton button;
    bool enabled;
    unsigned long debounceTicks;
    unsigned long clickTicks;
    unsigned long pressTicks;
};

#endif // BUTTON_MANAGER_H


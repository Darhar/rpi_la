#pragma once

#include <gpiod.h>

class GPIO {
public:
    GPIO(int pin);
    ~GPIO();

    bool init();
    void set(bool value);

private:
    int pin;

    gpiod_chip* chip;
    gpiod_line* line;
};
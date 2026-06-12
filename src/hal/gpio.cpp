#include "gpio.h"

#include <cstdio>

GPIO::GPIO(int pin)
    : pin(pin),
      chip(nullptr),
      line(nullptr) {
}

GPIO::~GPIO() {

    if (line)
        gpiod_line_release(line);

    if (chip)
        gpiod_chip_close(chip);
}

bool GPIO::init() {

    chip = gpiod_chip_open_by_name("gpiochip0");

    if (!chip) {
        perror("gpiod_chip_open");
        return false;
    }

    line = gpiod_chip_get_line(chip, pin);

    if (!line) {
        perror("gpiod_chip_get_line");
        return false;
    }

    if (gpiod_line_request_output(
            line,
            "rpi-terminal",
            0) < 0) {

        perror("gpiod_line_request_output");
        return false;
    }

    return true;
}

void GPIO::set(bool value) {

    gpiod_line_set_value(line, value ? 1 : 0);
}
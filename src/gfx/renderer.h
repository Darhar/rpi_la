// renderer.h

#pragma once

#include <stdint.h>

#include "../drivers/ili9488.h"
#include "../terminal/screenbuffer.h"
#include "palette.h"

static constexpr int LCD_BYTES = WIDTH * HEIGHT / 2;


class Renderer {
public:

    Renderer(
        ILI9488& display,
        ScreenBuffer& screen);

    void render();

    uint8_t screenB[LCD_BYTES];

private:

    ILI9488& lcd;
    ScreenBuffer& buf;

};
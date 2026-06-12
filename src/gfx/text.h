#pragma once

#include <cstdint>

constexpr int FONT_W = 8;
constexpr int FONT_H = 16;

void drawChar(
    uint8_t* fb,
    int fbWidth,
    int fbHeight,
    int x,
    int y,
    char ch,
    uint8_t fg,
    uint8_t bg,
    bool transparentBg = false
);

void drawText(
    uint8_t* fb,
    int fbWidth,
    int fbHeight,
    int x,
    int y,
    const char* text,
    uint8_t fg,
    uint8_t bg,
    bool transparentBg = false
);
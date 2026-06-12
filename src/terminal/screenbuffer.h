#pragma once

#include <cstdint>
#include <cstring>
#include "rect.h"
#include "../gfx/font8x16.h"

using Pixel = uint8_t;

constexpr Pixel PIXEL_COLOUR_MASK = 0x07;
constexpr Pixel PIXEL_TRANSPARENT = 0x80;
constexpr Pixel PIXEL_DIRTY       = 0x10;

class ScreenBuffer
{
public:
    static constexpr int WIDTH  = 480;
    static constexpr int HEIGHT = 320;
    static constexpr int SIZE   = WIDTH * HEIGHT;
    static constexpr int MAX_DIRTY_RECTS = 32;

    ScreenBuffer();

    void clear(uint8_t colour);
    void setPixel(int x, int y, uint8_t colour);
    Pixel getPixel(int x, int y) const;
    uint8_t physicalColourFor(uint8_t colour, int x, int y) const;
    void setPixelRaw(int x, int y, uint8_t rgb111);
    void fillRect(int x, int y, int w, int h, uint8_t colour);
    void drawBox(int x, int y, int w, int h, uint8_t colour);
    void lineH(int x0, int x1, int y, uint8_t colour);
    void lineV(int x, int y0, int y1, uint8_t colour);

    void markDirtyRect(int x, int y, int w, int h);
    int dirtyCount() const;
    const Rect& dirtyRect(int index) const;
    void clearDirty();
    void drawChar(int x, int y, char c, Pixel fg);
    void drawChar(int x, int y, char c, Pixel fg, Pixel bg);

    void drawText(int x, int y, const char* text, Pixel fg);
    void drawText(int x, int y, const char* text, Pixel fg, Pixel bg);
private:
    Pixel m_pixels[SIZE];
    Rect m_dirty[MAX_DIRTY_RECTS];
    int m_dirtyCount = 0;
};

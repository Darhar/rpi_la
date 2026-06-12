// renderer.cpp

#include "renderer.h"
#include <cstdio>
#include <utility>


Renderer::Renderer(ILI9488& display,ScreenBuffer& screen)
    :lcd(display),buf(screen)
{}

static inline uint8_t colourOf(Pixel p)
{
    return p & PIXEL_COLOUR_MASK;//00000111
}

static inline uint8_t packPixels(Pixel left, Pixel right)
{
    return ((left  & 0x07) << 3) | (right & 0x07);
}

void Renderer::render()
{
    int count = buf.dirtyCount();

    for(int i = 0; i < count; i++)
    {
        Rect r = buf.dirtyRect(i);

        int x0 = r.x & ~1;
        int x1 = r.x + r.w;
        uint8_t* dst = screenB;

        if(x1 & 1) x1++;

        if(x1 > ScreenBuffer::WIDTH) x1 = ScreenBuffer::WIDTH;

        for(int y = r.y; y < r.y + r.h; y++)
        {
            for(int x = x0; x < x1; x += 2)
            {
                Pixel left  = buf.getPixel(x, y);
                Pixel right = buf.getPixel(x + 1, y);
                *dst++ = packPixels(left, right);
            }
        }

        lcd.setWindow(x0, r.y, x1 - 1, r.y + r.h - 1);
        lcd.writePixels(screenB, dst - screenB);
    }

    buf.clearDirty();
}
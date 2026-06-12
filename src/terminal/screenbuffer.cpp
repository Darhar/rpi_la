#include "screenbuffer.h"
#include "../gfx/palette.h"
#include <cstdio>
ScreenBuffer::ScreenBuffer()
{
    clear(0);
}

void ScreenBuffer::clear(uint8_t colour)
{
    Pixel p = colour & PIXEL_COLOUR_MASK;

    for(int i = 0; i < SIZE; i++)
        m_pixels[i] = p;

    m_dirtyCount = 0;
    markDirtyRect(0, 0, WIDTH, HEIGHT);
}
void ScreenBuffer::setPixel(int x, int y, uint8_t colour)
{
    if(x < 0 || x >= WIDTH || y < 0 || y >= HEIGHT)
        return;

    m_pixels[y * WIDTH + x] =
        physicalColourFor(colour, x, y);

    markDirtyRect(x, y, 1, 1);
}

uint8_t ScreenBuffer::physicalColourFor(uint8_t colour, int x, int y) const
{
    return ::physicalColourFor(colour, x, y);
}

void ScreenBuffer::setPixelRaw(int x, int y, uint8_t rgb111)
{
    if(x < 0 || x >= WIDTH || y < 0 || y >= HEIGHT)
        return;

    m_pixels[y * WIDTH + x] = rgb111 & 0x07;
    markDirtyRect(x, y, 1, 1);
}

Pixel ScreenBuffer::getPixel(int x, int y) const
{
    if(x < 0 || x >= WIDTH || y < 0 || y >= HEIGHT)
        return 0;

    return m_pixels[y * WIDTH + x];
}

void ScreenBuffer::fillRect(int x, int y, int w, int h, uint8_t colour)
{
    if(w <= 0 || h <= 0)
        return;

    int x0 = x;
    int y0 = y;
    int x1 = x + w;
    int y1 = y + h;

    if(x0 < 0)      x0 = 0;
    if(y0 < 0)      y0 = 0;
    if(x1 > WIDTH)  x1 = WIDTH;
    if(y1 > HEIGHT) y1 = HEIGHT;

    if(x0 >= x1 || y0 >= y1)
        return;

    for(int yy = y0; yy < y1; yy++)
    {
        Pixel* row = &m_pixels[yy * WIDTH + x0];

        for(int xx = x0; xx < x1; xx++)
            *row++ = physicalColourFor(colour, xx, yy);
    }

    markDirtyRect(x0, y0, x1 - x0, y1 - y0);
}

void ScreenBuffer::drawBox(int x, int y, int w, int h, uint8_t colour)
{
    fillRect(x, y, w, h, colour);
}

void ScreenBuffer::lineH(int x0, int x1, int y, uint8_t colour)
{
    if(y < 0 || y >= HEIGHT)
        return;

    if(x0 < 0) x0 = 0;
    if(x1 >= WIDTH) x1 = WIDTH - 1;
    if(x0 > x1) return;

    for(int x = x0; x <= x1; ++x)
        setPixel(x, y, colour);
}

void ScreenBuffer::lineV(int x, int y0, int y1, uint8_t colour)
{
    if(x < 0 || x >= WIDTH)
        return;

    if(y0 < 0) y0 = 0;
    if(y1 >= HEIGHT) y1 = HEIGHT - 1;
    if(y0 > y1) return;

    for(int y = y0; y <= y1; ++y)
        setPixel(x, y, colour);
}


void ScreenBuffer::drawChar(int x, int y, char c, Pixel fg)
{
    if((uint8_t)c >= 128) return;

    fg &= PIXEL_COLOUR_MASK;

    const uint8_t* glyph = font8x16[(uint8_t)c];

    for(int row = 0; row < 16; row++)
    {
        uint8_t bits = glyph[row];

        for(int col = 0; col < 8; col++)
        {
            if(bits & (0x80 >> col))
            {
                setPixel(x + col, y + row, fg);
            }
        }
    }
}
void ScreenBuffer::drawChar(int x, int y, char c, Pixel fg, Pixel bg)
{
    if((uint8_t)c >= 128) return;

    fg &= PIXEL_COLOUR_MASK;
    bg &= PIXEL_COLOUR_MASK;

    const uint8_t* glyph = font8x16[(uint8_t)c];

    for(int row = 0; row < 16; row++)
    {
        uint8_t bits = glyph[row];

        for(int col = 0; col < 8; col++)
        {
            Pixel colour =
                (bits & (0x80 >> col)) ? fg : bg;

            setPixel(x + col, y + row, colour);
        }
    }
}
void ScreenBuffer::drawText(int x, int y, const char* text, Pixel fg)
{
    int startX = x;

    while(*text)
    {
        if(*text == '\n')
        {
            x = startX;
            y += 16;
        }
        else
        {
            drawChar(x, y, *text, fg);
            x += 8;
        }

        text++;
    }
}
void ScreenBuffer::drawText(int x, int y, const char* text, Pixel fg, Pixel bg)
{
    int startX = x;

    while(*text)
    {
        if(*text == '\n')
        {
            x = startX;
            y += 16;
        }
        else
        {
            drawChar(x, y, *text, fg, bg);
            x += 8;
        }

        text++;
    }
}
void ScreenBuffer::markDirtyRect(int x, int y, int w, int h)
{
    if(w <= 0 || h <= 0)
        return;

    int x0 = x;
    int y0 = y;
    int x1 = x + w;
    int y1 = y + h;

    if(x0 < 0)      x0 = 0;
    if(y0 < 0)      y0 = 0;
    if(x1 > WIDTH)  x1 = WIDTH;
    if(y1 > HEIGHT) y1 = HEIGHT;

    if(x0 >= x1 || y0 >= y1)
        return;

    Rect r = { x0, y0, x1 - x0, y1 - y0 };

    for(int i = 0; i < m_dirtyCount; i++)
    {
        if(rectsOverlapOrTouch(m_dirty[i], r))
        {
            m_dirty[i] = mergeRects(m_dirty[i], r);
            return;
        }
    }

    if(m_dirtyCount < MAX_DIRTY_RECTS)
    {
        m_dirty[m_dirtyCount++] = r;
        return;
    }

    Rect merged = r;

    for(int i = 0; i < m_dirtyCount; i++)
        merged = mergeRects(merged, m_dirty[i]);

    m_dirty[0] = merged;
    m_dirtyCount = 1;
}

int ScreenBuffer::dirtyCount() const
{
    return m_dirtyCount;
}

const Rect& ScreenBuffer::dirtyRect(int index) const
{
    return m_dirty[index];
}

void ScreenBuffer::clearDirty()
{
    m_dirtyCount = 0;
}

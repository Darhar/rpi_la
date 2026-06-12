#include "text.h"
#include "font8x16.h"

static inline void putPixel4(
    uint8_t* fb,
    int fbWidth,
    int fbHeight,
    int x,
    int y,
    uint8_t colour
)
{
    if(x < 0 || y < 0 || x >= fbWidth || y >= fbHeight)
        return;

    colour &= 0x0F;

    int pixelIndex = y * fbWidth + x;
    int byteIndex  = pixelIndex >> 1;

    if((pixelIndex & 1) == 0)
    {
        // even pixel: high nibble
        fb[byteIndex] &= 0x0F;
        fb[byteIndex] |= colour << 4;
    }
    else
    {
        // odd pixel: low nibble
        fb[byteIndex] &= 0xF0;
        fb[byteIndex] |= colour;
    }
}

void drawChar(
    uint8_t* fb,
    int fbWidth,
    int fbHeight,
    int x,
    int y,
    char ch,
    uint8_t fg,
    uint8_t bg,
    bool transparentBg
)
{
    const uint8_t* glyph = font8x16[(uint8_t)ch & 0x7F];

    for(int row = 0; row < FONT_H; row++)
    {
        uint8_t bits = glyph[row];

        for(int col = 0; col < FONT_W; col++)
        {
            bool pixelOn = bits & (0x80 >> col);

            if(pixelOn)
            {
                putPixel4(fb, fbWidth, fbHeight, x + col, y + row, fg);
            }
            else if(!transparentBg)
            {
                putPixel4(fb, fbWidth, fbHeight, x + col, y + row, bg);
            }
        }
    }
}

void drawText(
    uint8_t* fb,
    int fbWidth,
    int fbHeight,
    int x,
    int y,
    const char* text,
    uint8_t fg,
    uint8_t bg,
    bool transparentBg
)
{
    int cx = x;
    int cy = y;

    while(*text)
    {
        char ch = *text++;

        if(ch == '\n')
        {
            cx = x;
            cy += FONT_H;
            continue;
        }

        if(ch == '\r')
        {
            cx = x;
            continue;
        }

        drawChar(
            fb,
            fbWidth,
            fbHeight,
            cx,
            cy,
            ch,
            fg,
            bg,
            transparentBg
        );

        cx += FONT_W;
    }
}
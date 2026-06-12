#include "palette.h"

static const ColourPair logicalPalette[32] =
{
    {PHYS_BLACK,  PHYS_BLACK},  // 00000 black
    {PHYS_BLUE,   PHYS_BLUE},   // 00001 blue
    {PHYS_GREEN,  PHYS_GREEN},  // 00010 green
    {PHYS_CYAN,   PHYS_CYAN},   // 00011 cyan
    {PHYS_RED,    PHYS_RED},    // 00100 red
    {PHYS_PURPLE, PHYS_PURPLE}, // 00101 purple
    {PHYS_YELLOW, PHYS_YELLOW}, // 00110 yellow
    {PHYS_WHITE,  PHYS_WHITE},  // 00111 white

    {PHYS_BLACK,  PHYS_WHITE},  // 01000 black + white
    {PHYS_BLUE,   PHYS_WHITE},  // 01001 light blue
    {PHYS_GREEN,  PHYS_WHITE},  // 01010 light green
    {PHYS_CYAN,   PHYS_WHITE},  // 01011 light cyan
    {PHYS_RED,    PHYS_WHITE},  // 01100 light red
    {PHYS_PURPLE, PHYS_WHITE},  // 01101 light purple
    {PHYS_YELLOW, PHYS_WHITE},  // 01110 light yellow
    {PHYS_BLACK,  PHYS_BLACK},  // 01111 spare

    {PHYS_CYAN,   PHYS_GREEN},  // 10000 cyan + green
    {PHYS_BLUE,   PHYS_BLACK},  // 10001 dark blue
    {PHYS_GREEN,  PHYS_BLACK},  // 10010 dark green
    {PHYS_CYAN,   PHYS_BLACK},  // 10011 dark cyan
    {PHYS_RED,    PHYS_BLACK},  // 10100 dark red
    {PHYS_PURPLE, PHYS_BLACK},  // 10101 dark purple
    {PHYS_YELLOW, PHYS_BLACK},  // 10110 dark yellow
    {PHYS_PURPLE, PHYS_BLUE},   // 10111 purple + blue

    {PHYS_YELLOW, PHYS_RED},    // 11000 yellow + red
    {PHYS_BLUE,   PHYS_YELLOW}, // 11001 blue + yellow
    {PHYS_GREEN,  PHYS_PURPLE}, // 11010 green + purple
    {PHYS_CYAN,   PHYS_BLUE},   // 11011 cyan + blue
    {PHYS_RED,    PHYS_CYAN},   // 11100 red + cyan
    {PHYS_PURPLE, PHYS_RED},    // 11101 purple + red
    {PHYS_YELLOW, PHYS_GREEN},  // 11110 yellow + green
    {PHYS_WHITE,  PHYS_BLACK},  // 11111 spare / inverse grey
};

const ColourPair& palettePair(uint8_t colour)
{
    return logicalPalette[colour & 0x1F];
}

uint8_t physicalColourFor(uint8_t colour, int x, int y)
{
    colour &= 0x1F;

    const ColourPair& p = logicalPalette[colour];

    if(colour < 8)
        return p.a & 0x07;

    if(((x ^ y) & 1) == 0)
        return p.a & 0x07;

    return p.b & 0x07;
}
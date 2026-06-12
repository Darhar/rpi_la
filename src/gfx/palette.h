#pragma once

#include <cstdint>


enum Colour : uint8_t
{
    COL_BLACK   = 0b00000, // 0
    COL_BLUE    = 0b00001, // 1
    COL_GREEN   = 0b00010, // 2
    COL_CYAN    = 0b00011, // 3
    COL_RED     = 0b00100, // 4
    COL_PURPLE  = 0b00101, // 5
    COL_YELLOW  = 0b00110, // 6
    COL_WHITE   = 0b00111, // 7

    COL_GREY_BW      = 0b01000, // black + white
    COL_LIGHT_BLUE   = 0b01001,
    COL_LIGHT_GREEN  = 0b01010,
    COL_LIGHT_CYAN   = 0b01011,
    COL_LIGHT_RED    = 0b01100,
    COL_LIGHT_PURPLE = 0b01101,
    COL_LIGHT_YELLOW = 0b01110,
    COL_EXTRA_15     = 0b01111,

    COL_CYAN_GREEN   = 0b10000,
    COL_DARK_BLUE    = 0b10001,
    COL_DARK_GREEN   = 0b10010,
    COL_DARK_CYAN    = 0b10011,
    COL_DARK_RED     = 0b10100,
    COL_DARK_PURPLE  = 0b10101,
    COL_DARK_YELLOW  = 0b10110,
    COL_PURPLE_BLUE  = 0b10111,

    COL_YELLOW_RED   = 0b11000,
    COL_BLUE_YELLOW  = 0b11001,
    COL_GREEN_PURPLE = 0b11010,
    COL_CYAN_BLUE    = 0b11011,
    COL_RED_CYAN     = 0b11100,
    COL_PURPLE_RED   = 0b11101,
    COL_YELLOW_GREEN = 0b11110,
    COL_EXTRA_31     = 0b11111
};

struct ColourPair
{
    uint8_t a;
    uint8_t b;
};

constexpr uint8_t PHYS_BLACK  = 0b000;
constexpr uint8_t PHYS_BLUE   = 0b001;
constexpr uint8_t PHYS_GREEN  = 0b010;
constexpr uint8_t PHYS_CYAN   = 0b011;
constexpr uint8_t PHYS_RED    = 0b100;
constexpr uint8_t PHYS_PURPLE = 0b101;
constexpr uint8_t PHYS_YELLOW = 0b110;
constexpr uint8_t PHYS_WHITE  = 0b111;

const ColourPair& palettePair(uint8_t colour);
uint8_t physicalColourFor(uint8_t colour, int x, int y);
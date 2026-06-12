#pragma once

struct Rect
{
    int x;
    int y;
    int w;
    int h;
};

static inline bool rectsOverlapOrTouch(const Rect& a, const Rect& b)
{
    return !(a.x + a.w < b.x ||
             b.x + b.w < a.x ||
             a.y + a.h < b.y ||
             b.y + b.h < a.y);
}

static inline Rect mergeRects(const Rect& a, const Rect& b)
{
    int x0 = (a.x < b.x) ? a.x : b.x;
    int y0 = (a.y < b.y) ? a.y : b.y;

    int ax1 = a.x + a.w;
    int ay1 = a.y + a.h;
    int bx1 = b.x + b.w;
    int by1 = b.y + b.h;

    int x1 = (ax1 > bx1) ? ax1 : bx1;
    int y1 = (ay1 > by1) ? ay1 : by1;

    return { x0, y0, x1 - x0, y1 - y0 };
}
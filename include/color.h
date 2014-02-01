#ifndef COLOR_H_INCLUDED
#define COLOR_H_INCLUDED

#include <cstdint>
#include "util.h"

using namespace std;

struct Color final
{
    float b, g, r, a; /// a is opacity -- 0 is transparent and 1 is opaque
    Color(float v, float a = 1)
    {
        r = g = b = v;
        this->a = a;
    }
    Color()
    {
        r = g = b = a = 0;
    }
    Color(float r, float g, float b, float a = 1)
    {
        this->r = r;
        this->g = g;
        this->b = b;
        this->a = a;
    }
    uint8_t ri() const
    {
        return ifloor(limit(r * 256.0f, 0.0f, 255.0f));
    }
    uint8_t gi() const
    {
        return ifloor(limit(g * 256.0f, 0.0f, 255.0f));
    }
    uint8_t bi() const
    {
        return ifloor(limit(b * 256.0f, 0.0f, 255.0f));
    }
    uint8_t ai() const
    {
        return ifloor(limit(a * 256.0f, 0.0f, 255.0f));
    }
    void ri(uint8_t v)
    {
        r = (unsigned)v * (1.0f / 255.0f);
    }
    void gi(uint8_t v)
    {
        g = (unsigned)v * (1.0f / 255.0f);
    }
    void bi(uint8_t v)
    {
        b = (unsigned)v * (1.0f / 255.0f);
    }
    void ai(uint8_t v)
    {
        a = (unsigned)v * (1.0f / 255.0f);
    }
};

inline const Color interpolate(const float t, const Color a, const Color b)
{
    return Color(interpolate(t, a.r, b.r), interpolate(t, a.g, b.g), interpolate(t, a.b, b.b), interpolate(t, a.a, b.a));
}

#endif // COLOR_H_INCLUDED

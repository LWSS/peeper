#pragma once

struct Color
{
#ifdef __cplusplus
    Color (){}
    Color( unsigned char _r, unsigned char _g, unsigned char _b, unsigned char _a )
    {
        r = _r;
        g = _g;
        b = _b;
        a = _a;
    }
#endif
    unsigned char r;
    unsigned char g;
    unsigned char b;
    unsigned char a;
};
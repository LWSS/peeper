#pragma once

struct Color
{
    Color (){}
    Color( unsigned char _r, unsigned char _g, unsigned char _b, unsigned char _a )
    {
        r = _r;
        g = _g;
        b = _b;
        a = _a;
    }
    unsigned char r;
    unsigned char g;
    unsigned char b;
    unsigned char a;
};


namespace Peeper
{
    // Non-zero on fail
    int Open();
    void Close();

    // Call this after you've updated all the primitives
    // Peeper Primitives are cached, so it will draw the same shapes until you give it new data.
    void SubmitDraws();
    // Call this only when you stop drawing and want to clear the screen
    void StopDraws();

    // Primitive shapes that are drawn. Must be called after Open()
    bool AddLine( int x0, int y0, int x1, int y1, Color color, float thickness );
    bool AddCircle( int x0, int y0, Color color, float circleRadius, int circleSegments, float thickness );
    bool AddText( int x0, int y0, Color color, const char *text );
}
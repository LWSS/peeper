#pragma once

#include "../src/Types.h"

namespace Peeper
{
    // Non-zero on fail
    int Open();
    void Close();

    // Call this after you've updated all the primitives
    // Peeper Primitives are cached, so it will draw the same shapes until you give it new data.
    void SubmitDraws();
    // Call this only when you stop drawing and want to clear the screen
    void ClearDraws();

    // Primitive shapes that are drawn. Must be called after Open()
    bool AddLine( int x0, int y0, int x1, int y1, Color color, float thickness );
    bool AddRect( int x0, int y0, int x1, int y1, Color color, float thickness );
    bool AddRectFilled( int x0, int y0, int x1, int y1, Color color );
    bool AddCircle( int x0, int y0, Color color, float circleRadius, int circleSegments, float thickness );
    bool AddCircleFilled( int x0, int y0, Color color, float circleRadius, int circleSegments );
    bool AddText( int x0, int y0, Color color, const char *text );
}
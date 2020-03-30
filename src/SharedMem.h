#pragma once

#include <cstdint>
#include <semaphore.h>

#define SHM_NAME "peeper"
#define SEMAPHORE_NAME "peeper_semaphore"

enum DrawType
{
    DRAW_LINE,
    DRAW_RECT,
    DRAW_RECT_FILLED,
    DRAW_CIRCLE,
    DRAW_CIRCLE_FILLED,
    DRAW_TEXT
};

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
struct DrawRequest
{
    DrawRequest(){}
    DrawType type;
    int x0, y0, x1, y1;
    int circleSegments;
    float circleRadius;
    float thickness;
    Color color;
    char text[256];
};

#define MAX_REQUESTS 512

struct Settings
{
    bool bTest;
};

struct SharedRegion
{
    uint8_t version;
    struct Settings settings;
    uint16_t numRequests;
    struct DrawRequest requests[MAX_REQUESTS];
};

/**** Each cpp file that includes this file gets these. ****/
static struct SharedRegion *sharedRegion;

static int sharedMemoryFD; // /dev/shm file descriptor
static sem_t *semaphore;
/***********************************************************/
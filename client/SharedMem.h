#pragma once
#include <cstdint>
#include <semaphore.h>

/** Internal Peeper Header, do not include **/

#define SHM_NAME "peeper"
#define SEMAPHORE_NAME "peeper_semaphore"
#define MAX_REQUESTS 512
#define MAX_TEXT_LEN 256

enum DrawType
{
    DRAW_LINE,
    DRAW_RECT,
    DRAW_RECT_FILLED,
    DRAW_CIRCLE,
    DRAW_CIRCLE_FILLED,
    DRAW_TEXT
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
    char text[MAX_TEXT_LEN];
};

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
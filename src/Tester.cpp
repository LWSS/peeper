#include "SharedMem.h"

#include <sys/mman.h>
#include <sys/stat.h>        /* For mode constants */
#include <sys/types.h>
#include <fcntl.h>           /* For O_* constants */
#include <cstring> //memcpy
#include <cstdlib>
#include <unistd.h> //ftruncate
#include <cstdio>
#include <cstdarg>

int currentDrawIndex = 0;

bool AddDrawLine( int x0, int y0, int x1, int y1, Color color, float thickness )
{
    if( currentDrawIndex >= (MAX_REQUESTS-1) )
        return false;

    sharedRegion->requests[currentDrawIndex].type = DRAW_LINE;
    sharedRegion->requests[currentDrawIndex].x0 = x0;
    sharedRegion->requests[currentDrawIndex].y0 = y0;
    sharedRegion->requests[currentDrawIndex].x1 = x1;
    sharedRegion->requests[currentDrawIndex].y1 = y1;
    sharedRegion->requests[currentDrawIndex].color = color;
    sharedRegion->requests[currentDrawIndex].thickness = thickness;

    currentDrawIndex++;
    return true;
}

bool AddDrawCircle( int x0, int y0, Color color, float circleRadius, int circleSegments, float thickness )
{
    if( currentDrawIndex >= (MAX_REQUESTS-1) )
        return false;

    sharedRegion->requests[currentDrawIndex].type = DRAW_CIRCLE;
    sharedRegion->requests[currentDrawIndex].x0 = x0;
    sharedRegion->requests[currentDrawIndex].y0 = y0;
    sharedRegion->requests[currentDrawIndex].color = color;
    sharedRegion->requests[currentDrawIndex].circleRadius = circleRadius;
    sharedRegion->requests[currentDrawIndex].circleSegments = circleSegments;
    sharedRegion->requests[currentDrawIndex].thickness = thickness;

    currentDrawIndex++;
    return true;
}

/* This only needs to be called if you are done drawing to empty the screen.
 * For changes, just call SubmitDraw()
 */
void StopDraws()
{
    if( sharedRegion->numRequests != 0 ){
        sharedRegion->numRequests = 0;
        sem_post( semaphore );
    }
}

void SubmitDraws()
{
    if( currentDrawIndex <= 0 ){
        StopDraws();
        return;
    }

    sharedRegion->numRequests = currentDrawIndex;

    // release semaphore to peeper so it can copy drawRequests
    sem_post( semaphore );

    // wait until peeper has copied them out.
    //sem_wait( semaphore );

    currentDrawIndex = 0;
}

int main()
{
    // Open /dev/shm/ shared memory
    sharedMemoryFD = shm_open( SHM_NAME, O_RDWR, 0);
    if( sharedMemoryFD < 0 ){
        abort();
    }

    // Set the size.
    ftruncate( sharedMemoryFD, sizeof(SharedRegion) );

    // Allocate the memory with the mmap and the fd
    sharedRegion = (SharedRegion*)mmap( NULL, sizeof(struct SharedRegion), PROT_READ | PROT_WRITE, MAP_SHARED, sharedMemoryFD, 0 );

    if( sharedRegion == MAP_FAILED ){
        abort();
    }

    // Open/Create a semaphore by it's unique name
    semaphore = sem_open( SEMAPHORE_NAME, O_RDWR, S_IRWXU, 0);
    if( semaphore == (void*)-1 ){
        abort();
    }

    for( int i = 0; i < 500; i++ ){
        //AddDrawLine( 0, 0, (1920 / 100) * i, (1080 / 100) * i, Color(255, 0, 0, 255), 1.0f );
        AddDrawCircle( (1920/100) * i, 500, Color(255, 0, 0, 255), 45.0f, 10, 1.0f );
    }

    SubmitDraws();

    sleep(3);

    StopDraws();

    for( int i = 0; i < 500; i++ ){
        //AddDrawLine( 0, 0, (1920 / 100) * i, (1080 / 100) * i, Color(255, 0, 0, 255), 1.0f );
        AddDrawCircle( (1920/100) * i, 500, Color(255, 0, 0, 255), 20.0f, 10, 1.0f );
    }

    SubmitDraws();

    sleep(3);

    StopDraws();

    return 0;
}
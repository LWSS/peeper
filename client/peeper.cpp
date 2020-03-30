#include "peeper.h"

#include "SharedMem.h"

#include <sys/mman.h>
#include <sys/stat.h>        /* For mode constants */
#include <sys/types.h>
#include <fcntl.h>           /* For O_* constants */
#include <cstring> //memcpy
#include <unistd.h> //ftruncate

static int currentDrawIndex = 0;

bool Peeper::AddLine( int x0, int y0, int x1, int y1, Color color, float thickness ) {
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

bool Peeper::AddCircle( int x0, int y0, Color color, float circleRadius, int circleSegments, float thickness ) {
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

bool Peeper::AddText(int x0, int y0, Color color, const char *text) {
    if( currentDrawIndex >= (MAX_REQUESTS-1) )
        return false;

    sharedRegion->requests[currentDrawIndex].type = DRAW_TEXT;
    sharedRegion->requests[currentDrawIndex].x0 = x0;
    sharedRegion->requests[currentDrawIndex].y0 = y0;
    sharedRegion->requests[currentDrawIndex].color = color;
    strncpy( sharedRegion->requests[currentDrawIndex].text, text, MAX_TEXT_LEN-1 );
    sharedRegion->requests[currentDrawIndex].text[MAX_TEXT_LEN-1] = '\0';

    currentDrawIndex++;
    return true;
}

void Peeper::SubmitDraws( ) {
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

void Peeper::StopDraws( ) {
    if( sharedRegion->numRequests != 0 ){
        sharedRegion->numRequests = 0;
        sem_post( semaphore );
    }
}

int Peeper::Open( ) {
    // Open /dev/shm/ shared memory
    sharedMemoryFD = shm_open( SHM_NAME, O_RDWR, 0);
    if( sharedMemoryFD < 0 ){
        return 1;
    }

    // Set the size.
    ftruncate( sharedMemoryFD, sizeof(SharedRegion) );

    // Allocate the memory with the mmap and the fd
    sharedRegion = (SharedRegion*)mmap( NULL, sizeof(struct SharedRegion), PROT_READ | PROT_WRITE, MAP_SHARED, sharedMemoryFD, 0 );

    if( sharedRegion == MAP_FAILED ){
        return 2;
    }

    // Open/Create a semaphore by it's unique name
    semaphore = sem_open( SEMAPHORE_NAME, O_RDWR, S_IRWXU, 0);
    if( semaphore == (void*)-1 ){
        return 3;
    }

    return 0;
}


void Peeper::Close( ) {
    munmap( sharedRegion, sizeof(struct SharedRegion) );
    sem_close( semaphore );
    close( sharedMemoryFD );
}

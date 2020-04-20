// peeper example code
#include <cstdio>
#include <unistd.h>

#include "../client/peeper.h"

int main()
{
    // open and init peeper
    int open = Peeper::Open();
    if( open ){
        printf("Peeper open failed! Code: %d\n", open);
        return 1;
    }
    for( int i = 0; i < 50; i++ ){
        Peeper::AddLine( 0, 0, (1920 / 50) * i, 1080, Color(255, 0, 0, 255), 1.0f );
    }
    bool filled = false;
    for( int i = 0; i < 50; i++ ){
        if( filled ){
            Peeper::AddCircleFilled( (1920/50) * i, 500, Color(255, 0, 0, 255), 20.0f, 10 );
        } else {
            Peeper::AddCircle( (1920/50) * i, 500, Color(255, 0, 0, 255), 20.0f, 10, 1.0f );
        }
        filled = !filled;
    }

    for( int i = 0; i < 50; i++ ){
        if( filled ){
            Peeper::AddRectFilled( (1920/50) * i, 250, ((1920/50) * i) + 30, 270, Color( 0, 255, 0, 255 )  );
        } else {
            Peeper::AddRect( (1920/50) * i, 250, ((1920/50) * i) + 30, 270, Color( 0, 255, 0, 255 ), 1.0f );
        }
        filled = !filled;
    }

    Peeper::AddText(900, 100, Color(255, 165, 0, 255), "This is the Peeper test screen. Enjoy the shapes! This will close in several seconds. Hope you have 1080p lol.\n");

    Peeper::SubmitDraws();
    sleep(8);


    // Draw requests are cached in peeper in case of low data update-rate. (ex: your game only updates at 20hz, it will draw the old data until new data is there)
    // You need to clear the screen before stopping otherwise it will be left there.
    Peeper::ClearDraws();

    // dont forget to cleanup the objects.
    Peeper::Close();
    return 0;
}
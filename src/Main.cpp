#define _GNU_SOURCE

#include <cstdint>
#include <dlfcn.h>
#include <SDL2/SDL.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/stat.h>        /* For mode constants */
#include <sys/types.h>
#include <fcntl.h>           /* For O_* constants */
#include <unistd.h>
#include <thread>
#include <mutex>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengles3.h"

#include <examples/libs/gl3w/GL/gl3w.h>

#include <EGL/egl.h>

#include "SharedMem.h"

typedef EGLBoolean (*swapBuffersFn)( EGLDisplay, EGLSurface );
swapBuffersFn origEGLswapBuffers;

typedef int (* eventFilterFn)( void *, SDL_Event * );
eventFilterFn origEventFilter;

bool menuOpen = false;

EGLContext context;

std::thread sharedMemoryReader;
std::mutex drawMutex;

int numRequests = 0;
DrawRequest drawRequests[MAX_REQUESTS];

void Log(char const * const format, ...)
{
    char buffer[4096];
    static bool bFirst = true;
    FILE *logFile;

    if ( bFirst ) {
        logFile = fopen("/tmp/peeper.log", "w"); // create new log
        fprintf(logFile, "--Start of log--\n");
        bFirst = false;
    } else {
        logFile = fopen("/tmp/peeper.log", "a"); // append to log
    }
    setbuf( logFile, nullptr ); // Turns off buffered I/O; decreases performance, but no unflushed buffer if crash occurs.
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, 4096, format, args);
    fprintf(logFile, buffer);
    va_end(args);
    fclose(logFile);
}

// Read shared memory here instead of the swapbuffers hook.
// We do this here instead of swapbuffers because sem_wait() stalls until a change is detected.
void ReadSharedMemory()
{
    while( sharedRegion ){
        // Wait for semaphore to be changed, this means there is new shared memory.
        // This also locks the semaphore
        if( !sem_wait( semaphore ) ){
            {
                // Scoped mutex-lock, we do this to add drawRequests.
                std::unique_lock<std::mutex> lock( drawMutex );

                numRequests = sharedRegion->numRequests;

                memcpy( drawRequests, sharedRegion->requests, ( sizeof( DrawRequest ) * MAX_REQUESTS ) );
            }

            //int sem;
            //sem_getvalue( semaphore, &sem );
            //Log("sem(%d)\n", sem);
        }
    }
}

// Draw from drawRequests[]. Called from swapbuffers
void DrawRequests()
{
    /*
    static uint64_t callCount = 0;
    static std::time_t t = 0;
    std::time_t currentTime = std::time(0);
    callCount++;
    if( (currentTime - t) >= 1 ){
        Log("Draw Calls in last second: %lld\n", callCount);
        callCount = 0;
        t = currentTime;
    }*/

    std::unique_lock<std::mutex> lock( drawMutex );

    // No new data.
    if( numRequests <= 0 )
        return;

    for( int i = 0; i < numRequests; i++ ){
        const DrawRequest &request = drawRequests[i];
        switch( request.type ){
            case DRAW_LINE:
                ImGui::GetWindowDrawList()->AddLine( ImVec2(request.x0, request.y0), ImVec2(request.x1, request.y1),
                                                     ImColor( request.color.r, request.color.g, request.color.b, request.color.a ), request.thickness );
                break;
            case DRAW_RECT:
                ImGui::GetWindowDrawList()->AddRect( ImVec2(request.x0, request.y0), ImVec2(request.x1, request.y1),
                                                     ImColor( request.color.r, request.color.g, request.color.b, request.color.a ) );
                break;
            case DRAW_RECT_FILLED:
                ImGui::GetWindowDrawList()->AddRectFilled( ImVec2(request.x0, request.y0), ImVec2(request.x1, request.y1),
                                                           ImColor( request.color.r, request.color.g, request.color.b, request.color.a ) );
                break;
            case DRAW_CIRCLE:
                ImGui::GetWindowDrawList()->AddCircle( ImVec2(request.x0, request.y0), request.circleRadius,
                                                       ImColor( request.color.r, request.color.g, request.color.b, request.color.a ), request.circleSegments, request.thickness );

                break;
            case DRAW_CIRCLE_FILLED:
                ImGui::GetWindowDrawList()->AddCircleFilled( ImVec2(request.x0, request.y0), request.circleRadius,
                                                             ImColor( request.color.r, request.color.g, request.color.b, request.color.a ), request.circleSegments );
                break;
            case DRAW_TEXT:
                ImGui::GetWindowDrawList()->AddText( ImVec2(request.x0, request.y0), ImColor( request.color.r, request.color.g, request.color.b, request.color.a ),request.text);
                break;
        }
    }
}

extern "C" {
    int EventFilter(void * userdata, SDL_Event * event)
    {
        ImGuiIO& io = ImGui::GetIO();
        switch( event->type ){
            case SDL_MOUSEWHEEL:
                if( !io.WantCaptureMouse )
                    goto orig;

                if( event->wheel.y > 0 )
                    io.MouseWheel = 1;
                if( event->wheel.y < 0 )
                    io.MouseWheel = -1;

                return 1;

            case SDL_MOUSEBUTTONDOWN:
                if( !io.WantCaptureMouse )
                    goto orig;

                if (event->button.button == SDL_BUTTON_LEFT) io.MouseDown[0] = true;
                if (event->button.button == SDL_BUTTON_RIGHT) io.MouseDown[1] = true;
                if (event->button.button == SDL_BUTTON_MIDDLE) io.MouseDown[2] = true;

                return 1;

            case SDL_MOUSEBUTTONUP:
                if( !io.WantCaptureMouse )
                    goto orig;

                if (event->button.button == SDL_BUTTON_LEFT) io.MouseDown[0] = false;
                if (event->button.button == SDL_BUTTON_RIGHT) io.MouseDown[1] = false;
                if (event->button.button == SDL_BUTTON_MIDDLE) io.MouseDown[2] = false;

                return 1;

            case SDL_TEXTINPUT:
                if( !io.WantCaptureKeyboard )
                    goto orig;

                io.AddInputCharactersUTF8( event->text.text );

                return 1;

            case SDL_KEYDOWN:
            case SDL_KEYUP:
                if ( event->key.keysym.sym == SDLK_INSERT && event->type == SDL_KEYDOWN ){
                    menuOpen = !menuOpen;
                    Log("Setting menuopen = %s\n", menuOpen ? "open" : "false" );
                }
                if( !io.WantCaptureKeyboard )
                    goto orig;

                int key = event->key.keysym.sym & ~SDLK_SCANCODE_MASK;
                io.KeysDown[key] = (event->type == SDL_KEYDOWN);
                io.KeyShift = ((SDL_GetModState() & KMOD_SHIFT) != 0);
                io.KeyCtrl = ((SDL_GetModState() & KMOD_CTRL) != 0);
                io.KeyAlt = ((SDL_GetModState() & KMOD_ALT) != 0);
                io.KeySuper = ((SDL_GetModState() & KMOD_GUI) != 0);

                return 1;
        }

        // We want the original program to process non-input events even when menu is open, such as resize
        orig:
        return origEventFilter( userdata, event );
    }

    EGLBoolean eglSwapBuffers( EGLDisplay display, EGLSurface surface )
    {
        static bool bFirst = true;
        // We init this stuff here because it's after the parent has set up SDL, egl, etc.
        if( bFirst ){

            gl3wInit();
            ImGui::CreateContext();
            ImGuiIO& io = ImGui::GetIO();

            // Get the Original Event Filter callback from SDL
            if( SDL_GetEventFilter(&origEventFilter, NULL) == SDL_FALSE ){
                Log("Couldn't get event filter from SDL!\n");
                abort();
            }
            // Set our new One
            SDL_SetEventFilter( EventFilter, NULL );

            // Fixup some keycodes for SDL
            io.KeyMap[ImGuiKey_Tab] = SDLK_TAB;
            io.KeyMap[ImGuiKey_LeftArrow] = SDL_SCANCODE_LEFT;
            io.KeyMap[ImGuiKey_RightArrow] = SDL_SCANCODE_RIGHT;
            io.KeyMap[ImGuiKey_UpArrow] = SDL_SCANCODE_UP;
            io.KeyMap[ImGuiKey_DownArrow] = SDL_SCANCODE_DOWN;
            io.KeyMap[ImGuiKey_PageUp] = SDL_SCANCODE_PAGEUP;
            io.KeyMap[ImGuiKey_PageDown] = SDL_SCANCODE_PAGEDOWN;
            io.KeyMap[ImGuiKey_Home] = SDL_SCANCODE_HOME;
            io.KeyMap[ImGuiKey_End] = SDL_SCANCODE_END;
            io.KeyMap[ImGuiKey_Delete] = SDLK_DELETE;
            io.KeyMap[ImGuiKey_Backspace] = SDLK_BACKSPACE;
            io.KeyMap[ImGuiKey_Enter] = SDLK_RETURN;
            io.KeyMap[ImGuiKey_Escape] = SDLK_ESCAPE;
            io.KeyMap[ImGuiKey_A] = SDLK_a;
            io.KeyMap[ImGuiKey_C] = SDLK_c;
            io.KeyMap[ImGuiKey_V] = SDLK_v;
            io.KeyMap[ImGuiKey_X] = SDLK_x;
            io.KeyMap[ImGuiKey_Y] = SDLK_y;
            io.KeyMap[ImGuiKey_Z] = SDLK_z;

            int width, height;
            eglQuerySurface(display, surface, EGL_WIDTH, &width);
            eglQuerySurface(display, surface, EGL_HEIGHT, &height);
            io.DisplaySize = ImVec2( width, height );

            ImGui_ImplOpenGL3_Init("#version 100");
            ImGui::StyleColorsDark();

            bFirst = false;
        }


        ImGui_ImplOpenGL3_NewFrame();

        ImGuiIO& io = ImGui::GetIO();

        io.MouseDrawCursor = menuOpen;
        io.WantCaptureMouse = menuOpen;
        io.WantCaptureKeyboard = menuOpen;

        static double lastTime = 0.0f;
        Uint32 time = SDL_GetTicks();
        double currentTime = time / 1000.0;
        io.DeltaTime = lastTime > 0.0 ? (float)(currentTime - lastTime) : (float)(1.0f / 60.0f);

        int width, height;
        eglQuerySurface(display, surface, EGL_WIDTH, &width);
        eglQuerySurface(display, surface, EGL_HEIGHT, &height);
        io.DisplaySize = ImVec2( width, height );

        if( io.WantCaptureMouse ){
            int mx, my;
            SDL_GetMouseState(&mx, &my);

            io.MousePos = ImVec2((float)mx, (float)my);

            SDL_ShowCursor(io.MouseDrawCursor ? 0: 1);
        }
        ImGui::NewFrame();
            ImGui::SetNextWindowPos(ImVec2(0,0));
            ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
            // Create a big see-thru background so we can draw anywhere.
        ImGui::Begin("peeper-bg", NULL, ImVec2(0,0), 0.0f, ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoScrollWithMouse);
            DrawRequests(); // Draw requests from queue.
                if( menuOpen ){
                    ImGui::Begin("Demo window");
                    if(ImGui::Button("Hello!")){
                        Log("Hi gamer!\n");
                    }
                    char text[64];
                    ImGui::InputText("#bigtext", text, 64 );
                    ImGui::End();
                }
            ImGui::End(); // end of background
        ImGui::EndFrame();
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        return origEGLswapBuffers( display, surface );
    }
}

int __attribute__((constructor)) Startup()
{
    Log("Peeper loaded.\n");

    // Open /dev/shm/ shared memory, Create if needed.
    sharedMemoryFD = shm_open( SHM_NAME, O_RDWR | O_CREAT, S_IRWXU);
    if( sharedMemoryFD < 0 ){
        Log("Can't open shared mem!\n");
        abort();
    }
    // Set the size.
    ftruncate( sharedMemoryFD, sizeof(SharedRegion) );

    // Allocate the memory with the mmap and the fd
    sharedRegion = (SharedRegion*)mmap( NULL, sizeof(struct SharedRegion), PROT_READ | PROT_WRITE, MAP_SHARED, sharedMemoryFD, 0 );

    if( sharedRegion == MAP_FAILED ){
        Log("failed to mmap shared region!\n");
        abort();
    }

    // Open/Create a semaphore by it's unique name
    semaphore = sem_open( SEMAPHORE_NAME, O_CREAT | O_EXCL, S_IRWXU, 0);
    if( semaphore == (void*)-1 ){
        Log("Failed to create semaphore!\n");
        abort();
    }

    // Create a new thread for sharedMemory reading and let it run on its own.
    sharedMemoryReader = std::thread( ReadSharedMemory );
    sharedMemoryReader.detach();



    origEGLswapBuffers = (swapBuffersFn)dlsym(RTLD_NEXT, "eglSwapBuffers");
    if( !origEGLswapBuffers ){
        Log("EGLswapBuffers null!(%s)\n", dlerror());
        abort();
    }

    return 0;
}


// Called when program closes.
void __attribute__((destructor)) Shutdown()
{
    Log("Peeper Unload called!\n");

    munmap( sharedRegion, sizeof(struct SharedRegion) );
    sharedRegion = nullptr;
    close( sharedMemoryFD );
    shm_unlink( SHM_NAME );

    sem_close( semaphore );
    sem_unlink( SEMAPHORE_NAME );
}
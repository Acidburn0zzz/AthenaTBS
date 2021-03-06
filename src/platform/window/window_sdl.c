#include "private_window.h"
#include <SDL2/SDL.h>
#include <string.h>
#include <assert.h>

struct Athena_SDL2_Window{
    SDL_Window *window;
};

void *Athena_Private_CreateHandle(){
    struct Athena_SDL2_Window *window = malloc(sizeof(struct Athena_SDL2_Window));
    memset(window, 0, sizeof(struct Athena_SDL2_Window));
    return window;
}

int Athena_Private_DestroyHandle(void *handle){
    struct Athena_SDL2_Window * const window = handle;
    if(window->window)
        SDL_DestroyWindow(window->window);
    free(window);
    return 0;
}

int Athena_Private_CreateWindow(void *handle, int x, int y, unsigned w, unsigned h, const char *title){
    struct Athena_SDL2_Window * const window = handle;
    if(!window)
        return -1;
    if(!title)
        title = "";

    if(!SDL_WasInit(SDL_INIT_VIDEO|SDL_INIT_EVENTS)){
        SDL_SetHint(SDL_HINT_FRAMEBUFFER_ACCELERATION, "0");
        SDL_SetHint(SDL_HINT_MAC_CTRL_CLICK_EMULATE_RIGHT_CLICK, "1");
        SDL_SetHint(SDL_HINT_VIDEO_HIGHDPI_DISABLED, 0);
        SDL_Init(SDL_INIT_VIDEO|SDL_INIT_EVENTS);
    }

    window->window = SDL_CreateWindow(title, x, y, w, h, SDL_WINDOW_HIDDEN);
    return 0;
}

int Athena_Private_ShowWindow(void *handle){
    struct Athena_SDL2_Window * const window = handle;
    if(!window)
        return -1;
    SDL_ShowWindow(window->window);
    return 0;
}

int Athena_Private_HideWindow(void *handle){
    struct Athena_SDL2_Window * const window = handle;
    if(!window)
        return -1;
    SDL_HideWindow(window->window);
    return 0;
}

static SDL_Surface *Athena_Private_SurfaceOfFormat(unsigned w, unsigned h, unsigned format, const void *RGB){
const unsigned long
    r0 = 0x00FF0000,
    g0 = 0x0000FF00,
    b0 = 0x000000FF,
    a0 = 0xFF000000,
    r1 = 0x0000FF00,
    g1 = 0x00FF0000,
    b1 = 0xFF000000,
    a1 = 0x000000FF;

const unsigned depth = (format>1)?3:4;

    if(format==0)
        return SDL_CreateRGBSurfaceFrom((void *)RGB, w, h, depth<<3, w*depth, r0, g0, b0, a0);
    else if(format==1)
        return SDL_CreateRGBSurfaceFrom((void *)RGB, w, h, depth<<3, w*depth, r1, g1, b1, a1);
    else if(format==2)
        return SDL_CreateRGBSurfaceFrom((void *)RGB, w, h, depth<<3, w*depth, 0, 0, 0, 0);
    else return NULL;
}

int Athena_Private_Update(void *handle, unsigned format, const void *RGB, unsigned w, unsigned h){
    struct Athena_SDL2_Window * const window = handle;

    SDL_Surface * const surface = Athena_Private_SurfaceOfFormat(w, h, format, RGB),
        * const win_surface = SDL_GetWindowSurface(window->window);
    SDL_Rect dst = {0, 0};
    
    dst.w = w;
    dst.h = h;

    SDL_BlitSurface(surface, NULL, win_surface, &dst);
    SDL_FreeSurface(surface);
    return 0;
}

int Athena_Private_FlipWindow(void *handle){
    struct Athena_SDL2_Window * const window = handle;
    if(!window)
        return -1;
    else{
        SDL_Surface *surface = SDL_GetWindowSurface(window->window);
        SDL_UpdateWindowSurface(window->window);
        SDL_FillRect(surface, NULL, SDL_MapRGBA(surface->format, 0, 0, 0, 0xFF));
    }
    return 0;
}

unsigned Athena_Private_GetEvent(void *handle, struct Athena_Event *to){
    struct Athena_SDL2_Window * const window = handle;
    SDL_Event event;

    if(!window || !SDL_PollEvent(&event))
        return 0;

    memset(to, 0, sizeof(struct Athena_Event));

    if(event.type == SDL_QUIT){
        to->type = athena_quit_event;
        return 1;
    }

    /* We check x/y just to stop out-of-window clicks on OS X, particularly on the titlebar or menubar */
    if(event.type == SDL_MOUSEBUTTONDOWN && event.button.x && event.button.y){
        to->type = athena_click_event;
        to->x = event.button.x;
        to->y = event.button.y;
        to->state = event.type == SDL_MOUSEBUTTONDOWN;

        switch(event.button.button){
            case SDL_BUTTON_LEFT:
                to->which = athena_left_mouse_button;
                break;
            case SDL_BUTTON_MIDDLE:
                to->which = athena_middle_mouse_button;
                break;
            case SDL_BUTTON_RIGHT:
                to->which = athena_right_mouse_button;
                break;
            default:
                to->which = athena_unknown_mouse_button;
                break;
        }

        return 1;
    }

    to->type = athena_unknown_event;
    return 1;    
}

int Athena_Private_IsKeyPressed(void *handle, unsigned key){
    if(key >= 0x80)
        return 0;
    else{
        char code[2] = {0, '\0'};
        code[0] = key;
        
        return SDL_GetKeyboardState(NULL)[SDL_GetScancodeFromKey(SDL_GetKeyFromName(code))];
    }
}

int Athena_Private_GetMousePosition(void *handle, int *x, int *y){
    SDL_GetMouseState(x, y);
    return 0;
}

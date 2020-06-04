/* Copyright (C) 2020 Mark Sowden <markelswo@gmail.com>
 * */

#include <SDL2/SDL.h>

#include "launcher.h"

typedef struct SysWindow {
	SDL_Window      *sdlWindowPtr;
	SDL_GLContext   *sdlGLContext;
} SysWindow;
static SysWindow *mainWindow = NULL;

static SDL_TimerID timer = 0;
static unsigned int numTicks = 0;

SysWindow *Sys_GetMainWindow( void ) {
	return mainWindow;
}

void Sys_GetWindowSize( SysWindow *windowPtr, int *width, int *height ) {
	SDL_GL_GetDrawableSize( windowPtr->sdlWindowPtr, width, height );
}

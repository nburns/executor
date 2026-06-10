/*
 * Copyright 1998 by Abacus Research and
 * Development, Inc.  All rights reserved.
 */

#if !defined (__syswm_vars_h__)
#define __syswm_vars_h__

#if defined (_WIN32) && !defined (WIN32)
#define WIN32
#endif

#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>

/* System dependent variables */
#if defined(__unix__)
/* * */
extern Display *SDL_Display;
extern Window sdl_x11_window;  /* renamed: SDL_Window is a type in SDL2 */

#elif defined(_WIN32)
/* * */
extern HWND sdl_win32_hwnd;

#endif /* OS */

#endif

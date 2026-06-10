/* Copyright 1994, 1995, 1996 by Abacus Research and
 * Development, Inc.  All rights reserved.
 */

#if !defined (OMIT_RCSID_STRINGS)
char ROMlib_rcsid_sdlwm[] = "$Id: sdlwm.c 88 2005-05-25 03:59:37Z ctm $";
#endif

#include <stdio.h>
#include <SDL2/SDL.h>

extern SDL_Window *sdl_get_window(void);

/* Globals */
int host_cursor_depth = 1;

/* Window manager interface functions */

void
ROMlib_SetTitle (char *title)
{
  SDL_Window *w = sdl_get_window();
  if (w)
    SDL_SetWindowTitle(w, title);
}

char *
ROMlib_GetTitle (void)
{
  SDL_Window *w = sdl_get_window();
  return w ? (char *)SDL_GetWindowTitle(w) : NULL;
}

void
ROMlib_FreeTitle (char *title)
{
}

/* This is really inefficient.  We should hash the cursors */
void
host_set_cursor (char *cursor_data,
                 unsigned short cursor_mask[16],
                 int hotspot_x, int hotspot_y)
{
  SDL_Cursor *old_cursor, *new_cursor;

  old_cursor = SDL_GetCursor();
  new_cursor = SDL_CreateCursor((unsigned char *) cursor_data,
				(unsigned char *) cursor_mask,
				16, 16, hotspot_x, hotspot_y);
  if ( new_cursor != NULL )
    {
      SDL_SetCursor(new_cursor);
      SDL_FreeCursor(old_cursor);
    }
}

int
host_set_cursor_visible (int show_p)
{
  return(SDL_ShowCursor(show_p));
}

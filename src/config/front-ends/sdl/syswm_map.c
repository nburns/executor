/*
 * Copyright 1998 by Abacus Research and
 * Development, Inc.  All rights reserved.
 */

#if !defined (OMIT_RCSID_STRINGS)
char ROMlib_rcsid_syswm_map[] = "$Id: syswm_map.c 63 2004-12-24 18:19:43Z ctm $";
#endif

#include "syswm_vars.h"
#include "syswm_map.h"

#define USE_WINDOWS_NOT_MAC_TYPEDEFS_AND_DEFINES

#include "rsys/common.h"

#include "sdlscrap.h"

#if defined (CYGWIN32)
#include "win_clip.h"
#endif

extern SDL_Window *sdl_get_window(void);

/* System dependent variables */
#if defined(__unix__)
/* * */
Display *SDL_Display;
Window sdl_x11_window;

static int screen_width;
static int screen_height;

int
os_current_screen_width (void)
{
  return screen_width;
}

int
os_current_screen_height (void)
{
  return screen_height;
}

#elif defined(_WIN32)

#include "win_screen.h"

HWND sdl_win32_hwnd;

#endif /* OS */

/* Initialize the system dependent variables */
PUBLIC int
sdl_syswm_init(void)
{
  int retval;
  SDL_SysWMinfo info;

  /* Grab the window manager specific information */
  SDL_VERSION(&info.version);
  if ( SDL_GetWindowWMInfo(sdl_get_window(), &info) == SDL_TRUE )
    {
#if defined(__unix__)
/* * */
      SDL_Display = info.info.x11.display;
      sdl_x11_window = info.info.x11.window;

      {
	Screen *screen;

	screen = DefaultScreenOfDisplay(SDL_Display);
	screen_width = WidthOfScreen (screen);
	screen_height = HeightOfScreen (screen);
      }

#elif defined(_WIN32)
/* * */
      sdl_win32_hwnd = info.info.win.window;

#endif /* OS */
      retval = 0;
    }
  else
    {
      retval = -1;
    }
  return(retval);
}

#if defined(linux)

/* Handle system dependent events */
PUBLIC int
sdl_syswm_event(const SDL_Event *event)
{
  int retval;

  switch (event->syswm.msg->msg.x11.event.type)
    {
    case SelectionRequest:
      export_scrap(event);
      break;
    default:
      break;
    }
  retval = 1; /* NOTE: this looks wrong ... but that's how Sam wrote it */

  return retval;
}

#elif defined (CYGWIN32)

/* Handle system dependent events */
PUBLIC int
sdl_syswm_event(const SDL_Event *event)
{
  int retval;

  retval = 0;
  switch (event->syswm.msg->msg.win.msg)
    {
    case WM_SYSCOMMAND:
      if (event->syswm.msg->msg.win.wParam == SC_MAXIMIZE)
	{
	  ROMlib_recenter_window ();
	  retval = 1;
	}
      break;
    case WM_RENDERFORMAT:
      if (event->syswm.msg->msg.win.wParam == CF_DIB)
	write_pict_as_dib_to_clipboard ();
      else if (event->syswm.msg->msg.win.wParam ==
	       ROMlib_executor_format (TICK ("PICT")))
	write_pict_as_pict_to_clipboard ();
      break;
    case WM_RENDERALLFORMATS:
      write_pict_as_pict_to_clipboard ();
      write_pict_as_dib_to_clipboard ();
      break;
    default:
      break;
    }

  return retval;
}

#else

/* Handle system dependent events */
PUBLIC int
sdl_syswm_event(const SDL_Event *event)
{
  int retval;

  retval = 0;
  return retval;
}
#endif

/* Copyright 1994 - 1999 by Abacus Research and
 * Development, Inc.  All rights reserved.
 */

#if !defined (OMIT_RCSID_STRINGS)
char ROMlib_rcsid_sdlwin[] = "$Id: sdlwin.c 63 2004-12-24 18:19:43Z ctm $";
#endif

#include "rsys/common.h"
#include "rsys/srcblt.h"
#include "rsys/refresh.h"

#if defined (CYGWIN32)
#include "win_screen.h"
#endif

#include <SDL2/SDL.h>

#include "sdlevents.h"
#include "syswm_map.h"

/* SDL2 video objects */
static SDL_Window   *window;
static SDL_Renderer *renderer;
static SDL_Texture  *texture;   /* 32bpp streaming texture for display */
static SDL_Surface  *screen;    /* 8bpp software surface - our framebuffer */

/* These variables are required by the vdriver interface. */
uint8 *vdriver_fbuf;
int vdriver_row_bytes;
int vdriver_width = 0;
int vdriver_height = 0;
int vdriver_bpp = 8, vdriver_log2_bpp;
int vdriver_max_bpp, vdriver_log2_max_bpp;
vdriver_modes_t *vdriver_mode_list;

/* The modes structure is just checked for error messages, fake it here */
static vdriver_modes_t sdl_impotent_modes = { 0, 0 };


/* SDL vdriver implementation */

void
vdriver_opt_register (void)
{
}

PUBLIC boolean_t ROMlib_fullscreen_p = FALSE;
PUBLIC boolean_t ROMlib_hwsurface_p = FALSE;

SDL_Window *
sdl_get_window (void)
{
  return window;
}

boolean_t
vdriver_init (int _max_width, int _max_height, int _max_bpp,
	      boolean_t fixed_p, int *argc, char *argv[])
{
  if (SDL_Init(SDL_INIT_VIDEO) < 0)
    return FALSE;
  sdl_events_init();

  vdriver_max_bpp = 8;
  vdriver_log2_max_bpp = ROMlib_log2[vdriver_max_bpp];
  vdriver_mode_list = &sdl_impotent_modes;

  atexit(vdriver_shutdown);
  return TRUE;
}

boolean_t
vdriver_acceptable_mode_p (int width, int height, int bpp,
			   boolean_t grayscale_p, boolean_t exact_match_p)
{
  if (!width)  width  = vdriver_width;
  if (!height) height = vdriver_height;
  if (!bpp)    bpp    = vdriver_bpp;

  if (grayscale_p != vdriver_grayscale_p)
    return FALSE;
  return bpp <= 8;
}

boolean_t
vdriver_set_mode (int width, int height, int bpp, boolean_t grayscale_p)
{
  Uint32 window_flags;

  if (!width) {
    width = vdriver_width;
    if (!width)
      width = VDRIVER_DEFAULT_SCREEN_WIDTH;
  }

  if (!height) {
    height = vdriver_height;
    if (!height)
      height = VDRIVER_DEFAULT_SCREEN_HEIGHT;
  }

  if (!bpp) bpp = vdriver_bpp;

  if (!vdriver_acceptable_mode_p(width, height, bpp, grayscale_p, FALSE))
    return FALSE;

  window_flags = SDL_WINDOW_SHOWN;
  if (getenv("SDL_FULLSCREEN") != NULL || ROMlib_fullscreen_p)
    window_flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;

  /* Free existing surface and texture before resizing */
  if (screen)  { SDL_FreeSurface(screen);     screen  = NULL; }
  if (texture) { SDL_DestroyTexture(texture); texture = NULL; }

  if (!window) {
    window = SDL_CreateWindow("Executor",
                              SDL_WINDOWPOS_UNDEFINED,
                              SDL_WINDOWPOS_UNDEFINED,
                              width, height, window_flags);
    if (!window) return FALSE;

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer)
      renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    if (!renderer) {
      SDL_DestroyWindow(window);
      window = NULL;
      return FALSE;
    }
  } else {
    SDL_SetWindowSize(window, width, height);
  }

  /* 8bpp software surface - pixels written here by the emulator */
  screen = SDL_CreateRGBSurface(0, width, height, 8, 0, 0, 0, 0);
  if (!screen) return FALSE;

  /* 32bpp streaming texture for GPU-accelerated display */
  texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
                              SDL_TEXTUREACCESS_STREAMING, width, height);
  if (!texture) {
    SDL_FreeSurface(screen);
    screen = NULL;
    return FALSE;
  }

  vdriver_width     = width;
  vdriver_height    = height;
  vdriver_bpp       = 8;
  vdriver_log2_bpp  = ROMlib_log2[8];
  vdriver_row_bytes = screen->pitch;
  vdriver_fbuf      = (uint8 *)screen->pixels;

  sdl_syswm_init();

  return TRUE;
}

void
vdriver_set_colors (int first_color, int num_colors, const ColorSpec *colors)
{
  int i;
  SDL_Color *sdl_cmap;

  sdl_cmap = (SDL_Color *)alloca(num_colors * sizeof(SDL_Color));
  for (i = 0; i < num_colors; ++i) {
    sdl_cmap[i].r = (CW(colors[i].rgb.red)   >> 8);
    sdl_cmap[i].g = (CW(colors[i].rgb.green) >> 8);
    sdl_cmap[i].b = (CW(colors[i].rgb.blue)  >> 8);
    sdl_cmap[i].a = 255;
  }
  if (screen && screen->format && screen->format->palette)
    SDL_SetPaletteColors(screen->format->palette, sdl_cmap,
                         first_color, num_colors);
  /* Palette change requires a full screen redraw */
  vdriver_update_screen(0, 0, vdriver_height, vdriver_width, FALSE);
}

void
vdriver_get_colors (int first_color, int num_colors, ColorSpec *colors)
{
  gui_fatal ("`!vdriver_fixed_clut_p' and `vdriver_get_colors ()' called");
}

/* Convert 8bpp indexed surface to 32bpp and upload/render to window */
static void
upload_and_render (int top, int left, int bottom, int right)
{
  SDL_Surface *conv;

  if (!screen || !texture || !renderer)
    return;

  conv = SDL_ConvertSurfaceFormat(screen, SDL_PIXELFORMAT_ARGB8888, 0);
  if (!conv)
    return;

  if (right > left && bottom > top) {
    SDL_Rect rect;
    uint8_t *src;

    rect.x = left;  rect.y = top;
    rect.w = right - left;  rect.h = bottom - top;
    src = (uint8_t *)conv->pixels + top * conv->pitch + left * 4;
    SDL_UpdateTexture(texture, &rect, src, conv->pitch);
  }
  SDL_FreeSurface(conv);

  SDL_RenderClear(renderer);
  SDL_RenderCopy(renderer, texture, NULL, NULL);
  SDL_RenderPresent(renderer);
}

int
vdriver_update_screen_rects (int num_rects, const vdriver_rect_t *r,
                             boolean_t cursor_p)
{
  /* Upload the full screen once even if multiple rects requested */
  upload_and_render(0, 0, vdriver_height, vdriver_width);
  return 0;
}

int
vdriver_update_screen (int top, int left, int bottom, int right,
		       boolean_t cursor_p)
{
  if (top    < 0)              top    = 0;
  if (left   < 0)              left   = 0;
  if (bottom > vdriver_height) bottom = vdriver_height;
  if (right  > vdriver_width)  right  = vdriver_width;

  upload_and_render(top, left, bottom, right);
  return 0;
}

void
vdriver_flush_display (void)
{
}

void
vdriver_shutdown (void)
{
  SDL_Quit();
}

/* host functions that should go away */

/* shadow buffer; created on demand */
unsigned char *vdriver_shadow_fbuf = NULL;

void
host_flush_shadow_screen (void)
{
  int top_long, left_long, bottom_long, right_long;

  /* Lazily allocate a shadow screen.  We won't be doing refresh that often,
   * so don't waste the memory unless we need it.  Note: memory never reclaimed
   */
  if (vdriver_shadow_fbuf == NULL)
    {
      vdriver_shadow_fbuf = malloc(vdriver_row_bytes * vdriver_height);
      memcpy (vdriver_shadow_fbuf, vdriver_fbuf,
                                 vdriver_row_bytes * vdriver_height);
      vdriver_update_screen (0, 0, vdriver_height, vdriver_width, FALSE);
    }
  else if (find_changed_rect_and_update_shadow ((uint32 *) vdriver_fbuf,
						(uint32 *) vdriver_shadow_fbuf,
						(vdriver_row_bytes
						 / sizeof (uint32)),
						vdriver_height,
						&top_long, &left_long,
						&bottom_long, &right_long))
    {
      vdriver_update_screen (top_long, (left_long * 32) >> vdriver_log2_bpp,
			     bottom_long,
			     (right_long * 32) >> vdriver_log2_bpp, FALSE);
    }
}

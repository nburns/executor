/* define `SDL' the Simple DirectMedia Layer front-end */
#if !defined (SDL)

/* Define the front end before including SDL headers to prevent
   recursive self-inclusion on case-insensitive filesystems. */
#define SDL

#include <SDL2/SDL.h>
#include <SDL2/SDL_thread.h>

/* We need this for the syn68k_addr_t definition */
#include <syn68k_public.h>

extern syn68k_addr_t
       handle_sdl_events(syn68k_addr_t interrupt_addr, void *unused);

extern boolean_t ROMlib_fullscreen_p;
extern boolean_t ROMlib_hwsurface_p;
extern SDL_cond *ROMlib_shouldbeawake_cond;
extern SDL_mutex *ROMlib_shouldbeawake_mutex;


#endif /* !SDL */

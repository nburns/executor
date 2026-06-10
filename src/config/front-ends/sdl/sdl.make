# Use parent include dir so headers are accessed as <SDL2/SDL.h>,
# avoiding name collision with executor's local sdl.h on case-insensitive FS
INCLUDES += $(shell sdl2-config --cflags | sed 's|-I\([^ ]*\)/SDL2|-I\1|g')

SDL_SRC = sdlwin.c sdlevents.c sdlwm.c sdlscrap.c sdlquit.c \
          syswm_map.c sdl_mem.c SDL_bmp.c

ifneq (,$(findstring linux,$(HOST)))
  SDL_SRC += sdlX.c
endif

FRONT_END_SRC = $(SDL_SRC)
FRONT_END_OBJ = $(FRONT_END_SRC:.c=.o) 

ifneq (,$(findstring macosx,$(HOST)))
  FRONT_END_OBJ += macosx_main.o
endif


ifneq (,$(findstring mingw,$(HOST)))
  FRONT_END_LIBS += -lmingw32
endif
# SDL_LIB_DIR is defined in the OS-specific makefile
ifneq (,$(SDL_LIB_DIR))
  FRONT_END_LIBS += -L$(SDL_LIB_DIR)
endif

FRONT_END_LIBS += $(shell sdl2-config --libs)

ifneq (,$(findstring linux,$(HOST)))
  FRONT_END_LIBS += -ldl -L/usr/X11R6/lib -lX11 -lpthread
  INCLUDES += -I/usr/X11R6/include
  CFLAGS += -D_REENTRANT
endif

ifneq (,$(findstring macosx,$(HOST)))
  FRONT_END_LIBS += -framework Cocoa
  CFLAGS += -D_REENTRANT
endif

clean::
	rm -f $(FRONT_END_OBJ)

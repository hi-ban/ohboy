# make -B -f Makefile.linux_sdl

CC = gcc
LD = $(CC)

CFLAGS = -O3
LDFLAGS = -s

GNUBOY = ./gnuboy

OBJS = $(GNUBOY)/lcd.o $(GNUBOY)/refresh.o $(GNUBOY)/lcdc.o $(GNUBOY)/palette.o $(GNUBOY)/cpu.o $(GNUBOY)/mem.o \
	$(GNUBOY)/rtc.o $(GNUBOY)/hw.o $(GNUBOY)/sound.o \
	$(GNUBOY)/events.o $(GNUBOY)/keytable.o \
	$(GNUBOY)/loader.o $(GNUBOY)/save.o $(GNUBOY)/debug.o $(GNUBOY)/emu.o $(GNUBOY)/main.o \
	$(GNUBOY)/rccmds.o $(GNUBOY)/rckeys.o $(GNUBOY)/rcvars.o $(GNUBOY)/rcfile.o $(GNUBOY)/exports.o \
	$(GNUBOY)/split.o $(GNUBOY)/path.o $(GNUBOY)/inflate.o \
	$(GNUBOY)/sys/sdl/SFont.o  \
	gui_sdl.o keymap.o \
	main.o menu.o \
	./ubytegui/dialog.o ./ubytegui/font.o ./ubytegui/gui.o ./ubytegui/pixmap.o \

## NOTE inc paths are from TinyCore SDL-dev
DEFS = -DIS_LITTLE_ENDIAN -DALT_PATH_SEP
INCS = -I$(GNUBOY) -I$(GNUBOY)/sys/sdl -I/usr/include/ -I/tmp/tcloop/SDL-dev/usr/local/
LIBS =  -lSDLmain -lSDL

# Use SDL_image - for use with SFont (rather than FREETYPE_TTF)
# NOTE can still use SFont without SDL_image
#USE_SDL_IMAGE = True
ifdef USE_SDL_IMAGE
	LIBS +=  -lSDL_image
	DEFS += -DOHBOY_USE_SDL_IMAGE
endif

## Requirements for (optional) .zip support
## Disable with GNUBOY_USE_MINIZIP defined
OBJS += $(GNUBOY)/unzip/unzip.o $(GNUBOY)/unzip/ioapi.o
LIBS += -lz

############
# Simple disable sound support
# not as cleanly implemented as in core gnuboy as it
# still includes unused pcm/sound code... But it works :-)
GNUBOY_DISABLE_SDL_SOUND = True
ifdef GNUBOY_DISABLE_SDL_SOUND
	OBJS += $(GNUBOY)/sys/dummy/nosound.o
	DEFS += -DGNUBOY_DISABLE_SDL_SOUND
endif
############

# USE_FREETYPE_TTF enables the use of Freetype2library (not SDL_ttf) and uses a ttf file at runtime
# quality of results on screen is excellent, if not set SFont is used.
# SFont can have excellent results but the bitmap font supplied at the moment is not pretty.
#USE_FREETYPE_TTF = True
ifdef USE_FREETYPE_TTF
	INCS += -I/usr/include/freetype2/ -I/usr/include/freetype2/freetype
	LIBS +=  -lfreetype
	DEFS += -DUBYTE_USE_FREETYPE
endif

# USE_LIBPNG if set will use libpng AND attempt to load png file from "etc".
# if not set will use attempt to load bmp file "etc".
#USE_LIBPNG = True
ifdef USE_LIBPNG
	DEFS += -DUBYTE_USE_LIBPNG
	LIBS += -lpng
endif

DEFS += -DGNUBOY_NO_SCREENSHOT

MYCC = $(CC) $(CFLAGS) $(INCS) $(DEFS)

all: ohboy_ver.h ohboy

# FIXME need better dependency handling
menu.o: ohboy_ver.h

ohboy_ver.h:
	sh gen_ohboyver.sh > ohboy_ver.h

ohboy: $(OBJS)
	$(LD) $(LDFLAGS) $(OBJS) -o $@ $(LIBS)

clean:
	rm -f ohboy ohboy.exe *.o ubytegui/*.o $(GNUBOY)/*.o
	rm -f ohboy ohboy.exe *.o ubytegui/*.o $(GNUBOY)/*.o $(GNUBOY)/sys/dingoo/*.o   $(APP_NAME).elf $(APP_NAME).bin $(APP_NAME).app $(APP_NAME).sim

$(GNUBOY)/main.o:
	$(MYCC) -Dmain=gnuboy_main -c $(GNUBOY)/main.c -o $@

.c.o:
	$(MYCC) -c $< -o $@

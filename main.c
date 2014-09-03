#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>  /* for getcwd() */

/* posix includes for mkdir */
#include <sys/types.h>
#include <sys/stat.h>


#ifdef WIZ
#include <fcntl.h>
#include <sys/mman.h>
#endif

#include <SDL/SDL.h>
#ifdef OHBOY_USE_SDL_IMAGE
#include "SDL/SDL_image.h"
#endif /* OHBOY_USE_SDL_IMAGE */

#include "SFont.h"
#include "font8px.h"  /* built in FPS font */
/*
#include "font14px.h"
*/
#include "font_pressstart8x.h"

#include "gnuboy.h"
#include "fb.h"
#include "input.h"
#include "rc.h"
#include "pcm.h"
#include "ubytegui/gui.h"
#include "hw.h"
#include "loader.h"
#include "gui_sdl.h"
#include "menu.h"

#ifdef __GNUC__
#       define unlikely(x)     __builtin_expect((x),0)
#       define prefetch(x, y)  __builtin_prefetch((x),(y))
#else
#       define unlikely(x)     (x)
#   define prefetch(x, y)
#endif

void ohb_loadrom(char *rom);

/* fps */
/*static int sdl_showfps 0 = OFF, 1 = Text Only, 2 = Text in a box (white text on black background) */
static int fps_current_count = 0; 
static int fps_last_count = 0; 
static int fps_last_time = 0; 
static int fps_current_time = 0;
static char fps_str[20] = {0};

static SDL_Surface *fps_font_bitmap_surface=NULL;
static SDL_Surface *menu_font_bitmap_surface=NULL;
static SDL_Surface *bgimage_bitmap_surface=NULL;
SFont_Font* fps_font=NULL;
SFont_Font* menu_font=NULL;

SDL_Surface *bordersf = NULL;

SDL_Rect myrect;
/* fps */

#define FONT_NAME "pressstart.ttf"
#define FONT_SIZE 8
#define SFONT_NAME "pressstart_font"

static font_t *font;

struct fb fb;
static int upscaler=0, frameskip=0, sdl_showfps=0, cpu_speed=0, bmpenabled=0, statesram=1;
static char* romdir=0, pal=0;
char *border;
char *gbcborder;
static int osd_persist=0;
static int dvolume=0;
#ifdef DINGOO_BUILD
static int button_a=1, button_b=2, button_x=0, button_y=0, button_l=0, button_r=0;
#else
static int button_a=2, button_b=1, button_x=0, button_y=0, button_l=0, button_r=0;
#endif /* DINGOO_BUILD */
#ifdef GNUBOY_HARDWARE_VOLUME
static int  sndlvl=10;
#endif /* GNBOY_HARDWARE_VOLUME */
#ifdef GCWZERO
static int alt_menu_combo=0;
#endif /* GCWZERO */

rcvar_t vid_exports[] =
{
#ifdef GNUBOY_HARDWARE_VOLUME
	RCV_INT("sndlvl", &sndlvl),
#endif /* GNBOY_HARDWARE_VOLUME */
	RCV_INT("upscaler", &upscaler),
	RCV_INT("frameskip", &frameskip),
	RCV_INT("showfps", &sdl_showfps),
	RCV_INT("cpuspeed",&cpu_speed),
	RCV_INT("bmpenabled", &bmpenabled),
	RCV_STRING("border",&border),
	RCV_STRING("gbcborder",&gbcborder),
	RCV_STRING("romdir",&romdir),
	RCV_STRING("palette",&pal),
	RCV_INT("button_a", &button_a),
	RCV_INT("button_b", &button_b),
	RCV_INT("button_x", &button_x),
	RCV_INT("button_y", &button_y),
	RCV_INT("button_l", &button_l),
	RCV_INT("button_r", &button_r),
	RCV_INT("statesram", &statesram),
#ifdef GCWZERO
	RCV_INT("alt_menu_combo", &alt_menu_combo),
#endif /* GCWZERO */
	RCV_END
};

struct fb vid_fb;
static un16 vid_frame[160*144];
SDL_Surface *screen;
int framecounter = 0;

#ifndef GNUBOY_DISABLE_SDL_SOUND
#ifndef OHBOY_DISABLE_SDL_SOUND
struct pcm pcm;
rcvar_t pcm_exports[] =
{
	RCV_END
};
#endif /* OHBOY_DISABLE_SDL_SOUND */
#endif /* GNUBOY_DISABLE_SDL_SOUND */

#define PCM_BUFFER 4096
#define PCM_FRAME 512
#define PCM_SAMPLERATE 44100

#define UP_MASK 	0x83
#define DOWN_MASK 	0x38
#define LEFT_MASK 	0x0E
#define RIGHT_MASK 	0xE0

n16 *buf;
n16 pcm_buffer[PCM_BUFFER << 1];
byte pcm_frame[PCM_FRAME << 1];
volatile int pcm_buffered = 0;

n16 *pcm_head, *pcm_tail;
int pcm_soft_volume=0x80, pcm_bufferlen;

SDL_Joystick * sdl_joy;
static const int joy_commit_range = 3276;
static char Xstatus, Ystatus;
static int use_joy=0;

#ifdef CAANOO
enum
{
     BTN_A = 0,     //       A /             1
     BTN_X = 1,     //       X /             2
     BTN_B = 2,     //       B /             3
     BTN_Y = 3,     //       Y /             4
     BTN_L = 4,     //       L /         5, L1
     BTN_R = 5,     //       R /         6, L2
     BTN_HOME = 6,  //    Home /         7, R1
     BTN_HOLD = 7,  //    Hold /         8, R2
     BTN_HELP1 = 8, //  Help I /        Select
     BTN_HELP2 = 9, // Help II /         Start
     BTN_TACT = 10, //    Tact / L Thumb Stick
     BTN_UP = 11, 
     BTN_DOWN = 12,
     BTN_LEFT = 13,
     BTN_RIGHT = 14,

};

void Caanoo_PushAnalogEvent(int btn, int pressed){
        
    SDL_Event event;
        
    event.type = (pressed)?SDL_JOYBUTTONDOWN:SDL_JOYBUTTONUP;
    event.jbutton.button = btn;
    event.jbutton.state = (pressed)?SDL_PRESSED:SDL_RELEASED;
    event.jbutton.which = 0;
        
    SDL_PushEvent(&event);      
}

void Caanoo_UpdateAnalog(void){
        
    static int buttonsPrevLeft = 0;
	static int buttonsPrevRight = 0;
	static int buttonsPrevUp = 0;
	static int buttonsPrevDown = 0;
    int buttonsNowLeft = 0;
    int buttonsNowRight = 0;
    int buttonsNowUp = 0;
    int buttonsNowDown = 0;
        
    SDL_JoystickUpdate();
                
    if (SDL_JoystickGetAxis( sdl_joy, 0 ) < -24000) buttonsNowLeft |= (1 << BTN_LEFT);
    else if (SDL_JoystickGetAxis( sdl_joy, 0 ) >  24000) buttonsNowRight |= (1 << BTN_RIGHT);
    else if (SDL_JoystickGetAxis( sdl_joy, 1 ) < -24000) buttonsNowUp |= (1 << BTN_UP);
    else if (SDL_JoystickGetAxis( sdl_joy, 1 ) >  24000) buttonsNowDown |= (1 << BTN_DOWN);
    
    if (buttonsNowLeft != buttonsPrevLeft)
    {
        if ((buttonsNowLeft & (1 << BTN_LEFT)) != (buttonsPrevLeft & (1 << BTN_LEFT)))
        {
                Caanoo_PushAnalogEvent(BTN_LEFT, (buttonsNowLeft & (1 << BTN_LEFT)));
        }
    }
    buttonsPrevLeft = buttonsNowLeft;
	
	if (buttonsNowRight != buttonsPrevRight)
    {
        if ((buttonsNowRight & (1 << BTN_RIGHT)) != (buttonsPrevRight & (1 << BTN_RIGHT)))
        {
                Caanoo_PushAnalogEvent(BTN_RIGHT, (buttonsNowRight & (1 << BTN_RIGHT)));
        }
    }
    buttonsPrevRight = buttonsNowRight;
	
	if (buttonsNowUp != buttonsPrevUp)
    {
        if ((buttonsNowUp & (1 << BTN_UP)) != (buttonsPrevUp & (1 << BTN_UP)))
        {
                Caanoo_PushAnalogEvent(BTN_UP, (buttonsNowUp & (1 << BTN_UP)));
        }
    }
    buttonsPrevUp = buttonsNowUp;

	if (buttonsNowDown != buttonsPrevDown)
    {
        if ((buttonsNowDown & (1 << BTN_DOWN)) != (buttonsPrevDown & (1 << BTN_DOWN)))
        {
                Caanoo_PushAnalogEvent(BTN_DOWN, (buttonsNowDown & (1 << BTN_DOWN)));
        }
    }
    buttonsPrevDown = buttonsNowDown;
	
}
#endif

rcvar_t joy_exports[] =
{
	RCV_BOOL("joy", &use_joy),
	RCV_END
};

/* keymap - mappings of the form { scancode, localcode } - from sdl/keymap.c */
extern int keymap[][2];


void vid_init() {


	SDL_LockSurface(screen);
	vid_fb.w = screen->w;
	vid_fb.h = screen->h;
	vid_fb.pelsize = screen->format->BytesPerPixel;
	vid_fb.pitch = screen->pitch;
	vid_fb.indexed = fb.pelsize == 1;
	vid_fb.ptr = screen->pixels;
	vid_fb.cc[0].r = screen->format->Rloss;
	vid_fb.cc[0].l = screen->format->Rshift;
	vid_fb.cc[1].r = screen->format->Gloss;
	vid_fb.cc[1].l = screen->format->Gshift;
	vid_fb.cc[2].r = screen->format->Bloss;
	vid_fb.cc[2].l = screen->format->Bshift;
	SDL_UnlockSurface(screen);

	fb.w = 160; /* Gameboy native res - width */
	fb.h = 144; /* Gameboy native res - height */
	fb.pelsize = 2;
	fb.pitch = 320;
	fb.indexed = 0;
	fb.cc[0].r = screen->format->Rloss;
	fb.cc[0].l = screen->format->Rshift;
	fb.cc[1].r = screen->format->Gloss;
	fb.cc[1].l = screen->format->Gshift;
	fb.cc[2].r = screen->format->Bloss;
	fb.cc[2].l = screen->format->Bshift;
	fb.ptr = vid_frame;
	fb.enabled = 1;
	fb.dirty = 0;
	
	SDL_FreeSurface (bordersf);
	vid_fb.first_paint = 1;	

	if (bmpenabled == 0){
		bordersf = SDL_LoadBMP("etc"DIRSEP"black.bmp");}
	else if (bmpenabled == 1)
	{
#ifdef OHBOY_USE_SDL_IMAGE
		if (hw.cgb){bordersf = IMG_Load(gbcborder);}
		if (!hw.cgb){bordersf = IMG_Load(border);}
#else
		if (hw.cgb){bordersf = SDL_LoadBMP(gbcborder);}
		if (!hw.cgb){bordersf = SDL_LoadBMP(border);}
#endif /*OHBOY_USE_SDL_IMAGE*/
		if ((upscaler == 3) || (upscaler == 4) || (upscaler == 7)) {bordersf = SDL_LoadBMP("etc"DIRSEP"black.bmp");}
		#if defined(DINGOO_OPENDINGUX)
		if (bordersf == NULL){                 /*Fix for flickering screen when borders are set to On but no border is loaded, and double buffer is used*/
			bordersf = SDL_LoadBMP("etc"DIRSEP"black.bmp");
		}
		#endif /*DINGOO_OPENDINGUX*/
	}
}

void paint_menu_bg() {

	pixmap_t *pix;

	#ifdef UBYTE_USE_LIBPNG
	#ifdef DINGOO_SIM
		/* do nothing */
	#else
		pix = pixmap_loadpng("etc"DIRSEP"launch.png");

		if(pix){
			SDL_LockSurface(screen);
			x = (screen->w - pix->width)/2;
			y = (screen->h - pix->height)/2;
			osd_drawpixmap(pix,x,y,0);
			pixmap_free(pix);
			SDL_UnlockSurface(screen);
		}
	#endif /* DINGOO_SIM */
	#else
		/*
		** Use SDL_LoadBMP() from base SDL 
		** TODO use IMG_Load() from SDL_image
		*/
		
		SDL_Surface *tmp_surface=NULL;
	#ifdef DINGOO_SIM
		/* do nothing */
	#else
		tmp_surface = SDL_LoadBMP("etc"DIRSEP"launch.bmp");

		if (tmp_surface)
		{
			bgimage_bitmap_surface = SDL_DisplayFormat(tmp_surface);
			SDL_FreeSurface(tmp_surface);
			if (bgimage_bitmap_surface)
			{
				/*
				SDL_Rect src, dest;
				
				src.x = 0;
				src.y = 0;
				src.w = bgimage_bitmap_surface->w;
				src.h = bgimage_bitmap_surface->h;
				 
				dest.x = 0;
				dest.y = 0;
				dest.w = bgimage_bitmap_surface->w;
				dest.h = bgimage_bitmap_surface->h;
				*/
				
				SDL_UnlockSurface(screen);
				SDL_BlitSurface(bgimage_bitmap_surface, NULL, screen, NULL);
				/*
				SDL_BlitSurface(bgimage_bitmap_surface, &src, screen, &dest);
				*/
				SDL_Flip(screen);                                              /*Fix for launch BMP not showing when using double buffer*/
				SDL_BlitSurface(bgimage_bitmap_surface, NULL, screen, NULL);   /*Paints the launch BMP two times*/
				
				SDL_FreeSurface(bgimage_bitmap_surface);
				SDL_Flip(screen);
				/*
				SDL_LockSurface(screen);  // for some reason this prevents SFont from displaying.....
				*/
			}
		}
	#endif /* DINGOO_SIM */
	#endif /* UBYTE_USE_LIBPNG */
}

void vid_setpal(int i, int r, int g, int b){
    /*
    **  complete NOOP
    **  internal gameboy framebuffer fb.ptr/vid_frame[] is used instead
    **  and then used to blit to screen at vid_end()
    */
}

/*
** This appears to be the GPL algorithm described at http://scale2x.sourceforge.net/algorithm.html
*/
#define SCALE3X(l,r,t,b,E0,E1,E2,E3,E4,E5,E6,E7,E8)\
A = src[t+l], B = src[t], C = src[t+r];\
D = src[ l ], E = src[0], F = src[ r ];\
G = src[b+l], H = src[b], I = src[b+r];\
if (B != H && D != F) {\
	E0 += D == B ? D : E;\
	E1 += (D == B && E != C) || (B == F && E != A) ? B : E;\
	E2 += B == F ? F : E;\
	E3 += (D == B && E != G) || (D == H && E != A) ? D : E;\
	E4 += E;\
	E5 += (B == F && E != I) || (H == F && E != C) ? F : E;\
	E6 += D == H ? D : E;\
	E7 += (D == H && E != I) || (H == F && E != G) ? H : E;\
	E8 += H == F ? F : E;\
} else {\
	E0 += E;\
	E1 += E;\
	E2 += E;\
	E3 += E;\
	E4 += E;\
	E5 += E;\
	E6 += E;\
	E7 += E;\
	E8 += E;\
}

#ifdef WIZ

static un16 buffer[3][216];

void ohb_scale3x(){

	un16 *dst = buffer[0];
	un16 *src = (un16*)fb.ptr+159;
	un16 *base = (un16*)vid_fb.ptr + 9612;
	int x,y;

	un16 A,B,C,D,E,F,G,H,I;

	memset(buffer,0,432*3);

	// Top-right
	SCALE3X(-1,0,0,160,dst[216],dst[0],dst[0],dst[216],dst[0],dst[0],dst[217],dst[1],dst[1]);
	dst++, src+=160;
	// right
	for(y=1; y<143; y+=2){
		SCALE3X(-1,0,-160,160,dst[216],dst[0],dst[0],dst[217],dst[1],dst[1],dst[217],dst[1],dst[1]);
		dst+=2, src+=160;
		SCALE3X(-1,0,-160,160,dst[216],dst[0],dst[0],dst[216],dst[0],dst[0],dst[217],dst[1],dst[1]);
		dst++, src+=160;
	}
	// bottom-Right
	SCALE3X(-1,0,-160,0,dst[216],dst[0],dst[0],dst[217],dst[1],dst[1],dst[217],dst[1],dst[1]);
	dst+=2, src+=160;


	for(x=158; x>1; x-=2){
		src = (un16*)fb.ptr + x;
		// top
		SCALE3X(-1,1,0,160,dst[216],dst[216],dst[0],dst[216],dst[216],dst[0],dst[217],dst[217],dst[1]);
		dst++, src+=160;
		// middle
		for(y=1; y<143; y+=2){
			SCALE3X(-1,1,-160,160,dst[216],dst[216],dst[0],dst[217],dst[217],dst[1],dst[217],dst[217],dst[1]);
			dst+=2, src+=160;
			SCALE3X(-1,1,-160,160,dst[216],dst[216],dst[0],dst[216],dst[216],dst[0],dst[217],dst[217],dst[1]);
			dst++, src+=160;
		}
		// bottom
		SCALE3X(-1,1,-160,0,dst[216],dst[216],dst[0],dst[217],dst[217],dst[1],dst[217],dst[217],dst[1]);
		dst+=2, src+=160;

		memcpy(base,buffer[0],432);
		memcpy(base+240,buffer[1],432);
		memcpy(base+480,buffer[2],432);
		dst=buffer[0];
		base += 720;

		memset(buffer,0,432*3);

		src = (un16*)fb.ptr + x-1;
		// top
		SCALE3X(-1,1,0,160,dst[216],dst[0],dst[0],dst[216],dst[0],dst[0],dst[217],dst[1],dst[1]);
		dst++, src+=160;
		// middle
		for(y=1; y<143; y+=2){
			SCALE3X(-1,1,-160,160,dst[216],dst[0],dst[0],dst[217],dst[1],dst[1],dst[217],dst[1],dst[1]);
			dst+=2, src+=160;
			SCALE3X(-1,1,-160,160,dst[216],dst[0],dst[0],dst[216],dst[0],dst[0],dst[217],dst[1],dst[1]);
			dst++, src+=160;
		}
		// bottom
		SCALE3X(-1,1,-160,0,dst[216],dst[0],dst[0],dst[217],dst[1],dst[1],dst[217],dst[1],dst[1]);
		dst+=2, src+=160;
	}

	src = fb.ptr;
	// top-left
	SCALE3X(0,1,0,160,dst[216],dst[216],dst[0],dst[216],dst[216],dst[0],dst[217],dst[217],dst[1]);
	dst++, src+=160;
	// left
	for(y=1; y<143; y+=2){
		SCALE3X(0,1,-160,160,dst[216],dst[216],dst[0],dst[217],dst[217],dst[1],dst[217],dst[217],dst[1]);
		dst+=2, src+=160;
		SCALE3X(0,1,-160,160,dst[216],dst[216],dst[0],dst[216],dst[216],dst[0],dst[217],dst[217],dst[1]);
		dst++, src+=160;
	}
	// bottom-right
	SCALE3X(0,1,-160,0,dst[216],dst[216],dst[0],dst[217],dst[217],dst[1],dst[217],dst[217],dst[1]);
	dst+=2, src+=160;

	memcpy(base,buffer[0],432);
	memcpy(base+240,buffer[1],432);
	memcpy(base+480,buffer[2],432);
}
#else

static un16 buffer[3][240];

/*
** Appears to expect source/dest framebuffer pixels to be 16bpp
** But expects source to be in RGB343 format. Looks like it may expect dest to be RGB565.
** 1.5 scaler with "smoothing", i.e. 160x144 -> 239x215 BUT centered/centred in a 320x240 screen.
** It looks like it does a 2 pass conversion:
**      1) increase 3x
**      2) decrease by 2x
*/
void ohb_scale3x(){

	un16 *dst = buffer[0];
	un16 *src = (un16*)fb.ptr;
	un16 *base = (un16*)vid_fb.ptr + 3880;
	int x,y;

	un16 A,B,C,D,E,F,G,H,I;

	memset(buffer,0,480*3);

	// Top-left
	SCALE3X(0,1,0,160,dst[0],dst[0],dst[1],dst[0],dst[0],dst[1],dst[240],dst[240],dst[241]);
	dst++, src++;
	// Top
	for(x=1; x<159; x+=2){
		SCALE3X(-1,1,0,160,dst[0],dst[1],dst[1],dst[0],dst[1],dst[1],dst[240],dst[241],dst[241]);
		dst+=2, src++;
		SCALE3X(-1,1,0,160,dst[0],dst[0],dst[1],dst[0],dst[0],dst[1],dst[240],dst[240],dst[241]);
		dst++, src++;
	}
	// Top-Right
	SCALE3X(-1,0,0,160,dst[0],dst[1],dst[1],dst[0],dst[1],dst[1],dst[240],dst[241],dst[241]);
	dst+=2, src++;

	for(y=1; y<142; y+=2){
		// left
		SCALE3X(0,1,-160,160,dst[0],dst[0],dst[1],dst[240],dst[240],dst[241],dst[240],dst[240],dst[241]);
		dst++, src++;
		// middle
		for(x=1; x<159; x+=2){
			SCALE3X(-1,1,-160,160,dst[0],dst[1],dst[1],dst[240],dst[241],dst[241],dst[240],dst[241],dst[241]);
			dst+=2, src++;
			SCALE3X(-1,1,-160,160,dst[0],dst[0],dst[1],dst[240],dst[240],dst[241],dst[240],dst[240],dst[241]);
			dst++, src++;
		}
		// right
		SCALE3X(-1,0,-160,160,dst[0],dst[1],dst[1],dst[240],dst[241],dst[241],dst[240],dst[241],dst[241]);
		dst+=2, src++;

		memcpy(base,buffer[0],480);
		memcpy(base+320,buffer[1],480);
		memcpy(base+640,buffer[2],480);
		dst=(un16*)buffer;
		base += 960;

		memset(buffer,0,480*3);
		// left
		SCALE3X(0,1,-160,160,dst[0],dst[0],dst[1],dst[0],dst[0],dst[1],dst[240],dst[240],dst[241]);
		dst++, src++;
		// middle
		for(x=1; x<159; x+=2){
			SCALE3X(-1,1,-160,160,dst[0],dst[1],dst[1],dst[0],dst[1],dst[1],dst[240],dst[241],dst[241]);
			dst+=2, src++;
			SCALE3X(-1,1,-160,160,dst[0],dst[0],dst[1],dst[0],dst[0],dst[1],dst[240],dst[240],dst[241]);
			dst++, src++;
		}
		// right
		SCALE3X(-1,0,-160,160,dst[0],dst[1],dst[1],dst[0],dst[1],dst[1],dst[240],dst[241],dst[241]);
		dst+=2, src++;
	}

	// left
	SCALE3X(0,1,-160,0,dst[0],dst[0],dst[1],dst[240],dst[240],dst[241],dst[240],dst[240],dst[241]);
	dst++, src++;
	// middle
	for(x=1; x<159; x+=2){
		SCALE3X(-1,1,-160,0,dst[0],dst[1],dst[1],dst[240],dst[241],dst[241],dst[240],dst[241],dst[241]);
		dst+=2, src++;
		SCALE3X(-1,1,-160,0,dst[0],dst[0],dst[1],dst[240],dst[240],dst[241],dst[240],dst[240],dst[241]);
		dst++, src++;
	}
	// right
	SCALE3X(-1,0,-160,0,dst[0],dst[1],dst[1],dst[240],dst[241],dst[241],dst[240],dst[241],dst[241]);
	dst+=2, src++;

	memcpy(base,buffer[0],480);
	memcpy(base+320,buffer[1],480);
	memcpy(base+640,buffer[2],480);
}
#endif

/****************************************************************/
/* Upscale from 160x144 to 320x240 */
void gb_upscale(uint32_t *to, uint32_t *from)
{
        uint32_t reg1, reg2, reg3, reg4;
        unsigned int x,y;
 
        /* Before:
         *    a b
         *    c d
         *    e f
         *
         * After (parenthesis = average):
         *    a      a      b      b
         *    (a,c)  (a,c)  (b,d)  (b,d)
         *    c      c      d      d
         *    (c,e)  (c,e)  (d,f)  (d,f)
         *    e      e      f      f
         */
 
        for (y=0; y < 240/5; y++) {
                for(x=0; x < 320/4; x++) {
                        prefetch(to+4, 1);
 
                        /* Read b-a */
                        reg2 = *from;
                        reg1 = reg2 & 0xffff0000;
                        reg1 |= reg1 >> 16;
 
                        /* Write b-b */
                        *(to+1) = reg1;
                        reg2 = reg2 & 0xffff;
                        reg2 |= reg2 << 16;
 
                        /* Write a-a */
                        *to = reg2;
 
                        /* Read d-c */
                        reg4 = *(from + 160/2);
                        reg3 = reg4 & 0xffff0000;
                        reg3 |= reg3 >> 16;
 
                        /* Write d-d */
                        *(to + 2*320/2 +1) = reg3;
                        reg4 = reg4 & 0xffff;
                        reg4 |= reg4 << 16;
 
                        /* Write c-c */
                        *(to + 2*320/2) = reg4;
 
                        /* Write (b,d)-(b,d) */
                        if (unlikely(reg1 != reg3))
                                reg1 = ((reg1 & 0xf7def7de) >> 1) + ((reg3 & 0xf7def7de) >> 1);
                        *(to + 320/2 +1) = reg1;
 
                        /* Write (a,c)-(a,c) */
                        if (unlikely(reg2 != reg4))
                                reg2 = ((reg2 & 0xf7def7de) >> 1) + ((reg4 & 0xf7def7de) >> 1);
                        *(to + 320/2) = reg2;
 
                        /* Read f-e */
                        reg2 = *(from++ + 2*160/2);
                        reg1 = reg2 & 0xffff0000;
                        reg1 |= reg1 >> 16;
 
                        /* Write f-f */
                        *(to + 4*320/2 +1) = reg1;
                        reg2 = reg2 & 0xffff;
                        reg2 |= reg2 << 16;
 
                        /* Write e-e */
                        *(to + 4*320/2) = reg2;
 
                        /* Write (d,f)-(d,f) */
                        if (unlikely(reg2 != reg4))
                                reg2 = ((reg2 & 0xf7def7de) >> 1) + ((reg4 & 0xf7def7de) >> 1);
                        *(to++ + 3*320/2) = reg2;
 
                        /* Write (c,e)-(c,e) */
                        if (unlikely(reg1 != reg3))
                                reg1 = ((reg1 & 0xf7def7de) >> 1) + ((reg3 & 0xf7def7de) >> 1);
                        *(to++ + 3*320/2) = reg1;
                }
 
                to += 4*320/2;
                from += 2*160/2;
        }
}
 
/* Upscale from 160x144 to 240x216 */
void ayla_scale15x(uint32_t *to, uint32_t *from)
{
        /* Before:
         *    a b c d
         *    e f g h
         *
         * After (parenthesis = average):
         *    a      (a,b)      b      c      (c,d)      d
         *    (a,e)  (a,b,e,f)  (b,f)  (c,g)  (c,d,g,h)  (d,h)
         *    e      (e,f)      f      g      (g,h)      h
         */
 
        uint32_t reg1, reg2, reg3, reg4, reg5;
        size_t x, y;
 
        for (y=0; y<216/3; y++) {
                for (x=0; x<240/6; x++) {
                        prefetch(to+4, 1);
 
                        /* Read b-a */
                        reg1 = *from;
                        reg5 = reg1 >> 16;
                        if (unlikely((reg1 & 0xffff) != reg5)) {
                                reg2 = (reg1 & 0xf7de0000) >> 1;
                                reg1 = (reg1 & 0xffff) + reg2 + ((reg1 & 0xf7de) << 15);
                        }
 
                        /* Write (a,b)-a */
                        *to = reg1;
 
                        /* Read f-e */
                        reg3 = *(from++ + 160/2);
                        reg2 = reg3 >> 16;
                        if (unlikely((reg3 & 0xffff) != reg2)) {
                                reg4 = (reg3 & 0xf7de0000) >> 1;
                                reg3 = (reg3 & 0xffff) + reg4 + ((reg3 & 0xf7de) << 15);
                        }
 
                        /* Write (e,f)-e */
                        *(to + 2*320/2) = reg3;
 
                        /* Write (a,b,e,f)-(a,e) */
                        if (unlikely(reg1 != reg3))
                                reg1 = ((reg1 & 0xf7def7de) >> 1) + ((reg3 & 0xf7def7de) >> 1);
                        *(to++ + 320/2) = reg1;
 
                        /* Read d-c */
                        reg1 = *from;
                        reg4 = reg1 << 16;
 
                        /* Write c-b */
                        reg5 |= reg4;
                        *to = reg5;
 
                        /* Read h-g */
                        reg3 = *(from++ + 160/2);
 
                        /* Write g-f */
                        reg2 |= (reg3 << 16);
                        *(to + 2*320/2) = reg2;
 
                        /* Write (c,g)-(b,f) */
                        if (unlikely(reg2 != reg5))
                                reg2 = ((reg5 & 0xf7def7de) >> 1) + ((reg2 & 0xf7def7de) >> 1);
                        *(to++ + 320/2) = reg2;
 
                        /* Write d-(c,d) */
                        if (unlikely((reg1 & 0xffff0000) != reg4)) {
                                reg2 = (reg1 & 0xf7def7de) >> 1;
                                reg1 = (reg1 & 0xffff0000) | ((reg2 + (reg2 >> 16)) & 0xffff);
                        }
                        *to = reg1;
 
                        /* Write h-(g,h) */
                        if (unlikely((reg3 & 0xffff) != reg3 >> 16)) {
                                reg2 = (reg3 & 0xf7def7de) >> 1;
                                reg3 = (reg3 & 0xffff0000) | ((reg2 + (reg2 >> 16)) & 0xffff);
                        }
                        *(to + 2*320/2) = reg3;
 
                        /* Write (d,h)-(c,d,g,h) */
                        if (unlikely(reg1 != reg3))
                                reg1 = ((reg1 & 0xf7def7de) >> 1) + ((reg3 & 0xf7def7de) >> 1);
                        *(to++ + 320/2) = reg1;
                }
 
                to += 2*360/2;
                from += 160/2;
        }
}

/*
 * Approximately bilinear scaler, 160x144 to 280x240
 *
 * Copyright (C) 2014 hi-ban, Nebuleon <nebuleon.fumika@gmail.com>
 *
 * This function and all auxiliary functions are free software; you can
 * redistribute them and/or modify them under the terms of the GNU Lesser
 * General Public License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * These functions are distributed in the hope that they will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

// Support math
#define Half(A) (((A) >> 1) & 0x7BEF)

// Error correction expressions to piece back the lower bits together
#define RestHalf(A) ((A) & 0x0821)

// Error correction expressions for halves
#define Corr1_1(A, B)     ((A) & (B) & 0x0821)

// Halves
#define Weight1_1(A, B)   (Half(A) + Half(B) + Corr1_1(A, B))

/* Upscales a 160x144 image to 280x240 using an approximate bilinear
 * resampling algorithm that only uses integer math.
 *
 * Input:
 *   src: A packed 160x144 pixel image. The pixel format of this image is
 *     RGB 565.
 * Output:
 *   dst: A packed 280x240 pixel image. The pixel format of this image is
 *     RGB 565.
 */

void upscale_160x144_to_280x240_bilinearish(uint32_t* dst, uint32_t* src)
{
	uint16_t* Src16 = (uint16_t*) src;
	uint16_t* Dst16 = (uint16_t*) dst;
	// There are 40 blocks of 4 pixels horizontally, and 48 of 3 vertically.
	// Each block of 4x3 becomes 7x5.
	uint32_t BlockX, BlockY;
	uint16_t* BlockSrc;
	uint16_t* BlockDst;
	for (BlockY = 0; BlockY < 48; BlockY++)
	{
		BlockSrc = Src16 + BlockY * 160 * 3;
		BlockDst = Dst16 + BlockY * 320 * 5;
		for (BlockX = 0; BlockX < 40; BlockX++)
		{
			/* Horizontally:
			 * Before(4):
			 * (a)(b)(c)(d)
			 * After(7):
			 * (a)(ab)(b)(bc)(c)(cd)(d)
			 *
			 * Vertically:
			 * Before(3): After(5):
			 * (a)       (a)
			 * (b)       (ab)
			 * (c)       (b)
			 *           (bc)
			 *           (c)

			 */

			// -- Row 1 --
			uint16_t  _1 = *(BlockSrc               );
			*(BlockDst               ) = _1;
			uint16_t  _2 = *(BlockSrc            + 1);
			*(BlockDst            + 1) = Weight1_1(_1, _2);
			*(BlockDst            + 2) = _2;
			uint16_t  _3 = *(BlockSrc            + 2);
			*(BlockDst            + 3) = Weight1_1(_2, _3);
			*(BlockDst            + 4) = _3;
			uint16_t  _4 = *(BlockSrc            + 3);
			*(BlockDst            + 5) = Weight1_1(_3, _4);
			*(BlockDst            + 6) = _4;

			// -- Row 2 --
			uint16_t _5 = *(BlockSrc + 160 *  1    );
			*(BlockDst + 320 *  1    ) = Weight1_1(_1, _5);
			uint16_t _6 = *(BlockSrc + 160 *  1 + 1);
			*(BlockDst + 320 *  1 + 1) = Weight1_1(Weight1_1(_1, _2), Weight1_1(_5, _6));
			*(BlockDst + 320 *  1 + 2) = Weight1_1(_2, _6);
			uint16_t _7 = *(BlockSrc + 160 *  1 + 2);
			*(BlockDst + 320 *  1 + 3) = Weight1_1(Weight1_1(_2, _3), Weight1_1(_6, _7));
			*(BlockDst + 320 *  1 + 4) = Weight1_1(_3, _7);
			uint16_t _8 = *(BlockSrc + 160 *  1 + 3);
			*(BlockDst + 320 *  1 + 5) = Weight1_1(Weight1_1(_3, _4), Weight1_1(_7, _8));
			*(BlockDst + 320 *  1 + 6) = Weight1_1(_4, _8);
			
			// -- Row 3 --
			*(BlockDst + 320 *  2    ) = _5;
			*(BlockDst + 320 *  2 + 1) = Weight1_1(_5, _6);
			*(BlockDst + 320 *  2 + 2) = _6;
			*(BlockDst + 320 *  2 + 3) = Weight1_1(_6, _7);
			*(BlockDst + 320 *  2 + 4) = _7;
			*(BlockDst + 320 *  2 + 5) = Weight1_1(_7, _8);
			*(BlockDst + 320 *  2 + 6) = _8;

			// -- Row 4 --
			uint16_t _9 = *(BlockSrc + 160 *  2    );
			*(BlockDst + 320 *  3    ) = Weight1_1(_5, _9);
			uint16_t _10 = *(BlockSrc + 160 *  2 + 1);
			*(BlockDst + 320 *  3 + 1) = Weight1_1(Weight1_1(_5, _6), Weight1_1(_9, _10));
			*(BlockDst + 320 *  3 + 2) = Weight1_1(_6, _10);
			uint16_t _11 = *(BlockSrc + 160 *  2 + 2);
			*(BlockDst + 320 *  3 + 3) = Weight1_1(Weight1_1(_6, _7), Weight1_1(_10, _11));
			*(BlockDst + 320 *  3 + 4) = Weight1_1(_7, _11);
			uint16_t _12 = *(BlockSrc + 160 *  2 + 3);
			*(BlockDst + 320 *  3 + 5) = Weight1_1(Weight1_1(_7, _8), Weight1_1(_11, _12));
			*(BlockDst + 320 *  3 + 6) = Weight1_1(_8, _12);

			// -- Row 5 --
			*(BlockDst + 320 *  4    ) = _9;
			*(BlockDst + 320 *  4 + 1) = Weight1_1(_9, _10);
			*(BlockDst + 320 *  4 + 2) = _10;
			*(BlockDst + 320 *  4 + 3) = Weight1_1(_10, _11);
			*(BlockDst + 320 *  4 + 4) = _11;
			*(BlockDst + 320 *  4 + 5) = Weight1_1(_11, _12);
			*(BlockDst + 320 *  4 + 6) = _12;

			BlockSrc += 4;
			BlockDst += 7;
		}
	}
}

/****************************************************************/

/*
** Assumes 16bpp.
** Assumes src pixels are in the same format as the dest pixels.
** Copies gameboy 160x144 to screen, can be used to copy to arbitary rectangle
** (for example, the center/centre of a 320x240 screen)
*/
void ohb_no_scale(){
    /* No scaling */
    /*
    **  TODO add frame support :-)
    **  Need a way to paint the first time (and after menu),
    **  e.g. tie into dirty flag for physical fb (vid_fb.dirty)
    */

#define NO_SCALE_OFFSET 11600 + (320 * 11) /* (320 - 160) / 2)  + 240*((240 - 144) / 2) == 80 + 48 == middle('ish) of screen */
    /* could make NO_SCALE_OFFSET a variable and count pixels until transparent is hit (or some other chroma-key color/colour) */
	un16 *src = (un16 *) fb.ptr; /* NOTE this needs to be byte aligned! */
	un16 *dst = (un16*) vid_fb.ptr + NO_SCALE_OFFSET;
	int x=0, y=0;

	for(y=0; y<144; y++){
		memcpy(dst, src, 2*160);
		src += 160;
		dst += 320;
	}
}

void ohb_hardware_15x_scale(){
    /* Hardware scaling */

	un16 *src = (un16 *) fb.ptr;
	un16 *dst = (un16*) vid_fb.ptr + 1688;
	int x=0, y=0;

	for(y=0; y<144; y++){
		memcpy(dst, src, 2*160);
		src += 160;
		dst += 208;
	}
}

void ohb_hardware_1666x_scale(){
    /* Hardware scaling */

	un16 *src = (un16 *) fb.ptr;
	un16 *dst = (un16*) vid_fb.ptr + 16;
	int x=0, y=0;

	for(y=0; y<144; y++){
		memcpy(dst, src, 2*160);
		src += 160;
		dst += 192;
	}
}

void ohb_hardware_fullscreen_scale(){
    /* Hardware scaling */

	un16 *src = (un16 *) fb.ptr;
	un16 *dst = (un16*) vid_fb.ptr + 0;
	int x=0, y=0;

	for(y=0; y<144; y++){
		memcpy(dst, src, 2*160);
		src += 160;
		dst += 160;
	}
}

/*
** Assumes 16bpp. RGB565
** Assumes src pixels are in the same format as the dest pixels.
** Fast, full screen 320x240 (only), no aspect ratio preservation
*/
void ohb_ayla_dingoo_scale(){
    /* Full screen scaling (i.e. does NOT preserve aspect ratio) */

	un16 *src = (un16 *) fb.ptr;
	un16 *dst = (un16*)vid_fb.ptr /* + 3880 */;
	int x,y;

	gb_upscale((uint32_t *) dst, (uint32_t *) src);
}

void ohb_ayla_scale15x(){
    /* Ayla's 1.5x scaler */

	un16 *src = (un16 *) fb.ptr;
	un16 *dst = (un16*)vid_fb.ptr + 3880;
	int x,y;

	ayla_scale15x((uint32_t *) dst, (uint32_t *) src);
}

void ohb_scale_aspect(){
    /* bilinearish scaler */

	un16 *src = (un16 *) fb.ptr;
	un16 *dst = (un16*)vid_fb.ptr + 20;
	int x,y;

	upscale_160x144_to_280x240_bilinearish((uint32_t *) dst, (uint32_t *) src);
}

void scaler_init(int scaler_number){
	switch (upscaler){
		case 2: /* 1.5 scaler with some smoothing ala scale3x / scale2x */
			/* RGB343 */
			fb.cc[0].r = screen->format->Rloss+2;
			fb.cc[1].r = screen->format->Gloss+2;
			fb.cc[2].r = screen->format->Bloss+2;
			break;
		case 1: /* Ayla's 1.5x scaler */
		case 3: /* Bilinearish 1.666x scaler */
		case 4: /* Ayla full screen 320x240 (no aspect ration preservation) */
#ifdef GCWZERO
		case 5: /* Hardware 1.5x scaler */
		case 6: /* Hardware 1.666x scaler */
		case 7: /* Hardware Fullscreen scaler */
#endif
		case 0: /* no scale, that is, native */
		default:
			/* RGB565 */
			fb.cc[0].r = screen->format->Rloss;
			fb.cc[1].r = screen->format->Gloss;
			fb.cc[2].r = screen->format->Bloss;
			//ohb_ayla_dingoo_scale();
			break;
	}
	vid_fb.dirty = 1;
	pal_dirty();
}

void vid_preinit(){
}

void vid_close(){

if(bmpenabled == 1)
	SDL_FreeSurface(bordersf);
}

void vid_settitle(char *title){
}
#ifdef WIZ
int ohb_updatecpu(int mhz){

	static int current_mhz = 0, def_v=0;
	volatile unsigned int *memregl;
	int mdiv, pdiv, sdiv = 0;
	int memdev, v=0;

	if(mhz==current_mhz) return 1;

	if(mhz){
		pdiv = 9;
		mdiv = (mhz * pdiv) / 27;
		if (mdiv & ~0x3ff) return 0;
		v = (pdiv<<18) | (mdiv<<8) | sdiv;
	}

	memdev = open("/dev/mem", O_RDWR);
	memregl	= mmap(0, 0x20000, PROT_READ|PROT_WRITE, MAP_SHARED, memdev, 0xc0000000);

	if(!def_v){
		def_v = memregl[0xf004>>2];
	} else if(!mhz){
		v = def_v;
	}

	memregl[0xf004>>2] = v;
	memregl[0xf07c>>2] |= 0x8000;

	munmap((void *)memregl, 0x20000);
	close(memdev);

	current_mhz = mhz;

	return v;
}
#else
int ohb_updatecpu(int mhz){
	return 1;
}
#endif

void vid_begin(){
	ohb_updatecpu(cpu_speed);
	if(frameskip>=0)
		vid_fb.enabled = framecounter==0;
	fb.enabled = vid_fb.enabled;
	fb.ptr = vid_frame;
	
		/* add border to display if enabled and not using fullscreen
	   this should be integrated with vid_fb.dirty probably... */
	if(vid_fb.first_paint && bmpenabled == 1){
	
		SDL_Rect rect;
		rect.x = 0;
		rect.y = 0;
		rect.w = screen->w;
		rect.h = screen->h;

		uint32_t colour = my_color; // (BGR format bbbbbbbb gggggggg rrrrrrrr)
		uint8_t r,g,b;
		b = (colour & 0xFF0000) >> 16; //Remove G and R, shift it left by two bytes
		g = (colour & 0x00FF00) >> 8; //Remove B and R, shift it left by one byte
		r = (colour & 0x0000FF); //Remove B and G, this is already on the left.
		printf("r: %i g: %i b: %i\n", r, g, b);

		uint16_t hexcolor;
		hexcolor = ((r & 0xf8) << 8) | //mask out three bits, shift em to their place (rrrrr 000000 00000)
				((g & 0xfc) << 3) | //mask out two bits, move em to their place (rrrrr gggggg 00000)
				((b & 0xf8) >> 3); //mask out three bits, shift em to their plae (rrrrr gggggg bbbbb)
		SDL_FillRect(screen, &rect, hexcolor);
		#if defined(DINGOO_OPENDINGUX)
		SDL_Flip(screen);                                        /*Fix for flickering borders with double buffer*/
		SDL_FillRect(screen, &rect, hexcolor);                   /*Paints the border color two times*/
		#endif /*DINGOO_OPENDINGUX*/
	
		vid_init;
		if (!upscaler) {
			SDL_Rect border1;
			border1.x=0;
			border1.y=0;
			border1.w=320;
			border1.h=240;
			SDL_BlitSurface(bordersf, &border1, screen, NULL);
			#if defined(DINGOO_OPENDINGUX)
			SDL_Flip(screen);                                    /*Fix for flickering borders with double buffer*/
			SDL_BlitSurface(bordersf, &border1, screen, NULL);   /*Paints the border image two times*/
			#endif /*DINGOO_OPENDINGUX*/
		}
		if ((upscaler > 0) && (upscaler < 3)) {
			SDL_Rect border1;
			border1.x=0;
			border1.y=240;
			border1.w=320;
			border1.h=240;
			SDL_BlitSurface(bordersf, &border1, screen, NULL);
			#if defined(DINGOO_OPENDINGUX)
			SDL_Flip(screen);                                    /*Fix for flickering borders with double buffer*/
			SDL_BlitSurface(bordersf, &border1, screen, NULL);   /*Paints the border image two times*/
			#endif /*DINGOO_OPENDINGUX*/
		}
		if (upscaler == 3) {
			SDL_Rect border1;
			border1.x=0;
			border1.y=0;
			border1.w=320;
			border1.h=240;
			SDL_BlitSurface(bordersf, &border1, screen, NULL);
			#if defined(DINGOO_OPENDINGUX)
			SDL_Flip(screen);                                    /*Fix for flickering borders with double buffer*/
			SDL_BlitSurface(bordersf, &border1, screen, NULL);   /*Paints the border image two times*/
			#endif /*DINGOO_OPENDINGUX*/
		}
#ifdef GCWZERO
		if (upscaler == 5) {
			FILE* aspect_ratio_file = fopen("/sys/devices/platform/jz-lcd.0/keep_aspect_ratio", "w");
			if (aspect_ratio_file)
			{ 
				fwrite("1", 1, 1, aspect_ratio_file);
				fclose(aspect_ratio_file);
			}
			SDL_Rect border1;
			border1.x=56;
			border1.y=39;
			border1.w=208;
			border1.h=160;
			SDL_BlitSurface(bordersf, &border1, screen, NULL);
			#if defined(DINGOO_OPENDINGUX)
			SDL_Flip(screen);                                    /*Fix for flickering borders with double buffer*/
			SDL_BlitSurface(bordersf, &border1, screen, NULL);   /*Paints the border image two times*/
			#endif /*DINGOO_OPENDINGUX*/
		}
		if (upscaler == 6) {
			FILE* aspect_ratio_file = fopen("/sys/devices/platform/jz-lcd.0/keep_aspect_ratio", "w");
			if (aspect_ratio_file)
			{ 
				fwrite("1", 1, 1, aspect_ratio_file);
				fclose(aspect_ratio_file);
			}
			SDL_Rect border1;
			border1.x=64;
			border1.y=47;
			border1.w=192;
			border1.h=144;
			SDL_BlitSurface(bordersf, &border1, screen, NULL);
			#if defined(DINGOO_OPENDINGUX)
			SDL_Flip(screen);                                    /*Fix for flickering borders with double buffer*/
			SDL_BlitSurface(bordersf, &border1, screen, NULL);   /*Paints the border image two times*/
			#endif /*DINGOO_OPENDINGUX*/
		}
		if (upscaler == 7) {
			FILE* aspect_ratio_file = fopen("/sys/devices/platform/jz-lcd.0/keep_aspect_ratio", "w");
			if (aspect_ratio_file)
			{ 
				fwrite("0", 1, 1, aspect_ratio_file);
				fclose(aspect_ratio_file);
			}
			SDL_Rect border1;
			border1.x=0;
			border1.y=0;
			border1.w=320;
			border1.h=240;
			SDL_BlitSurface(bordersf, &border1, screen, NULL);
			#if defined(DINGOO_OPENDINGUX)
			SDL_Flip(screen);                                    /*Fix for flickering borders with double buffer*/
			SDL_BlitSurface(bordersf, &border1, screen, NULL);   /*Paints the border image two times*/
			#endif /*DINGOO_OPENDINGUX*/
		}
#endif
		vid_fb.first_paint = 0;
		vid_fb.dirty = 0;
	} 
	#if defined(DINGOO_OPENDINGUX)
	if(vid_fb.first_paint && bmpenabled == 0){            /*This also fixes flickering screen with double buffer when borders are disabled*/
		SDL_Rect rect;                                           /*Paints the background in black color when there is no border*/
		rect.x = 0;
		rect.y = 0;
		rect.w = screen->w;
		rect.h = screen->h;
		SDL_FillRect(screen, &rect, 0x000000);
	}
	#endif /*DINGOO_OPENDINGUX*/
	if(vid_fb.first_paint && bmpenabled == 2){            /*Fill the borders with the background color*/
		SDL_Rect rect;
		rect.x = 0;
		rect.y = 0;
		rect.w = screen->w;
		rect.h = screen->h;

		uint32_t colour = my_color; // (BGR format bbbbbbbb gggggggg rrrrrrrr)
		uint8_t r,g,b;
		b = (colour & 0xFF0000) >> 16; //Remove G and R, shift it left by two bytes
		g = (colour & 0x00FF00) >> 8; //Remove B and R, shift it left by one byte
		r = (colour & 0x0000FF); //Remove B and G, this is already on the left.
		printf("r: %i g: %i b: %i\n", r, g, b);

		uint16_t hexcolor;
		hexcolor = ((r & 0xf8) << 8) | //mask out three bits, shift em to their place (rrrrr 000000 00000)
				((g & 0xfc) << 3) | //mask out two bits, move em to their place (rrrrr gggggg 00000)
				((b & 0xf8) >> 3); //mask out three bits, shift em to their plae (rrrrr gggggg bbbbb)
		SDL_FillRect(screen, &rect, hexcolor);
	}
}

void osd_volume(){
    int rounded=0; /* NOTE TODO FIXME unused */
	static const char *volstr = "Volume";
	static int x = 0;
	int w;
	if(!x) x = font_textwidth(font,volstr)+4;

	w = (pcm_soft_volume * (screen->w-x-4)) >> 8;

	osd_cls(0,screen->h-font->height-4,screen->w,font->height+4);
	osd_drawtext(font,volstr,2,screen->h+font->descent-2,gui_maprgb(0xFF,0xFF,0xFF));
	osd_drawrect(2+x,screen->h-font->height-2,w,font->height,gui_maprgb(0xFF,0xFF,0xFF), rounded);
}

void VideoEnterGame() {
#ifdef GCWZERO
	if (upscaler < 5) {
		screen = SDL_SetVideoMode(320, 240, 16, SDL_HWSURFACE | SDL_DOUBLEBUF);
	}
	if (upscaler == 5) {
		screen = SDL_SetVideoMode(208, 160, 16, SDL_HWSURFACE | SDL_DOUBLEBUF);
	}
	if (upscaler == 6) {
		screen = SDL_SetVideoMode(192, 144, 16, SDL_HWSURFACE | SDL_DOUBLEBUF);
	}
	if (upscaler == 7) {
		screen = SDL_SetVideoMode(160, 144, 16, SDL_HWSURFACE | SDL_DOUBLEBUF);
	}
#endif
}

void VideoExitGame() {
#ifdef GCWZERO
	screen = SDL_SetVideoMode(320, 240, 16, SDL_HWSURFACE | SDL_DOUBLEBUF);
#endif
}

void vid_end() {
	if(fb.enabled){
		vid_fb.ptr = screen->pixels;
		SDL_LockSurface(screen);

		if(vid_fb.dirty){
			memset(vid_fb.ptr,0,vid_fb.pitch*vid_fb.h);
			vid_fb.dirty = 0;
		}

		switch (upscaler){
			case 1:
				ohb_ayla_scale15x();
				break;
			case 2:
				ohb_scale3x();
				break;
			case 3:
				ohb_scale_aspect();
				break;
			case 4:
				ohb_ayla_dingoo_scale();
				break;
#ifdef GCWZERO
			case 5:
				ohb_hardware_15x_scale();
				break;
			case 6:
				ohb_hardware_1666x_scale();
				break;
			case 7:
				ohb_hardware_fullscreen_scale();
				break;
#endif
			case 0:
			default:
				ohb_no_scale();
				break;
		}

		if(osd_persist || dvolume){
			osd_volume();
			vid_fb.dirty=1;
			if(osd_persist) osd_persist--;
		}
		SDL_UnlockSurface(screen);
        
        /* 
        ** NOTE Oh Boy now has the option of 2 font systems...
        ** 1) the original Oh Boy ttf approach - gui_drawtext()
        ** 2) SFont (part of SDL gnuboy)
        ** Oh Boy uses SFont (bitmap approach) from gnuboy for FPS indicator
        ** Rest of the GUI (menu) can either use TTF or SFont.
        ** TODO start using SFont in gui_drawtext(), and expose gui_drawtext() to this module.
        */
        if (sdl_showfps)
        {
            fps_current_time = SDL_GetTicks();
            fps_current_count++;
            if (sdl_showfps == 1)
            {
                snprintf(fps_str, 19, "%02d", fps_last_count);
				myrect.w = SFont_TextWidth(fps_font, fps_str);
                SDL_FillRect(screen, &myrect, 0 );
            }

            SFont_Write(screen, fps_font, 0,0, fps_str);
            if ( fps_current_time - fps_last_time >= 1000 )
            {
                /* reset fps count every second (not every frame) */
                fps_last_count = fps_current_count;
                fps_current_count = 0;
                fps_last_time = fps_current_time;
            }
        }

		SDL_Flip(screen);
	}
	framecounter++;
	if(framecounter>frameskip) framecounter = 0;

}

static void audio_callback(void *d, byte *stream, int len) {
	if(pcm_buffered==0) return;
	memcpy(stream, pcm_tail, len);
	pcm_tail += pcm.len;
	if(pcm_tail == pcm_buffer+pcm_bufferlen) pcm_tail = pcm_buffer;
	pcm_buffered -= pcm.len;
}

#ifndef GNUBOY_DISABLE_SDL_SOUND
#ifndef OHBOY_DISABLE_SDL_SOUND
void pcm_init(){
	SDL_AudioSpec as;
	SDL_InitSubSystem(SDL_INIT_AUDIO);

	as.freq = PCM_SAMPLERATE;
	as.format = AUDIO_S16;
	as.channels = 2;
	as.samples = PCM_FRAME;
	as.callback = audio_callback;
	as.userdata = 0;

	if (SDL_OpenAudio(&as, 0))
		return;

	pcm.hz = as.freq;
	pcm.stereo = as.channels - 1;
	pcm.len = PCM_FRAME<<pcm.stereo;
	pcm.buf = pcm_frame;
	pcm.pos = 0;
	memset(pcm.buf, 0, pcm.len);

	pcm_head = pcm_buffer;
	pcm_tail = pcm_buffer;
	pcm_bufferlen = PCM_BUFFER << pcm.stereo;
	pcm_buffered = 0;

	SDL_PauseAudio(0);
}

int pcm_submit(){
	static int lastbuffer = 0;
	byte *src;
	n16 sample;
#define DEBUG_DISABLE_SOUND
#undef DEBUG_DISABLE_SOUND
#ifdef DEBUG_DISABLE_SOUND
return 0; /* no sound */
#endif /* DEBUG_DISABLE_SOUND */
    
	if(pcm.pos<pcm.len)
		return 1;

	if (pcm_buffered>=1) while(pcm_buffered == pcm_bufferlen);

	src = pcm.buf;

	pcm_soft_volume += dvolume;
	if(pcm_soft_volume < 0)
		pcm_soft_volume = 0;
	else if(pcm_soft_volume >256)
		pcm_soft_volume = 256;

	while(pcm.pos){
		sample = (*src++) - 128;
		sample = sample * pcm_soft_volume;
		*(pcm_head++) = sample;
		pcm.pos--;
	}
	if(pcm_head == pcm_buffer + pcm_bufferlen)
		pcm_head = pcm_buffer;

	pcm_buffered += pcm.len;
	if(frameskip==-1){
		if(pcm_buffered<lastbuffer)
			vid_fb.enabled = 0;
		else if(pcm_buffered==pcm_bufferlen || pcm_buffered==0)
			vid_fb.enabled = 1;
	}
	lastbuffer = pcm_buffered;

	return 1;
}

void pcm_close() {
	SDL_CloseAudio();
}
#endif /* OHBOY_DISABLE_SDL_SOUND */
#endif /* GNUBOY_DISABLE_SDL_SOUND */


void *sys_timer()
{
	Uint32 *tv;

	tv = malloc(sizeof *tv);
	*tv = SDL_GetTicks() * 1000;
	return tv;
}

int sys_elapsed(void *in_ptr)
{
	Uint32 *cl;
	Uint32 now;
	Uint32 usecs;

	cl = (Uint32 *) in_ptr;
	now = SDL_GetTicks() * 1000;
	usecs = now - *cl;
	*cl = now;
	return (int) usecs;
}


void sys_sleep(int us)
{
	if(us>0) SDL_Delay(us/1000);
}

void sys_sanitize(char *s)
{
    /*
    ** used to be used to convert Windows style path\file into
    ** unix style path/file as gnuboy code then assumed unix
    ** Now a NOOP
    */
}

void sys_initpath(char *exe)
{
	char *buf, *home, *p;

	home = strdup(exe);
	sys_sanitize(home);
	p = strrchr(home, DIRSEP_CHAR);
	if (p) *p = 0;
	else
	{
		buf = ".";
		rc_setvar("rcpath", rcv_string, &buf);
		rc_setvar("savedir", rcv_string, &buf);
		return;
	}
	buf = malloc(strlen(home) + 8);
	sprintf(buf, ".;%s%s", home, DIRSEP);
	rc_setvar("rcpath", rcv_string, &buf);
	sprintf(buf, ".", home);
	rc_setvar("savedir", rcv_string, &buf);
	free(buf);
}

void sys_checkdir(char *path, int wr) {
}

static int mapscancode(SDLKey sym)
{
	/* this could be faster:  */
	/*  build keymap as int keymap[256], then ``return keymap[sym]'' */

	int i;
	for (i = 0; keymap[i][0]; i++)
		if (keymap[i][0] == sym)
			return keymap[i][1];
	if (sym >= '0' && sym <= '9')
		return sym;
	if (sym >= 'a' && sym <= 'z')
		return sym;
	return 0;
}

void ev_poll()
{
	event_t ev;
	SDL_Event event;
	int axisval;

#ifdef GP2X
	while (GP2X_PollEvent(&event))
#else
	while (SDL_PollEvent(&event))
#endif
	{
		switch(event.type)
		{
		case SDL_ACTIVEEVENT:
			if (event.active.state == SDL_APPACTIVE)
				fb.enabled = event.active.gain;
			break;
		case SDL_KEYDOWN:
			if(event.key.keysym.sym==SDLK_EQUALS){
				dvolume = 1;
			} else if(event.key.keysym.sym==SDLK_MINUS){
				dvolume = - 1;
#ifdef DINGOO_NATIVE
			} else if(event.key.keysym.sym==SDLK_PAUSE){
				dvolume = 0;
				osd_persist = 0;
				hw.pad = 0;
				menu();
			}
#endif
#ifdef DINGOO_OPENDINGUX
#ifdef GCWZERO
			} else if(event.key.keysym.sym==SDLK_HOME){
				dvolume = 0;
				osd_persist = 0;
				hw.pad = 0;
				VideoExitGame();
				paint_menu_bg();
				menu();
				VideoEnterGame();
			} else if (alt_menu_combo == 1){
				if((event.key.keysym.sym==SDLK_ESCAPE) || (event.key.keysym.sym==SDLK_RETURN)){
					Uint8 *keystate = SDL_GetKeyState(NULL);
					if ( keystate[SDLK_RETURN] && keystate[SDLK_ESCAPE] ){
						dvolume = 0;
						osd_persist = 0;
						hw.pad = 0;
						VideoExitGame();
						paint_menu_bg();
						menu();
						VideoEnterGame();
					}
				}
			}
#else
			} else if(event.key.keysym.sym==SDLK_TAB){
				Uint8 *keystate = SDL_GetKeyState(NULL);
				if ( keystate[SDLK_TAB] && keystate[SDLK_BACKSPACE] ){
					dvolume = 0;
					osd_persist = 0;
					hw.pad = 0;
					menu();
				}
			} else if(event.key.keysym.sym==SDLK_BACKSPACE){
				Uint8 *keystate = SDL_GetKeyState(NULL);
				if ( keystate[SDLK_TAB] && keystate[SDLK_BACKSPACE] ){
					dvolume = 0;
					osd_persist = 0;
					hw.pad = 0;
					menu();
				}
			}
#endif /*GCWZERO*/
#endif /*DINGOO_OPENDINGUX*/
#ifndef DINGOO_BUILD
			} else if(event.key.keysym.sym==SDLK_ESCAPE){
				dvolume = 0;
				osd_persist = 0;
				hw.pad = 0;
				menu();
			}
#endif
			ev.type = EV_PRESS;
			ev.code = mapscancode(event.key.keysym.sym);
			ev_postevent(&ev);
			break;
		case SDL_KEYUP:
			if(event.key.keysym.sym==SDLK_EQUALS){
				dvolume = 0;
				osd_persist = 60;
			} else if(event.key.keysym.sym==SDLK_MINUS){
				//TakeScreenShot((SDL_Surface *) NULL, (char *) NULL); /* this works in game and allows volume bar to be included in the screenshot (but takes screen shot every time volume is lowered). */
				dvolume = 0;
				osd_persist = 60;
			} else if (event.key.keysym.sym == SCREENSHOT_SDL_KEY){
				TakeScreenShot((SDL_Surface *) NULL, (char *) NULL);
			}
			ev.type = EV_RELEASE;
			ev.code = mapscancode(event.key.keysym.sym);
			ev_postevent(&ev);
			break;
		case SDL_JOYAXISMOTION:
			switch (event.jaxis.axis)
			{
			case 0: /* X axis */
				axisval = event.jaxis.value;
				if (axisval > joy_commit_range)
				{
					if (Xstatus==2) break;

					if (Xstatus==0)
					{
						ev.type = EV_RELEASE;
						ev.code = K_JOYLEFT;
        			  		ev_postevent(&ev);
					}

					ev.type = EV_PRESS;
					ev.code = K_JOYRIGHT;
					ev_postevent(&ev);
					Xstatus=2;
					break;
				}

				if (axisval < -(joy_commit_range))
				{
					if (Xstatus==0) break;

					if (Xstatus==2)
					{
						ev.type = EV_RELEASE;
						ev.code = K_JOYRIGHT;
						ev_postevent(&ev);
					}

					ev.type = EV_PRESS;
					ev.code = K_JOYLEFT;
					ev_postevent(&ev);
					Xstatus=0;
					break;
				}

				/* if control reaches here, the axis is centered,
				 * so just send a release signal if necisary */

				if (Xstatus==2)
				{
					ev.type = EV_RELEASE;
					ev.code = K_JOYRIGHT;
					ev_postevent(&ev);
				}

				if (Xstatus==0)
				{
					ev.type = EV_RELEASE;
					ev.code = K_JOYLEFT;
					ev_postevent(&ev);
				}
				Xstatus=1;
				break;

			case 1: /* Y axis*/
				axisval = event.jaxis.value;
				if (axisval > joy_commit_range)
				{
					if (Ystatus==2) break;

					if (Ystatus==0)
					{
						ev.type = EV_RELEASE;
						ev.code = K_JOYUP;
						ev_postevent(&ev);
					}

					ev.type = EV_PRESS;
					ev.code = K_JOYDOWN;
					ev_postevent(&ev);
					Ystatus=2;
					break;
				}

				if (axisval < -joy_commit_range)
				{
					if (Ystatus==0) break;

					if (Ystatus==2)
					{
						ev.type = EV_RELEASE;
						ev.code = K_JOYDOWN;
						ev_postevent(&ev);
					}

					ev.type = EV_PRESS;
					ev.code = K_JOYUP;
					ev_postevent(&ev);
					Ystatus=0;
					break;
				}

				/* if control reaches here, the axis is centered,
				 * so just send a release signal if necisary */

				if (Ystatus==2)
				{
					ev.type = EV_RELEASE;
					ev.code = K_JOYDOWN;
					ev_postevent(&ev);
				}

				if (Ystatus==0)
				{
					ev.type = EV_RELEASE;
					ev.code = K_JOYUP;
					ev_postevent(&ev);
				}
				Ystatus=1;
				break;
			}
			break;
		case SDL_JOYBUTTONUP:
			if(event.jbutton.button==16){
				dvolume = 0;
				osd_persist = 60;
			} else if(event.jbutton.button==17){
				dvolume = 0;
				osd_persist = 60;
			}
			if (event.jbutton.button>15) break;
			ev.type = EV_RELEASE;
			ev.code = K_JOY0 + event.jbutton.button;
			ev_postevent(&ev);
			break;
		case SDL_JOYBUTTONDOWN:
			if(event.jbutton.button==16){
				dvolume = 1;
			} else if(event.jbutton.button==17){
				dvolume = - 1;
			} else if(event.jbutton.button==6){
				dvolume = 0;
				osd_persist = 0;
				hw.pad = 0;
				menu();
			}
			if (event.jbutton.button>15) break;
			ev.type = EV_PRESS;
			ev.code = K_JOY0+event.jbutton.button;
			ev_postevent(&ev);
			break;
		case SDL_QUIT:
			exit(1);
			break;
		default:
			break;
		}
	}
}

static int bad_signals[] =
{
	/* These are all standard, so no need to #ifdef them... */
	SIGINT, SIGSEGV, SIGTERM, SIGFPE, SIGABRT, SIGILL,
#ifdef SIGQUIT
	SIGQUIT,
#endif
#ifdef SIGPIPE
	SIGPIPE,
#endif
	0
};

static void fatalsignal(int s)
{
	die("Signal %d\n", s);
}

static void catch_signals()
{
	int i;
	for (i = 0; bad_signals[i]; i++)
		signal(bad_signals[i], fatalsignal);
}

static void shutdown()
{
	ohb_updatecpu(0);
	dialog_close();
	vid_close();
	pcm_close();
	SDL_Quit();
	fb.enabled = 0;
}


void ohb_loadrom(char *rom){
	char *base, *save, *ext=0, *tok;
	sys_sanitize(rom);

	save = strdup(rom);
	base = strtok(save, DIRSEP);
	while(tok = strtok(NULL, DIRSEP))
		base = tok;
	tok = base;
	while(*tok){
		if(*tok == '.') ext = tok;
		tok++;
	}
	if(ext)
		*ext = 0;

	rc_setvar("savename", rcv_string, &base);
	free(save);

	loader_init(rom);
	emu_reset();
	vid_init();
	scaler_init(0);
}

#ifdef DINGOO_BUILD
/*
** basically a dirname() implementation
*/
char     exe_path[256];
char* exe_path_init(const char* inPath) {
	uintptr_t i, j;
	for(i = 0, j = 0; inPath[i] != '\0'; i++) {
		if((inPath[i] == '\\') || (inPath[i] == '/'))
			j = i + 1;
	}
	strncpy(exe_path, inPath, j);
	exe_path[j] = '\0';
	return exe_path;
}
#endif /* DINGOO_BUILD */

int main(int argc, char *argv[]){
	FILE *config;
	char *rom=NULL;
	int x, y;
	char *cpu;
	char *tmp_buf=NULL;

	SDL_Init(SDL_INIT_VIDEO|SDL_INIT_JOYSTICK);
#ifdef WIZ
	screen = WIZ_SetVideoMode(320, 240, 16, SDL_SWSURFACE);
#else
#ifdef DINGOO_OPENDINGUX
	screen = SDL_SetVideoMode(320, 240, 16, SDL_HWSURFACE | SDL_DOUBLEBUF);
#else
	screen = SDL_SetVideoMode(320, 240, 16, SDL_SWSURFACE);
#endif /* DINGOO_OPENDINGUX */
#endif /* WIZ */
	SDL_ShowCursor(0);

	sdl_joy = SDL_JoystickOpen(0);
	SDL_JoystickEventState(SDL_ENABLE);
	
	int my_color;

    /* Menu SFont start */
    /*
    font_bitmap_surface = get_free_universal_14_1bpp_font()
    menu_font = SFont_InitFont(font_bitmap_surface);
    menu_font_bitmap_surface = get_free_universal_14_1bpp_font();
    */
    
    /*
    menu_font_bitmap_surface = SDL_LoadBMP("FreeUniversal-Regular_14.bmp");
    menu_font_bitmap_surface = SDL_LoadBMP("14P_Copperplate_Blue.bmp");// too wide (and maybe tall?)
    menu_font_bitmap_surface = SDL_LoadBMP("14P_Arial_Plain_Red.bmp"); // too big
    menu_font_bitmap_surface = SDL_LoadBMP("smallstone.bmp"); // good size, not very clear (colours?)
    */

#ifdef OHBOY_USE_SDL_IMAGE
    if (!menu_font_bitmap_surface)
    {
	#ifdef DINGOO_SIM
		menu_font_bitmap_surface = IMG_Load("a:"DIRSEP"ohboy"DIRSEP"etc"DIRSEP SFONT_NAME ".png");
	#else
		menu_font_bitmap_surface = IMG_Load("etc" DIRSEP SFONT_NAME ".png");
	#endif /* DINGOO_SIM */
    }
#endif /* OHBOY_USE_SDL_IMAGE */
    if (!menu_font_bitmap_surface)
    {
	#ifdef DINGOO_SIM
		menu_font_bitmap_surface = SDL_LoadBMP("a:"DIRSEP"ohboy"DIRSEP"etc"DIRSEP SFONT_NAME ".bmp");
	#else
        menu_font_bitmap_surface = SDL_LoadBMP("etc" DIRSEP SFONT_NAME ".bmp");
	#endif /* DINGOO_SIM */
    }

    if (!menu_font_bitmap_surface)
    {
        /* use internal font */
        /*
        menu_font_bitmap_surface = get_font14px_font();
        */
        menu_font_bitmap_surface = get_pressstart_font();
        /*
        SDL_SaveBMP(menu_font_bitmap_surface, "menu_font_bitmap_surface.bmp");
        */
    }
    menu_font = SFont_InitFont(menu_font_bitmap_surface);
    if(!menu_font)
    {
        die("An error occured while setting up menu font.");
    }
    /* Menu SFont start */
    
    /* fps init - start */
    fps_font_bitmap_surface = get_default_data_font();
    /*
    SDL_SaveBMP(fps_font_bitmap_surface, "fps_font_bitmap_surface.bmp");
    */
    fps_font = SFont_InitFont(fps_font_bitmap_surface);
    if(!fps_font)
    {
        die("An error occured while setting up FPS font.");
    }
    myrect.x = 0;
    myrect.y = 0;
    myrect.w = 320;
    myrect.h = SFont_TextHeight(fps_font);
    /* fps init - end */
    
	font = font_load("etc" DIRSEP FONT_NAME, 0, FONT_SIZE);
	if(!dialog_init(font,gui_maprgb(255,255,255)))
		die("GUI: Could not initialise GUI (maybe missing font file)\n");
    

	init_exports();

	/* Start: gnuboy default settings */
    /* do not bind esc as menu grabs it...
	rc_command("bind esc quit");
    */
	rc_command("bind up +up");
	rc_command("bind down +down");
	rc_command("bind left +left");
	rc_command("bind right +right");
	rc_command("bind d +a");
	rc_command("bind s +b");
	rc_command("bind enter +start");
	rc_command("bind space +select");
	rc_command("bind tab +select");
	rc_command("bind joyup +up");
	rc_command("bind joydown +down");
	rc_command("bind joyleft +left");
	rc_command("bind joyright +right");
	rc_command("bind joy0 +b");
	rc_command("bind joy1 +a");
	rc_command("bind joy2 +select");
	rc_command("bind joy3 +start");
	rc_command("bind 1 \"set saveslot 1\"");
	rc_command("bind 2 \"set saveslot 2\"");
	rc_command("bind 3 \"set saveslot 3\"");
	rc_command("bind 4 \"set saveslot 4\"");
	rc_command("bind 5 \"set saveslot 5\"");
	rc_command("bind 6 \"set saveslot 6\"");
	rc_command("bind 7 \"set saveslot 7\"");
	rc_command("bind 8 \"set saveslot 8\"");
	rc_command("bind 9 \"set saveslot 9\"");
	rc_command("bind 0 \"set saveslot 0\"");
	rc_command("bind ins savestate");
	rc_command("bind del loadstate");
	/* End: gnuboy default settings */

#ifdef DINGOO_SIM
	rc_command("set savedir \"a:\\ohboy\\saves\\\"");
#else	
#ifdef DINGOO_OPENDINGUX
	static char *svdir;
	svdir = malloc(strlen(getenv("HOME")) + 36);
	sprintf(svdir, "set savedir %s/.ohboy/saves/", getenv("HOME"));
	rc_command(svdir);
	free (svdir);
#else	
	rc_command("set savedir saves");
#endif /* DINGOO_OPENDINGUX */
#endif /* DINGOO_SIM */
	rc_command("set stereo true");

#ifdef CAANOO
	rc_command("bind joyup +up");
	rc_command("bind joydown +down");
	rc_command("bind joyleft +left");
	rc_command("bind joyright +right");
	rc_command("bind joy0 +b");
	rc_command("bind joy2 +a");
	rc_command("bind joy1 +b");
	rc_command("bind joy3 +a");
	rc_command("bind joy4 +select");
	rc_command("bind joy5 +start");
	rc_command("bind joy8 +start");
	rc_command("bind joy9 +select");
#endif

#ifdef WIZ
	rc_command("bind joy12 +b");
	rc_command("bind joy13 +a");
	rc_command("bind joy14 +b");
	rc_command("bind joy15 +a");
	rc_command("bind joy10 +select");
	rc_command("bind joy11 +start");
#endif

#ifdef GP2X_ONLY
	rc_command("bind joyup +up");
	rc_command("bind joydown +down");
	rc_command("bind joyleft +left");
	rc_command("bind joyright +right");
	rc_command("bind joy12 +b");
	rc_command("bind joy13 +a");
	rc_command("bind joy14 +b");
	rc_command("bind joy15 +a");
	rc_command("bind joy10 +select");
	rc_command("bind joy11 +start");
	rc_command("bind joy8 +start");
	rc_command("bind joy9 +select");
#endif

#ifdef DINGOO_BUILD
	rc_command("bind space +a"); /* X button */
	rc_command("bind shift +b"); /* Y button - LSHIFT*/
	rc_command("unbind tab"); /* Left shoulder */
	rc_command("unbind backspace"); /* Right shoulder */
	rc_command("bind up +up"); /*  */
	rc_command("bind down +down"); /*  */
	rc_command("bind left +left"); /*  */
	rc_command("bind right +right"); /*  */
	rc_command("bind ctrl +a"); /* A button - LEFTCTRL */
	rc_command("bind alt +b"); /* B button - LEFTALT */
	rc_command("bind enter +start"); /* START button */
	rc_command("bind esc +select"); // SELECT button
#endif /* DINGOO_BUILD */


	rc_command("set upscaler 0");
	rc_command("set frameskip 0");
	rc_command("set clockspeed 0");
	
#ifdef DINGOO_OPENDINGUX
	{
		char *tmp_pwd;
		char tmp_commmand[sizeof(tmp_pwd) + 30];
		tmp_pwd = (getenv("HOME"));
		if (tmp_pwd == NULL)
		{
			strcpy (tmp_pwd, "roms");
		}
		snprintf(tmp_commmand, sizeof(tmp_commmand)-1, "set romdir \"%s\"", tmp_pwd);
		rc_command(tmp_commmand);
	}
#else
#ifdef DINGOO_BUILD
	exe_path_init(argv[0]); /* workout directory containing this exe */
	tmp_buf = exe_path;

	rc_setvar("romdir", rcv_string, &tmp_buf);
#else
	rc_command("set romdir \"roms\"");
#endif /* DINGOO_BUILD */
#endif /* DINGOO_OPENDINGUX */
#ifdef DINGOO_SIM
	rc_sourcefile("a:"DIRSEP"ohboy"DIRSEP"ohboy.rc");
	rc_sourcefile("a:"DIRSEP"ohboy"DIRSEP"bindings.rc");
	mkdir("a:"DIRSEP"ohboy"DIRSEP"saves", 0777); /* FIXME lookup "savedir" rc variable and mkdir that instead? */
	mkdir("a:"DIRSEP"ohboy"DIRSEP"palettes", 0777);
	mkdir("a:"DIRSEP"ohboy"DIRSEP"borders", 0777);
#else
#ifdef DINGOO_OPENDINGUX
	static char *dir1, *dir2, *dir3, *dir4, *ohboyrc, *bindingsrc;
	dir1 = malloc(strlen(getenv("HOME")) + 24);
	sprintf(dir1, "%s/.ohboy/", getenv("HOME"));
	mkdir(dir1, 0777);
	free (dir1);
	dir2 = malloc(strlen(getenv("HOME")) + 24);
	sprintf(dir2, "%s/.ohboy/saves/", getenv("HOME"));
	mkdir(dir2, 0777);
	free (dir2);
	dir3 = malloc(strlen(getenv("HOME")) + 28);
	sprintf(dir3, "%s/.ohboy/palettes/", getenv("HOME"));
	mkdir(dir3, 0777);
	free (dir3);
	dir4 = malloc(strlen(getenv("HOME")) + 28);
	sprintf(dir4, "%s/.ohboy/borders/", getenv("HOME"));
	mkdir(dir4, 0777);
	free (dir4);
	ohboyrc = malloc(strlen(getenv("HOME")) + 28);
	sprintf(ohboyrc, "%s/.ohboy/ohboy.rc", getenv("HOME"));
	rc_sourcefile(ohboyrc);
	free (ohboyrc);
	bindingsrc = malloc(strlen(getenv("HOME")) + 28);
	sprintf(bindingsrc, "%s/.ohboy/bindings.rc", getenv("HOME"));
	rc_sourcefile(bindingsrc);
	free (bindingsrc);
#else
	rc_sourcefile("ohboy.rc");
	rc_sourcefile("bindings.rc");
	mkdir("saves", 0777); /* FIXME lookup "savedir" rc variable and mkdir that instead? */
	mkdir("palettes", 0777);
	mkdir("borders", 0777);
#endif /* DINGOO_OPENDINGUX */
#endif /* DINGOO_SIM */

	paint_menu_bg();

#ifdef DINGOO_SIM
	rom = argv[0];
	atexit(shutdown);
#else
	if (argc == 2) /* dirt simple argument processing */
	{
		rom = malloc(PATH_MAX);
		strcpy(rom, argv[1]);
	}
	atexit(shutdown);
	while(!rom)
	{
		rom = launcher();
	}
	VideoEnterGame();
#endif /* DINGOO_SIM */

	memset(screen->pixels,0,screen->pitch*screen->h);

	vid_preinit();

#ifndef DISABLE_CATCH_SIGNALS
	/*
	** SDL with threads will raise all sorts of signals
	** may not want to catch them all.
	*/
	catch_signals();
#endif /* DISABLE_CATCH_SIGNALS */
	vid_init();
	pcm_init();
#ifdef GNUBOY_HARDWARE_VOLUME
	pcm_volume(sndlvl * 10);
#endif /* GNUBOY_HARDWARE_VOLUME */
	scaler_init(0);
	ohb_loadrom(rom);
	emu_run();
	return 0;
}

int GP2X_PollEvent(SDL_Event *ev){

	static unsigned int joy_status = 0;
	static signed int joy_x=0, joy_y=0;
	SDL_Event event;
	SDL_Event jevent;
	unsigned int status;

	status = joy_status;
		
#ifdef CAANOO
	Caanoo_UpdateAnalog();
#endif

	if (SDL_PollEvent(&event)) switch(event.type){
		case SDL_JOYBUTTONUP:
			if(event.jbutton.which==0 && event.jbutton.button<8){
				status &= ~(1<<event.jbutton.button);
				break;
			}
			*ev = event;
			return 1;
		case SDL_JOYBUTTONDOWN:
			if(event.jbutton.which==0 && event.jbutton.button<8){
				status |= 1<<event.jbutton.button;
				break;
			}
			*ev = event;
			return 1;
		default:
			*ev = event;
			return 1;
	} else {
		return 0;
	}

	if(status==joy_status) return 0;

#ifdef GP2X_ONLY
	jevent.type = SDL_JOYAXISMOTION;
	jevent.jaxis.which = 0;

	jevent.jaxis.axis = 1;
	jevent.jaxis.value = 0;
	jevent.jaxis.value += status & UP_MASK ? -32768 : 0;
	jevent.jaxis.value += status & DOWN_MASK ? 32767 : 0;
	if(joy_y != jevent.jaxis.value) SDL_PushEvent(&jevent);
	joy_y = jevent.jaxis.value;

	jevent.jaxis.axis = 0;
	jevent.jaxis.value = 0;
	jevent.jaxis.value += status & LEFT_MASK ? -32768 : 0;
	jevent.jaxis.value += status & RIGHT_MASK ? 32767 : 0;
	if(joy_x != jevent.jaxis.value) SDL_PushEvent(&jevent);
	joy_x = jevent.jaxis.value;
#endif
	joy_status = status;

	*ev = event;

	return 1;
}


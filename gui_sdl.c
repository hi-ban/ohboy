
#include <string.h>

#include <SDL/SDL.h>
#include "ohboy.h"
#include "gui_sdl.h"

#include "ubytegui/gui.h"

#if WIZ
#define GETPIXEL(fb,x,y) 		(((Uint16*)fb) + (screen->w-x)*screen->h)[y]
#define PUTPIXEL(fb,x,y,color) 	(GETPIXEL(fb,x,y) = color)
#else
#define GETPIXEL(fb,x,y) 		((Uint16*)(fb + y*screen->pitch))[x]
#define PUTPIXEL(fb,x,y,color) 	(GETPIXEL(fb,x,y) = color)
#endif

extern SDL_Surface *screen;
void *fbcopy;
int fbsz;

int gui_maprgb(int r, int g, int b){
	return SDL_MapRGB(screen->format,r,g,b);
}

int gui_pollevent(guievent_t *ev){
	SDL_Event event;


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

#endif

#ifdef GP2X
	if(GP2X_PollEvent(&event)) {
#else
	if(SDL_PollEvent(&event)) {
#endif

#ifdef DINGOO_BUILD
		if(event.type == SDL_KEYDOWN || event.type == SDL_KEYUP){
			ev->state = event.key.state == SDL_PRESSED ? GUI_PRESSED : GUI_RELEASED;
			switch(event.key.keysym.sym){
				case SDLK_LCTRL:
					ev->key = GUI_SELECT;
					break;
				case SDLK_LALT:
					ev->key = GUI_BACK;
					break;
				case SDLK_UP:
					ev->key = GUI_UP;
					break;
				case SDLK_DOWN:
					ev->key = GUI_DOWN;
					break;
				case SDLK_LEFT:
					ev->key = GUI_LEFT;
					break;
				case SDLK_RIGHT:
					ev->key = GUI_RIGHT;
					break;
				default:
					return 0;
			}
			return 1;
		}
#endif
#ifdef GP2X_ONLY
		if(event.type == SDL_JOYBUTTONUP || event.type == SDL_JOYBUTTONDOWN){
			ev->state = event.type == SDL_JOYBUTTONDOWN ? GUI_PRESSED : GUI_RELEASED;
			switch(event.jbutton.button){
				case 13:
					ev->key = GUI_SELECT;
					break;
				case 14:
					ev->key = GUI_BACK;
					break;
				case 0:
					ev->key = GUI_UP;
					break;
				case 4:
					ev->key = GUI_DOWN;
					break;
				case 2:
					ev->key = GUI_LEFT;
					break;
				case 6:
					ev->key = GUI_RIGHT;
					break;
				default:
					return 0;
			}
			return 1;
		}
#endif
#ifdef CAANOO
		if(event.type == SDL_JOYBUTTONUP || event.type == SDL_JOYBUTTONDOWN){
			ev->state = event.type == SDL_JOYBUTTONDOWN ? GUI_PRESSED : GUI_RELEASED;
			switch(event.jbutton.button){
				case BTN_B:
					ev->key = GUI_SELECT;
					break;
				case BTN_X:
					ev->key = GUI_BACK;
					break;
				case BTN_UP:
					ev->key = GUI_UP;
					break;
				case BTN_DOWN:
					ev->key = GUI_DOWN;
					break;
				case BTN_LEFT:
					ev->key = GUI_LEFT;
					break;
				case BTN_RIGHT:
					ev->key = GUI_RIGHT;
					break;
				default:
					return 0;
			}
			return 1;
		}
#endif
#ifndef DINGOO_BUILD || GP2X_ONLY || CAANOO
		if(event.type == SDL_KEYDOWN || event.type == SDL_KEYUP){
			ev->state = event.key.state == SDL_PRESSED ? GUI_PRESSED : GUI_RELEASED;
			switch(event.key.keysym.sym){
				case SDLK_RETURN:
					ev->key = GUI_SELECT;
					break;
				case SDLK_ESCAPE:
					ev->key = GUI_BACK;
					break;
				case SDLK_UP:
					ev->key = GUI_UP;
					break;
				case SDLK_DOWN:
					ev->key = GUI_DOWN;
					break;
				case SDLK_LEFT:
					ev->key = GUI_LEFT;
					break;
				case SDLK_RIGHT:
					ev->key = GUI_RIGHT;
					break;
				default:
					if (event.type == SDL_KEYUP && event.key.keysym.sym == SCREENSHOT_SDL_KEY)
						TakeScreenShot((SDL_Surface *) NULL, (char *) NULL);
					return 0;
			}
			return 1;
		}
#endif
	}
	return 0;
}

unsigned int darken(int color){
	unsigned char r,g,b;
	SDL_PixelFormat format = *screen->format;
	r = ((color >> format.Rshift) & (0xFF >> format.Rloss))>>2;
	g = ((color >> format.Gshift) & (0xFF >> format.Gloss))>>2;
	b = ((color >> format.Bshift) & (0xFF >> format.Bloss))>>2;
	return (r << format.Rshift) | (g << format.Gshift) | (b << format.Bshift);
}

unsigned int osd_darken(int color){
	unsigned char r,g,b;
	SDL_PixelFormat format = *screen->format;
	r = ((((color >> format.Rshift) & (0xFF >> format.Rloss))>>2)) + (63 >> format.Rloss);
	g = ((((color >> format.Gshift) & (0xFF >> format.Gloss))>>2)) + (63 >> format.Gloss);
	b = ((((color >> format.Bshift) & (0xFF >> format.Bloss))>>2)) + (63 >> format.Bloss);
	return (r << format.Rshift) | (g << format.Gshift) | (b << format.Bshift);
}

void gui_drawpixmap(pixmap_t *pix, int x, int y, int color, int invert){

	int i,j;
	unsigned short sa,da;
	unsigned char a,sr,sg,sb,dr=0,dg=0,db=0;
	SDL_PixelFormat format;
	int l,r,t,b;
	l = x<gui.clip.x ? gui.clip.x : x;
	r = x+pix->width>gui.clip.x+gui.clip.w ? gui.clip.x+gui.clip.w : x+pix->width;
	t = y<gui.clip.y ? gui.clip.y : y;
	b = y+pix->height>gui.clip.y+gui.clip.h ? gui.clip.y+gui.clip.h : y+pix->height;

	format = *screen->format;

	if(pix->pelsize == 1){
		sr=(color&format.Rmask)>>format.Rshift;
		sg=(color&format.Gmask)>>format.Gshift;
		sb=(color&format.Bmask)>>format.Bshift;
	}

	for(j=t; j<b; j++){
		for(i=l; i<r; i++){
			pixmap_getargb(pix,i-l,j-t,&a,&sr,&sg,&sb);
			if(pix->pelsize != 1){
				sr >>= format.Rloss;
				sg >>= format.Gloss;
				sb >>= format.Bloss;
			}

			color = GETPIXEL(fbcopy,i,j);
			dr=(color&format.Rmask)>>(format.Rshift+2);
			dg=(color&format.Gmask)>>(format.Gshift+2);
			db=(color&format.Bmask)>>(format.Bshift+2);

			if(invert){
				da = 1+a;
				sa = 256-a;
			} else {
				da = 256-a;
				sa = 1+a;
			}

			dr = (sa*sr + da*dr)>>8;
			dg = (sa*sg + da*dg)>>8;
			db = (sa*sb + da*db)>>8;

			color = (dr<<format.Rshift) | (dg<<format.Gshift) | (db<<format.Bshift);

			PUTPIXEL(screen->pixels,i,j,color);
		}
	}
}

void osd_drawpixmap(pixmap_t *pix, int x, int y, int color){

	int i,j;
	unsigned short sa,da;
	unsigned char a,sr,sg,sb,dr=0,dg=0,db=0;
	SDL_PixelFormat format;
	int l,r,t,b;
	l = x<0 ? 0 : x;
	r = x+pix->width > screen->w ? screen->w : x+pix->width;
	t = y<0 ? 0 : y;
	b = y+pix->height > screen->h ? screen->h : y+pix->height;

	format = *screen->format;

	if(pix->pelsize == 1){
		sr=(color&format.Rmask)>>format.Rshift;
		sg=(color&format.Gmask)>>format.Gshift;
		sb=(color&format.Bmask)>>format.Bshift;
	}

	for(j=t; j<b; j++){
		for(i=l; i<r; i++){
			pixmap_getargb(pix,i-l,j-t,&a,&sr,&sg,&sb);
			if(pix->pelsize != 1){
				sr >>= format.Rloss;
				sg >>= format.Gloss;
				sb >>= format.Bloss;
			}

			color = GETPIXEL(screen->pixels,i,j);
			dr=(color&format.Rmask)>>format.Rshift;
			dg=(color&format.Gmask)>>format.Gshift;
			db=(color&format.Bmask)>>format.Bshift;

			da = 256-a;
			sa = 1+a;

			dr = (sa*sr + da*dr)>>8;
			dg = (sa*sg + da*dg)>>8;
			db = (sa*sb + da*db)>>8;

			color = (dr<<format.Rshift) | (dg<<format.Gshift) | (db<<format.Bshift);

			PUTPIXEL(screen->pixels,i,j,color);
		}
	}
}

void gui_begin(){
	SDL_LockSurface(screen);
	fbsz = screen->h * screen->pitch;
	fbcopy = malloc(fbsz);
	memcpy(fbcopy,screen->pixels,fbsz);
	gui.clip.x = 0;
	gui.clip.y = 0;
	gui.clip.w = gui.w = screen->w;
	gui.clip.h = gui.h = screen->h;
}

int gui_update(){
	SDL_UnlockSurface(screen);
	SDL_Flip(screen);
	SDL_LockSurface(screen);
	return GUI_COPY;
}

void gui_end(){
	/* if not dirty then restore, if dirty force a redraw */
	memcpy(screen->pixels,fbcopy,fbsz);
	free(fbcopy);
	SDL_UnlockSurface(screen);
}

void osd_cls(int x, int y, int w, int h){
	int i,j;
	unsigned int color;
	for(j=y; j<y+h; j++){
		for(i=x; i<x+w; i++){
			color = GETPIXEL(screen->pixels,i,j);
			PUTPIXEL(screen->pixels,i,j,osd_darken(color));
		}
	}
}

void gui_cls(){
	int x,y;
	unsigned int color;
	for(y=gui.clip.y; y<gui.clip.y+gui.clip.h; y++){
		for(x=gui.clip.x; x<gui.clip.x+gui.clip.w; x++){
			color = GETPIXEL(fbcopy,x,y);
			PUTPIXEL(screen->pixels,x,y,darken(color));
		}
	}
}

void gui_drawrect(int x, int y, int w, int h, int color, int rounded){
	int l,r,t,b;
	int i,j,rl,rr;

	l = x<gui.clip.x ? gui.clip.x : x;
	r = x+w>gui.clip.x+gui.clip.w ? gui.clip.x+gui.clip.w : x+w;
	t = y<gui.clip.y ? gui.clip.y : y;
	b = y+h>gui.clip.y+gui.clip.h ? gui.clip.y+gui.clip.h : y+h;

	for(j=t; j<b; j++){
		if(j==y || j+1==y+h){
			rl = l==x ? l+1 : l;
			rr = r==x+w ? r-1 : r;
		} else {
			rl = l;
			rr = r;
		}

		for(i=rl; i<rr; i++)
			PUTPIXEL(screen->pixels,i,j,color);
	}
}

void osd_drawrect(int x, int y, int w, int h, int color, int rounded){
	int l,r,t,b;
	int i,j,rl,rr;

	l = x<0 ? 0 : x;
	r = x+w>screen->w ? screen->w : x+w;
	t = y<gui.clip.y ? gui.clip.y : y;
	b = y+h>screen->h ? screen->h : y+h;

	for(j=t; j<b; j++){
		if(j==y || j+1==y+h){
			rl = l==x ? l+1 : l;
			rr = r==x+w ? r-1 : r;
		} else {
			rl = l;
			rr = r;
		}

		for(i=rl; i<rr; i++)
			PUTPIXEL(screen->pixels,i,j,color);
	}
}

void osd_drawtext(font_t *font, const char *text, int x, int y, int color){
#ifdef UBYTE_USE_FREETYPE /* this is a nasty hack,... fixme! */
	glyph_t *glyph;

	if(!font || !text) return;

	while(*text){
		glyph = font->glyph[(int)(*text)];
		osd_drawpixmap(&(glyph->pixmap), x+glyph->left, y-glyph->top, color);
		x+= glyph->advance;
		text++;
	}
#else
    gui_drawtext(font, text, x, y, color, 0);
#endif /* UBYTE_USE_FREETYPE */
}


void gui_sleep(int s){
	if(s>0) SDL_Delay(s/1000);
}

/****************************************/

/*
**  TakeScreenShot of SDL surface, only needs SDL lib
**      surface is optional, if NULL is specified use the main video
**      filename is optional, if NULL is specified use 
*/
void TakeScreenShot(SDL_Surface *screen_to_save, char *filename)
{
    char *local_filename=NULL;
    SDL_Surface *local_screen=NULL;
    
    
    local_screen = screen_to_save;
    local_filename = filename;
    
    if (local_screen == NULL)
    {
        local_screen = SDL_GetVideoSurface();
        
        /*
        // if there is global screen ref go grab it ...
        extern SDL_Surface *screen;
        
        local_screen = screen;
        */
    }
    if (local_filename == NULL)
    {
        local_filename = SCREENSHOT_DEFAULT_FILENAME;
        /* TODO scan local dir and derive name? Use timestamp (note Dingoo has no clock). */
    }
    
    SDL_SaveBMP(local_screen, local_filename);
}

#include "gui.h"

gui_t gui = {.open = 0};

void gui_setclip(int x, int y, int w, int h){
	gui.clip.x = x;
	gui.clip.y = y;
	gui.clip.w = w;
	gui.clip.h = h;
}

void gui_clip(int x, int y, int w, int h){
	int right, bottom;

	if(x>gui.clip.x){
		gui.clip.w -= x - gui.clip.x;
		gui.clip.x = x;
	}

	if(y>gui.clip.y){
		gui.clip.h -= y - gui.clip.y;
		gui.clip.y = y;
	}

	if((right = x+w) < gui.clip.x + gui.clip.w)
		gui.clip.w = right - gui.clip.x;

	if((bottom = y+h) < gui.clip.y + gui.clip.h)
		gui.clip.h = bottom - gui.clip.y;

}

#ifdef UBYTE_USE_FREETYPE /* this is a nasty hack,... fixme! */
void gui_drawtext(font_t *font, const char *text, int x, int y, int color, int invert){
	glyph_t *glyph;

	if(!font || !text) return;

	while(*text){
		glyph = font->glyph[(int)(*text)];
		gui_drawpixmap(&(glyph->pixmap), x+glyph->left, y-glyph->top, color,invert);
		x+= glyph->advance;
		text++;
	}
}
#else /* UBYTE_USE_FREETYPE */

extern SDL_Surface *screen;
extern SFont_Font* fps_font;
extern SFont_Font* menu_font;


/* Use SFont */
void gui_drawtext(font_t *font, const char *text, int x, int y, int color, int invert)
{
    SFont_Font* gui_font=menu_font;

    /*
fprintf(stdout, "x%d, y%d, text>%s<\n",x, y, text); fflush(stdout);
fprintf(stdout, "DEBUG gui_drawtext\n"); fflush(stdout);
if (screen == NULL) fprintf(stdout, "screen == NULL\n"); fflush(stdout);
if (gui_font == NULL) fprintf(stdout, "gui_font == NULL\n"); fflush(stdout);
    */
    SDL_UnlockSurface(screen); /* what else is needed, e.g. relock? */
    SFont_Write(screen, gui_font, x, y-14, text);
    /*
	SDL_Flip(screen);
    */
	SDL_LockSurface(screen);
}

#endif /* UBYTE_USE_FREETYPE */

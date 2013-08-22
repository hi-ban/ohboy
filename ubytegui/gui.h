#ifndef GUI_H_INCLUDED
#define GUI_H_INCLUDED

#include "pixmap.h"
#include "font.h"
#include "dialog.h"

#define GUI_UP 1
#define GUI_DOWN 2
#define GUI_LEFT 3
#define GUI_RIGHT 4
#define GUI_SELECT 5
#define GUI_BACK 6

#define GUI_PRESSED 1
#define GUI_RELEASED 0

#define GUI_FLIP 2
#define GUI_COPY 1

typedef struct gui_s{
	short int x, y;
	short int w, h;
	struct clip_s{
		short int x, y;
		short int w, h;
	} clip;
	int open;
} gui_t;

typedef struct guievent_s{
	short int key, state;
} guievent_t;

extern gui_t gui;

int gui_pollevent(guievent_t *ev);
void gui_begin();
int gui_update();
void gui_end();
void gui_cls();
void gui_drawpixmap(pixmap_t *pix, int x, int y, int color, int invert);
void gui_drawrect(int x, int y, int w, int h, int color, int rounded);
void gui_setclip(int x, int y, int w, int h);
void gui_clip(int x, int y, int w, int h);
void gui_drawtext(font_t *font, const char *text, int x, int y, int color, int invert);


#endif // GUI_H_INCLUDED

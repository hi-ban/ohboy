#include <stdlib.h>
#include <string.h>


#include "dialog.h"
#include "font.h"
#include "pixmap.h"
#include "gui.h"

static font_t *dfont;
static dialog_t *dialog;
static int dfg;
static int dpad;
static pixmap_t *dscrollu, *dscrolld, *doptl, *doptr;

    /* These below allow to adjust the vertical position of all elements of the menu, when using Freetype or SFont. Positive values move elements down.*/
#ifdef UBYTE_USE_FREETYPE
#define HIGHLIGHT_RECT_OFFSET 0
#define TEXT_OFFSET 0
#define LRARROW_OFFSET 1
#define UPARROW_OFFSET 4
#define DOWNARROW_OFFSET -1
#define TITLE_OFFSET 2
#define TITLE_LINE_OFFSET 8
#define STATUS_OFFSET -2
#define STATUS_LINE_OFFSET -5
#else
    /* using SFont */
#define HIGHLIGHT_RECT_OFFSET 0  /* <--- This produces some glitches (for pressstart font) if set to something different than zero */
#define TEXT_OFFSET 3
#define LRARROW_OFFSET -1
#define UPARROW_OFFSET 4
#define DOWNARROW_OFFSET 1
#define TITLE_OFFSET 7
#define TITLE_LINE_OFFSET 8
#define STATUS_OFFSET 5
#define STATUS_LINE_OFFSET -5
#endif /* UBYTE_USE_FREETYPE */

int dialog_maxtextw(){
	return gui.w-dpad*2;
}

static pixmap_t *createvarrow(int w, int h, int down){
	int i, j, aw, qw, hw;
	unsigned char *dst;

	pixmap_t *pix = pixmap_new(w,h,1);

	dst = pix->ptr;
	qw = w>>2;
	hw = w>>1;

	for(j=0; j<h; j++){

		if(down){
			if(j<h-hw) i=qw, aw=w-qw;
			else i = j-hw, aw = w+hw-j;
		} else {
			if(j<hw) i = hw-j-1, aw = hw+j+1;
			else i=qw, aw=w-qw;
		}
		while(i<aw) (dst)[i++] = 0xFF;
		dst+=pix->pitch;
	}
	return pix;
}

static pixmap_t *createharrow(int w, int h, int right){
	int i, j, aw, qh, hh;
	unsigned char *dst;


	pixmap_t *pix = pixmap_new(w,h,1);

	dst = pix->ptr;
	qh = h>>2;
	hh = h>>1;

	for(j=0; j<h; j++){

		if(right){
			i = j>=qh && (h-j)>qh ? 0 : w-hh;
			aw = j < hh ? w-hh+j+1 : w+hh-j;
		} else {
			i = j < hh ? hh-j-1 : j-hh;
			aw = j>=qh && (h-j)>qh ? w : hh;
		}


		while(i<aw) dst[i++] = 0xFF;
		dst+=pix->pitch;
	}
	return pix;
}

int dialog_init(font_t *font, int fg){
	if(!font) return 0;
	dfont = font;
	dfg = fg;
	dpad = dfont->ascent & 0xFFFFC;
	dscrollu = createvarrow(dpad,dpad,0);
	dscrolld = createvarrow(dpad,dpad,1);
	doptl = createharrow(dpad*3/4,dpad,0);
	doptr = createharrow(dpad*3/4,dpad,1);
	return 1;
}

void dialog_close(){
	if(dialog) free(dialog);
	if(dfont) free(dfont);
}

void dialog_begin(const char* title, const char* status){
	int sz, len;
	sz = sizeof(dialog_t) + sizeof(field_t)*16;
	dialog = malloc(sz);
	dialog->sz = sz;
	dialog->title = title;
	dialog->status = status;
	dialog->field_count = 0;
	dialog->prompt_w = 0;
	dialog->body_w = 0;
	dialog->selected = 0;
}

static field_t *dialog_addfield(){
	field_t *field;
#ifndef DINGOO_NATIVE
	/* Really if notbyte aligned platform */
	if((int)&dialog->fields[dialog->field_count] >= dialog->sz){
		dialog->sz+= sizeof(field_t)*16;
		dialog = realloc(dialog, dialog->sz);
	}
#endif /* DINGOO_NATIVE */
	field = &(dialog->fields[dialog->field_count]);
	dialog->field_count++;
	memset(field, 0, sizeof(field_t));
	return field;
}

int dialog_text(const char *prompt, const char* body, int flags){

	field_t *field = dialog_addfield(prompt,FIELD_TEXT);

	field->type = FIELD_TEXT;
	field->prompt = prompt;
	field->body = body;
	field->flags = flags;
	field->prompt_w = font_textwidth(dfont, prompt);
	field->body_w = font_textwidth(dfont, body);

	return dialog->field_count;
}

int dialog_option(const char *prompt, const char** options, int* selected){

	int w=0;
	const char *body;
	field_t *field = NULL;

	if(!options || !selected) return 0;

	field = dialog_addfield(prompt,FIELD_OPTION);

	field->type = FIELD_OPTION;
	field->prompt = prompt;
	field->body = options[*selected];
	field->flags = FIELD_SELECTABLE;
	field->options = options;
	field->selected = selected;
	field->prompt_w = font_textwidth(dfont, prompt);
	field->body_w = 0;

	while(body = options[field->count]){
		w = font_textwidth(dfont, body);
		if(w>field->body_w)field->body_w = w;
		field->count++;
	}

	return dialog->field_count;
}

static int dialog_drawtitle(){
	int color = dfg;
	int pad = dpad;
	gui_setclip(0,0,gui.w,dialog->title_h);
	gui_cls();
	gui_drawtext(dfont,dialog->title,dialog->title_x,dfont->height+TITLE_OFFSET,color,0);
	gui_drawrect(0,dfont->height+TITLE_LINE_OFFSET,gui.w,1,color,0);
	if(dialog->field_count > dialog->visible_count){
		gui_drawpixmap(dscrollu, pad/4, dfont->height-dscrollu->height+UPARROW_OFFSET, color, 0);
	}
}

static int dialog_drawfield(int i){
	int x, y, w, h, pad, pad2, hpad, qpad, invert=0;
	int color;

	field_t *field = &(dialog->fields[i+dialog->pos]);

	y = i*dialog->field_h+dialog->title_h;

	gui_setclip(0, y+HIGHLIGHT_RECT_OFFSET, gui.w, dialog->field_h+HIGHLIGHT_RECT_OFFSET);
	gui_cls();

	pad = dpad;
	pad2 = pad << 1;
	hpad = pad >> 1;
	qpad = pad >> 2;

	color = dfg;


	if(i==dialog->selected-dialog->pos){
		w = font_textwidth(dfont,field->prompt)+pad2;
		if(dialog->prompt_w < w) w = dialog->prompt_w;
		gui_drawrect(dialog->prompt_x+hpad, y+HIGHLIGHT_RECT_OFFSET, w-pad, dialog->field_h, color, 1);
		if(field->type == FIELD_OPTION){
			gui_drawpixmap(doptl,dialog->body_x,y+dfont->ascent-doptl->height+LRARROW_OFFSET,color,0);
			w = font_textwidth(dfont,field->body);
			if(field->body_w < w) w = field->body_w;
			gui_drawpixmap(doptr,dialog->body_x+w+pad2-doptr->width,y+dfont->ascent-doptr->height+LRARROW_OFFSET,color,0);
		}
		invert = 1;
	}

	gui_setclip(dialog->prompt_x+pad, y, dialog->prompt_w-pad2, dialog->field_h);
	gui_drawtext(dfont,field->prompt,dialog->prompt_x+pad,y+dfont->ascent+TEXT_OFFSET,color,invert);
	gui_setclip(dialog->body_x+pad, y, dialog->body_w-pad2, dialog->field_h);
	if (field->body)
		gui_drawtext(dfont,field->body,dialog->body_x+pad,y+dfont->ascent+TEXT_OFFSET,color,0);
}

static int dialog_drawstatus(){

	int pad = dpad;
	int color = dfg;

	gui_setclip(0,gui.h-dialog->status_h,gui.w,dialog->status_h);
	gui_cls();

	gui_drawtext(dfont,dialog->status,dialog->status_x,gui.h+dfont->descent+STATUS_OFFSET,color,0);
	gui_drawrect(0,gui.h-dfont->height+STATUS_LINE_OFFSET,gui.w,1,color,0);

	if(dialog->field_count > dialog->visible_count){
		gui_drawpixmap(dscrolld,pad/4,gui.h+dfont->descent-dscrolld->height+DOWNARROW_OFFSET,color,0);
	}
}

static int dialog_drawdirty(dirty_t *dirty){
	int i;
	if(dirty->title) dialog_drawtitle();
	for(i=0; i<dirty->field_count; i++)
		dialog_drawfield(dirty->field_start+i);
	if(dirty->status) dialog_drawstatus();
}

int dialog_end(){

	guievent_t ev;
	field_t *field;
	dirty_t dirty;

	int i, pad, pad2, exit=0, delay=0;
	int up=0, down=0, left=0, right=0;

	gui_setclip(0,0,gui.w,gui.h);
	gui_cls();

	dialog->pos = 0;
	dialog->title_x = (gui.w-font_textwidth(dfont,dialog->title))/2;
	dialog->status_x = (gui.w-font_textwidth(dfont,dialog->status))/2;
	dialog->field_h = dfont->height;
	dialog->title_h = dfont->height+8;
	dialog->status_h = dfont->height+8;
	dialog->visible_count = (gui.h-dialog->title_h-dialog->status_h)/dialog->field_h;
	if(dialog->visible_count > dialog->field_count) dialog->visible_count = dialog->field_count;
	dialog->client_h = dialog->visible_count * dialog->field_h;
	dialog->title_h = (gui.h -dialog->client_h )/2;
	dialog->status_h = gui.h - dialog->title_h - dialog->client_h;

	pad = dpad;
	pad2 = dpad << 1;

	for(i=0; i<dialog->field_count; i++){
		int prompt_w=0, body_w=0;

		field = &(dialog->fields[i]);

		prompt_w = field->prompt_w;
		if(prompt_w) prompt_w += pad2;
		if(prompt_w > dialog->prompt_w) dialog->prompt_w = prompt_w;

		body_w = field->body_w;
		if(body_w) body_w += pad2;
		if(body_w > dialog->body_w) dialog->body_w = body_w;

		if(dialog->selected==i && !(field->flags & FIELD_SELECTABLE)) dialog->selected++;
	}

	if(dialog->selected == dialog->field_count) dialog->selected = -1;

	if((dialog->prompt_w + dialog->body_w) > gui.w){
		if(dialog->prompt_w>gui.w) dialog->prompt_w = gui.w;
		dialog->body_w = gui.w-dialog->prompt_w;
	}

	dialog->prompt_x = (gui.w - dialog->prompt_w - dialog->body_w)/2;
	dialog->body_x = dialog->prompt_x + dialog->prompt_w;

	dirty.update=0;
	dirty.field_start = 0;
	dirty.field_count = dialog->visible_count;
	dirty.title = 1;
	dirty.status = 1;

	dialog_drawdirty(&dirty);
	dirty.update = gui_update();

	while(!exit){

		field = &(dialog->fields[dialog->selected]);

		if(gui_pollevent(&ev)){
			switch(ev.key){
				case GUI_UP:
					up = ev.state;
					break;
				case GUI_DOWN:
					down = ev.state;
					break;
				case GUI_LEFT:
					left = ev.state;
					break;
				case GUI_RIGHT:
					right = ev.state;
					break;
				case GUI_SELECT:
					if(ev.state && field->type == FIELD_TEXT)
						exit = dialog->selected+1;
					break;
				case GUI_BACK:
					if(ev.state) exit = -1;
					break;
			}
			delay = ev.state ? -1 : 0;
		}

		if(delay>0){
			delay--;
		} else {
			if(delay<0) delay = 4;
			if(dirty.update == GUI_FLIP)
			dialog_drawdirty(&dirty);

			if(up){
				int s = dialog->selected;
				while(--s>=0 && !(dialog->fields[s].flags & FIELD_SELECTABLE));
				if(s>=0){
					if(s < dialog->pos){
						dialog->pos = s;
						if(dirty.update){
							dirty.field_start = 0;
							dirty.field_count = dialog->visible_count;
							dirty.title = 1;
							dirty.status = 1;
						}
					} else if(dirty.update){
						dirty.field_start = s-dialog->pos;
						dirty.field_count = dialog->selected-s+1;
						dirty.title = 0;
						dirty.status = 0;
					}
					dialog->selected = s;
				} else if(s<0){                      /* LOOP */ 
					dialog->pos = dialog->field_count-dialog->visible_count;
					s = dialog->field_count-1;
					if(dirty.update){
						dirty.field_start = 0;
						dirty.field_count = dialog->visible_count;
						dirty.title = 1;
						dirty.status = 1;
					}
					dialog->selected = s;
				}
				
			} else if(down) {

				int s = dialog->selected;

				while(++s<=(dialog->field_count-1) && !(dialog->fields[s].flags & FIELD_SELECTABLE));
				if(s<dialog->field_count){
					if(s >= dialog->pos+dialog->visible_count) {
						dialog->pos = s-dialog->visible_count+1;
						if(dirty.update){
							dirty.field_start = 0;
							dirty.field_count = dialog->visible_count;
							dirty.title = 1;
							dirty.status = 1;
						}
					} else if(dirty.update){
						dirty.field_start = dialog->selected-dialog->pos;
						dirty.field_count = s-dialog->selected+1;
						dirty.title = 0;
						dirty.status = 0;
					}
					dialog->selected = s;
				} else if(s>=dialog->field_count){     /* LOOP */
					dialog->pos = 0;
					s = 0;
					if(dirty.update){
						dirty.field_start = 0;
						dirty.field_count = dialog->visible_count;
						dirty.title = 1;
						dirty.status = 1;
					}
					dialog->selected = s;
				}
				
			} else if(dialog->field_count > dialog->visible_count && field->type == FIELD_TEXT){  /* FAST SCROLLING */		
				if(left){
					int s = dialog->selected;
					int p = dialog->pos;
					
					if (p <= 0 && s <= 0){
						p = dialog->field_count-dialog->visible_count;
						s = dialog->field_count-(dialog->visible_count-dialog->selected);
					} else {
						p = dialog->selected-dialog->visible_count;
						s = dialog->selected-dialog->visible_count;
					}
					if (p < 0){
						p = 0;
					}
					if (s < 0){
						s = 0;
					}
					if(dirty.update){
						dirty.field_start = 0;
						dirty.field_count = dialog->visible_count;
						dirty.title = 1;
						dirty.status = 1;
					}
					dialog->pos = p;
					dialog->selected = s;
					
				} else if(right){
					int s = dialog->selected;
					int p = dialog->pos;
					
					if (p >= dialog->field_count-dialog->visible_count && s >= dialog->field_count-1){
						p = 0;
						s = (dialog->selected+dialog->visible_count)-dialog->field_count;
					} else {
						p = dialog->selected+1;
						s = dialog->selected+dialog->visible_count;
					}
					if (p > (dialog->field_count-dialog->visible_count)){
						p = dialog->field_count-dialog->visible_count;
					}
					if (s > dialog->field_count-1){
						s = dialog->field_count-1;
					}
					if(dirty.update){
						dirty.field_start = 0;
						dirty.field_count = dialog->visible_count;
						dirty.title = 1;
						dirty.status = 1;
					}
					dialog->pos = p;
					dialog->selected = s;
				}
				
			} else if(field->type == FIELD_OPTION){

				if(left){
					(*field->selected)--;
					if(*field->selected < 0) *field->selected = field->count-1;
					field->body = field->options[*field->selected];
					if(dirty.update){
						dirty.field_start = dialog->selected-dialog->pos;
						dirty.field_count = 1;
						dirty.title = 0;
						dirty.status = 0;
					}
				} else if(right){
					(*field->selected)++;
					if(*field->selected >= field->count) *field->selected = 0;
					field->body = field->options[*field->selected];
					if(dirty.update){
						dirty.field_start = dialog->selected-dialog->pos;
						dirty.field_count = 1;
						dirty.title = 0;
						dirty.status = 0;
					}
				}
			}

			dialog_drawdirty(&dirty);
			dirty.update = gui_update();
			#if defined(GCWZERO)
			dialog_drawdirty(&dirty);         /*Fix for flickering menu when using double buffer*/
			dirty.update = gui_update();      /*Paints the menu 2 times to prevent flickering*/
			#endif /*GCWZERO*/
		}

		gui_sleep(50000);
	}

	if(exit==-1) exit = 0;

	free(dialog);
	dialog = NULL;
	return exit;
}



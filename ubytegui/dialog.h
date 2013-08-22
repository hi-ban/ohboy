#ifndef DIALOG_H_INCLUDED
#define DIALOG_H_INCLUDED

#include "font.h"

#define FIELD_TEXT 0
#define FIELD_OPTION 1

#define FIELD_SELECTABLE 1


typedef struct dirty_s{
	short int update;
	short int field_start, field_count;
	short int title, status;
} dirty_t;

typedef struct field_s{
	const char *prompt;
	const char *body;

	short int prompt_w, body_w;

	char type, flags;
	short int count;
	int* selected;
	const char **options;
} field_t;

typedef struct dialog_s{
	int sz;
	const char *title;
	short int prompt_x, prompt_w;
	short int body_x, body_w;
	short int title_x, title_h, status_x, status_h, field_h, client_h;
	const char *status;
	int selected, pos;
	int visible_count, field_count;
	pixmap_t *pmselected;
#ifdef DINGOO_NATIVE
	/* really a byte alignment issue.... */
	field_t fields[1<<16]; /* statically allocate. Breaking API seperation.. match file count in menu.c */
#else
	field_t fields[];
#endif /* DINGOO_NATIVE */
} dialog_t;

int dialog_init(font_t *font, int fg);
void dialog_close();
void dialog_begin(const char* title, const char* status);
int dialog_text(const char *prompt, const char* body, int flags);
int dialog_option(const char *prompt, const char** options, int* selected);
int dialog_end();

#endif // DIALOG_H_INCLUDED

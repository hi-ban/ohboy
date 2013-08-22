#ifndef PIXMAP_H_INCLUDED
#define PIXMAP_H_INCLUDED

typedef struct pixmap_s{
	short int width, height, pelsize, pitch;
	void *ptr;
} pixmap_t;

pixmap_t *pixmap_new(int w, int h, int pelsize);
pixmap_t *pixmap_loadpng(char* pic);
void pixmap_free(pixmap_t *pix);
void pixmap_getargb(pixmap_t *pix, int x, int y, unsigned char *a, unsigned char *r, unsigned char *g, unsigned char *b);
#endif // PIXMAP_H_INCLUDED


#include "stdlib.h"
#include <string.h>

#ifdef UBYTE_USE_LIBPNG
#include "png.h"
#endif /* UBYTE_USE_LIBPNG */
#include "pixmap.h"

pixmap_t *pixmap_new(int w, int h, int pelsize){
	int sz = sizeof(pixmap_t) + w*h*pelsize;
	pixmap_t *pix = malloc(sz);
	memset(pix,0,sz);
	pix->width = w;
	pix->height = h;
	pix->pitch = w * pelsize;
	pix->pelsize = pelsize;
	pix->ptr = (void*)pix+sizeof(pixmap_t);
	return pix;
}

void pixmap_free(pixmap_t *pix){
	free(pix);
}

void pixmap_getargb(pixmap_t *pix, int x, int y, unsigned char *a, unsigned char *r, unsigned char *g, unsigned char *b){
	unsigned char *src;
	src = pix->ptr + y*pix->pitch + x*pix->pelsize;
	*a = src[0];
	if(pix->pelsize == 4){
		*r = src[1];
		*g = src[2];
		*b = src[3];
	}
}


#ifdef DINGOO_NATIVE
void die(char *fmt, ...);
#endif /* DINGOO_NATIVE */

pixmap_t *pixmap_loadpng(char* pic){
	pixmap_t* pix=NULL;
#ifdef UBYTE_USE_LIBPNG
	unsigned char header[8];
	FILE *in;
	unsigned int width, height, bit_depth, color_type, depth, y;

	png_bytep* rows;

	in = fopen(pic, "rb");
	if(!in){
		printf("failed to open PNG file\n");
		return NULL;
	}

	fread(header, 1, 8, in);
    if (png_sig_cmp(header, 0, 8)){
    	printf("bad header\n");
    	return NULL;
    }

	png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png_ptr) {
    	printf("failed to create read struct\n");
    	return NULL;
    }

    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr)
    {
    	printf("failed to create info struct\n");
        png_destroy_read_struct(&png_ptr,(png_infopp)NULL, (png_infopp)NULL);
        return NULL;
    }

    png_infop end_info = png_create_info_struct(png_ptr);
    if (!end_info)
    {
    	printf("failed to create end info struct\n");
        png_destroy_read_struct(&png_ptr, &info_ptr,(png_infopp)NULL);
        return NULL;
    }

    if (setjmp(png_jmpbuf(png_ptr)))
    {
        png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
        fclose(in);
        return NULL;
    }

	png_init_io(png_ptr, in);

	png_set_sig_bytes(png_ptr, 8);

	png_read_info(png_ptr, info_ptr);

	png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type, NULL, NULL, NULL);

	depth = color_type == PNG_COLOR_TYPE_GRAY ? 1 : 4;

	pix = malloc(sizeof(pixmap_t) + width*height*depth);
	rows = malloc(sizeof(png_bytep)*height);

	pix->width = width;
	pix->height = height;
	pix->pelsize = depth;
	pix->pitch = width*depth;
	pix->ptr = (void*)pix + sizeof(pixmap_t);

	if (color_type == PNG_COLOR_TYPE_PALETTE)
        png_set_palette_to_rgb(png_ptr);

    if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
    {
#ifdef DINGOO_NATIVE
        void die(char *fmt, ...);
        die("pixmap_loadpng(): gray png found, wanna png_set_gray_1_2_4_to_8.\n");
#else
		png_set_gray_1_2_4_to_8(png_ptr);
#endif /* DINGOO_NATIVE */
    }

    if (png_get_valid(png_ptr, info_ptr,PNG_INFO_tRNS))
		png_set_tRNS_to_alpha(png_ptr);

	if (bit_depth == 16)
        png_set_strip_16(png_ptr);

	if (color_type == PNG_COLOR_TYPE_RGB)
        png_set_filler(png_ptr, 0xFF, PNG_FILLER_BEFORE);

	if (color_type == PNG_COLOR_TYPE_RGB_ALPHA || color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
        png_set_swap_alpha(png_ptr);

	if (color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
          png_set_gray_to_rgb(png_ptr);

	for(y=0; y<height; y++){
		rows[y] = (void*)pix+sizeof(pixmap_t) + y*width*depth;
	}

	png_read_image(png_ptr, rows);
	png_read_end(png_ptr, end_info);
	png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
	fclose(in);
	free(rows);
#else
    (void) pic;
    /* TODO use code from http://www.nothings.org/stb_image.c ? */
#endif /* UBYTE_USE_LIBPNG */

	return pix;
}

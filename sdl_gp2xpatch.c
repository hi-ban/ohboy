
#include "SDL/sdl.h"
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#define FBIO_MAGIC 'D'
#define FBIO_LCD_CHANGE_CONTROL _IOW(FBIO_MAGIC, 90, unsigned int[2])
#define LCD_DIRECTION_ON_CMD 5 /* 320x240 */
#define LCD_DIRECTION_OFF_CMD 6 /* 240x320 */

#if WIZ
SDL_Surface *WIZ_SetVideoMode(int width, int height, int bitsperpixel, Uint32 flags){
	SDL_Surface *s;
	s = SDL_SetVideoMode(width, height, bitsperpixel, flags);
	if(s){
		unsigned int send[2];int fb_fd = open("/dev/fb0", O_RDWR);
		send[0] = LCD_DIRECTION_OFF_CMD;
		ioctl(fb_fd, FBIO_LCD_CHANGE_CONTROL, &send);
		close(fb_fd);
	}
	return s;
}
#endif


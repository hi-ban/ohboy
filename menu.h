#ifndef MENU_H_INCLUDED
#define MENU_H_INCLUDED


/* Probably DINGOO_NATIVE too... */
#ifndef PATH_MAX
#define PATH_MAX 1024
#endif /* PATH_MAX */


#define REQUEST_DIRS 2
#define REQUEST_FILES 1
#define REQUEST_FILES 1

int menu();
int launcher();

extern struct fb vid_fb;
extern char *border;
extern int my_color;

#endif /* MENU_H_INCLUDED */

#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <ctype.h>

#include <unistd.h> /* test! for chdir etc. */


#include <sys/stat.h>
#include <sys/types.h>
#include <limits.h>
#include <time.h>

#include "gnuboy/Version"
#include "ohboy_ver.h"

#include "gnuboy.h"

#include "fb.h"
#include "ubytegui/gui.h"
#include "ubytegui/dialog.h"
#include "input.h"
#include "hw.h"
#include "rc.h"
#include "loader.h"
#include "mem.h"
#include "sound.h"
#include "lcd.h"
#include "menu.h"


extern void scaler_init(int scaler_number); /* maybe keeping this and loosing the other stuff */

#ifdef DINGOO_NATIVE

#include <jz4740/cpu.h>  /* for cpu clock */

char *ctime(const time_t *timep);
char *ctime(const time_t *timep)
{
    char *x="not_time";
    return x;
}

#endif /* DINGOO_NATIVE */


char *menu_getext(char *s){
	char* ext = NULL;
	while(*s){
		if(*s++ == '.') ext=s;
	}

	return ext;
}

int filterfile(char *p, char *exts){
	char* ext;
	char* cmp;
	int n,i;

	if(exts==NULL) return 1;

	ext = exts;
	cmp = menu_getext(p);

	if(cmp==NULL) return 0;

	while(*ext != 0){
		n=0;
		while(*exts++ != ';'){
			n++;
			if(*exts==0) break;
		}

		i=0;
		while(tolower(cmp[i])==tolower(ext[i])){
			i++;
			if((i==n) && (cmp[i]==0)) return 1;
			if((i==n) || (cmp[i]==0)) break;
		}
		ext = exts;
	}
	return 0;
}

int fcompare(const void *a, const void *b){
	char *stra = *(char**)a;
	char *strb = *(char**)b;
	int cmp;
	return strcmp(stra,strb);
}

const char *basexname (const char *f) {     
	const char *p;     
	if ((p = strrchr (f, '/')) != NULL || (p = strrchr (f, '\\')) != NULL) {    
	return (p + 1);
	}
    return (f); 
}

char* menu_browsedir(char* fpathname, char* file, char *title, char *exts){
    /* this routine has side effects with fpathname and file, FIXME */
	DIR *dir;
	struct dirent *d;
	int n=0, i, j;
	char *files[1<<16];  /* 256Kb */
#ifndef  DT_DIR
	struct stat s;
	char tmpfname[PATH_MAX];
	char *tmpfname_end;
#endif /* DT_DIR */


	if(!(dir = opendir(fpathname))) return NULL; /* FIXME try parent(s) until out of paths */

	/* TODO FIXME add root directory check (to avoid adding .. as a menu option when at "/" or "x:\") */
	files[n] = malloc(4);
	strcpy(files[n], "..");
	strcat(files[n], DIRSEP);
	n++;

	d = readdir(dir);
	if(d && !strcmp(d->d_name,".")) d = readdir(dir);
	if(d && !strcmp(d->d_name,"..")) d = readdir(dir);

#ifndef  DT_DIR
	strcpy(tmpfname, fpathname);
	tmpfname_end = &tmpfname[0];
	tmpfname_end += strlen(tmpfname);
#endif /* DT_DIR */

	while(d){
#ifndef  DT_DIR
		/* can not lookup type from search result have to stat filename*/
		strcpy(tmpfname_end, d->d_name);
		stat(tmpfname, &s);
		if(S_ISDIR (s.st_mode))
#else
		if ((d->d_type & DT_DIR) == DT_DIR)
#endif /* DT_DIR */
		{
			files[n] = malloc(strlen(d->d_name)+2);
			strcpy(files[n], d->d_name);
			strcat(files[n], DIRSEP);
			n++;
		} else if(filterfile(d->d_name,exts)){
			files[n] = malloc(strlen(d->d_name)+1);
			strcpy(files[n], d->d_name);
			n++;
		}
		d  = readdir(dir);
	}
	closedir (dir);
	qsort(files+1,n-1,sizeof(char*),fcompare);

	dialog_begin(title, fpathname);

	for(i=0; i<n; i++){
		dialog_text(files[i],"",FIELD_SELECTABLE);
	}

	if(j = dialog_end()){
		if(file) {
			strcpy(file,files[j-1]);
		} else {
			file = files[j-1];
			files[j-1] = NULL;
		}
	}

	for(i=0; i<n; i++){
		free(files[i]);
	}

	return j ? file : NULL;

}

char* menu_requestfile(char* file, char *title, char* path, char *exts){
	char *dir;
	int allocmem = file == NULL;
	int tmplen = 0;
	char parent_dir_str[5];
#ifdef DINGOO_NATIVE
    static char dingoo_default_path[]="a:\\";
#endif /* DINGOO_NATIVE */

    /* TODO clear all the dyanamic memory allocations in this routine and require caller to pre-allocate */

	if (allocmem) file = malloc(PATH_MAX);
#ifdef DEBUG_ALWAYS_RETURN_ADJUSTRIS_GB
    strcpy(file, "adjustris.gb");
    return file;
#endif /* DEBUG_ALWAYS_RETURN_ADJUSTRIS_GB */
	if(path)
	{
		strcpy(file, path);
		tmplen = strlen(file);
		if (tmplen >=1)
		{
			if (file[tmplen-1] != DIRSEP_CHAR)
				strcat(file, DIRSEP); /* this is very fragile, e.g. what if dir was specified but does not exist */
		}
	}
	else
		strcpy(file, "");
    
	snprintf(parent_dir_str, sizeof(parent_dir_str), "..%s", DIRSEP);

	while(dir = menu_browsedir(file, file+strlen(file),title,exts)){
        if (!strcmp(dir, parent_dir_str))
        {
            /* need to go up a directory */
            dir--;
            if (dir > file)
            {
                *dir = '\0';
                while (dir >= file && *dir != DIRSEP_CHAR)
                {
                    *dir = '\0';
                    dir--;
                }
            }
            if (strlen(file) == 0)
            {
#ifdef DINGOO_NATIVE
                if (dingoo_default_path[0] == 'A')
                    dingoo_default_path[0] = 'B';
                else
                    dingoo_default_path[0] = 'A';
                sprintf(file, "%s", dingoo_default_path);
                /*
                **  If there is no MiniSD card in B:
                **  then an empty list is displayed
                **  click the parent directory to
                **  toggle back to A:
                */
#else
                sprintf(file, ".%s", DIRSEP);
#endif /* DINGOO_NATIVE */
                dir = file + strlen(file)+1;
                *dir = '\0';
            }
        }
		/*
		** Check to see if we have a file name,
		** or a new directory name to scan
		** Directory name will have trailing path sep character
		** FIXME check for "../" and dirname the dir
		*/
		tmplen = strlen(dir);
		if (tmplen >=1)
			if (dir[tmplen-1] != DIRSEP_CHAR)
			{
				if (allocmem) file = realloc(file, strlen(file)+1);
				break;
			}
	}
	if(!dir) file = NULL; /* FIXME what it file was not null when function was called, what about free'ing this is we allocated it? */

	return file;
}

char *menu_requestdir(const char *title, const char *path){
	char *dir=NULL, **ldirs;
	int ldirsz, ldirn=0, ret, l;
	DIR *cd;
	struct dirent *d;
#ifndef  DT_DIR
	struct stat s;
#endif /* DT_DIR */
	char *cdpath;
    
//#ifndef  DT_DIR
	char tmpfname[PATH_MAX];
	char *tmpfname_end;
//#endif /* DT_DIR */

	cdpath = malloc(PATH_MAX);

	strcpy(cdpath, path);
//#ifndef  DT_DIR
	strcpy(tmpfname, path);
	tmpfname_end = &tmpfname[0];
	tmpfname_end += strlen(tmpfname);
//#endif /* DT_DIR */

	while(!dir){
		cd = opendir(cdpath);

		dialog_begin(title, cdpath);
		dialog_text("[Select This Directory]",NULL,FIELD_SELECTABLE);
		dialog_text("/.. Parent Directory",NULL,FIELD_SELECTABLE);

		ldirsz = 16;
		ldirs = malloc(sizeof(char*)*ldirsz);

		d = readdir(cd);
		if(d && !strcmp(d->d_name,".")) d = readdir(cd);
		if(d && !strcmp(d->d_name,"..")) d = readdir(cd);

		while(d){
			if(ldirn >= ldirsz){
				ldirsz += 16;
				ldirs = realloc(ldirs,ldirsz*sizeof(char*));
			}

#ifndef  DT_DIR
			/* can not lookup type from search result have to stat filename*/
			strcpy(tmpfname_end, d->d_name);
			stat(tmpfname, &s);
			if(S_ISDIR (s.st_mode))
#else
			if ((d->d_type & DT_DIR) == DT_DIR)
#endif /* DT_DIR */
			{
				l = strlen(d->d_name);
				ldirs[ldirn] = malloc(l+2);
				strcpy(ldirs[ldirn], d->d_name);
				ldirs[ldirn][l] = DIRSEP_CHAR;
				ldirs[ldirn][l+1] = '\0';

				dialog_text(ldirs[ldirn],NULL,FIELD_SELECTABLE);
				ldirn++;
			}

			d = readdir(cd);
		}
		closedir(cd);

		switch(ret=dialog_end()){
			case 0:
				dir = (char*)-1;
				break;
			case 1:
				dir = strdup(cdpath);
				break;
			case 2:
				{
					/* need to go up a directory */
					char *tmp_str=NULL;
					tmp_str=cdpath;
					tmp_str+=strlen(tmp_str)-1;
					tmp_str--;
					if (tmp_str > cdpath)
					{
						*tmp_str = '\0';
						while (tmp_str >= cdpath && *tmp_str != DIRSEP_CHAR)
						{
							*tmp_str = '\0';
							tmp_str--;
						}
					}
					if (strlen(cdpath) == 0)
					{
						sprintf(cdpath, ".%s", DIRSEP);
						tmp_str = cdpath + strlen(cdpath)+1;
						*tmp_str = '\0';
					}
					strcpy(tmpfname, cdpath);
					tmpfname_end = &tmpfname[0];
					tmpfname_end += strlen(tmpfname);
				}
				break;
			default:
				strcpy(tmpfname_end, ldirs[ret-3]);
				tmpfname_end = &tmpfname[0];
				tmpfname_end += strlen(tmpfname);
				strcpy(cdpath, tmpfname);
				break;
		}

		while(ldirn)
			free(ldirs[--ldirn]);
		free(ldirs);
	}

	if(dir==(char*)-1)
		dir = NULL;

	free(cdpath);

	return dir;
}

char *menu_checkdir(char *path, char *alt_path){
	char *dir;
	DIR *direxist;
	direxist = opendir(path);
	if(direxist == NULL){		
		dir = alt_path;
	}else{
		closedir(direxist);
		dir = path;
	}
	return dir;
}

/* NOTE number of slot entries is indirectly related to font size */
/* only display one screen of save slots */
static const char *slots[] = {
    "Slot 0","Slot 1","Slot 2","Slot 3","Slot 4","Slot 5","Slot 6","Slot 7",
    "Slot 8","Slot 9","Slot 10","Slot 11","Slot 12","Slot 13","Slot 14","Slot 15",
    NULL};
static const char *emptyslot = "<Empty>";
static const char *not_emptyslot = "<Used>";

int menu_state(int save){

	char **statebody=NULL;
	char* name;

	int i, flags,ret, del=0,l;
#ifndef OHBOY_FILE_STAT_NOT_AVAILABLE
	/* Not all platforms implement stat()/fstat() */
	struct stat fstat;
	time_t time;
	char *tstr;
#endif

	char *savedir;
	char *savename;
	char *saveprefix;
	FILE *f;
	int sizeof_slots=0;
	int statesram=1;
    while (slots[sizeof_slots] != NULL)
        sizeof_slots++;
    statebody = malloc(sizeof_slots * sizeof(char*));  /* FIXME check for NULL return from malloc */

	statesram = rc_getint("statesram");
	savedir = rc_getstr("savedir");
	savename = rc_getstr("savename");
	saveprefix = malloc(strlen(savedir) + strlen(savename) + 2);
	sprintf(saveprefix, "%s%s%s", savedir, DIRSEP, savename);

	if(statesram==0){
		dialog_begin(save?"Save State":"                       Load State          (SRAM Off)",rom.name);
	}else if(statesram==1){
		dialog_begin(save?"Save State":"Load State",rom.name);
	}

	for(i=0; i<sizeof_slots; i++){

		name = malloc(strlen(saveprefix) + 5);
		sprintf(name, "%s.%03d", saveprefix, i);

#ifndef OHBOY_FILE_STAT_NOT_AVAILABLE
		/* if the file exists lookup the timestamp */
		if(!stat(name,&fstat)){
			time = fstat.st_mtime;
			tstr = ctime(&time);

			l = strlen(tstr);
			statebody[i] = malloc(l);
			strcpy(statebody[i],tstr);
			statebody[i][l-1]=0;
#else
		/* check if the file exists */
		if(f=fopen(name,"rb")){
			fclose(f);
			statebody[i] = (char*)not_emptyslot;
#endif /* OHBOY_FILE_STAT_NOT_AVAILABLE */
			flags = FIELD_SELECTABLE;
		} else {
			statebody[i] = (char*)emptyslot;
			flags = save ? FIELD_SELECTABLE : 0;
		}
		dialog_text(slots[i],statebody[i],flags);

		free(name);
	}

	if(ret=dialog_end()){
		name = malloc(strlen(saveprefix) + 5);
		sprintf(name, "%s.%03d", saveprefix, ret-1);
		if(save){
			if(f=fopen(name,"wb")){
				savestate(f);
				fclose(f);
			}
		}else if(statesram==0){
			if(f=fopen(name,"rb")){
				loadstate_nosram(f);
				fclose(f);
				vram_dirty();
				pal_dirty();
				sound_dirty();
				mem_updatemap();
			}
		}else if(statesram==1){
			if(f=fopen(name,"rb")){
				loadstate(f);
				fclose(f);
				vram_dirty();
				pal_dirty();
				sound_dirty();
				mem_updatemap();
			}
		}
		free(name);
	}

	for(i=0; i<sizeof_slots; i++)
		if(statebody[i] != emptyslot && statebody[i] != not_emptyslot) free(statebody[i]);

	free(saveprefix);
	return ret;
}

/*#############################DMG FILTERS##########################################*/

#define DMGFILT_COUNT 14
struct filt_s{
	char name[16];
	unsigned int red[4];
	unsigned int green[4];
	unsigned int blue[4];
}dmgfilt[DMGFILT_COUNT] = {
	{//Default Filter
		.name = "Default",
		.red   = { 95, 25, 0, 35 },
		.green = { 25, 170, 25, 35 },
		.blue  = { 25, 60, 125, 40 }
	},{//Dark Filter
		.name = "Low Contrast",
		.red   = { 60, 10, 0, 35 },
		.green = { 12, 75, 12, 35 },
		.blue  = { 10, 20, 65, 40 }
	},{//DMG Dark
		.name = "DMG Dark",
		.red   = { 32, 32, 32, 0 },
		.green = { 41, 41, 41, 24 },
		.blue  = { -11, -11, -11, 40 }
	},{//DMG Medium
		.name = "DMG Medium",
		.red   = { 32, 32, 32, 0 },
		.green = { 33, 33, 33, 48 },
		.blue  = { -13, -13, -13, 48 }
	},{//DMG Light
		.name = "DMG Light",
		.red   = { 32, 32, 32, 0 },
		.green = { 25, 25, 25, 72 },
		.blue  = { -19, -19, -19, 64 }
	},{//GBP Dark
		.name = "GBPocket Dark",
		.red   = { 37, 37, 37, 0 },
		.green = { 49, 49, 49, 24 },
		.blue  = { 29, 29, 29, 40 }
	},{//GBP Medium
		.name = "GBPocket Medium",
		.red   = { 37, 37, 37, 0 },
		.green = { 41, 41, 41, 48 },
		.blue  = { 26, 26, 26, 48 }
	},{//GBP Light
		.name = "GBPocket Light",
		.red   = { 37, 37, 37, 0 },
		.green = { 36, 36, 36, 64 },
		.blue  = { 21, 21, 21, 64 }
	},{//DemoVision
		.name = "DemoVision",
		.red   = { 61, 61, 61, 72 },
		.green = { 50, 50, 50, 88 },
		.blue  = { 48, 48, 48, 0 }
	},{//DemoVision Dark
		.name = "DemoVision Dark",
		.red   = { 73, 73, 73, 36 },
		.green = { 62, 62, 62, 51 },
		.blue  = { 48, 48, 48, 0 }
	},{//Color 0
		.name = "Grayscale",
		.red   = { 85, 85, 85, 0 },
		.green = { 85, 85, 85, 0 },
		.blue  = { 85, 85, 85, 0 }
	},{//Color 25
		.name = "Color 25%",
		.red   = { 105, 75, 75, 0 },
		.green = { 75, 105, 75, 0 },
		.blue  = { 75, 75, 105, 0 }
	},{//Color 50
		.name = "Color 50%",
		.red   = { 130, 63, 63, 0 },
		.green = { 63, 130, 63, 0 },
		.blue  = { 63, 63, 130, 0 }
	},{//Color 75
		.name = "Color 75%",
		.red   = { 170, 43, 43, 0 },
		.green = { 43, 170, 43, 0 },
		.blue  = { 43, 43, 170, 0 }
	}
} ;

int findfilt(){
	int *a, *b;
	int i,j;
	for(i=0; i<DMGFILT_COUNT; i++){
		a = dmgfilt[i].red ; b = rc_getvec("red");
		if(a[0] != b[0] || a[1] != b[1] || a[2] != b[2] || a[3] != b[3])
			continue;
		a = dmgfilt[i].green; b = rc_getvec("green");
		if(a[0] != b[0] || a[1] != b[1] || a[2] != b[2] || a[3] != b[3])
			continue;
		a = dmgfilt[i].blue; b = rc_getvec("blue");
		if(a[0] != b[0] || a[1] != b[1] || a[2] != b[2] || a[3] != b[3])
			continue;
		return i;
	}
	return 0;
}
	
char *ldmgfilters[] = {
	dmgfilt[0].name,
	dmgfilt[1].name,
	dmgfilt[2].name,
	dmgfilt[3].name,
	dmgfilt[4].name,
	dmgfilt[5].name,
	dmgfilt[6].name,
	dmgfilt[7].name,
	dmgfilt[8].name,
	dmgfilt[9].name,
	dmgfilt[10].name,
	dmgfilt[11].name,
	dmgfilt[12].name,
	dmgfilt[13].name,
	NULL
};

/*#################################################################################*/


const char *lbutton_a[] = {"None","Button A","Button B","Select","Start","Reset","Quit",NULL};
const char *lbutton_b[] = {"None","Button A","Button B","Select","Start","Reset","Quit",NULL};
const char *lbutton_x[] = {"None","Button A","Button B","Select","Start","Reset","Quit",NULL};
const char *lbutton_y[] = {"None","Button A","Button B","Select","Start","Reset","Quit",NULL};
const char *lbutton_l[] = {"None","Button A","Button B","Select","Start","Reset","Quit",NULL};
const char *lbutton_r[] = {"None","Button A","Button B","Select","Start","Reset","Quit",NULL};
const char *lcolorfilter[] = {"Off","On","GBC Only",NULL};
#ifdef GCWZERO
const char *lupscaler[] = {"Native (No scale)", "Software 1.5x", "Scale3x+Sample.75x", "Software 1.666x", "Software Fullscreen", "Hardware 1.5x", "Hardware 1.666x", "Hardware FullScreen", NULL};
#else
const char *lupscaler[] = {"Native (No scale)", "Ayla 1.5x Upscaler", "Scale3x+Sample.75x", "1.666x Upscaler", "Ayla Fullscreen", NULL};
#endif /* GCW ZERO */
const char *lframeskip[] = {"Auto","Off","1","2","3","4",NULL};
const char *lsdl_showfps[] = {"Off","On",NULL};
const char *lborderon[] = {"Off","Image File","BG Color",NULL};

#if WIZ
const char *lclockspeeds[] = {"Default","250 mhz","300 mhz","350 mhz","400 mhz","450 mhz","500 mhz","550 mhz","600 mhz","650 mhz","700 mhz","750 mhz",NULL};
#endif
#ifdef DINGOO_NATIVE
const char *lclockspeeds[] = { "Default", "200 mhz", "250 mhz", "300 mhz", "336 mhz", "360 mhz", "400 mhz", NULL };
#endif
const char *lsndlvl[] = {"0%", "10%", "20%", "30%", "40%", "50%", "60%", "70%", "80%", "90%", "100%", NULL};
const char *lstatesram[] = {"Off","Default (On)",NULL};



static char config[24][256];

int menu_options(){

	struct filt_s *filtp=0;
	int filt=0, skip=0, ret=0, cfilter=0, sfps=0, upscale=0, speed=0, i=0, sndlvl=10, statesram=1, borderon=0;
	char *romdir=0, *romtemp=0, *paldir=0, *loadpal=0, *pal=0, *paltemp=0, *palname=0, *borderdir=0, *loadborder=0, *border=0, *bordertemp=0, *bordername=0, *gbcborder=0, *gbcbordertemp=0, *gbcbordername=0;

	FILE *file;
#ifdef DINGOO_NATIVE
    /*
    **  100Mhz once caused Dingoo A320 MIPS to hang,
    **  when 100Mhz worked BW GB (Adjustris) game was running at 32 fps (versus 60 at 200Mhz).
    **  150Mhz has never worked on my Dingoo A320.
    */
    uintptr_t dingoo_clock_speeds[] = { 200000000, 250000000, 300000000, 336000000, 360000000, 400000000 /* , 430000000 Should not be needed */ };
    /*
    ** under-under clock option is for GB games.
    ** GB games can often be ran under the already
    ** underclocked Dingoo speed of 336Mhz
    */

    bool dingoo_clock_change_result;
	uintptr_t tempCore=336000000; /* default Dingoo A320 clock speed */
	uintptr_t tempMemory=112000000; /* default Dingoo A320 memory speed */
	cpu_clock_get(&tempCore, &tempMemory);
#endif /* DINGOO_NATIVE */

	filt = findfilt();
	cfilter = rc_getint("colorfilter");
	if(cfilter && !rc_getint("filterdmg")) cfilter = 2;
	upscale = rc_getint("upscaler");
	skip = rc_getint("frameskip")+1;
	sfps = rc_getint("showfps");
	sndlvl = rc_getint("sndlvl");
	statesram = rc_getint("statesram");
	borderon = rc_getint("bmpenabled");
	
#ifdef DINGOO_NATIVE
	speed = 0;
#else
	speed = rc_getint("cpuspeed")/50 - 4;
#endif /* DINGOO_NATIVE */
	if(speed<0) speed = 0;
	if(speed>11) speed = 11;
#ifdef DINGOO_SIM
#else
#ifdef DINGOO_OPENDINGUX
	romdir = menu_checkdir(rc_getstr("romdir"),getenv("HOME"));
#else
	romdir = rc_getstr("romdir");
#endif /* DINGOO_OPENDINGUX */
#endif /* DINGOO_SIM */

	romdir = romdir ? strdup(romdir) : strdup(".");
	
#ifdef DINGOO_SIM
    paldir = "a:\\ohboy\\palettes\\";
#else	
#ifdef DINGOO_OPENDINGUX
	paldir = malloc(strlen(getenv("HOME")) + 28);
	sprintf(paldir, "%s/.ohboy/palettes/", getenv("HOME"));
#else	
	paldir = "palettes";
#endif /* DINGOO_OPENDINGUX */
#endif /* DINGOO_SIM */
    pal = rc_getstr("palette");
	pal = pal ? strdup(pal) : strdup("Select Palette");
	
	paltemp = strdup(pal);
	palname = basexname(paltemp);
	
#ifdef DINGOO_SIM
    borderdir = "a:\\ohboy\\borders\\";
#else	
#ifdef DINGOO_OPENDINGUX
	borderdir = malloc(strlen(getenv("HOME")) + 28);
	sprintf(borderdir, "%s/.ohboy/borders/", getenv("HOME"));
#else	
	borderdir = "borders";
#endif /* DINGOO_OPENDINGUX */
#endif /* DINGOO_SIM */
    border = rc_getstr("border");
	border = border ? strdup(border) : strdup("Select DMG Border");	
	bordertemp = strdup(border);
	bordername = basexname(bordertemp);
	
	gbcborder = rc_getstr("gbcborder");
	gbcborder = gbcborder ? strdup(gbcborder) : strdup("Select GBC Border");	
	gbcbordertemp = strdup(gbcborder);
	gbcbordername = basexname(gbcbordertemp);
	
	start:

	dialog_begin("Options",NULL);

	dialog_text("Mono Palette",palname,FIELD_SELECTABLE);               /* 1 */
	dialog_option("Color Filter",lcolorfilter,&cfilter);        /* 2 */
	dialog_option("Filter Type",ldmgfilters,&filt);             /* 3 */
	dialog_option("Upscaler",lupscaler,&upscale);               /* 4 */
	dialog_option("Frameskip",lframeskip,&skip);                /* 5 */
	dialog_option("Show FPS",lsdl_showfps,&sfps);               /* 6 */
	dialog_option("Use Borders",lborderon,&borderon);               /* 7 */
	dialog_text("DMG Border Image",bordername,FIELD_SELECTABLE);             /* 8 */
	dialog_text("GBC Border Image",gbcbordername,FIELD_SELECTABLE);             /* 9 */
#if defined(WIZ) || defined(DINGOO_NATIVE)
	dialog_option("Clock Speed",lclockspeeds,&speed);           /* 10 */
#else
	dialog_text("Clock Speed","Default",0);                     /* 10 */
#endif
#ifdef DINGOO_SIM
	dialog_text("Rom Path", "Default", 0);                      /* 11 */
#else
	dialog_text("Rom Path",romdir,FIELD_SELECTABLE);            /* 11 */
#endif /* DINGOO_SIM */
#ifdef GNUBOY_HARDWARE_VOLUME
	dialog_option("Volume",lsndlvl,&sndlvl);   /* 12 */ /* this is not the OSD volume.. */
#else
	dialog_text("Volume", "Default", 0);                        /* 12 */ /* this is not the OSD volume.. */
#endif /* GNBOY_HARDWARE_VOLUME */
	dialog_option("State SRAM",lstatesram,&statesram);          /* 13 */
	dialog_text(NULL,NULL,0);                                   /* 14 */
	dialog_text("Apply",NULL,FIELD_SELECTABLE);                 /* 15 */
	dialog_text("Apply & Save",NULL,FIELD_SELECTABLE);          /* 16 */
	dialog_text("Cancel",NULL,FIELD_SELECTABLE);                /* 17 */


	switch(ret=dialog_end()){
		case 1: /* "Mono Palette" */
		    loadpal = menu_requestfile(NULL,"Select Palette",paldir,"pal");
			if(loadpal){
				pal = loadpal;
				paltemp = strdup(pal);
				palname = basexname(paltemp);
			}	
			goto start;
		case 8: /* "DMG Border Image" */
#ifdef OHBOY_USE_SDL_IMAGE
		    loadborder = menu_requestfile(NULL,"Select DMG Border Image",borderdir,"bmp;png");
#else
			loadborder = menu_requestfile(NULL,"Select DMG Border Image",borderdir,"bmp");
#endif /* OHBOY_USE_SDL_IMAGE */
			if(loadborder){
				border = loadborder;
				bordertemp = strdup(border);
				bordername = basexname(bordertemp);
			}	
			goto start;
		case 9: /* "GBC Border Image" */
#ifdef OHBOY_USE_SDL_IMAGE
		    loadborder = menu_requestfile(NULL,"Select GBC Border Image",borderdir,"bmp;png");
#else
			loadborder = menu_requestfile(NULL,"Select GBC Border Image",borderdir,"bmp");
#endif /* OHBOY_USE_SDL_IMAGE */
			if(loadborder){
				gbcborder = loadborder;
				gbcbordertemp = strdup(gbcborder);
				gbcbordername = basexname(gbcbordertemp);
			}	
			goto start;
		case 11: /* "Rom Path" romdir */
			romtemp = menu_requestdir("Select Rom Directory",romdir);
			if(romtemp){
				free(romdir);
				romdir = romtemp;
			}
			goto start;
		case 17: /* Cancel */
			return ret;
			break;
		case 15: /* Apply */
		case 16: /* Apply & Save */
			#ifdef GNUBOY_HARDWARE_VOLUME
			pcm_volume(sndlvl * 10);
			#endif /* GNBOY_HARDWARE_VOLUME */
			filtp = &dmgfilt[filt];
			if(speed)
			{
#ifdef DINGOO_NATIVE
                /*
                ** For now do NOT plug in into settings system, current
                ** (Wiz) speed system is focused on multiples of 50Mhz.
                ** Dingoo default clock speed is 336Mhz (CPU certified for
                ** 360, 433MHz is supposed to be possible).
                ** Only set clock speed if changed in options each and
                ** everytime - do not use config file
                */
                --speed;
                /* check menu response is withing the preset array range/size */
                if (speed > (sizeof(dingoo_clock_speeds)/sizeof(uintptr_t) - 1) )
                    speed = 0;
                
                tempCore = dingoo_clock_speeds[speed];
                dingoo_clock_change_result = cpu_clock_set(tempCore);
                
                tempCore=tempMemory=0;
                cpu_clock_get(&tempCore, &tempMemory); /* currently unused */
                /* TODO display clock speed next to on screen FPS indicator */
#endif /* DINGOO_NATIVE */
    
				speed = speed*50 + 200;
			}
			
			#ifdef DINGOO_NATIVE /* FIXME Windows too..... if (DIRSEP_CHAR == '\\').... */
			{
				char tmp_pal[PATH_MAX];
				char *destp, *srcp;
				destp = &tmp_pal[0];
				srcp = pal;
				
				/* escape the path seperator (should escape other things too.) */
				while(*destp = *srcp++)
				{
					if (*destp == DIRSEP_CHAR)
					{
						destp++;
						*destp = DIRSEP_CHAR;
					}
					destp++;
				}
			
				sprintf(config[0], "source \"%s\"", tmp_pal);
				sprintf(config[1], "set palette \"%s\"", tmp_pal);
			}
			#else
			sprintf(config[0],"source \"%s\"",pal);
			sprintf(config[1],"set palette \"%s\"",pal);
			#endif /* DINGOO_NATIVE */
			sprintf(config[2],"set colorfilter %i",cfilter!=0);
			sprintf(config[3],"set filterdmg %i",cfilter==1);
			sprintf(config[4],"set upscaler %i",upscale);
			sprintf(config[5],"set frameskip %i",skip-1);
			sprintf(config[6],"set showfps %i",sfps);
			sprintf(config[7],"set bmpenabled %i",borderon);
			#ifdef DINGOO_NATIVE /* FIXME Windows too..... if (DIRSEP_CHAR == '\\').... */
			{
				char tmp_bord[PATH_MAX];
				char *destb, *srcb;
				destb = &tmp_bord[0];
				srcb = border;
				
				/* escape the path seperator (should escape other things too.) */
				while(*destb = *srcb++)
				{
					if (*destb == DIRSEP_CHAR)
					{
						destb++;
						*destb = DIRSEP_CHAR;
					}
					destb++;
				}
			
				sprintf(config[8], "set border \"%s\"", tmp_bord);
			}
			#else
			sprintf(config[8],"set border \"%s\"",border);
			#endif /* DINGOO_NATIVE */
			#ifdef DINGOO_NATIVE /* FIXME Windows too..... if (DIRSEP_CHAR == '\\').... */
			{
				char tmp_gbcbord[PATH_MAX];
				char *destc, *srcc;
				destc = &tmp_gbcbord[0];
				srcc = gbcborder;
				
				/* escape the path seperator (should escape other things too.) */
				while(*destc = *srcc++)
				{
					if (*destc == DIRSEP_CHAR)
					{
						destc++;
						*destc = DIRSEP_CHAR;
					}
					destc++;
				}
			
				sprintf(config[9], "set gbcborder \"%s\"", tmp_gbcbord);
			}
			#else
			sprintf(config[9],"set gbcborder \"%s\"",gbcborder);
			#endif /* DINGOO_NATIVE */
			sprintf(config[10],"set cpuspeed %i",speed);
			#ifdef DINGOO_NATIVE /* FIXME Windows too..... if (DIRSEP_CHAR == '\\').... */
			{
				char tmp_path[PATH_MAX];
				char *dest, *src;
				dest = &tmp_path[0];
				src = romdir;
				
				/* escape the path seperator (should escape other things too.) */
				while(*dest = *src++)
				{
					if (*dest == DIRSEP_CHAR)
					{
						dest++;
						*dest = DIRSEP_CHAR;
					}
					dest++;
				}
			
				sprintf(config[11], "set romdir \"%s\"", tmp_path);
			}
			#else
			sprintf(config[11],"set romdir \"%s\"",romdir);
			#endif /* DINGOO_NATIVE */
			sprintf(config[12],"set red 0x%.6x 0x%.6x 0x%.6x 0x%.6x", filtp->red[0], filtp->red[1], filtp->red[2], filtp->red[3]);
			sprintf(config[13],"set green 0x%.6x 0x%.6x 0x%.6x 0x%.6x", filtp->green[0], filtp->green[1], filtp->green[2], filtp->green[3]);
			sprintf(config[14],"set blue 0x%.6x 0x%.6x 0x%.6x 0x%.6x", filtp->blue[0], filtp->blue[1], filtp->blue[2], filtp->blue[3]);
			sprintf(config[15], "set sndlvl %i", sndlvl);
			sprintf(config[16], "set statesram %i", statesram);
			
			for(i=0; i<17; i++)
				rc_command(config[i]);

			pal_dirty();

			if (ret == 16){ /* Apply & Save */
#ifdef DINGOO_SIM
				file = fopen("a:"DIRSEP"ohboy"DIRSEP"ohboy.rc","w");
#else
#ifdef DINGOO_OPENDINGUX
				static char *ohboyrc;
				ohboyrc = malloc(strlen(getenv("HOME")) + 28);
				sprintf(ohboyrc, "%s/.ohboy/ohboy.rc", getenv("HOME"));
				file = fopen(ohboyrc,"w");
				free (ohboyrc);
#else
				file = fopen("ohboy.rc","w");
#endif /* DINGOO_OPENDINGUX */
#endif /* DINGOO_SIM */
				for(i=0; i<17; i++){
					fputs(config[i],file);
					fputs("\n",file);
				}
				fclose(file);
			}
			vid_init();
			scaler_init(0);

		break;
	}

	free(romdir);
	free(paldir);
	free(borderdir);

	return ret;
}

int menu_controls(){

	int ret=0, i=0;
#ifdef DINGOO_BUILD
	int btna=1, btnb=2, btnx=0, btny=0, btnl=0, btnr=0;
#else
	int btna=2, btnb=1, btnx=0, btny=0, btnl=0, btnr=0;
#endif

	FILE *file;

	btna = rc_getint("button_a");
	btnb = rc_getint("button_b");
	btnx = rc_getint("button_x");
	btny = rc_getint("button_y");
	btnl = rc_getint("button_l");
	btnr = rc_getint("button_r");

	start:

	dialog_begin("Controls",NULL);

	#if defined(CAANOO)
	dialog_text("Caanoo","Gameboy",0);                       /* 1 */
	#endif
	#if defined(WIZ)
	dialog_text("Wiz","Gameboy",0);                          /* 1 */
	#endif
	#if defined(DINGOO_NATIVE)
	dialog_text("Dingoo","Gameboy",0);                       /* 1 */
	#endif
	#if defined(DINGOO_OPENDINGUX)
	#if defined(GCWZERO)
	dialog_text("GCW Zero","Gameboy",0);                     /* 1 */
	#else
	dialog_text("Dingoo","Gameboy",0);                       /* 1 */
	#endif /* GCWZERO */
	#endif /* DINGOO_OPENDINGUX */
	#if defined(GP2X_ONLY)
	dialog_text("GP2X","Gameboy",0);                         /* 1 */
	#endif
	dialog_text(NULL,NULL,0);                                /* 2 */
	dialog_option("Button A",lbutton_a,&btna);               /* 3 */
	dialog_option("Button B",lbutton_b,&btnb);               /* 4 */
	dialog_option("Button X",lbutton_x,&btnx);               /* 5 */
	dialog_option("Button Y",lbutton_y,&btny);               /* 6 */
	dialog_option("Button L",lbutton_l,&btnl);               /* 7 */
	dialog_option("Button R",lbutton_r,&btnr);               /* 8 */
	dialog_text(NULL,NULL,0);                                /* 9 */
	dialog_text("Apply",NULL,FIELD_SELECTABLE);              /* 10 */
	dialog_text("Apply & Save",NULL,FIELD_SELECTABLE);       /* 11 */
	dialog_text("Cancel",NULL,FIELD_SELECTABLE);             /* 12 */

	switch(ret=dialog_end()){
		case 12: /* Cancel */
			return ret;
			break;
		case 10: /* Apply */
		case 11: /* Apply & Save */
		    sprintf(config[0],"#KEY BINDINGS#");
			sprintf(config[1],"set button_a %i",btna);
			sprintf(config[2],"set button_b %i",btnb);
			sprintf(config[3],"set button_x %i",btnx);
			sprintf(config[4],"set button_y %i",btny);
			sprintf(config[5],"set button_l %i",btnl);
			sprintf(config[6],"set button_r %i",btnr);
			#if defined(CAANOO)
			    if (btna == 0) {sprintf(config[7],"unbind joy0");}
			    if (btnb == 0) {sprintf(config[8],"unbind joy2");}
			    if (btnx == 0) {sprintf(config[9],"unbind joy1");}
			    if (btny == 0) {sprintf(config[10],"unbind joy3");}
			    if (btnl == 0) {sprintf(config[11],"unbind joy4");}
			    if (btnr == 0) {sprintf(config[12],"unbind joy5");}

			    if (btna == 1) {sprintf(config[7],"bind joy0 +a");}
			    if (btnb == 1) {sprintf(config[8],"bind joy2 +a");}
			    if (btnx == 1) {sprintf(config[9],"bind joy1 +a");}
			    if (btny == 1) {sprintf(config[10],"bind joy3 +a");}
			    if (btnl == 1) {sprintf(config[11],"bind joy4 +a");}
			    if (btnr == 1) {sprintf(config[12],"bind joy5 +a");}

			    if (btna == 2) {sprintf(config[7],"bind joy0 +b");}
			    if (btnb == 2) {sprintf(config[8],"bind joy2 +b");}
			    if (btnx == 2) {sprintf(config[9],"bind joy1 +b");}
			    if (btny == 2) {sprintf(config[10],"bind joy3 +b");}
			    if (btnl == 2) {sprintf(config[11],"bind joy4 +b");}
			    if (btnr == 2) {sprintf(config[12],"bind joy5 +b");}

			    if (btna == 3) {sprintf(config[7],"bind joy0 +select");}
			    if (btnb == 3) {sprintf(config[8],"bind joy2 +select");}
			    if (btnx == 3) {sprintf(config[9],"bind joy1 +select");}
			    if (btny == 3) {sprintf(config[10],"bind joy3 +select");}
			    if (btnl == 3) {sprintf(config[11],"bind joy4 +select");}
			    if (btnr == 3) {sprintf(config[12],"bind joy5 +select");}

			    if (btna == 4) {sprintf(config[7],"bind joy0 +start");}
			    if (btnb == 4) {sprintf(config[8],"bind joy2 +start");}
			    if (btnx == 4) {sprintf(config[9],"bind joy1 +start");}
			    if (btny == 4) {sprintf(config[10],"bind joy3 +start");}
			    if (btnl == 4) {sprintf(config[11],"bind joy4 +start");}
			    if (btnr == 4) {sprintf(config[12],"bind joy5 +start");}

			    if (btna == 5) {sprintf(config[7],"bind joy0 reset");}
			    if (btnb == 5) {sprintf(config[8],"bind joy2 reset");}
			    if (btnx == 5) {sprintf(config[9],"bind joy1 reset");}
			    if (btny == 5) {sprintf(config[10],"bind joy3 reset");}
			    if (btnl == 5) {sprintf(config[11],"bind joy4 reset");}
			    if (btnr == 5) {sprintf(config[12],"bind joy5 reset");}

			    if (btna == 6) {sprintf(config[7],"bind joy0 quit");}
			    if (btnb == 6) {sprintf(config[8],"bind joy2 quit");}
			    if (btnx == 6) {sprintf(config[9],"bind joy1 quit");}
			    if (btny == 6) {sprintf(config[10],"bind joy3 quit");}
			    if (btnl == 6) {sprintf(config[11],"bind joy4 quit");}
			    if (btnr == 6) {sprintf(config[12],"bind joy5 quit");}
			#endif
			#if defined(DINGOO_BUILD)
			#if defined(GCWZERO)
			    if (btna == 0) {sprintf(config[7],"unbind ctrl");}
			    if (btnb == 0) {sprintf(config[8],"unbind alt");}
			    if (btnx == 0) {sprintf(config[9],"unbind shift");}
			    if (btny == 0) {sprintf(config[10],"unbind space");}
			    if (btnl == 0) {sprintf(config[11],"unbind tab");}
			    if (btnr == 0) {sprintf(config[12],"unbind backspace");}

			    if (btna == 1) {sprintf(config[7],"bind ctrl +a");}
			    if (btnb == 1) {sprintf(config[8],"bind alt +a");}
			    if (btnx == 1) {sprintf(config[9],"bind shift +a");}
			    if (btny == 1) {sprintf(config[10],"bind space +a");}
			    if (btnl == 1) {sprintf(config[11],"bind tab +a");}
			    if (btnr == 1) {sprintf(config[12],"bind backspace +a");}

			    if (btna == 2) {sprintf(config[7],"bind ctrl +b");}
			    if (btnb == 2) {sprintf(config[8],"bind alt +b");}
			    if (btnx == 2) {sprintf(config[9],"bind shift +b");}
			    if (btny == 2) {sprintf(config[10],"bind space +b");}
			    if (btnl == 2) {sprintf(config[11],"bind tab +b");}
			    if (btnr == 2) {sprintf(config[12],"bind backspace +b");}

			    if (btna == 3) {sprintf(config[7],"bind ctrl +select");}
			    if (btnb == 3) {sprintf(config[8],"bind alt +select");}
			    if (btnx == 3) {sprintf(config[9],"bind shift +select");}
			    if (btny == 3) {sprintf(config[10],"bind space +select");}
			    if (btnl == 3) {sprintf(config[11],"bind tab +select");}
			    if (btnr == 3) {sprintf(config[12],"bind backspace +select");}

			    if (btna == 4) {sprintf(config[7],"bind ctrl +start");}
			    if (btnb == 4) {sprintf(config[8],"bind alt +start");}
			    if (btnx == 4) {sprintf(config[9],"bind shift +start");}
			    if (btny == 4) {sprintf(config[10],"bind space +start");}
			    if (btnl == 4) {sprintf(config[11],"bind tab +start");}
			    if (btnr == 4) {sprintf(config[12],"bind backspace +start");}

			    if (btna == 5) {sprintf(config[7],"bind ctrl reset");}
			    if (btnb == 5) {sprintf(config[8],"bind alt reset");}
			    if (btnx == 5) {sprintf(config[9],"bind shift reset");}
			    if (btny == 5) {sprintf(config[10],"bind space reset");}
			    if (btnl == 5) {sprintf(config[11],"bind tab reset");}
			    if (btnr == 5) {sprintf(config[12],"bind backspace reset");}

			    if (btna == 6) {sprintf(config[7],"bind ctrl quit");}
			    if (btnb == 6) {sprintf(config[8],"bind alt quit");}
			    if (btnx == 6) {sprintf(config[9],"bind shift quit");}
			    if (btny == 6) {sprintf(config[10],"bind space quit");}
			    if (btnl == 6) {sprintf(config[11],"bind tab quit");}
			    if (btnr == 6) {sprintf(config[12],"bind backspace quit");}
			#else
			    if (btna == 0) {sprintf(config[7],"unbind ctrl");}
			    if (btnb == 0) {sprintf(config[8],"unbind alt");}
			    if (btnx == 0) {sprintf(config[9],"unbind space");}
			    if (btny == 0) {sprintf(config[10],"unbind shift");}
			    if (btnl == 0) {sprintf(config[11],"unbind tab");}
			    if (btnr == 0) {sprintf(config[12],"unbind backspace");}

			    if (btna == 1) {sprintf(config[7],"bind ctrl +a");}
			    if (btnb == 1) {sprintf(config[8],"bind alt +a");}
			    if (btnx == 1) {sprintf(config[9],"bind space +a");}
			    if (btny == 1) {sprintf(config[10],"bind shift +a");}
			    if (btnl == 1) {sprintf(config[11],"bind tab +a");}
			    if (btnr == 1) {sprintf(config[12],"bind backspace +a");}

			    if (btna == 2) {sprintf(config[7],"bind ctrl +b");}
			    if (btnb == 2) {sprintf(config[8],"bind alt +b");}
			    if (btnx == 2) {sprintf(config[9],"bind space +b");}
			    if (btny == 2) {sprintf(config[10],"bind shift +b");}
			    if (btnl == 2) {sprintf(config[11],"bind tab +b");}
			    if (btnr == 2) {sprintf(config[12],"bind backspace +b");}

			    if (btna == 3) {sprintf(config[7],"bind ctrl +select");}
			    if (btnb == 3) {sprintf(config[8],"bind alt +select");}
			    if (btnx == 3) {sprintf(config[9],"bind space +select");}
			    if (btny == 3) {sprintf(config[10],"bind shift +select");}
			    if (btnl == 3) {sprintf(config[11],"bind tab +select");}
			    if (btnr == 3) {sprintf(config[12],"bind backspace +select");}

			    if (btna == 4) {sprintf(config[7],"bind ctrl +start");}
			    if (btnb == 4) {sprintf(config[8],"bind alt +start");}
			    if (btnx == 4) {sprintf(config[9],"bind space +start");}
			    if (btny == 4) {sprintf(config[10],"bind shift +start");}
			    if (btnl == 4) {sprintf(config[11],"bind tab +start");}
			    if (btnr == 4) {sprintf(config[12],"bind backspace +start");}

			    if (btna == 5) {sprintf(config[7],"bind ctrl reset");}
			    if (btnb == 5) {sprintf(config[8],"bind alt reset");}
			    if (btnx == 5) {sprintf(config[9],"bind space reset");}
			    if (btny == 5) {sprintf(config[10],"bind shift reset");}
			    if (btnl == 5) {sprintf(config[11],"bind tab reset");}
			    if (btnr == 5) {sprintf(config[12],"bind backspace reset");}

			    if (btna == 6) {sprintf(config[7],"bind ctrl quit");}
			    if (btnb == 6) {sprintf(config[8],"bind alt quit");}
			    if (btnx == 6) {sprintf(config[9],"bind space quit");}
			    if (btny == 6) {sprintf(config[10],"bind shift quit");}
			    if (btnl == 6) {sprintf(config[11],"bind tab quit");}
			    if (btnr == 6) {sprintf(config[12],"bind backspace quit");}
			#endif /* GCWZERO */
			#endif /* DINGOO_BUILD */
			#if defined(WIZ) || defined(GP2X_ONLY)
			    if (btna == 0) {sprintf(config[7],"unbind joy12");}
			    if (btnb == 0) {sprintf(config[8],"unbind joy13");}
			    if (btnx == 0) {sprintf(config[9],"unbind joy14");}
			    if (btny == 0) {sprintf(config[10],"unbind joy15");}
			    if (btnl == 0) {sprintf(config[11],"unbind joy10");}
			    if (btnr == 0) {sprintf(config[12],"unbind joy11");}

			    if (btna == 1) {sprintf(config[7],"bind joy12 +a");}
			    if (btnb == 1) {sprintf(config[8],"bind joy13 +a");}
			    if (btnx == 1) {sprintf(config[9],"bind joy14 +a");}
			    if (btny == 1) {sprintf(config[10],"bind joy15 +a");}
			    if (btnl == 1) {sprintf(config[11],"bind joy10 +a");}
			    if (btnr == 1) {sprintf(config[12],"bind joy11 +a");}

			    if (btna == 2) {sprintf(config[7],"bind joy12 +b");}
			    if (btnb == 2) {sprintf(config[8],"bind joy13 +b");}
			    if (btnx == 2) {sprintf(config[9],"bind joy14 +b");}
			    if (btny == 2) {sprintf(config[10],"bind joy15 +b");}
			    if (btnl == 2) {sprintf(config[11],"bind joy10 +b");}
			    if (btnr == 2) {sprintf(config[12],"bind joy11 +b");}

			    if (btna == 3) {sprintf(config[7],"bind joy12 +select");}
			    if (btnb == 3) {sprintf(config[8],"bind joy13 +select");}
			    if (btnx == 3) {sprintf(config[9],"bind joy14 +select");}
			    if (btny == 3) {sprintf(config[10],"bind joy15 +select");}
			    if (btnl == 3) {sprintf(config[11],"bind joy10 +select");}
			    if (btnr == 3) {sprintf(config[12],"bind joy11 +select");}

			    if (btna == 4) {sprintf(config[7],"bind joy12 +start");}
			    if (btnb == 4) {sprintf(config[8],"bind joy13 +start");}
			    if (btnx == 4) {sprintf(config[9],"bind joy14 +start");}
			    if (btny == 4) {sprintf(config[10],"bind joy15 +start");}
			    if (btnl == 4) {sprintf(config[11],"bind joy10 +start");}
			    if (btnr == 4) {sprintf(config[12],"bind joy11 +start");}

			    if (btna == 5) {sprintf(config[7],"bind joy12 reset");}
			    if (btnb == 5) {sprintf(config[8],"bind joy13 reset");}
			    if (btnx == 5) {sprintf(config[9],"bind joy14 reset");}
			    if (btny == 5) {sprintf(config[10],"bind joy15 reset");}
			    if (btnl == 5) {sprintf(config[11],"bind joy10 reset");}
			    if (btnr == 5) {sprintf(config[12],"bind joy11 reset");}

			    if (btna == 6) {sprintf(config[7],"bind joy12 quit");}
			    if (btnb == 6) {sprintf(config[8],"bind joy13 quit");}
			    if (btnx == 6) {sprintf(config[9],"bind joy14 quit");}
			    if (btny == 6) {sprintf(config[10],"bind joy15 quit");}
			    if (btnl == 6) {sprintf(config[11],"bind joy10 quit");}
			    if (btnr == 6) {sprintf(config[12],"bind joy11 quit");}
			#endif
			for(i=0; i<13; i++)
				rc_command(config[i]);

			pal_dirty();

			if (ret == 11){ /* Apply & Save */
#ifdef DINGOO_SIM
				file = fopen("a:"DIRSEP"ohboy"DIRSEP"bindings.rc","w");
#else
#ifdef DINGOO_OPENDINGUX
				static char *bindingsrc;
				bindingsrc = malloc(strlen(getenv("HOME")) + 28);
				sprintf(bindingsrc, "%s/.ohboy/bindings.rc", getenv("HOME"));
				file = fopen(bindingsrc,"w");
				free (bindingsrc);
#else
				file = fopen("bindings.rc","w");
#endif /* DINGOO_OPENDINGUX */
#endif /* DINGOO_SIM */
				for(i=0; i<13; i++){
					fputs(config[i],file);
					fputs("\n",file);
				}
				fclose(file);
			}
		break;
	}
	return ret;
}

int menu_about(){

int ret=0;

char version_str[80];
char ohboy_ver_str[80];

snprintf(version_str, sizeof(version_str)-1, "Gnuboy %s", VERSION);
snprintf(ohboy_ver_str, sizeof(ohboy_ver_str)-1, "%s Unofficial", OHBOY_VER);

	start:

	dialog_begin("About OhBoy",NULL);

	dialog_text(NULL,"OhBoy is a Gameboy emulator for",0);
	dialog_text(NULL,"small handhelds, using the",0);
	dialog_text(NULL,"Gnuboy emulation core with a",0);
	dialog_text(NULL,"basic menu system.",0);
	dialog_text(NULL,NULL,FIELD_SELECTABLE); /* Not visible, but always selected, allows to exit to main menu by pressing the "selection" button. You can also go to main menu pressing the "back" button */
	dialog_text(NULL,"OhBoy version:",0);
	dialog_text(NULL,ohboy_ver_str,0);
	dialog_text(NULL,NULL,0);
	dialog_text(NULL,"Using:",0);
	dialog_text(NULL,version_str,0);
	dialog_text(NULL,NULL,0);
	dialog_text(NULL,"More info:",0);
	dialog_text(NULL,"http://github.com/hi-ban/ohboy/",0);
	dialog_text(NULL,"http://ohboy.googlecode.com/",0);
	dialog_text(NULL,"http://gnuboy.googlecode.com/",0);

	switch(ret=dialog_end()){
		case 1:
			return ret;
			break;
	}
	return ret;
}

int menu(){

	char *dir;
	int mexit=0;
	static char *loadrom;
	int old_upscale = 0, new_upscale = 0;

	old_upscale = rc_getint("upscaler");
	gui_begin();
	while(!mexit){
#ifndef DINGOO_BUILD || CAANOO
		dialog_begin(rom.name,"OhBoy");
#endif
#ifdef DINGOO_NATIVE
		dialog_begin(rom.name,"Slide Power to open the menu");
#endif
#ifdef DINGOO_OPENDINGUX
#ifdef GCWZERO
		dialog_begin(rom.name,"Slide Power to open the menu");
#else
		dialog_begin(rom.name,"Press L+R to open the menu");
#endif /*GCWZERO*/
#endif /*DINGOO_OPENDINGUX*/
#ifdef CAANOO
		dialog_begin(rom.name,"Press Home to open the menu");
#endif
		dialog_text("Back to Game",NULL,FIELD_SELECTABLE);
		dialog_text("Load State",NULL,FIELD_SELECTABLE);
		dialog_text("Save State",NULL,FIELD_SELECTABLE);
		dialog_text("Reset Game",NULL,FIELD_SELECTABLE);
		dialog_text(NULL,NULL,0);
		dialog_text("Load ROM",NULL,FIELD_SELECTABLE);
		dialog_text("Options",NULL,FIELD_SELECTABLE);
		dialog_text("Controls",NULL,FIELD_SELECTABLE);
		dialog_text("About",NULL,FIELD_SELECTABLE);
		dialog_text("Quit","",FIELD_SELECTABLE);

		switch(dialog_end()){
			case 2:
				if(menu_state(0)) mexit=1;
				break;
			case 3:
				if(menu_state(1)) mexit=1;
				break;
			case 4:
				rc_command("reset");
				mexit=1;
				break;
			case 6:
#ifdef DINGOO_OPENDINGUX
				dir = menu_checkdir(rc_getstr("romdir"),getenv("HOME"));
#else
				dir = rc_getstr("romdir");
#endif /* DINGOO_OPENDINGUX */
				if(loadrom = menu_requestfile(NULL,"Select Rom",dir,"gb;gbc;zip;gz;gbz")) {
					loader_unload();
					ohb_loadrom(loadrom);
					mexit=1;
				}
				break;
			case 7:
				if(menu_options()) mexit=1;
				break;
			case 8:
				if(menu_controls()) mexit=1;
				break;
			case 9:
				if(menu_about()) mexit=0;
				break;
			case 10:
				exit(0);
				break;
			default:
				mexit=1;
				break;
		}
	}
	new_upscale = rc_getint("upscaler");
	if (old_upscale != new_upscale)
		scaler_init(new_upscale);
	gui_end();
	vid_fb.first_paint = 1;

	return 0;
}

/*#include VERSION*/

int launcher(){;

	char *rom = 0;
	char version_str[80];	
#ifdef DINGOO_OPENDINGUX
	char *dir = menu_checkdir(rc_getstr("romdir"),getenv("HOME"));
#else
	char *dir = rc_getstr("romdir");
#endif /* DINGOO_OPENDINGUX */
    
    snprintf(version_str, sizeof(version_str)-1, "gnuboy %s", VERSION);

	gui_begin();

launcher:
#ifndef DINGOO_BUILD || CAANOO
	dialog_begin("OHBOY",NULL);
#endif
#ifdef DINGOO_NATIVE
	dialog_begin("OHBOY","Slide Power to open the menu");
#endif
#ifdef DINGOO_OPENDINGUX
#ifdef GCWZERO
	dialog_begin("OHBOY","Slide Power to open the menu");
#else
	dialog_begin("OHBOY","Press L+R to open the menu");
#endif /*GCWZERO*/
#endif /*DINGOO_OPENDINGUX*/
#ifdef CAANOO
	dialog_begin("OHBOY","Press Home to open the menu");
#endif
	dialog_text("Load ROM",NULL,FIELD_SELECTABLE);
	dialog_text("Options",NULL,FIELD_SELECTABLE);
	dialog_text("Controls",NULL,FIELD_SELECTABLE);
	dialog_text("About",NULL,FIELD_SELECTABLE);
	dialog_text("Quit","",FIELD_SELECTABLE);

	switch(dialog_end()){
		case 1:
			rom = menu_requestfile(NULL,"Select Rom",dir,"gb;gbc;zip;gz;gbz");
			if(!rom) goto launcher;
			break;
		case 2:
			if(!menu_options()) goto launcher;
			break;
		case 3:
			if(!menu_controls()) goto launcher;
			break;
		case 4:
			if(!menu_about()) goto launcher;
			break;
		case 5:
			exit(0);
		default:
			goto launcher;
	}

	gui_end();
	vid_fb.first_paint = 1;

	return rom;
}


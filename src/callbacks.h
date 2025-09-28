#include <iup.h>

#ifndef CALLBACKS
#define CALLBACKS

#define CAT_LIMIT 50
#define URL_LIMIT 500

int opmlimport_cb(void);
int opmlexport_cb(void);
int exit_cb(void);
int addcat_cb(void);
int remocat_cb(void);
int addfeed_cb(void);
int remofeed_cb(void);
int feedselection_cb(Ihandle *h, int selected, int status);
int itemselection_cb(Ihandle *item, char* text, int pos, int state);
int rclick_cb(Ihandle *h, int id);
int themes_cb(void);
int timer_cb(void);
int switch_cb(void);
int copy_cb(void);
int open_cb(void);
int gfilter_cb(void);
int lfilter_cb(void);

#endif

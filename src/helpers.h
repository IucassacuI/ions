#ifndef HELPERS
#define HELPERS
#include "buffers.h"

void reftreedata(void);
char *readline(FILE *fp);
char *getcurrfeed(buffers *list);
void setmetadata(void);
char *color(char *feed, int rw_access, char *rgbcolor);
void setitem(int pos);
int  update_one(FILE *fp, char *url);
void updatefeed(void);
void updatefeeds(void);
void thread_update(void);
void showerror(int status, char *url);
FILE *librarian(void);

#endif

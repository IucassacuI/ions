#include <iup/iup.h>

#ifndef UI
#define UI

char    *readline(int fd);
void 	  drawmenu(void);
Ihandle *inittree(void);
Ihandle *inititembox(void);
Ihandle *initentrybox(void);
void 	  drawtree(void);
Ihandle *initfeedbox(void);
void 	  settheme(void);

#endif

#ifndef HELPERS
#define HELPERS

char *readline(int fd);
char *getcurrfeed(void);
void setmetadata(void);
char *color(char *feed, int rw_access, char *rgbcolor);
void setitem(int pos);
int  update_one(int fd, char *url);
void updatefeed(void);
void updatefeeds(void);
void thread_update(void);
void showerror(int status, char *url);
int librarian(void);

#endif

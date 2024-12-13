#include <cutils.h>
#include <iup/iup.h>
#include <iup/iup_config.h>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "callbacks.h"
#include "helpers.h"

int opmlimport_cb(void){
	IupMessage("ACTION", "a implementar import de OPML...");

	return IUP_DEFAULT;
}

int opmlexport_cb(void){
	IupMessage("ACTION", "a implementar export de OPML...");

	return IUP_DEFAULT;
}

int exit_cb(void){
	return IUP_CLOSE;
}

int addcat_cb(void){
	Ihandle *tree = IupGetHandle("tree");
	Ihandle *config = IupGetHandle("config");

	char cat[CAT_LIMIT] = "";
	const char *categories;

	int status = IupGetParam("Adicionar categoria", NULL, 0, "%s\n", &cat);
	if(status == 0 || str_equal(cat, "N/A"))
		return IUP_DEFAULT;

	IupSetAttribute(tree, "ADDBRANCH0", cat);

	categories = IupConfigGetVariableStr(config, "CAT", "LIST");

	char *new = str_format("%s,", cat);

	if(!categories){
		IupConfigSetVariableStr(config, "CAT", "LIST", new);
		return IUP_DEFAULT;
	}

	char *list = mem_alloc(strlen(categories)+CAT_LIMIT);
	mem_copy(list, categories);

	mem_copy(list, str_concat(list, new));
	IupConfigSetVariableStr(config, "CAT", "LIST", list);

	mem_freeall(false);
	return IUP_DEFAULT;
}

int remocat_cb(void){
	Ihandle *config = IupGetHandle("config");
	Ihandle *tree = IupGetHandle("tree");

	int selected = IupGetInt(tree, "VALUE");
	char *kindattr = str_format("KIND%d", selected);
	char *titleattr = str_format("TITLE%d", selected);

	char *kind = IupGetAttribute(tree, kindattr);
	char *title = IupGetAttribute(tree, titleattr);

	if(!str_equal(kind, "BRANCH") || selected == 0){
		IupMessageError(NULL, "Selecione uma categoria.");
		return IUP_DEFAULT;
	}

	int status = IupAlarm("Remover categoria", "Tem ceteza?", "Sim", "Não", NULL);
	if(status != 1)
		return IUP_DEFAULT;

	IupSetAttribute(tree, "DELNODE", "SELECTED");

	const char *categories = IupConfigGetVariableStr(config, "CAT", "LIST");

	char *copy = mem_alloc(strlen(categories)+1);
	mem_copy(copy, categories);

	char *new = mem_alloc(strlen(categories)+1);

	char *formatted = str_format("%s,", title);

	mem_copy(new, str_sub(copy, formatted, ""));

	IupConfigSetVariableStr(config, "CAT", "LIST", new);

	const char *feeds = IupConfigGetVariableStr(config, "CAT", title);
	int count = str_count(feeds, ",");
	char **list = str_split(feeds, ",");

	int fd = librarian();

	for(int i = 0; i < count; i++){
		char *command = str_format("REMOVE %s", list[i]);
		write(fd, command, strlen(command));

		char *status = readline(fd);

		if(str_include(status, "ERROR")){
			int err = atoi(str_split(status, " ")[1]);
			showerror(err, list[i]);
		}
	}

	IupConfigSetVariableStr(config, "CAT", title, "");

	close(fd);
	return IUP_DEFAULT;
}

int addfeed_cb(void){
	Ihandle *tree = IupGetHandle("tree");
	Ihandle *config = IupGetHandle("config");

	int selected = IupGetInt(tree, "VALUE");

	char *kindattr = str_format("KIND%d", selected);
	char *kind = IupGetAttribute(tree, kindattr);

	if(!str_equal(kind, "BRANCH") || selected == 0){
		IupMessageError(NULL, "Selecione uma categoria");
		return IUP_DEFAULT;
	}

	char url[URL_LIMIT] = "";

	Ihandle *clipboard = IupClipboard();
	char *text = IupGetAttribute(clipboard, "TEXT");

	if(text != NULL && (str_include(text, "https://") || str_include(text, "http://")))
		strncpy(url, text, URL_LIMIT);

	IupDestroy(clipboard);

	int status = IupGetParam("Adicionar feed", NULL, 0, "%s\n", &url);
	if(status == 0)
		return IUP_DEFAULT;

	int fd = librarian();

	status = update_one(fd, url);
	if(status != 0)
		return IUP_DEFAULT;

	close(fd);

	char *titleattr = str_format("TITLE%d", selected);

	char *category = IupGetAttribute(tree, titleattr);
	const char *list = IupConfigGetVariableStr(config, "CAT", category);

	char *item = str_format("%s,", url);

	if(!list){
		IupConfigSetVariableStr(config, "CAT", category, item);
	} else {
		char *copy = mem_alloc(strlen(list)+URL_LIMIT);
		mem_copy(copy, list);
		mem_copy(copy, str_concat(copy, item));

		IupConfigSetVariableStr(config, "CAT", category, copy);
	}

	char *command2 = str_format("METADATA %s", url);

	fd = librarian();
	write(fd, command2, strlen(command2));

	char *stat = readline(fd);
	if(str_include(stat, "ERROR")){
		int err = atoi(str_split(stat, " ")[1]);
		showerror(err, url);
		return IUP_DEFAULT;
	}

	char *leaf = str_format("ADDLEAF%d", selected);

	char *title = readline(fd);

	IupSetStrAttribute(tree, leaf, title);

	close(fd);
	return IUP_DEFAULT;
}

int remofeed_cb(void){
	Ihandle *tree = IupGetHandle("tree");
	Ihandle *config = IupGetHandle("config");

	int selected = IupGetInt(tree, "VALUE");

	char *kindattr = str_format("KIND%d", selected);
	char *kind = IupGetAttribute(tree, kindattr);

	if(str_equal(kind, "BRANCH")){
		IupMessageError(NULL, "Selecione um feed");
		return IUP_DEFAULT;
	}

	char *parentattr = str_format("PARENT%d", selected);
	int catid = IupGetInt(tree, parentattr);
	char *titleattr = str_format("TITLE%d", catid);

	char *category = IupGetAttribute(tree, titleattr);
	const char *feeds = IupConfigGetVariableStr(config, "CAT", category);
	char *currfeed = getcurrfeed();

	char *copy = mem_alloc(strlen(feeds)+1);
	mem_copy(copy, feeds);

	char *new = mem_alloc(strlen(feeds)+1);

	mem_copy(new, str_sub(copy, str_format("%s,", currfeed), ""));

	IupConfigSetVariableStr(config, "CAT", category, new);

	IupSetAttribute(tree, "DELNODE", "SELECTED");

	char *command = str_format("REMOVE %s", currfeed);

	int fd = librarian();
	write(fd, command, strlen(command));

	char *status = readline(fd);

	if(str_include(status, "ERROR")){
		int err = atoi(str_split(status, " ")[1]);
		showerror(err, currfeed);
	}

	close(fd);

	return IUP_DEFAULT;
}

int feedselection_cb(Ihandle *h, int selected, int status){
	Ihandle *itembox = IupGetHandle("itembox");
	Ihandle *list = IupGetHandle("list");
	Ihandle *tree = IupGetHandle("tree");

	IupSetAttribute(list, "1", NULL);

	char *kindattr = str_format("KIND%d", selected);
	char *kind = IupGetAttribute(tree, kindattr);

	if(str_equal(kind, "BRANCH") || status == 0)
		return IUP_DEFAULT;

	setmetadata();

	char *feed = getcurrfeed();

	color(feed, 1, IupGetGlobal("DLGFGCOLOR"));

	char *command = str_format("ITEMS %s", feed);

	int fd = librarian();
	write(fd, command, strlen(command));

	char *stat = readline(fd);

	if(str_include(stat, "ERROR")){
		int err = atoi(str_split(stat, " ")[1]);
		showerror(err, feed);
		return IUP_DEFAULT;
	}

	int counter = 0;
	while(1){
		char *item = readline(fd);
		if(strlen(item) == 0)
			break;

		counter++;
		IupSetStrAttribute(list, str_format("%d", counter), item);
		IupMap(list);
		IupRefresh(itembox);
	}

	close(fd);
	return IUP_DEFAULT;
}

int itemselection_cb(Ihandle *item, char* text, int pos, int state){
	if(state == 0)
		return IUP_DEFAULT;

	pos--;
	setitem(pos);

	return IUP_DEFAULT;
}

int rclick_cb(Ihandle *h, int id){
	Ihandle *tree = IupGetHandle("tree");
	IupSetInt(tree, "VALUE", id);

	char *kindattr = str_format("KIND%d", id);

	char *kind = IupGetAttribute(tree, kindattr);

	if(!str_equal(kind, "LEAF") && id != 0)
		return IUP_DEFAULT;

	Ihandle *upitem;

	if(id == 0){
	        upitem = IupItem("Atualizar todos", NULL);
		IupSetCallback(upitem, "ACTION", (Icallback) thread_update);
	} else {
		upitem = IupItem("Atualizar feed", NULL);
		IupSetCallback(upitem, "ACTION", (Icallback) updatefeed);
	}

	Ihandle *menu = IupMenu(upitem, NULL);

	IupPopup(menu, IUP_MOUSEPOS, IUP_MOUSEPOS);
	feedselection_cb(NULL, id, 1);

	return IUP_DEFAULT;
}

int themes_cb(void){
	Ihandle *config = IupGetHandle("config");

	enum {def, dos, hollywood, blue} selected = IupConfigGetVariableInt(config, "THEME", "CURRENT");
	int status = IupGetParam("Temas", NULL, 0, "%l|(Padrão)|DOS|Hollywood|Azulado|\n", &selected);

	if(status == 0)
		return IUP_DEFAULT;

	IupConfigSetVariableInt(config, "THEME", "CURRENT", selected);

	char *binname = IupGetGlobal("EXEFILENAME");
	IupExecute(binname, "");

	return IUP_CLOSE;
}

int timer_cb(void){
	Ihandle *config = IupGetHandle("config");

	int interval = IupConfigGetVariableInt(config, "TIMER", "INTERVAL");
	int unit = IupConfigGetVariableInt(config, "TIMER", "UNIT");

	int status = IupGetParam("Temporizador", NULL, 0, "Atualizar a cada: %i[1,]\n%l|Minutos|Horas|\n", &interval, &unit);

	if((interval <= 0) || (status != 1))
		return IUP_DEFAULT;

	IupConfigSetVariableInt(config, "TIMER", "INTERVAL", interval);
	IupConfigSetVariableInt(config, "TIMER", "UNIT", unit);

	int time = (interval*1000)*pow(60, unit+1);

	Ihandle *old = IupGetHandle("timer");
	IupDestroy(old);

	Ihandle *timer = IupTimer();
	IupSetCallback(timer, "ACTION_CB", (Icallback) thread_update);
	IupSetInt(timer, "TIME", time);
	IupSetAttribute(timer, "RUN", "YES");
	IupSetHandle("timer", timer);

	return IUP_DEFAULT;
}

int switch_cb(void){
	Ihandle *timer = IupGetHandle("timer");
	Ihandle *config = IupGetHandle("config");

	int time = IupGetInt(timer, "TIME");

	if(time == 0){
		IupMessage("Temporizador", "Ainda não definido");
		return IUP_DEFAULT;
	}

	int run = IupGetInt(timer, "RUN");

	run = !run;

	if(run)
		IupMessage("Temporizador", "Ativado");
	else
		IupMessage("Temporizador", "Desativado");

	IupSetInt(timer, "RUN", run);
	IupConfigSetVariableInt(config, "TIMER", "RUN", run);

	return IUP_DEFAULT;
}

int copy_cb(void){
	Ihandle *hyperlink = IupGetHandle("entryhyperlink");
	char *url = IupGetAttribute(hyperlink, "URL");

	Ihandle *clipboard = IupClipboard();
	IupSetStrAttribute(clipboard, "TEXT", url);

	IupDestroy(clipboard);

	return IUP_DEFAULT;
}

int open_cb(void){
	Ihandle *hyperlink = IupGetHandle("entryhyperlink");
	char *url = IupGetAttribute(hyperlink, "URL");

	IupExecute("firefox", url);

	return IUP_DEFAULT;
}

int gfilter_cb(void){
	Ihandle *config = IupGetHandle("config");

	char words[10000] = "";

	const char *w = IupConfigGetVariableStr(config, "FILTER", "GLOBAL");

	if(w != NULL){
		strncpy(words, w, 9999);
	}

	int status = IupGetParam("Filtro global", NULL, 0, "%s\n", words);
	if(status != 1){
		return IUP_DEFAULT;
	}

	IupConfigSetVariableStr(config, "FILTER", "GLOBAL", words);

	return IUP_DEFAULT;
}

int lfilter_cb(void){
	Ihandle *cfg = IupGetHandle("config");

	const char *cats = IupConfigGetVariableStr(cfg, "CAT", "LIST");
	int count = str_count(cats, ",");

	char **list = str_split(cats, ",");

	char *fmt = "Categoria: %l|";
	for(int i = 0; i < count; i++){
		fmt = str_concat(fmt, list[i]);
		fmt = str_concat(fmt, "|");
	}

	fmt = str_concat(fmt, "\n");

	int choice = 0;
	int status = IupGetParam("Filtro local", NULL, 0, fmt, &choice);
	if(status != 1){
		return IUP_DEFAULT;
	}

	char words[10000] = "";

	const char *w = IupConfigGetVariableStr(cfg, "FILTER", list[choice]);
	if(w != NULL){
		strncpy(words, w, 9999);
	}

	status = IupGetParam("Filtro local", NULL, 0, "%s\n", words);
	if(status != 1){
		return IUP_DEFAULT;
	}

	IupConfigSetVariableStr(cfg, "FILTER", list[choice], words);

	return IUP_DEFAULT;
}

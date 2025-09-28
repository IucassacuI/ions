#include <iup.h>
#include <iup_config.h>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "callbacks.h"
#include "helpers.h"
#include "buffers.h"
#include "strings.h"

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
	buffers list = {0};
	Ihandle *tree = IupGetHandle("tree");
	Ihandle *config = IupGetHandle("config");

	char cat[CAT_LIMIT] = "";
	const char *categories;

	int status = IupGetParam("Adicionar categoria", NULL, 0, "%s\n", &cat);
	if(status == 0 || str_equal(cat, "N/A"))
		return IUP_DEFAULT;

	IupSetAttribute(tree, "ADDBRANCH0", cat);

	categories = IupConfigGetVariableStr(config, "CAT", "LIST");

	char *new = str_format(&list, "%s,", cat);

	if(!categories){
		IupConfigSetVariableStr(config, "CAT", "LIST", new);
		return IUP_DEFAULT;
	}

	char *catlist = buf_alloc(&list, strlen(categories)+CAT_LIMIT);
	buf_strcopy(catlist, categories);

	buf_strcopy(catlist, str_concat(&list, catlist, new));
	IupConfigSetVariableStr(config, "CAT", "LIST", catlist);

	buf_free(&list);
	return IUP_DEFAULT;
}

int remocat_cb(void){
	buffers list = {0};
	Ihandle *config = IupGetHandle("config");
	Ihandle *tree = IupGetHandle("tree");

	int selected = IupGetInt(tree, "VALUE");
	char *kindattr = str_format(&list, "KIND%d", selected);
	char *titleattr = str_format(&list, "TITLE%d", selected);

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

	char *copy = buf_alloc(&list, strlen(categories)+1);
	buf_strcopy(copy, categories);

	char *new = buf_alloc(&list, strlen(categories)+1);

	char *formatted = str_format(&list, "%s,", title);

	buf_strcopy(new, str_sub(&list, copy, formatted, ""));

	IupConfigSetVariableStr(config, "CAT", "LIST", new);

	const char *feeds = IupConfigGetVariableStr(config, "CAT", title);
	if(feeds == NULL)
	  return IUP_DEFAULT;

	int count = str_count(feeds, ",");
	char **feedlist = str_split(&list, feeds, ",");

	FILE *fp = librarian();

	for(int i = 0; i < count; i++){
		char *command = str_format(&list, "REMOVE %s", feedlist[i]);
		fprintf(fp, command);

		char *status = readline(fp);

		if(str_include(status, "ERROR")){
			int err = atoi(str_split(&list, status, " ")[1]);
			showerror(err, feedlist[i]);
		}

		free(status);
	}

	IupConfigSetVariableStr(config, "CAT", title, "");

	reftreedata();
	fclose(fp);
	buf_free(&list);
	return IUP_DEFAULT;
}

int addfeed_cb(void){
	buffers list = {0};
	Ihandle *tree = IupGetHandle("tree");
	Ihandle *config = IupGetHandle("config");

	int selected = IupGetInt(tree, "VALUE");

	char *kindattr = str_format(&list, "KIND%d", selected);
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

	if(str_include(url, "youtube:"))
		strcpy(url, str_sub(&list, url, "youtube:", "https://youtube.com/feeds/videos.xml?channel_id="));

	FILE *fp = librarian();

	status = update_one(fp, url);
	if(status != 0)
		return IUP_DEFAULT;

	fclose(fp);

	char *titleattr = str_format(&list, "TITLE%d", selected);

	char *category = IupGetAttribute(tree, titleattr);
	const char *catlist = IupConfigGetVariableStr(config, "CAT", category);

	char *item = str_format(&list, "%s,", url);

	if(!catlist){
		IupConfigSetVariableStr(config, "CAT", category, item);
	} else {
		char *copy = buf_alloc(&list, strlen(catlist)+URL_LIMIT);
		buf_strcopy(copy, catlist);
		copy = str_concat(&list, copy, item);

		IupConfigSetVariableStr(config, "CAT", category, copy);
	}

	char *command2 = str_format(&list, "METADATA %s", url);

	fp = librarian();
	fprintf(fp, command2);

	char *stat = readline(fp);
	if(str_include(stat, "ERROR")){
		int err = atoi(str_split(&list, stat, " ")[1]);
		showerror(err, url);

		free(stat);
		buf_free(&list);
		return IUP_DEFAULT;
	}

	char *leaf = str_format(&list, "ADDLEAF%d", selected);

	char *title = readline(fp);

	IupSetStrAttribute(tree, leaf, title);
	reftreedata();
	free(title);

	fclose(fp);
	buf_free(&list);
	return IUP_DEFAULT;
}

int remofeed_cb(void){
	buffers list = {0};
	Ihandle *tree = IupGetHandle("tree");
	Ihandle *config = IupGetHandle("config");

	int selected = IupGetInt(tree, "VALUE");

	char *kindattr = str_format(&list, "KIND%d", selected);
	char *kind = IupGetAttribute(tree, kindattr);

	if(str_equal(kind, "BRANCH")){
		IupMessageError(NULL, "Selecione um feed");
		return IUP_DEFAULT;
	}

	char *parentattr = str_format(&list, "PARENT%d", selected);
	int catid = IupGetInt(tree, parentattr);
	char *titleattr = str_format(&list, "TITLE%d", catid);

	char *category = IupGetAttribute(tree, titleattr);
	const char *feeds = IupConfigGetVariableStr(config, "CAT", category);
	char *currfeed = getcurrfeed(&list);

	char *copy = buf_alloc(&list, strlen(feeds)+1);
	buf_strcopy(copy, feeds);

	char *new = buf_alloc(&list, strlen(feeds)+1);

	new = str_sub(&list, copy, str_format(&list, "%s,", currfeed), "");

	IupConfigSetVariableStr(config, "CAT", category, new);

	IupSetAttribute(tree, "DELNODE", "SELECTED");

	char *command = str_format(&list, "REMOVE %s", currfeed);

	FILE *fp = librarian();
	fprintf(fp, command);

	char *status = readline(fp);

	if(str_include(status, "ERROR")){
		int err = atoi(str_split(&list, status, " ")[1]);
		showerror(err, currfeed);
	}

	reftreedata();

	fclose(fp);
	free(status);
	buf_free(&list);

	return IUP_DEFAULT;
}

int feedselection_cb(Ihandle *h, int selected, int status){
	buffers list = {0};
	Ihandle *itembox = IupGetHandle("itembox");
	Ihandle *feedlist = IupGetHandle("list");
	Ihandle *tree = IupGetHandle("tree");

	IupSetAttribute(feedlist, "1", NULL);

	char *kindattr = str_format(&list, "KIND%d", selected);
	char *kind = IupGetAttribute(tree, kindattr);

	if(str_equal(kind, "BRANCH") || status == 0)
		return IUP_DEFAULT;

	setmetadata();

	char *feed = getcurrfeed(&list);

	color(feed, 1, IupGetGlobal("DLGFGCOLOR"));

	char *command = str_format(&list, "ITEMS %s", feed);

	FILE *fp = librarian();
	fprintf(fp, command);

	char *stat = readline(fp);

	if(str_include(stat, "ERROR")){
		int err = atoi(str_split(&list, stat, " ")[1]);
		showerror(err, feed);

		free(stat);
		buf_free(&list);
		return IUP_DEFAULT;
	}

	free(stat);

	int counter = 0;
	while(1){
		char *item = readline(fp);
		if(strlen(item) == 0)
			break;

		counter++;
		IupSetStrAttribute(feedlist, str_format(&list, "%d", counter), item);
		IupMap(feedlist);
		IupRefresh(itembox);

		free(item);
	}

	fclose(fp);
	buf_free(&list);
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
	buffers list = {0};
	Ihandle *tree = IupGetHandle("tree");
	IupSetInt(tree, "VALUE", id);

	char *kindattr = str_format(&list, "KIND%d", id);
	char *kind = IupGetAttribute(tree, kindattr);

	if(!str_equal(kind, "LEAF") && id != 0){
		buf_free(&list);
		return IUP_DEFAULT;
	}

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

	buf_free(&list);
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
	buffers list = {0};
	Ihandle *cfg = IupGetHandle("config");

	const char *cats = IupConfigGetVariableStr(cfg, "CAT", "LIST");
	int count = str_count(cats, ",");

	char **catlist = str_split(&list, cats, ",");

	char *fmt = "Categoria: %l|";
	for(int i = 0; i < count; i++){
		fmt = str_concat(&list, fmt, catlist[i]);
		fmt = str_concat(&list, fmt, "|");
	}

	fmt = str_concat(&list, fmt, "\n");

	int choice = 0;
	int status = IupGetParam("Filtro local", NULL, 0, fmt, &choice);
	if(status != 1){
		return IUP_DEFAULT;
	}

	char words[10000] = "";

	const char *w = IupConfigGetVariableStr(cfg, "FILTER", catlist[choice]);
	if(w != NULL){
		strncpy(words, w, 9999);
	}

	status = IupGetParam("Filtro local", NULL, 0, "%s\n", words);
	if(status != 1){
		return IUP_DEFAULT;
	}

	IupConfigSetVariableStr(cfg, "FILTER", catlist[choice], words);

	return IUP_DEFAULT;
}

#include <cutils.h>
#include <iup/iup.h>
#include <iup/iup_config.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include "helpers.h"

char *readline(int fd){
	char *buffer = mem_alloc(10);
	int size = 10;
	int pos = 0;

	while(1){
		if(pos == size){
			size += 10;
			buffer = mem_ralloc(buffer, size);
		}
	
		read(fd, buffer + pos, 1);

		if(buffer[pos] == '\n'){
			buffer[pos] = 0;
			break;
		}

		pos++;
	}

	return buffer;
}

char *getcurrfeed(void){
	Ihandle *tree = IupGetHandle("tree");
	Ihandle *config = IupGetHandle("config");

	int selected = IupGetInt(tree, "VALUE");

	char *parentattr = str_format("PARENT%d", selected);
	int parentid = IupGetInt(tree, parentattr);

	selected -= parentid;
	char *titleattr = str_format("TITLE%d", parentid);

	char *cat = IupGetAttribute(tree, titleattr);
	const char *feeds = IupConfigGetVariableStr(config, "CAT", cat);

	char *copy = mem_alloc(strlen(feeds)+1);
	mem_copy(copy, feeds);

	int commacount = str_count(copy, ",");

	char **feedlist = str_split(copy, ",");
	return mem_at(feedlist, sizeof(char *), commacount-selected);
}

void setmetadata(void){
	int fd = librarian();

	char *feed = getcurrfeed();
	char *command = str_format("METADATA %s", feed);
	write(fd, command, strlen(command));

	char *status = readline(fd);

	if(str_include(status, "ERROR")){
		int err = atoi(str_split(status, " ")[1]);
		showerror(err, feed);
		return;
	}

	char *title = readline(fd);

	Ihandle *feedtitle = IupGetHandle("feedtitle");
	IupSetStrAttribute(feedtitle, "TITLE", title);

	char *author = readline(fd);

	Ihandle *feedauthor = IupGetHandle("feedauthor");
	IupSetStrAttribute(feedauthor, "TITLE", author);

	char *hyperlink = readline(fd);

	Ihandle *feedhyperlink = IupGetHandle("feedhyperlink");
	IupSetStrAttribute(feedhyperlink, "TITLE", hyperlink);
	IupSetStrAttribute(feedhyperlink, "URL", hyperlink);

	char *published = readline(fd);

	Ihandle *feedpubdate = IupGetHandle("feedpubdate");
	IupSetStrAttribute(feedpubdate, "TITLE", published);

	char *updated = readline(fd);

	Ihandle *feedupdated = IupGetHandle("feedupdated");
	IupSetStrAttribute(feedupdated, "TITLE", updated);

	Ihandle *feedbox = IupGetHandle("feedbox");
	IupRefresh(feedbox);

	close(fd);
	mem_freeall(false);
}

char *color(char *url, int rw_access, char* rgbcolor){
	char *command = str_format("METADATA %s", url);
	int fd = librarian();
	write(fd, command, strlen(command));

	char *status = readline(fd);
	if(str_include(status, "ERROR")){
		int err = atoi(str_split(status, " ")[1]);
		showerror(err, url);
		return "";
	}

	char *name = readline(fd);

	close(fd);

	Ihandle *tree = IupGetHandle("tree");
	int nodes = IupGetInt(tree, "COUNT");

	char *title;
	char *colorattr;
	for(int node = 0; node < nodes; node++){
		title = IupGetAttribute(tree, str_format("TITLE%d", node));
		colorattr = str_format("COLOR%d", node);

		if(str_equal(title, name) && rw_access == 0){
			return IupGetAttribute(tree, colorattr);
		}

		if(str_equal(title, name) && rw_access == 1){
			IupSetAttribute(tree, colorattr, rgbcolor);
			return "";
		}
	}

	return "";
}

void setitem(int pos){
	char *feed = getcurrfeed();
	char *command = str_format("ITEM %s %d", feed, pos);

	int fd = librarian();
	write(fd, command, strlen(command));

	char *status = readline(fd);
	
	if(str_include(status, "ERROR")){
		int err = atoi(str_split(status, " ")[1]);
		showerror(err, feed);
		return;
	}

	char *title = readline(fd);
	char *pubdate = readline(fd);
	char *update = readline(fd);
	char *url = readline(fd);

	Ihandle *entrytitle = IupGetHandle("entrytitle");
	IupSetStrAttribute(entrytitle, "TITLE", title);

	Ihandle *hyperlink = IupGetHandle("entryhyperlink");
	IupSetStrAttribute(hyperlink, "TITLE", url);
	IupSetStrAttribute(hyperlink, "URL", url);

	Ihandle *entrypubdate = IupGetHandle("entrypubdate");
	IupSetStrAttribute(entrypubdate, "TITLE", pubdate);

	Ihandle *entryupdate = IupGetHandle("entryupdate");
	IupSetStrAttribute(entryupdate, "TITLE", update);

	Ihandle *entrybox = IupGetHandle("entrybox");
	IupRefresh(entrybox);

	close(fd);
	mem_freeall(false);
}

int update_one(int fd, char *feed){
	char *command = str_format("UPDATE %s", feed);
	char *filter_out = "";

	Ihandle *config = IupGetHandle("config");
	const char *filter_words = IupConfigGetVariableStr(config, "FILTER", "GLOBAL");

	if(filter_words != NULL){
		char *words_copy = mem_alloc(strlen(filter_words)+1);
		mem_copy(words_copy, filter_words);
		filter_out = words_copy;
	}

	const char *catlist = IupConfigGetVariableStr(config, "CAT", "LIST");

	char **cats = str_split(catlist, ",");
	int size = str_count(catlist, ",");

	char *category;
	for(int i = 0; i < size; i++){
		const char *catfeeds = IupConfigGetVariableStr(config, "CAT", cats[i]);
		if(catfeeds == NULL){ // categoria vazia, primeiro feed.
			category = NULL;
			break;
		}

		if(str_include(catfeeds, feed)){
			category = cats[i];
			break;
		}
	}

	const char *cat_filter;
	if(category == NULL)
		cat_filter = NULL;
	else
		cat_filter = IupConfigGetVariableStr(config, "FILTER", category);

	if(cat_filter != NULL){
		if(!str_equal(filter_out, ""))
			filter_out = str_concat(filter_out, ",");

		filter_out = str_concat(filter_out, str_format("%s", cat_filter));
	}

	if(!str_equal(filter_out, "")){
		command = str_concat(command, str_format(" %s", filter_out));
	}

	write(fd, command, strlen(command));

	char *status = readline(fd);
	
	if(str_include(status, "ERROR")){
		int err = atoi(str_split(status, " ")[1]);
		showerror(err, feed);
		return err;
	}

	return 0;
}

void updatefeed(void){
	char *feed = getcurrfeed();

	int fd = librarian();

	int err = update_one(fd, feed);
	if(err){
		showerror(err, feed);
		return;
	}

	char *status = readline(fd);

	close(fd);

	if(str_equal(status, "true"))
		IupMessage("Notificação", "Feed atualizado");
	else
		IupMessage("Notificação", "Nada de novo por aqui...");

	mem_freeall(false);
}

void updatefeeds(void){
	Ihandle *config = IupGetHandle("config");

	const char *cats = IupConfigGetVariableStr(config, "CAT", "LIST");
	if(cats == NULL)
		return;

	int commacount = str_count(cats, ",");
	char **catlist = str_split(cats, ",");

	for(int cat = 0; cat < commacount; cat++){
		const char *feedlist = IupConfigGetVariableStr(config, "CAT", mem_at(catlist, sizeof(char *), cat));

		int feedscount = str_count(feedlist, ",");
		char **feeds = str_split(feedlist, ",");

		char *url;
		for(int feed = 0; feed < feedscount; feed++){
			url = mem_at(feeds, sizeof(char *), feed);

			char *c = color(url, 0, NULL);
			if(str_equal(c, "") || str_equal(c, "0 0 255")){
				continue;
			}

			color(url, 1, "127 127 127");

			int fd = librarian();

			int err = update_one(fd, url);
			if(err != 0){
				color(url, 1, IupGetGlobal("DLGFGCOLOR"));
				continue;
			}

			char *result = readline(fd);

			close(fd);

			if(str_equal(result, "true"))
				color(url, 1, "0 0 255");
			else
				color(url, 1, IupGetGlobal("DLGFGCOLOR"));
		}

	}

	mem_freeall(false);
}

void thread_update(void){
	Ihandle *thread = IupThread();
	IupSetCallback(thread, "THREAD_CB", (Icallback) updatefeeds);
	IupSetAttribute(thread, "START", "YES");
}

void showerror(int status, char *url){
	char *msgs[] = {
		"dummy",
		"Feed inválido.",
		"Falha na conexão.",
		"Falha ao ler documento XML.",
		"Diretório inacesível.",
		"Falha de marshaling/unmarshaling.",
		"Falha ao remover feed do cache."
	};

	char *msg = str_format("%s:\n", url);

	if(status <= 6){
		msg = str_concat(msg, msgs[status]);
	} else {
		msg = str_concat(msg, str_format("Recebido HTTP %d. Esperava HTTP 200 (OK).", status));
	}

	IupMessageError(NULL, msg);
	return;
}

int librarian(void){
	int fd;
	struct sockaddr_un server = {0};

	fd = socket(AF_UNIX, SOCK_STREAM, 0);
	server.sun_family = AF_UNIX;
	strcpy(server.sun_path, "./sock");

	if(connect(fd, (struct sockaddr *) &server, sizeof(server)) == -1){
		IupMessageError(NULL, "Fatal: conexão com o servidor local não pôde ser estabelecida.");
		exit(1);
	}

	return fd;
}

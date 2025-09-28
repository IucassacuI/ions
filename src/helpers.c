#include <iup.h>
#include <iup_config.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include "helpers.h"
#include "buffers.h"
#include "strings.h"

void reftreedata(void){
	buffers list = {0};
	Ihandle *cfg = IupGetHandle("config");
	Ihandle *tree = IupGetHandle("tree");

	const char *catlist = IupConfigGetVariableStr(cfg, "CAT", "LIST");

	int count = str_count(catlist, ",");
	char **cats = str_split(&list, catlist, ",");

	int total_nodes = 1;

	for(int i = 0; i < count; i++){
		const char *feedlist = IupConfigGetVariableStr(cfg, "CAT", cats[count - 1 - i]);
		if(feedlist == NULL)
			continue;

		char **feeds = str_split(&list, feedlist, ",");
		int feedcount = str_count(feedlist, ",");

		for(int j = 0; j < feedcount; j++){
			FILE *fp = librarian();
			fprintf(fp, "ITEMS %s", feeds[feedcount - 1 - j]);

			char *status = readline(fp);
			if(!str_equal(status, "OK")){
				free(status);
				fclose(fp);
				continue;
			}

			free(status);
			fclose(fp);

			int pos = total_nodes + j;

			if(str_equal("BRANCH", IupGetAttribute(tree, str_format(&list, "KIND%d", pos)))){
				total_nodes++;
				pos++;
			}

			IupSetStrAttribute(tree, str_format(&list, "FEED%d", pos), feeds[feedcount - 1 - j]);
		}
		total_nodes += feedcount;

	}

	buf_free(&list);
}

char *readline(FILE *fp){
	int size = 0;
	char *buffer = NULL;
	int chars = getline(&buffer, &size, fp);

	if(chars == -1)
		buffer[0] = 0;
	else
		buffer[strlen(buffer) - 1] = 0;

	return buffer;
}

char *getcurrfeed(buffers *list){
	Ihandle *tree = IupGetHandle("tree");
	int selected = IupGetInt(tree, "VALUE");

	return IupGetAttribute(tree, str_format(list, "FEED%d", selected));
}

void setmetadata(void){
	buffers list = {0};
	FILE *fp = librarian();

	char *feed = getcurrfeed(&list);
	char *command = str_format(&list, "METADATA %s", feed);
	fprintf(fp, command);

	char *status = readline(fp);

	if(str_include(status, "ERROR")){
		char **split = str_split(&list, status, " ");
		int err = atoi(split[1]);
		showerror(err, feed);

		buf_free(&list);
		free(status);
		return;
	}

	free(status);

	char *title = readline(fp);

	Ihandle *feedtitle = IupGetHandle("feedtitle");
	IupSetStrAttribute(feedtitle, "TITLE", title);
	free(title);

	char *author = readline(fp);

	Ihandle *feedauthor = IupGetHandle("feedauthor");
	IupSetStrAttribute(feedauthor, "TITLE", author);
	free(author);

	char *hyperlink = readline(fp);

	Ihandle *feedhyperlink = IupGetHandle("feedhyperlink");
	IupSetStrAttribute(feedhyperlink, "TITLE", hyperlink);
	IupSetStrAttribute(feedhyperlink, "URL", hyperlink);
	free(hyperlink);

	char *published = readline(fp);

	Ihandle *feedpubdate = IupGetHandle("feedpubdate");
	IupSetStrAttribute(feedpubdate, "TITLE", published);
	free(published);

	char *updated = readline(fp);

	Ihandle *feedupdated = IupGetHandle("feedupdated");
	IupSetStrAttribute(feedupdated, "TITLE", updated);
	free(updated);

	Ihandle *feedbox = IupGetHandle("feedbox");
	IupRefresh(feedbox);

	buf_free(&list);
	fclose(fp);
}

char *color(char *url, int rw_access, char* rgbcolor){
	buffers list = {0};
	char *command = str_format(&list, "METADATA %s", url);
	FILE *fp = librarian();
	fprintf(fp, command);

	char *status = readline(fp);
	if(str_include(status, "ERROR")){
		char **split = str_split(&list, status, " ");
		int err = atoi(split[1]);

		buf_free(&list);
		free(status);
		showerror(err, url);
		return "";
	}

	free(status);

	char *name = readline(fp);

	fclose(fp);

	Ihandle *tree = IupGetHandle("tree");
	int nodes = IupGetInt(tree, "COUNT");

	char *title;
	char *colorattr;
	for(int node = 0; node < nodes; node++){
		title = IupGetAttribute(tree, str_format(&list, "TITLE%d", node));
		colorattr = str_format(&list, "COLOR%d", node);

		if(str_equal(title, name) && rw_access == 0){
			free(name);
			buf_free(&list);
			return IupGetAttribute(tree, colorattr);
		}

		if(str_equal(title, name) && rw_access == 1){
			free(name);
			buf_free(&list);
			IupSetAttribute(tree, colorattr, rgbcolor);
			return "";
		}
	}

	buf_free(&list);
	return "";
}

void setitem(int pos){
	buffers list = {0};
	char *feed = getcurrfeed(&list);
	char *command = str_format(&list, "ITEM %s %d", feed, pos);

	FILE *fp = librarian();
	fprintf(fp, command);

	char *status = readline(fp);

	if(str_include(status, "ERROR")){
		int err = atoi(str_split(&list, status, " ")[1]);
		showerror(err, feed);

		free(status);
		buf_free(&list);
		return;
	}

	free(status);

	char *title = readline(fp);
	char *pubdate = readline(fp);
	char *update = readline(fp);
	char *url = readline(fp);

	Ihandle *entrytitle = IupGetHandle("entrytitle");
	IupSetStrAttribute(entrytitle, "TITLE", title);
	free(title);

	Ihandle *hyperlink = IupGetHandle("entryhyperlink");
	IupSetStrAttribute(hyperlink, "TITLE", url);
	IupSetStrAttribute(hyperlink, "URL", url);
	free(url);

	Ihandle *entrypubdate = IupGetHandle("entrypubdate");
	IupSetStrAttribute(entrypubdate, "TITLE", pubdate);
	free(pubdate);

	Ihandle *entryupdate = IupGetHandle("entryupdate");
	IupSetStrAttribute(entryupdate, "TITLE", update);
	free(update);

	Ihandle *entrybox = IupGetHandle("entrybox");
	IupRefresh(entrybox);

	buf_free(&list);
	fclose(fp);
}

int update_one(FILE *fp, char *feed){
	buffers list = {0};
	char *command = str_format(&list, "UPDATE %s", feed);
	char *filter_out = "";

	Ihandle *config = IupGetHandle("config");
	const char *filter_words = IupConfigGetVariableStr(config, "FILTER", "GLOBAL");

	if(filter_words != NULL){
		char *words_copy = buf_alloc(&list, strlen(filter_words)+1);
		buf_strcopy(words_copy, filter_words);

		filter_out = words_copy;
	}

	const char *catlist = IupConfigGetVariableStr(config, "CAT", "LIST");

	char **cats = str_split(&list, catlist, ",");
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
			filter_out = str_concat(&list, filter_out, ",");

		filter_out = str_concat(&list, filter_out, str_format(&list, "%s", cat_filter));
	}

	if(!str_equal(filter_out, "")){
		command = str_concat(&list, command, str_format(&list, " %s", filter_out));
	}

	fprintf(fp, command);

	char *status = readline(fp);

	if(str_include(status, "ERROR")){
		char **split = str_split(&list, status, " ");
		int err = atoi(split[1]);
		showerror(err, feed);

		free(status);
		buf_free(&list);
		return err;
	}

	free(status);
	buf_free(&list);
	return 0;
}

void updatefeed(void){
	buffers list = {0};
	char *feed = getcurrfeed(&list);

	FILE *fp = librarian();

	int err = update_one(fp, feed);
	if(err){
		showerror(err, feed);
		buf_free(&list);
		return;
	}

	char *status = readline(fp);

	fclose(fp);

	if(str_equal(status, "true"))
		IupMessage("Notificação", "Feed atualizado");
	else
		IupMessage("Notificação", "Nada de novo por aqui...");

	buf_free(&list);
	free(status);
}

void updatefeeds(void){
	buffers list = {0};
	Ihandle *config = IupGetHandle("config");

	const char *cats = IupConfigGetVariableStr(config, "CAT", "LIST");
	if(cats == NULL)
		return;

	int commacount = str_count(cats, ",");
	char **catlist = str_split(&list, cats, ",");

	for(int cat = 0; cat < commacount; cat++){
		const char *feedlist = IupConfigGetVariableStr(config, "CAT", buf_at(catlist, cat));

		int feedscount = str_count(feedlist, ",");
		char **feeds = str_split(&list, feedlist, ",");

		char *url;
		for(int feed = 0; feed < feedscount; feed++){
			url = buf_at(feeds, feed);

			char *c = color(url, 0, NULL);
			if(str_equal(c, "") || str_equal(c, "0 0 255")){
				continue;
			}

			color(url, 1, "127 127 127");

			FILE *fp = librarian();

			int err = update_one(fp, url);
			if(err != 0){
				color(url, 1, IupGetGlobal("DLGFGCOLOR"));
				continue;
			}

			char *result = readline(fp);

			fclose(fp);

			if(str_equal(result, "true"))
				color(url, 1, "0 0 255");
			else
				color(url, 1, IupGetGlobal("DLGFGCOLOR"));

			free(result);
		}
	}

	buf_free(&list);
}

void thread_update(void){
	Ihandle *thread = IupThread();
	IupSetCallback(thread, "THREAD_CB", (Icallback) updatefeeds);
	IupSetAttribute(thread, "START", "YES");
}

void showerror(int status, char *url){
	buffers list = {0};
	char *msgs[] = {
		"dummy",
		"Feed inválido.",
		"Falha na conexão.",
		"Falha ao ler documento XML.",
		"Diretório inacesível.",
		"Falha de marshaling/unmarshaling.",
		"Falha ao remover feed do cache."
	};

	char *msg = str_format(&list, "%s:\n", url);

	if(status <= 6){
		msg = str_concat(&list, msg, msgs[status]);
	} else {
		msg = str_concat(&list, msg, str_format(&list, "Recebido HTTP %d. Esperava HTTP 200 (OK).", status));
	}

	IupMessageError(NULL, msg);
	buf_free(&list);
	return;
}

FILE *librarian(void){
	int fd;
	struct sockaddr_un server = {0};

	fd = socket(AF_UNIX, SOCK_STREAM, 0);
	server.sun_family = AF_UNIX;
	strcpy(server.sun_path, "./sock");

	if(connect(fd, (struct sockaddr *) &server, sizeof(server)) == -1){
		IupMessageError(NULL, "Fatal: conexão com o servidor local não pôde ser estabelecida.");
		exit(1);
	}

	return fdopen(fd, "w+");
}

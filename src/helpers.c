#include <cutils.h>
#include <iup.h>
#include <iup_config.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <windows.h>
#include "helpers.h"

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
	char *feed = getcurrfeed();

	char *command = str_format("librarian.exe --feed \"%s\" --metadata", feed);
	int err = librarian(command);
	if(err){
		showerror(err, feed);
		return;
	}

	FILE *out = fopen("out", "r");

	char *title = mem_alloc(1000);
	mem_read(title, out);

	Ihandle *feedtitle = IupGetHandle("feedtitle");
	IupSetStrAttribute(feedtitle, "TITLE", title);

	char *author = mem_alloc(100);
	mem_read(author, out);

	Ihandle *feedauthor = IupGetHandle("feedauthor");
	IupSetStrAttribute(feedauthor, "TITLE", author);

	char *hyperlink = mem_alloc(2000);
	mem_read(hyperlink, out);

	Ihandle *feedhyperlink = IupGetHandle("feedhyperlink");
	IupSetStrAttribute(feedhyperlink, "TITLE", hyperlink);
	IupSetStrAttribute(feedhyperlink, "URL", hyperlink);

	char *published = mem_alloc(100);
	mem_read(published, out);

	Ihandle *feedpubdate = IupGetHandle("feedpubdate");
	IupSetStrAttribute(feedpubdate, "TITLE", published);

	char *updated = mem_alloc(100);
	mem_read(updated, out);

	fclose(out);

	Ihandle *feedupdated = IupGetHandle("feedupdated");
	IupSetStrAttribute(feedupdated, "TITLE", updated);

	Ihandle *feedbox = IupGetHandle("feedbox");
	IupRefresh(feedbox);
}

char *color(char *url, int rw_access, char* rgbcolor){
	char *command = str_format("librarian.exe --feed \"%s\" --metadata", url);
	int err = librarian(command);

	if(err){
		showerror(err, url);
		return "";
	}

	FILE *out = fopen("out", "r");

	char *name = mem_alloc(100);
	mem_read(name, out);

	fclose(out);

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

	char *command = str_format("librarian.exe --feed \"%s\" --item %d", feed, pos);

	int err = librarian(command);
	if(err != 0){
		showerror(err, feed);
		return;
	}

	FILE *out = fopen("out", "r");

	char *title = mem_alloc(1000);
	mem_read(title, out);

	char *url = mem_alloc(2000);
	mem_read(url, out);

	char *pubdate = mem_alloc(100);
	mem_read(pubdate, out);

	char *update = mem_alloc(100);
	mem_read(update, out);

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

	fclose(out);
}

int update_one(char *feed){
	char *command = str_format("librarian.exe --feed \"%s\" --update", feed);
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
		command = str_concat(command, str_format(" --filter \"%s\"", filter_out));
	}

	int exitcode = librarian(command);
	if(exitcode != 0)
		showerror(exitcode, feed);

	return exitcode;
}

void updatefeed(void){
	char *feed = getcurrfeed();

	int err = update_one(feed);
	if(err)
		return;

	FILE *out = fopen("out", "r");

	char *status = mem_alloc(10);
	mem_read(status, out);

	fclose(out);

	if(str_equal(status, "true"))
		IupMessage("Notificação", "Feed atualizado");
	else
		IupMessage("Notificação", "Nada de novo por aqui...");
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

			int exitcode = update_one(url);
			if(exitcode != 0){
				color(url, 1, IupGetGlobal("DLGFGCOLOR"));
				continue;
			}

			FILE *out = fopen("out", "r");

			char *result = mem_alloc(10);
			mem_read(result, out);

			fclose(out);

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

int librarian(char *command){
	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));

	CreateProcess(NULL, command, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi);
	WaitForSingleObject(pi.hProcess, INFINITE);

	DWORD exitcode = 0;
	GetExitCodeProcess(pi.hProcess, &exitcode);

	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);

	return (int) exitcode;
}
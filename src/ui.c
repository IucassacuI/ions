#include <cutils.h>
#include <iup/iup.h>
#include <iup/iup_config.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/unistd.h>
#include "helpers.h"
#include "callbacks.h"
#include "ui.h"

Ihandle *menu;
Ihandle *file_submenu, *fitems, *opmlimport_item, *opmlexport_item, *exit_item;
Ihandle *edit_submenu, *eitems, *addcat_item, *remocat_item, *addfeed_item, *remofeed_item;
Ihandle *options_submenu, *oitems, *themes_item, *timer_item, *switch_item;
Ihandle *filter_submenu, *filitems, *gfilter_item, *lfilter_item;

void drawmenu(void){
	Ihandle *dialog = IupGetHandle("dialog");

	opmlimport_item = IupItem("Importar OPML...", NULL);
	opmlexport_item = IupItem("Exportar OPML...", NULL);
	exit_item = IupItem("Sair", NULL);

	IupSetCallback(opmlimport_item, "ACTION", (Icallback) opmlimport_cb);
	IupSetCallback(opmlexport_item, "ACTION", (Icallback) opmlexport_cb);
	IupSetCallback(exit_item, "ACTION", (Icallback) exit_cb);

	fitems = IupMenu(opmlimport_item, opmlexport_item, exit_item, NULL);
	file_submenu = IupSubmenu("&Arquivo", fitems);

	addfeed_item = IupItem("Adicionar feed...", NULL);
	remofeed_item = IupItem("Remover feed", NULL);
	addcat_item = IupItem("Adicionar categoria...", NULL);
	remocat_item = IupItem("Remover categoria", NULL);

	IupSetCallback(addfeed_item, "ACTION", (Icallback) addfeed_cb);
	IupSetCallback(remofeed_item, "ACTION", (Icallback) remofeed_cb);
	IupSetCallback(addcat_item, "ACTION", (Icallback) addcat_cb);
	IupSetCallback(remocat_item, "ACTION", (Icallback) remocat_cb);

	eitems = IupMenu(addcat_item, remocat_item, addfeed_item, remofeed_item, NULL);
	edit_submenu = IupSubmenu("&Editar", eitems);

	themes_item = IupItem("Tema...", NULL);
	timer_item = IupItem("Temporizador", NULL);
	switch_item = IupItem("Ativar/desativar temporizador", NULL);

	IupSetCallback(themes_item, "ACTION", (Icallback) themes_cb);
	IupSetCallback(timer_item, "ACTION", (Icallback) timer_cb);
	IupSetCallback(switch_item, "ACTION", (Icallback) switch_cb);

	oitems = IupMenu(themes_item, timer_item, switch_item, NULL);
	options_submenu = IupSubmenu("&Opções", oitems);

	gfilter_item = IupItem("Global...", NULL);
	IupSetCallback(gfilter_item, "ACTION", (Icallback) gfilter_cb);

	lfilter_item = IupItem("Local...", NULL);
	IupSetCallback(lfilter_item, "ACTION", (Icallback) lfilter_cb);

	filitems = IupMenu(gfilter_item, lfilter_item, NULL);
	filter_submenu = IupSubmenu("&Filtros", filitems);

	menu = IupMenu(file_submenu, edit_submenu, options_submenu, filter_submenu, NULL);

	IupSetAttributeHandle(dialog, "MENU", menu);
}

Ihandle *inittree(void){
	Ihandle *tree = IupTree();
	IupSetAttribute(tree, "MAXSIZE", "150x");
	IupSetAttribute(tree, "EXPAND", "YES");
	IupSetCallback(tree, "RIGHTCLICK_CB",(Icallback) rclick_cb);
	IupSetCallback(tree, "SELECTION_CB", (Icallback) feedselection_cb);

	IupSetHandle("tree", tree);
	return tree;
}

Ihandle *inititembox(void){
	Ihandle *label = IupLabel("Item: ");

	Ihandle *list = IupList(NULL);
	IupSetAttributes(list, "DROPDOWN=YES, SIZE=150, VISIBLEITEMS=10, DROPEXPAND=NO");
	IupSetCallback(list, "ACTION", (Icallback) itemselection_cb);
	IupSetHandle("list", list);

	Ihandle *itembox = IupHbox(label, list, NULL);
	IupSetHandle("itembox", itembox);
	return itembox;
}

Ihandle *initentrybox(void){
	Ihandle *title = IupLabel("N/A");
	Ihandle *tlabel = IupLabel("Título: ");

	Ihandle *pubdate = IupLabel("N/A");
	Ihandle *plabel = IupLabel("Publicado: ");

	Ihandle *update = IupLabel("N/A");
	Ihandle *ulabel = IupLabel("Atualizado: ");

	Ihandle *hyperlink = IupLink("example.com", "N/A");
	Ihandle *hlabel = IupLabel("Hyperlink: ");

	Ihandle *titlebox = IupHbox(tlabel, title, NULL);
	Ihandle *pubbox = IupHbox(plabel, pubdate, NULL);
	Ihandle *upbox = IupHbox(ulabel, update, NULL);
	Ihandle *hyperbox = IupHbox(hlabel, hyperlink, NULL);

	Ihandle *fill = IupFill();
	IupSetAttribute(fill, "SIZE", "5");

	Ihandle *box = IupVbox(fill, titlebox, pubbox, upbox, hyperbox, NULL);

	IupSetHandle("entrytitle", title);
	IupSetHandle("entrypubdate", pubdate);
	IupSetHandle("entryupdate", update);
	IupSetHandle("entryhyperlink", hyperlink);
	IupSetHandle("entrybox", box);
	return box;
}

void drawtree(void){
	Ihandle *config = IupGetHandle("config");
	Ihandle *tree = IupGetHandle("tree");

	IupSetAttribute(tree, "TITLE", "Feeds");

	const char *list = IupConfigGetVariableStr(config, "CAT", "LIST");
	if(!list)
		return;

	char *catcopy = mem_alloc(strlen(list)+1);
	mem_copy(catcopy, list);

	char **catlist = str_split(catcopy, ",");
	int counter = str_count(catcopy, ",");

	for(int i = 0; i < counter; i++){
		IupSetAttribute(tree, "ADDBRANCH0", mem_at(catlist, sizeof(char *), i));

		const char *feedlist = IupConfigGetVariableStr(config, "CAT", catlist[i]);
		if(feedlist == NULL)
			return;

		char *feedcopy = mem_alloc(strlen(feedlist)+1);
		mem_copy(feedcopy, feedlist);

		char **feeds = str_split(feedcopy, ",");
		int feedcount = str_count(feedcopy, ",");

		for(int j = 0; j < feedcount; j++){
			int fd = librarian();

			char *command = str_format("METADATA %s", mem_at(feeds, sizeof(char *), j));
			write(fd, command, strlen(command));

			char *status = readline(fd);

			if(str_include(status, "ERROR")){
				int err = atoi(str_split(status, " ")[1]);
				showerror(err, mem_at(feeds, sizeof(char *), j));
				continue;
			}

			char *title = readline(fd);
			IupSetStrAttribute(tree, "ADDLEAF1", title);

			close(fd);
		}
	}

	IupSetAttribute(tree, "EXPANDALL", "NO");

	mem_freeall(false);
}

Ihandle *initfeedbox(void){
	Ihandle *tlabel = IupLabel("Feed: ");
	Ihandle *title = IupLabel("N/A");

	Ihandle *alabel = IupLabel("De: ");
	Ihandle *author = IupLabel("N/A");

	Ihandle *plabel = IupLabel("Publicado: ");
	Ihandle *published = IupLabel("N/A");

	Ihandle *ulabel = IupLabel("Atualizado: ");
	Ihandle *updated = IupLabel("N/A");

	Ihandle *hlabel = IupLabel("Hyperlink: ");
	Ihandle *hyperlink = IupLink("example.com" ,"N/A");

	Ihandle *thbox = IupHbox(tlabel, title, NULL);
	Ihandle *chbox = IupHbox(alabel, author, NULL);
	Ihandle *phbox = IupHbox(plabel, published, NULL);
	Ihandle *uhbox = IupHbox(ulabel, updated, NULL);
	Ihandle *hybox = IupHbox(hlabel, hyperlink, NULL);

	Ihandle *box = IupVbox(thbox, chbox, phbox, uhbox, hybox, NULL);

	IupSetHandle("feedtitle", title);
	IupSetHandle("feedauthor", author);
	IupSetHandle("feedpubdate", published);
	IupSetHandle("feedupdated", updated);
	IupSetHandle("feedhyperlink", hyperlink);
	IupSetHandle("feedbox", box);

	return box;
}

void settheme(void){
	Ihandle *config = IupGetHandle("config");

	int theme = IupConfigGetVariableInt(config, "THEME", "CURRENT");

	char *dbgcolor[] = {"255 255 255", "0 0 0", "0 0 0", "#1D2B41"};
	char *tbgcolor[] = {"255 255 255", "0 0 0", "0 0 0", "#1D2B41"};
	char *tfgcolor[] = {"0 0 0", "255 255 255", "0 255 0", "#6894AE"};
	char *font[] = {"Arial", "Consolas", "Courier", "Calibri"};

	IupSetGlobal("DLGFGCOLOR", tfgcolor[theme]);
	IupSetGlobal("DLGBGCOLOR", dbgcolor[theme]);
	IupSetGlobal("TXTBGCOLOR", tbgcolor[theme]);
	IupSetGlobal("TXTFGCOLOR", tfgcolor[theme]);
	IupSetGlobal("DEFAULTFONTFACE", font[theme]);
}

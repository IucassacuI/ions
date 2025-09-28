#include <iup.h>
#include <iup_config.h>
#include <math.h>
#include <stdio.h>
#include <unistd.h>
#include "callbacks.h"
#include "helpers.h"
#include "ui.h"

Ihandle *hbox, *dialog;
Ihandle *config;

int main(int argc, char **argv){
	IupOpen(&argc, &argv);
	IupSetGlobal("UTF8MODE", "YES");
	IupSetLanguage("PORTUGUESE");

	if(IupExecute("./librarian", "") != 1){
		IupMessageError(NULL, "Fatal: o servidor local não pôde ser inicializado.");
		return 1;
	}

	config = IupConfig();
	IupSetAttribute(config, "APP_NAME", "Ions");
	IupConfigLoad(config);
	IupSetHandle("config", config);

	int interval = IupConfigGetVariableInt(config, "TIMER", "INTERVAL");
	int unit = IupConfigGetVariableInt(config, "TIMER", "UNIT");
	int run = IupConfigGetVariableInt(config, "TIMER", "RUN");
	int time = (interval*1000)*pow(60, unit+1);

	Ihandle *timer = IupTimer();
	IupSetInt(timer, "TIME", time);
	IupSetCallback(timer, "ACTION_CB", (Icallback) thread_update);

	if(run)
		IupSetAttribute(timer, "RUN", "YES");

	IupSetHandle("timer", timer);

	settheme();

	Ihandle *tree = inittree();

	Ihandle *feedbox = initfeedbox();

	Ihandle *itembox = inititembox();

	Ihandle *entrybox = initentrybox();

	Ihandle *filler1 = IupFill();
	IupSetAttribute(filler1, "SIZE", "5");

	Ihandle *filler2 = IupFill();
	IupSetAttribute(filler2, "SIZE", "5");

	Ihandle *filler3 = IupFill();
	IupSetAttribute(filler3, "SIZE", "5");

	Ihandle *sep = IupFlatSeparator();
	IupSetAttribute(sep, "ORIENTATION", "HORIZONTAL");
	IupSetAttribute(sep, "EXPAND", "HORIZONTAL");

	Ihandle *vbox = IupVbox(filler1, feedbox, filler2, sep, filler3, itembox, entrybox, NULL);

	Ihandle *filler4 = IupFill();
	IupSetAttribute(filler4, "SIZE", "10");

	Ihandle *container = IupHbox(filler4, vbox, NULL);
	hbox = IupHbox(tree, container, NULL);
	IupSetHandle("hbox", hbox);

	dialog = IupDialog(hbox);
	IupSetAttributes(dialog, "SIZE=HALFxHALF, TITLE=\"Receptor de Íons\"");

	IupSetCallback(dialog, "K_cC", (Icallback) copy_cb);
	IupSetCallback(dialog, "K_cA", (Icallback) open_cb);

	IupSetHandle("dialog", dialog);

	drawmenu();

	IupShowXY(dialog, IUP_CENTER, IUP_CENTER);

	drawtree();

	if(run)
		thread_update();

	IupMainLoop();

	IupConfigSave(config);
	IupDestroy(config);
	IupDestroy(timer);

	IupClose();

	FILE *fp = librarian();
	fprintf(fp, "QUIT");

	fclose(fp);
	return 0;
}

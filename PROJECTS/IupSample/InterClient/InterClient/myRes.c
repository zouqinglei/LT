#include "stdio.h"
#include "stdlib.h"
#include "myRes.h"



static int exit_cb(Ihandle *ih)
{
    int ret = IupAlarm("Warning", "Are you sure exit?", "YES", "NO",NULL);
    if(ret == 1)
    {
        return IUP_CLOSE;
    }

    return IUP_DEFAULT;
}

static int about_cb(Ihandle *ih)
{
    IupMessage("About", "   InterClient v1.0\n\nAutors:\n   zouqinglei\n   zouqinglei@163.com");
    return IUP_DEFAULT;
}


void createMenu()
{
    Ihandle * dlg;
    Ihandle * menu, * file_menu, * help_menu;
    Ihandle * exit_item;
    Ihandle * about_item;

    exit_item = IupItem("Exit",NULL);
    IupSetAttribute(exit_item, "IMAGE", "IUP_MessageError");

    about_item = IupItem("About...",NULL);
    IupSetAttribute(about_item, "IMAGE", "IUP_MessageInfo");

    IupSetCallback(exit_item, "ACTION", (Icallback)exit_cb);
    IupSetCallback(about_item, "ACTION", (Icallback)about_cb);




    file_menu = IupMenu(IupSeparator(),exit_item, NULL);

    help_menu = IupMenu(about_item, NULL);

    menu = IupMenu(IupSubmenu("File",file_menu),
                   IupSubmenu("Help",help_menu),
                   NULL);

    dlg = IupGetHandle("_DIALOG");

    IupSetAttributeHandle(dlg, "MENU", menu);
   

}


Ihandle * createStatusbar()
{
    Ihandle * statusbar;

    statusbar = IupLabel("ready");
    IupSetAttribute(statusbar, "NAME", "STATUSBAR");
    IupSetAttribute(statusbar, "EXPAND", "HORIZONTAL");
    IupSetAttribute(statusbar, "PADDING", "10x5");

    return statusbar;
}
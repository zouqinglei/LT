#include "stdio.h"
#include "stdlib.h"
#include "myRes.h"

#include "myServer.h"

static int start_cb(Ihandle *ih)
{
    IupMessage("Warning", "InterServer Start...");
    startServer();
    return IUP_DEFAULT;
}

static int stop_cb(Ihandle *ih)
{
    IupMessage("Warning", "InterServer Stopped.");
    stopServer();
    return IUP_DEFAULT;
}

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
    IupMessage("About", "   InterServer v1.0\n\nAutors:\n   zouqinglei\n   zouqinglei@163.com");
    return IUP_DEFAULT;
}


void createMenu()
{
    Ihandle * dlg;
    Ihandle * menu, * file_menu, * help_menu;
    Ihandle * start_item, * stop_item, * exit_item;
    Ihandle * about_item;

    start_item = IupItem("Start",NULL);
    IupSetAttribute(start_item, "IMAGE", "IUP_MediaPlay");

    stop_item = IupItem("Stop",NULL);
    IupSetAttribute(stop_item, "IMAGE", "IUP_MediaStop");

    exit_item = IupItem("Exit",NULL);
    IupSetAttribute(exit_item, "IMAGE", "IUP_MessageError");

    about_item = IupItem("About...",NULL);
    IupSetAttribute(about_item, "IMAGE", "IUP_MessageInfo");

    IupSetCallback(start_item, "ACTION", (Icallback)start_cb);
    IupSetCallback(stop_item, "ACTION", (Icallback)stop_cb);
    IupSetCallback(exit_item, "ACTION", (Icallback)exit_cb);
    IupSetCallback(about_item, "ACTION", (Icallback)about_cb);

    file_menu = IupMenu(start_item, stop_item, IupSeparator(),exit_item, NULL);

    help_menu = IupMenu(about_item, NULL);

    menu = IupMenu(IupSubmenu("File",file_menu),
                   IupSubmenu("Help",help_menu),
                   NULL);

    dlg = IupGetHandle("_DIALOG");

    IupSetAttributeHandle(dlg, "MENU", menu);
   

}

Ihandle * createToolbar()
{
    Ihandle * toolbar;
    Ihandle * btnStart, * btnStop, * btnExit, * btnAbout;

    btnStart = IupButton(NULL,NULL);
    IupSetAttribute(btnStart, "IMAGE", "IUP_MediaPlay");
    IupSetAttribute(btnStart, "FLAT", "Yes");
    IupSetAttribute(btnStart, "CANFOCUS", "No"); 
    IupSetAttribute(btnStart, "TIP", "Start Server");

    btnStop = IupButton(NULL,NULL);
    IupSetAttribute(btnStop, "IMAGE", "IUP_MediaStop");
    IupSetAttribute(btnStop, "FLAT", "Yes");
    IupSetAttribute(btnStop, "CANFOCUS", "No"); 

    btnExit = IupButton(NULL,NULL);
    IupSetAttribute(btnExit, "IMAGE", "IUP_MessageError");
    IupSetAttribute(btnExit, "FLAT", "Yes");
    IupSetAttribute(btnExit, "CANFOCUS", "No"); 

    btnAbout = IupButton(NULL,NULL);
    IupSetAttribute(btnAbout, "IMAGE", "IUP_MessageInfo");
    IupSetAttribute(btnAbout, "FLAT", "Yes");
    IupSetAttribute(btnAbout, "CANFOCUS", "No"); 


    toolbar = IupHbox(btnStart,
                      btnStop,
                      btnExit,
                      IupSetAttributes(IupLabel(NULL), "SEPARATOR=VERTICAL"),
                      btnAbout,
                      NULL);

    IupSetAttribute(toolbar, "MARGIN", "5x5");
    IupSetAttribute(toolbar, "GAP", "2");


    IupSetCallback(btnStart, "ACTION", (Icallback)start_cb);
    IupSetCallback(btnStop, "ACTION", (Icallback)stop_cb);
    IupSetCallback(btnExit, "ACTION", (Icallback)exit_cb);
    IupSetCallback(btnAbout, "ACTION", (Icallback)about_cb);

    return toolbar;
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



Ihandle * createMatrix()
{
    Ihandle * mat;
 
    mat = IupMatrix(NULL);
    IupSetAttribute(mat, "NAME", "matrix");
    IupSetAttribute(mat, "EXPAND", "Yes");
    IupSetAttribute(mat, "RESIZEMATRIX", "YES");
    IupSetAttribute(mat, "SCROLLBAR", "YES");
    IupSetAttribute(mat, "NUMLIN_VISIBLE", "0");
 
    IupSetAttribute(mat, "NUMCOL", "2");
    IupSetAttribute(mat, "NUMLIN", "0");

    IupSetAttribute(mat,"NUMCOL_VISIBLE","0");
    IupSetAttribute(mat,"NUMLIN_VISIBLE","0");

    IupSetAttribute(mat, "0:0", "ClientID");
    IupSetAttribute(mat, "0:1", "CLIENT IP");
    IupSetAttribute(mat, "0:2", "PORT");

    return mat;
}
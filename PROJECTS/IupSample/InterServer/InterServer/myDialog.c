#include "stdio.h"
#include "stdlib.h"
#include "myDialog.h"
#include "windows.h"


#include "myRes.h"
#include "myServer.h"


/****************************************
 * local static variable
 ***************************************/

static Ihandle * dlg;

static int dlg_show(Ihandle * ih)
{
    IupSetAttribute(IupGetHandle("_DIALOG"), "HIDETASKBAR", "NO");
    return IUP_DEFAULT;
}

static int dlg_exit(Ihandle * ih)
{
    IupDestroy(IupGetHandle( "_DIALOG"));
    return IUP_DEFAULT;
}

static int showPopmenu()
{
    Ihandle * menu;
    Ihandle * item_show, * item_exit;

    item_show = IupItem("Show", NULL);
    item_exit = IupItem("Exit", NULL);

    IupSetCallback(item_show, "ACTION", dlg_show);
    IupSetCallback(item_exit, "ACTION", dlg_exit);

    menu = IupMenu(item_show, item_exit,NULL);
    
    IupPopup(menu, IUP_MOUSEPOS, IUP_MOUSEPOS);
    IupDestroy(menu);

    return IUP_DEFAULT;
}

static int trayClick_cb(Ihandle * ih, int button, int pressed, int dclick)
{
   
    if(button == 1 && pressed == 1)
    {
        IupSetAttribute(ih, "HIDETASKBAR", "NO");
    }
    else if(button == 3 && pressed == 1)
    {
        showPopmenu();
    }

    return IUP_DEFAULT;
}

static int close_cb(Ihandle * ih)
{
    IupSetAttribute(ih, "HIDETASKBAR", "YES");
    return IUP_IGNORE;
}

static int destroy_cb(Ihandle * ih)
{
    freeServer();
    return IUP_DEFAULT;
}


static int k_esc(Ihandle * ih)
{
    IupDestroy(ih);
    return IUP_DEFAULT;
}

static int map_cb(Ihandle * ih)
{
    initServer();

    return IUP_DEFAULT;
}

static const char * getModulePath(char * modulePath, int size)
{
    int len;
    int offset;

    GetModuleFileName(NULL, modulePath, size);
    
    len = strlen(modulePath);
    offset = len - 1;
    while(offset != 0)
    {
        if(modulePath[offset] == '\\' || modulePath[offset] == '/')
        {
            modulePath[offset] = '\0';
            break;
        }
        offset--;

    }

    return modulePath;

}


void createDialog()
{
    Ihandle * vbox;
    Ihandle * label, * toolbar, * statusbar;
    Ihandle * mat;


    char modulePath[MAX_PATH];
    getModulePath(modulePath, MAX_PATH);


    toolbar = createToolbar();
    label = IupLabel(NULL);
    IupSetAttributes(label, "SEPARATOR=HORIZONTAL");
    statusbar = createStatusbar();

    mat = createMatrix();

    vbox = IupVbox(toolbar,
                   mat,
                   label,
                   statusbar,
                   NULL);

    dlg = IupDialog(vbox);
    IupSetHandle("_DIALOG", dlg);

    createMenu();

    IupSetStrAttribute(dlg, "TITLE", "InterServer");
    IupSetStrAttribute(dlg, "SIZE", "320x200");
    IupSetStrf(dlg, "ICON", "%s\\chat.ico", modulePath);


    IupSetStrAttribute(dlg, "TRAY", "YES");
    IupSetStrAttribute(dlg, "TRAYTIP", "InterServer");
    IupSetStrf(dlg, "TRAYIMAGE", "%s\\chat.ico", modulePath);
        
    IupSetCallback(dlg, "TRAYCLICK_CB", (Icallback)trayClick_cb);
    IupSetCallback(dlg, "CLOSE_CB", (Icallback)close_cb);
    IupSetCallback(dlg, "K_ESC", (Icallback)k_esc);
    IupSetCallback(dlg, "MAP_CB", (Icallback)map_cb);

    IupShowXY(dlg, IUP_CENTER, IUP_CENTER);
    //IupSetStrAttribute(dlg, "HIDETASKBAR", "YES");

}

void myMatrixAppend(unsigned int clientID, const char * clientIP, unsigned short port)
{
    Ihandle * mat;
    int line;

    mat = IupGetDialogChild(dlg, "matrix");

    line = IupGetInt(mat, "NUMLIN");
    line += 1;
    IupSetStrf(mat, "ADDLIN", "%d", line);
    
    
    IupSetIntId2(mat, "", line, 0, clientID);
    IupSetAttributeId2(mat, "", line, 1, clientIP);
    IupSetIntId2(mat, "", line, 2, port);

    IupUpdate(mat);
}

void myMatrixDelete(unsigned int clientID)
{
    Ihandle * mat;
    int line, numLine;
    int ID;

    mat = IupGetDialogChild(dlg, "matrix");
    
    numLine = IupGetInt(mat, "NUMLIN");

    for(line = 1; line <= numLine; line++)
    {
        ID = IupGetIntId2(mat, "", line, 0);
        if(ID == (int)clientID)
        {
            IupSetStrf(mat, "DELLIN", "%d", line);
            break;
        }
    }

    IupUpdate(mat);
}
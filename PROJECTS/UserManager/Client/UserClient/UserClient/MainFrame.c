#include "stdio.h"
#include "stdlib.h"
#include "MainFrame.h"
#include "windows.h"

#include "lt_c_foundation.h"
#include "lt_mqs.h"
#include "service.h"
#include "formLogin.h"
#include "UserMat.h"

static char modulePath[MAX_PATH];

static char username[128] = "";
static char password[128] = "";


static int on_exit_cb(Ihandle * ih)
{
    svr_destroy();
    return IUP_DEFAULT;
}


static int close_cb(Ihandle * ih)
{
   
    return IUP_DEFAULT;
}

static int on_map_cb(Ihandle * ih)
{
    
    return IUP_DEFAULT;
}

static int destroy_cb(Ihandle * ih)
{
    return IUP_DEFAULT;
}


static int k_esc(Ihandle * ih)
{
    IupDestroy(ih);
    return IUP_DEFAULT;
}


static int on_refresh_cb(Ihandle * ih)
{
    LoadUserData();
    return IUP_DEFAULT;
}

static int on_add_cb(Ihandle * ih)
{
    AppendRow();
    return IUP_DEFAULT;
}

static int on_del_cb(Ihandle * ih)
{
    DeleteRow();
    return IUP_DEFAULT;
}

static int on_save_cb(Ihandle * ih)
{
     SaveUserData();

    return IUP_DEFAULT;
}

static int on_edit_cb(Ihandle * ih)
{


    return IUP_DEFAULT;
}

static int on_about_cb(Ihandle *ih)
{
    IupMessage("About", "   UserClient v1.0\n\nAutor:\n   zouqinglei\n   zouqinglei@163.com\n All rights reserved.");
    return IUP_DEFAULT;
}


static int on_login_cb(Ihandle *ih)
{
    int ret;
    json_object * paramObj;
    json_object * resultObj;
    const char * errcod;
    const char * errstr;

    BOOL bLogin;

    if(svr_GetHandle() == NULL)
    {
        if(svr_init() == FALSE)
        {
            IupMessage("Warning", "Can't connect ltmqs server.");
            return IUP_DEFAULT;
        }
    }

    bLogin = FALSE;
    username[0] = '\0';
    password[0] = '\0';
    ret = IupGetParam("Login", NULL, 0,
        "Bt %u[,Cancel] \n"
        "UserName: %s\n"
        "PassWord: %s\n",
        username, password);

    if(ret == 1)
    {
        
        paramObj = jo_new_object();
        resultObj = jo_new_object();

        joo_set_string(paramObj, "username", username);
        joo_set_string(paramObj, "password", password);

        ret = ltns_call(svr_GetHandle(), "USER", "LOGIN",  paramObj, resultObj, 10);

        if(ret == 0)
        {
            errcod = joo_get_string(resultObj, "errcod");
            errstr = joo_get_string(resultObj, "errstr");
            if(strcmp(errcod, "SUCCESS") == 0)
            {
                IupMessage("Warning", "Login Success");
                bLogin = TRUE;
            }
            else
            {
                IupMessage("Warning", errstr);
            }
        }
        else
        {
            IupMessagef("Warning", "An error occured: %d", ret);
        }

        jo_put(paramObj);
        jo_put(resultObj);
    }

    if(bLogin)
    {
        //load data
        LoadUserData();
    }

    return IUP_DEFAULT;
}


static int on_logout_cb(Ihandle *ih)
{
    int ret;
    json_object * paramObj;
    json_object * resultObj;
    const char * errcod;
    const char * errstr;

    BOOL bLogout;

    if(svr_GetHandle() == NULL)
    {   
        IupMessage("Warning", "Maybe login first.");
        return IUP_DEFAULT;  
    }
 
    paramObj = jo_new_object();
    resultObj = jo_new_object();

    joo_set_string(paramObj, "username", username);

    ret = ltns_call(svr_GetHandle(), "USER", "LOGOUT",  paramObj, resultObj, 10);

    if(ret == 0)
    {
        errcod = joo_get_string(resultObj, "errcod");
        errstr = joo_get_string(resultObj, "errstr");
        if(strcmp(errcod, "SUCCESS") == 0)
        {
            IupMessage("Warning", "Logout Success");
            bLogout = TRUE;
        }
        else
        {
            IupMessage("Warning", errstr);
        }
    }
    else
    {
        IupMessagef("Warning", "An error occured: %d", ret);
    }

    jo_put(paramObj);
    jo_put(resultObj);
    
    if(bLogout)
    {
        svr_destroy();
    }

    return IUP_DEFAULT;
}

/***********************************************************
 *
 **********************************************************/

void createMenu()
{
    Ihandle * dlg;
    Ihandle * menu;
    Ihandle *file_menu, *loginItem, *logoutItem, *exit_item;
    Ihandle *help_menu, *about_item;
    Ihandle * hImage;

    char imagePath[MAX_PATH];
    
    //login
    loginItem = IupItem("Login", NULL);
    sprintf(imagePath, "%s\\res\\login.png", modulePath);
    hImage = IupLoadImage(imagePath);
    IupSetAttributeHandle(loginItem, "IMAGE", hImage);

    IupSetCallback(loginItem, "ACTION", (Icallback)on_login_cb);

    //logout
    logoutItem = IupItem("Logout", NULL);

    IupSetCallback(logoutItem, "ACTION", (Icallback)on_logout_cb);
    
    //exit
    exit_item = IupItem("Exit",NULL);
    IupSetAttribute(exit_item, "IMAGE", "IUP_MessageError");

    //about
    about_item = IupItem("About...",NULL);
    IupSetAttribute(about_item, "IMAGE", "IUP_MessageInfo");//"IUP_MessageInfo");

    IupSetCallback(exit_item, "ACTION", (Icallback)on_exit_cb);
    IupSetCallback(about_item, "ACTION", (Icallback)on_about_cb);

    file_menu = IupMenu(loginItem, logoutItem,IupSeparator(),exit_item, NULL);
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
    Ihandle * btnRefresh, * btnAdd, * btnDel, * btnEdit, * btnSave, * btnAbout;
    Ihandle * hImage;

    char imagePath[MAX_PATH];

    //refresh
    btnRefresh = IupButton(NULL,NULL);
    IupSetAttribute(btnRefresh, "FLAT", "Yes");
    IupSetAttribute(btnRefresh, "CANFOCUS", "No"); 
    IupSetAttribute(btnRefresh, "TIP", "Add");

    sprintf(imagePath, "%s\\res\\refresh.png", modulePath);
    hImage = IupLoadImage(imagePath);
    IupSetAttributeHandle(btnRefresh, "IMAGE", hImage);

    IupSetCallback(btnRefresh, "ACTION", (Icallback)on_refresh_cb);

    //add
    btnAdd = IupButton(NULL,NULL);
    IupSetAttribute(btnAdd, "FLAT", "Yes");
    IupSetAttribute(btnAdd, "CANFOCUS", "No"); 
    IupSetAttribute(btnAdd, "TIP", "Add");

    sprintf(imagePath, "%s\\res\\edit_add.png", modulePath);
    hImage = IupLoadImage(imagePath);
    IupSetAttributeHandle(btnAdd, "IMAGE", hImage);

    IupSetCallback(btnAdd, "ACTION", (Icallback)on_add_cb);

    //Del
    btnDel = IupButton(NULL,NULL);
    IupSetAttribute(btnDel, "FLAT", "Yes");
    IupSetAttribute(btnDel, "CANFOCUS", "No"); 
    IupSetAttribute(btnDel, "TIP", "Del");

    sprintf(imagePath, "%s\\res\\edit_del.png", modulePath);
    hImage = IupLoadImage(imagePath);
    IupSetAttributeHandle(btnDel, "IMAGE", hImage);

    IupSetCallback(btnDel, "ACTION", (Icallback)on_del_cb);



    //Edit
    btnEdit = IupButton(NULL,NULL);
    IupSetAttribute(btnEdit, "IMAGE", "IUP_MessageError");
    IupSetAttribute(btnEdit, "FLAT", "Yes");
    IupSetAttribute(btnEdit, "CANFOCUS", "No"); 
    IupSetAttribute(btnEdit, "TIP", "Edit");

    IupSetCallback(btnEdit, "ACTION", (Icallback)on_edit_cb);

    //Save
    btnSave = IupButton(NULL,NULL);
    IupSetAttribute(btnSave, "FLAT", "Yes");
    IupSetAttribute(btnSave, "CANFOCUS", "No"); 
    IupSetAttribute(btnSave, "TIP", "Save");

    sprintf(imagePath, "%s\\res\\save.png", modulePath);
    hImage = IupLoadImage(imagePath);
    IupSetAttributeHandle(btnSave, "IMAGE", hImage);

    IupSetCallback(btnSave, "ACTION", (Icallback)on_save_cb);



    //about
    btnAbout = IupButton(NULL,NULL);
   
    IupSetAttribute(btnAbout, "FLAT", "Yes");
    IupSetAttribute(btnAbout, "CANFOCUS", "No"); 
    IupSetAttribute(btnAbout, "TIP", "About");

    sprintf(imagePath, "%s\\res\\info.png", modulePath);
    hImage = IupLoadImage(imagePath);
    IupSetAttributeHandle(btnAbout, "IMAGE", hImage);

    IupSetCallback(btnAbout, "ACTION", (Icallback)on_about_cb);

    //toolbar
    toolbar = IupHbox(btnRefresh,
                      btnAdd,
                      btnDel,
                      //btnEdit,
                      btnSave,
                      IupSetAttributes(IupLabel(NULL), "SEPARATOR=VERTICAL"),
                      btnAbout,
                      NULL);

    IupSetAttribute(toolbar, "MARGIN", "5x5");
    IupSetAttribute(toolbar, "GAP", "2");

    return toolbar;
}


Ihandle * createStatusbar()
{
   
    Ihandle * statusbar;

    statusbar = IupLabel("ready");
    IupSetAttribute(statusbar, "NAME", "STATUSBAR");
    IupSetAttribute(statusbar, "EXPAND", "HORIZONTAL");
    IupSetAttribute(statusbar, "PADDING", "10x5");

    IupSetAttribute(statusbar, "FGCOLOR", "0 0 128");

    return statusbar;
}

static Ihandle * createSurface()
{
    Ihandle * vbox;
    
    Ihandle *toolbar, *mat, *statusbar;
    Ihandle * separator;
    
    toolbar = createToolbar();
    mat = createUserMat();
    
    separator = IupLabel("");
    IupSetAttribute(separator, "SEPARATOR", "HORIZONTAL");

    statusbar = createStatusbar();

    vbox = IupVbox(toolbar,
                   mat,
                   separator,
                   statusbar,
                   NULL);

    IupSetAttribute(vbox, "GAP", "5");

    return vbox;
}

/**************************************************
 * interface function
 *************************************************/

void createMainFrame()
{
    Ihandle * dlg, * vbox;
    Ihandle * hImage;
    char imagePath[MAX_PATH];

    lt_GetModulePath(modulePath, MAX_PATH);

    vbox = createSurface();

    dlg = IupDialog(vbox);
    IupSetHandle("_DIALOG", dlg);

    createMenu();

    IupSetStrAttribute(dlg, "TITLE", "User Manage System");
    IupSetStrAttribute(dlg, "SIZE", "300x200");

    sprintf(imagePath, "%s\\res\\user_group.png", modulePath);
    hImage = IupLoadImage(imagePath);
    IupSetAttributeHandle(dlg, "ICON", hImage);

    IupSetCallback(dlg, "CLOSE_CB", (Icallback)close_cb);
    IupSetCallback(dlg, "K_ESC", (Icallback)k_esc);
    IupSetCallback(dlg, "MAP_CB", (Icallback)on_map_cb);

    IupShowXY(dlg, IUP_CENTER, IUP_CENTER);

    
}

#include "stdio.h"
#include "stdlib.h"
#include "myDialog.h"
#include "windows.h"

#include "myRes.h"

#include "lt_c_foundation.h"
#include "myClient.h"

static Ihandle * vbox;

static Ihandle * hboxHost, * labelHost, * txtHost, * labelPort, * txtPort;
static Ihandle * hboxConnect, * btnConnect, * btnDisConnect;

static Ihandle * hboxName, * labelName, * txtName;
static Ihandle * txtMessage;
static Ihandle * txtInput;
static Ihandle * hboxSend, * btnSend;

static Ihandle * label, * statusbar;


static int close_cb(Ihandle * ih)
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

static int map_cb(Ihandle * ih)
{
    Ihandle * mat;

    mat = IupGetDialogChild(ih, "matrix");

    IupSetAttribute(mat, "ADDLIN", "0-3");


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

static int btnConnect_cb(Ihandle * ih)
{
    char * hostIP;
    unsigned short port;

    hostIP = IupGetAttribute(txtHost, "VALUE");
    port = IupGetInt(txtPort, "VALUE");

    clientInit(hostIP, (unsigned short)port);
    clientConnect();

    return IUP_DEFAULT;
}

static int btnDisConnect_cb(Ihandle * ih)
{
    clientDisConnect();
    clientDestroy();

    return IUP_DEFAULT;
}

static int btnSend_cb(Ihandle * ih)
{
    const char * name = IupGetAttribute(txtName, "VALUE");
    const char * input = IupGetAttribute(txtInput, "VALUE");
    
    clientSend(name, input);

    return IUP_DEFAULT;
}


static Ihandle * createSurface()
{
    labelHost = IupLabel("Host:");
    txtHost = IupText(NULL);
    IupSetAttribute(txtHost, "RASTERSIZE", "150x");
    IupSetAttribute(txtHost, "VALUE", "127.0.0.1");
    labelPort = IupLabel("Port:");
    txtPort = IupText(NULL);
    IupSetAttribute(txtPort, "VALUE", "9001");

    hboxHost = IupHbox(labelHost, txtHost, labelPort, txtPort, NULL);
    IupSetAttribute(hboxHost, "GAP", "20");
    IupSetAttribute(hboxHost, "MARGIN", "5x1");

    btnConnect = IupButton("Connect",NULL);
    btnDisConnect = IupButton("Disconnect", NULL);

    hboxConnect = IupHbox(btnConnect,  btnDisConnect,  NULL);
    IupSetAttribute(hboxConnect, "GAP", "20");
    IupSetAttribute(hboxConnect, "MARGIN", "5x5");

    labelName = IupLabel("Name:");
    txtName = IupText("");
    hboxName = IupHbox(labelName,txtName,NULL);
    IupSetAttribute(hboxName, "GAP", "10");

    txtMessage = IupText(NULL);
    IupSetAttribute(txtMessage, "EXPAND", "YES");
    IupSetAttribute(txtMessage, "MULTILINE","YES");
    IupSetAttribute(txtMessage, "READONLY","YES");
    IupSetAttribute(txtMessage, "SCROLLBAR","VERTICAL");

    txtInput = IupText(NULL);
    IupSetAttribute(txtInput, "EXPAND", "HORIZONTAL");


    btnSend = IupButton("Send",NULL);
    hboxSend = IupHbox(IupFill(), btnSend, NULL);

    label = IupLabel(NULL);
    IupSetAttributes(label, "SEPARATOR=HORIZONTAL");
    statusbar = createStatusbar();

    vbox = IupVbox(
                   hboxHost,
                   hboxConnect,
                   hboxName,
                   txtMessage,
                   txtInput,
                   hboxSend,
                   label,
                   statusbar,
                   NULL);


    IupSetAttribute(vbox, "GAP", "5");

    IupSetCallback(btnConnect, "ACTION", btnConnect_cb);
    IupSetCallback(btnDisConnect, "ACTION", btnDisConnect_cb);
    IupSetCallback(btnSend, "ACTION", btnSend_cb);
    return vbox;
}


void createDialog()
{
    Ihandle * dlg, * vbox;

    char modulePath[MAX_PATH];
    getModulePath(modulePath, MAX_PATH);

    vbox = createSurface();

    dlg = IupDialog(vbox);
    IupSetHandle("_DIALOG", dlg);
    createMenu();

    IupSetStrAttribute(dlg, "TITLE", "InterClient");
    IupSetStrAttribute(dlg, "SIZE", "300x200");
    IupSetStrf(dlg, "ICON", "%s\\chat.ico", modulePath);


    IupSetCallback(dlg, "CLOSE_CB", (Icallback)close_cb);
    IupSetCallback(dlg, "K_ESC", (Icallback)k_esc);
    IupSetCallback(dlg, "MAP_CB", (Icallback)map_cb);

    IupShowXY(dlg, IUP_CENTER, IUP_CENTER);
}

void dlgShowRecvMessage(const char * name, const char * message)
{
    int lineCount = IupGetInt(txtMessage, "LINECOUNT ");
    if(lineCount > 1000)
    {
        IupSetStrf(txtMessage, "VALUE", "%s say: %s", name, message);
    }
    IupSetStrf(txtMessage, "APPEND", "%s say: %s", name, message);
}

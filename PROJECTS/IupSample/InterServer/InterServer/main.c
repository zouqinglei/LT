/*
    main.c
    write by zouql 20151019
    zouqinglei@163.com
*/

#include "iup.h"
#include "myDialog.h"

int main(int argc, char * argv[])
{
    IupOpen(&argc, &argv);
    IupImageLibOpen();
    IupControlsOpen();
    
    createDialog();
    
    IupMainLoop();
    IupClose();

	return 0;
}

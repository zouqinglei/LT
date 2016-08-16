
#include "iup.h"
#include "iupmatrixex.h"
#include "MainFrame.h"

int main(int argc, char * argv[])
{

   
    IupOpen(&argc, &argv);
    IupImageLibOpen();
    IupControlsOpen();
    IupMatrixExOpen();
    
    createMainFrame();
    
    IupMainLoop();
    IupClose();


	return 0;
}

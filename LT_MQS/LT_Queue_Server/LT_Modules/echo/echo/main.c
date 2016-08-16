#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lt_mqs.h"

static void echo(LtSvcInfo * pInfo)
{
    const char * param_input;
    char * tmp;

    param_input = joo_get_string(pInfo->paramObj, "input");
    tmp = (char *)malloc(strlen(param_input) + 1);
    strcpy(tmp, param_input);
    strupr(tmp);

    joo_set_string(pInfo->resultObj, "out", tmp);
    free((void * )tmp);

    return;
}

int main(int argc, char *argv[])
{
    //init area

    ltns_register("ECHO", echo);

    ltns_serverloop("ECHO", argc, argv);

    //destroy area

    return 0;
}

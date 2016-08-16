/* 
* lt_client.c
* ltmq and ltas monitor client program
* write by zouql 20141024
* zouqinglei@163.com
* 
* modified by zouqinglei@163.com 20160318
*/

#include "lt_c_foundation.h"
#include "lt_mqs.h"
#include "printtable.h"

#define BUFSIZE 1024
#define prompt "$>"

extern int ltns_start(LTMQHandle hConn, const char * module_name);
extern int ltns_stop(LTMQHandle hConn, const char * module_name);

LTMQHandle hMQ;

struct cmdsw {
	char *cmd;		/* command name */
	int a1;			/* min number of args */
	int a2;			/* max number of args */
	int (*fun)();	/* function */
};
int do_cmd(char *cmd);

int help_cmd(int ac,char **av);
int print_cmd(int ac,char **av);
int browse_cmd(int ac,char **av);
int quit_cmd(int ac,char **av);
int stop_cmd(int ac,char **av);
int start_cmd(int ac,char **av);
int create_cmd(int ac,char **av);
int delete_cmd(int ac,char **av);
int clear_cmd(int ac,char **av);
int default_cmd(short ncaller,int ac,char **av);

struct cmdsw commands[] = 
{
	{"help",0,0,  help_cmd},
	{"print",0,0, print_cmd},
    {"browse",0,0,browse_cmd},
	{"create",0,0,create_cmd},
	{"delete",0,0,delete_cmd},
    {"clear",0,0,clear_cmd},
    {"stop",0,0, stop_cmd},
    {"start",0,0, start_cmd},
	{"quit",0,0,  quit_cmd},
	{NULL,0,0,NULL}
};

char *helptx[] = {
	"help    : print this help.",
	"print   : print list. \n\tsyntax: print [client | queue | queue queue_name | module]",
    "browse  : browse queue message. \n\tsyntax: browse queue_name start limit",
	
	"start   : start module.  \n\tsyntax: start [all | module_name]",
    "stop    : stop module.  \n\tsyntax: stop [all | module_name]",
	
	"create  : create qname. \n\tsyntax: create queue_name persistent(Y/N) max_msgnum remark",
	"delete  : delete one queue. \n\tsyntax: delete queue_name",
    "clear   : clear a queue's all messages. \n\tsyntax: clear queue_name",
	"quit  quit!. \n\tsyntax: quit",
     0
};

int readline(int fd, char *buf, int nbytes)
{
   int numread = 0;
   int returnval;
#ifndef _WIN32_WCE
   while (numread < nbytes - 1) {
      returnval = read(fd, buf + numread, 1);
      if ((returnval == -1) && (errno == WSAEINTR))
         continue;
      if ( (returnval == 0) && (numread == 0) )
         return 0;
      if (returnval == 0)
         break;
      if (returnval == -1)
         return -1;
      numread++;  
      if (buf[numread-1] == '\n') {
         buf[numread-1] = '\0';
         return numread; 
      }  
   }    
   errno = WSAGetLastError();
#endif
   return -1;
}


int main(int argc,char *argv[])
{
	char buf[BUFSIZE];
	int num;
  
    char *host;
	unsigned short port;
    int i;
    struct timeval tv;

	host = STR_NEW("127.0.0.1");
	port = 8100;
	
	for (i = 1; i < argc; i++) 
	{
		if (argv[i][0] == '-') 
		{
			switch (argv[i][1]) 
			{
			case 'h':
				if(i < argc -1)
					host = STR_NEW(argv[i + 1]);
				else
				{
					printf("Usage: %s  -h host -p port",argv[0]);
					exit(0);
				}
				break;
				
			case 'p':
				if(i < argc -1)
					port = atoi(argv[i + 1]);
				break;
			}
		}
	}

    lt_initial();
	hMQ = ltmq_init(host, port, 1);

	if(hMQ == NULL)
	{
		printf(" Connect failed! \n");
		exit(0);
	}
	
	printf("    ltmqs monitor 1.0\n  Write by zouqinglei@163.com All right reserved.\n");
	printf("print help for help.\n");
	printf("%s",prompt);

    tv.tv_sec = 5;
    tv.tv_usec = 0;
	while (1) 
	{
        num = readline(0, buf, BUFSIZE);
        if(num > 0)
        {
		    if(do_cmd(buf) == -1)
			    break;
		    printf("\n%s",prompt);
        }
	}
	
	if(host)
		free(host);
	
	ltmq_term(hMQ);
    printf("Connect closed!\n");
    
	exit(0);
}

int do_cmd(char *cmd)
{
	char *p;
	struct cmdsw *csp;

	int ret;
    char ** argvp;
    int numtokens;
	
	ret = 0;	
	/*
	* parse command
	*/
    numtokens = lt_makeargv(cmd, " \t", &argvp);
	if(numtokens <= 0)
    {
        printf("failed to parse the string you given:%s\n");
        return 0;
    }
	
	/*
	* find command and call it
	*/
	p = argvp[0];
	for (csp = commands; csp->cmd; csp++) {
		if (!strcmp(csp->cmd, p)) {
			ret = (csp->fun)(numtokens, argvp);
			break;
		}
	}
	if (!csp->cmd)
		printf("%s: command not found.\n", p);

    lt_freemakeargv(argvp);

	return ret;	
}



int help_cmd(int ac,char **av)
{
    char **p;
	
	for(p = helptx; *p; p++)
	{
		printf("%s\n",(*p));
	}
	return 0;
}

/*********************************************************
 * print 
 ********************************************************/

/*
 "ECHO": {
                 "path":"server.exe",
                 "cmdline": "",
                 "min": 1,
                 "max": 1,  
                 "show": 0,
                 "mode": {"NORMAL", "MAINTENANCE"}
                 "cur":[{"handle":xxx,"pid":xxx},{"handle":xxx,"pid":xxx}]
                },
*/
static void printMod(const char * modName, struct json_object * modObj)
{
    int i,count;
    struct json_object * curObj;
    struct json_object * procObj;

    curObj = joo_get(modObj, "cur");
    count = joa_length(curObj);

    printf("%-*s %-*s %-*s\n",
        15, modName,
        30, joo_get_string(modObj, "path"),
        10, joo_get_string(modObj, "cmdline"));

    printf("%-*s min:%-*s max:%-*s cur:%-*d mode:%-*s\n",
        15, "",
        3,  joo_get_string(modObj, "min"),
        3,  joo_get_string(modObj, "max"),
        3,  count,
        10, joo_get_string(modObj, "mode"));

    for(i = 0; i < count; i++)
    {
        procObj = joa_get_idx(curObj, i);
        printf("%-*s %-*s %-*s\n", 
            15, "",
            5, "PID:",
            10, joo_get_string(procObj, "pid"));
    }

    printf("\n");

}

/*
ltmq_info(LTMQHandle hConn,
              const char * type,
              const char * name,
              json_object * recordObj)
*/
int print_cmd(int ac,char **av)
{
    int ret;
    char * strType;
    json_object * recordObj;

    if(ac < 2 || ac > 3)
    {
        return help_cmd(ac,av);
    }

    strType = strupr(STR_NEW(av[1]));
    recordObj = jo_new_object();

    if(ac == 2)
    {
        ret = ltmq_info(hMQ, strType, NULL, recordObj);
    }
    else if(ac == 3)
    {
        ret = ltmq_info(hMQ, strType, av[2], recordObj);
    }

    free(strType);

    if(ret == 0)
    {
        print_table(recordObj);
        jo_put(recordObj);
    }
    else
    {
        printf("ret %d\n", ret);
    }
   
    return 0;
}

int browse_cmd(int ac,char **av)
{
    int ret;
    json_object * recordObj;
    static int start = 0;
    static int limit;

    recordObj = jo_new_object();
    limit = 10;

    if(ac == 2)
    {
        ret = ltmq_browse(hMQ, av[1], start,limit, recordObj);
    }
    else if(ac == 3)
    {
        start = atoi(av[2]);
        ret = ltmq_browse(hMQ, av[1], start,limit, recordObj);
    }
    else if(ac == 4)
    {
        start = atoi(av[2]);
        limit = atoi(av[3]);
        ret = ltmq_browse(hMQ, av[1], start,limit, recordObj);
    }
    else
    {
        printf("browse queue_name start limit.\n");
        return 0;
    }

    if(ret == 0)
    {
        start += ltr_getrowcount(recordObj);
        print_table(recordObj);
        jo_put(recordObj);
    }
    else
    {
        printf("ret %d.\n",ret);
    }

    return 0;
}



int start_cmd(int ac,char **av)
{
    int ret;

    if(ac == 2)
    {
        ret = ltns_start(hMQ, av[1]);
    }
    else
    {
        printf("start [all | module]\n");
        return 0;
    }

    printf("ret %d.\n",ret);
 
	return 0; 
}

int stop_cmd(int ac,char **av)
{
    int ret;

    if(ac == 2)
    {
        ret = ltns_stop(hMQ, av[1]);
    }
    else
    {
        printf("stop [all | module]\n");
        return 0;
    }

    printf("ret %d.\n",ret);
 
	return 0; 
}

/*
    {
        "req":"QUEUE.CREATE",
        "param":
        {
            "queue_name":"queue name",
            "persistent":"Y" or "N",
            "max_msgnum":0 default 99999,
            "max_msgsize":0 default 4M,
            "remark":""
        }         
     }
     
     reply:
     headobj
     {
        "errcod":"SUCCESS",
        "errstr":"The queue created success."
        "data": {}    
     }
*/

int create_cmd(int ac,char **av)
{
    int ret;
    int persistent;
    int max_msgnum;
   
    if(ac < 5)
    {
        printf("create queue_name persistent(Y/N) max_msgnum remark\n");
        return 0;
    }

    if(strcmp(av[2],"Y") == 0)
    {
        persistent = 1;
    }
    else
    {
        persistent = 0;
    }

    max_msgnum = atoi(av[3]);
   
    ret = ltmq_create(hMQ, av[1], persistent, max_msgnum, 0, av[4]);
    printf("ret %d.\n",ret);

    return 0;
}

int delete_cmd(int ac,char **av)
{
    int ret;

    if(ac < 2)
    {
        printf("delete queue_name\n");
        return 0;
    }

    ret = ltmq_delete(hMQ, av[1]);
    printf("ret %d\n",ret);

    return 0;
}

int clear_cmd(int ac,char **av)
{
    int ret;

    if(ac < 2)
    {
        printf("clear queue_name\n");
        return 0;
    }

    ret = ltmq_clear(hMQ, av[1]);
    printf("ret %d\n",ret);

    return 0;
}

int quit_cmd(int ac,char **av)
{
    return -1;
}


int default_cmd(short ncaller,int ac,char **av)
{
    printf("%s: command may be error.\n", av[0]);
    return 0;
}

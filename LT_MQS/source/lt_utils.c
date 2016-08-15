/* 
 * lt_utils.h
 * write by zouql 20140723
 * zouqinglei@163.com
     
    zouqinglei@163.com 
    All right reserved.
 */

#include "lt_utils.h"
#include "lt_commsocket.h"
#include "md5.h"

#ifndef __WIN__
#ifdef AIX5
#include "uuid.h"
#else
#include <uuid/uuid.h>
#endif
#endif


char * lt_dirname(char *path)
{
	static char singledot[] = ".";
	static char result[MAX_PATH];
	char *lastp;
	size_t len;
	static char slashe;
#ifdef WIN32
	slashe = '\\';
#else
	slashe = '/';
#endif
	/*
	* If `path' is a null pointer or points to an empty string,
	* return a pointer to the string ".".
	*/
	if ((path == NULL) || (*path == '\0'))
		return (singledot);
	
	/* Strip trailing slashes, if any. */
	lastp = path + strlen(path) - 1;
	while (lastp != path && *lastp == slashe)
		lastp--;
	
	/* Terminate path at the last occurence of '/'. */
	do {
		if (*lastp == slashe) {
			/* Strip trailing slashes, if any. */
			while (lastp != path && *lastp == slashe)
				lastp--;
			
			/* ...and copy the result into the result buffer. */
			len = (lastp - path) + 1 /* last char */;
			if (len > (MAX_PATH - 1))
				len = MAX_PATH - 1;
			
			memcpy(result, path, len);
			result[len] = '\0';
			
			return (result);
		}
	} while (--lastp >= path);
	
	/* No /'s found, return a pointer to the string ".". */
	return (singledot);
}

char * lt_basename(char *path, char *suffix)
{
	static char singledot[] = ".";
	static char result[MAX_PATH];
	char *p, *lastp;
	size_t len;
	static char slashe;
	int i;

#ifdef WIN32
	slashe = '\\';
#else
	slashe = '/';
#endif	
	/*
	* If `path' is a null pointer or points to an empty string,
	* return a pointer to the string ".".
	*/
	if ((path == NULL) || (*path == '\0'))
		return (singledot);
	
	/* Strip trailing slashes, if any. */
	lastp = path + strlen(path) - 1;
	while (lastp != path && *lastp == slashe)
		lastp--;
	
	/* Now find the beginning of this (final) component. */
	p = lastp;
	while (p != path && *(p - 1) != slashe)
		p--;
	
	/* ...and copy the result into the result buffer. */
	len = (lastp - p) + 1 /* last char */;
	if (len > (MAX_PATH - 1))
		len = MAX_PATH - 1;
	
	memcpy(result, p, len);
	result[len] = '\0';
	
    for(i = 0; i < (int)strlen(result); i++)
    {
        if(result[i] == '.')
        {
            result[i] = '\0';
            break;
        }
    }

	return (result);
}

int lt_makeargv(const char *s, const char *delimiters, char ***argvp)

{

   int i;

   int numtokens;

   const char *snew;

   char *t;



   if ((s == NULL) || (delimiters == NULL) || (argvp == NULL)) {

       return -1;

   }

   *argvp = NULL;                           

   snew = s + strspn(s, delimiters);         /* snew is real start of string */

   if ((t = malloc(strlen(snew) + 1)) == NULL) 

      return -1; 

   strcpy(t, snew);               

   numtokens = 0;

   if (strtok(t, delimiters) != NULL)     /* count the number of tokens in s */

      for (numtokens = 1; strtok(NULL, delimiters) != NULL; numtokens++) ; 



                             /* create argument array for ptrs to the tokens */

   if ((*argvp = malloc((numtokens + 1)*sizeof(char *))) == NULL) {

      free(t);

      return -1; 

   } 

                        /* insert pointers to tokens into the argument array */

   if (numtokens == 0) 

      free(t);

   else {

      strcpy(t, snew);

      **argvp = strtok(t, delimiters);

      for (i = 1; i < numtokens; i++)

          *((*argvp) + i) = strtok(NULL, delimiters);

    } 

    *((*argvp) + numtokens) = NULL;             /* put in final NULL pointer */

    return numtokens;

}     



void lt_freemakeargv(char **argv)

{

   if (argv == NULL)

      return;

   if (*argv != NULL)

      free(*argv);

   free(argv);

}


int lt_GenUUID(char *uuid,int size)
{
#ifdef __WIN__

#ifndef _WIN32_WCE
    UUID uuid1;
    unsigned char* str;

    UuidCreate(&uuid1);
    UuidToString(&uuid1, &str);
    strncpy(uuid,str,size);
    RpcStringFree(&str);
#endif
    return 0;
   
#else
    uuid_t uu;
    
    uuid_generate(uu);
    uuid_unparse(uu,uuid);
    return 0;

#endif
}


int lt_forkexec(char * exec_path_name,
                   char * cmd_line, 
                   int showWindow, 
                   unsigned long * procID,
                   void ** phProcess,
                   void ** phThread)
{
#ifdef WIN32
#ifndef _WIN32_WCE
	PROCESS_INFORMATION    pi;
	STARTUPINFO	        si;

    char cmd[MAX_PATH];
	
	memset(&si,0,sizeof(STARTUPINFO));
    si.cb = sizeof(STARTUPINFO); 

    if(showWindow == 0)
    {
        si.wShowWindow = SW_HIDE;
    }
    else
    {
        si.wShowWindow = TRUE;
    }
	si.dwFlags = STARTF_USESHOWWINDOW;
    
	if(!exec_path_name)
		return -1;

    _snprintf(cmd,MAX_PATH,"%s %s",exec_path_name, (cmd_line != NULL)?cmd_line:"");

	if( !CreateProcess(NULL, // specify the executable program
		cmd,   // the command line arguments
		NULL,       // ignored in Linux
		NULL,       // ignored in Linux
		FALSE,       // ignored in Linux
        (showWindow == 0)?0:CREATE_NEW_CONSOLE,
		NULL,       // ignored in Linux
		lt_dirname(exec_path_name),
		&si,
		&pi))
    {
        return -1;
	}
    
    if(procID) *procID = pi.dwProcessId;
    if(phProcess) *phProcess = pi.hProcess;
    if(phThread) *phThread = pi.hThread;
    
	return 0;
#endif
#else
	int    processId;
	char *mycmd_line;

	processId = 0;
	mycmd_line = (char *) malloc(strlen(cmd_line ) + 1 );
	
	if(mycmd_line == NULL)
		return EEMALLOC;
	
	strcpy(mycmd_line, cmd_line);
	
	if( ( processId = fork() ) == 0 )	
	{	
		char		*pArg, *pPtr;
		char		*argv[WR_MAX_ARG + 1];
		int		 argc;
		if( ( pArg = strrchr( exec_path_name, '/' ) ) != NULL )
			pArg++;
		else
			pArg = exec_path_name;
		argv[0] = pArg;
		argc = 1;
		
		if( cmd_line != NULL && *cmd_line != '\0' )
		{
			
			pArg = strtok_r(cmd_line, " ", &pPtr);
			
			while( pArg != NULL )
			{
				argv[argc] = pArg;
				argc++;
				if( argc >= WR_MAX_ARG )
					break;
				pArg = strtok_r(NULL, " ", &pPtr);
			}
		}
		argv[argc] = NULL;
		
		execv(exec_path_name, argv);
		free(mycmd_line);
		exit( -1 );
	}
	else if( processId == -1 )
	{
		processId = 0;
		free(mycmd_line);
		return EEFAIL;
	}
	
	return processId;
#endif
}

/*
/
   get the parent processID in windows
/

#define ProcessBasicInformation 0

typedef struct
{
    DWORD ExitStatus;
    DWORD PebBaseAddress;
    DWORD AffinityMask;
    DWORD BasePriority;
    ULONG UniqueProcessId;
    ULONG InheritedFromUniqueProcessId;
}   PROCESS_BASIC_INFORMATION;
// ntdll!NtQueryInformationProcess (NT specific!)
//
// The function copies the process information of the
// specified type into a buffer
//
// NTSYSAPI
// NTSTATUS
// NTAPI
// NtQueryInformationProcess(
//    IN HANDLE ProcessHandle,              // handle to process
//    IN PROCESSINFOCLASS InformationClass, // information type
//    OUT PVOID ProcessInformation,         // pointer to buffer
//    IN ULONG ProcessInformationLength,    // buffer size in bytes
//    OUT PULONG ReturnLength OPTIONAL      // pointer to a 32-bit
//                                          // variable that receives
//                                          // the number of bytes
//                                          // written to the buffer 
// );


typedef LONG (WINAPI *PROCNTQSIP)(HANDLE,UINT,PVOID,ULONG,PULONG);


PROCNTQSIP NtQueryInformationProcess;


unsigned long as_getppid()
{
#ifdef WIN32
    DWORD dwId;
    LONG                      status;
    DWORD                     dwParentPID = (DWORD)-1;
    HANDLE                    hProcess;
    PROCESS_BASIC_INFORMATION pbi;

    NtQueryInformationProcess = (PROCNTQSIP)GetProcAddress(
                                            GetModuleHandle("ntdll"),
											"NtQueryInformationProcess"
											);

    if (!NtQueryInformationProcess)
       return (DWORD)-1;

    dwId = GetCurrentProcessId();

    // Get process handle
    hProcess = OpenProcess(PROCESS_QUERY_INFORMATION,FALSE,dwId);
    if (!hProcess)
       return (DWORD)-1;

    // Retrieve information
    status = NtQueryInformationProcess( hProcess,
                                        ProcessBasicInformation,
                                        (PVOID)&pbi,
                                        sizeof(PROCESS_BASIC_INFORMATION),
                                        NULL
                                      );

    // Copy parent Id on success
    if  (!status)
    {
        dwParentPID = pbi.InheritedFromUniqueProcessId;
        
    }

    CloseHandle (hProcess);

   return dwParentPID;
#endif
   return (unsigned long)-1;
}


 
int as_KillProcessByNameAndID( unsigned long processID , const char * basename)
{
    int ret;
    HMODULE hMod;
    DWORD cbNeeded;
    HANDLE hProcess;
    char szProcessName[MAX_PATH] = "<unknown>";
    
 
    ret = -1;
    // Get a handle to the process.
 
    hProcess = OpenProcess( PROCESS_QUERY_INFORMATION |
                                   PROCESS_VM_READ,
                                   FALSE, processID );
 
    // Get the process name.
 
    if (NULL != hProcess )
    {
        
 
        if ( EnumProcessModules( hProcess, &hMod, sizeof(hMod), 
             &cbNeeded) )
        {
            GetModuleBaseName( hProcess, hMod, szProcessName, 
                               sizeof(szProcessName)/sizeof(TCHAR) );

            if(stricmp(szProcessName,basename) == 0)
            {
                ret = TerminateProcess(hProcess,0);
            }
        }
    }

    CloseHandle( hProcess );

    return ret;
}

*/

int ls_sprintf(char * dst,  int size, const char * src)
{
    int ret;

    if(!dst || !src)
       return -1;
    
    ret = _snprintf(dst,size,"%s",src);
    if(ret < 0 || ret == size)
        dst[size - 1] = '\0';

    return 0;
}


unsigned long lt_getpid()
{
#ifdef WIN32
    return GetCurrentProcessId();
#else
    return getpid();
#endif
}

unsigned long lt_getTickCount()
{
#ifdef WIN32
    return GetTickCount();
#else
    struct timeval now;
	gettimeofday(&now,NULL);

    return now * 1000 + now.tv_usec / 1000;
#endif
}


int lt_GenMD5(const char * str, char * md5, int size)
{
    char szDigest[16];

    if(str == NULL || md5 == NULL)
        return -1;

    if(size <= 32)
        return -1;

    MDString(str,szDigest);
	sprintf(md5,"%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
		(BYTE)szDigest[0],
		(BYTE)szDigest[1],
		(BYTE)szDigest[2],
		(BYTE)szDigest[3],
		(BYTE)szDigest[4],
		(BYTE)szDigest[5],
		(BYTE)szDigest[6],
		(BYTE)szDigest[7],
		(BYTE)szDigest[8],
		(BYTE)szDigest[9],
		(BYTE)szDigest[10],
		(BYTE)szDigest[11],
		(BYTE)szDigest[12],
		(BYTE)szDigest[13],
		(BYTE)szDigest[14],
		(BYTE)szDigest[15]);

    return 0;
}


const char * lt_GetModulePath(char * modulePath, int size)
{
    int len;
    int offset;
#ifdef WIN32
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
#endif
    return modulePath;
}
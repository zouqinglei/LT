/*
   lt_commsocket.cpp: 
   write by zouql 20080222.
   
   zouqinglei@163.com 
   All right reserved.
*/
#include "lt_commsocket.h"

/*return 1 if error, 0 if ok*/
static int lt_ignore_sigpipe()
{
#ifndef __WIN__

    struct sigaction act;
    if(sigaction(SIGPIPE,(struct sigaction *)NULL,&act)<0)
       return 1;
    if(act.sa_handler == SIG_DFL)
    {
        act.sa_handler = SIG_IGN;
        if(sigaction(SIGPIPE,&act,(struct sigaction *)NULL)<0)
            return 1;
    }
#endif
    return 0;
}

int lt_initial()
{
#ifdef  __WIN__
	WORD wVersionRequested;
	WSADATA wsaData;
	int err;
 
	wVersionRequested = MAKEWORD(1,1);
    err=WSAStartup(wVersionRequested, &wsaData);
#endif
	return 0;
}

int lt_destroy()
{
#ifdef  __WIN__
	WSACleanup();
#endif
	return 0;
}
/*            lt_setup_srv
   Return a file descriptor which is bound to a given port.
   parameter:
         hostname = NULL,
         s = number of port to bind to
   returns: file descriptor if successful and -1 on error.
*/
int lt_setup_srv(unsigned short port)
{
    int sock;
    int on;
    struct sockaddr_in server;
    on = 1;
  
	
	if(lt_ignore_sigpipe() != 0)
        return -1;

    BZERO(&server,sizeof(server));
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(port);
    
    if((sock = socket(AF_INET,SOCK_STREAM,0))<0)
        return -1;
#ifndef __WIN__
    setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,&on,sizeof(on));
#endif
    if(bind(sock,(struct sockaddr *)&server,sizeof(server)) < 0)
	{
		lt_close(sock);
        return -1;
	}
    if(listen(sock,5)<0)
	{
		lt_close(sock);
		return -1;
	}
    return sock;
}
/*           lt_accept
    Listen for accept a request from a particular host on a specified port.
    Parameter:
        socket = file descriptor previously bound to listening  port
        hostname = name of host to listen for
        hostnamesize = size of hostname 
    Returns: communication file descriptor or -1 on error.
    Comments: This function is used by the server to listen for commuication.
       It blocks until a remote request is received from the port bound to 
       the given file descriptor.
       hostname is filled with an ASCII string containing the remote host 
       name.hostnamesize point to the large of this buf.
*/
int lt_accept(int fd,char *hostname,int hostnamesize)
{
    struct sockaddr_in cliaddr;
	int newfd;
    unsigned int len = sizeof(struct sockaddr_in);
   
	newfd = accept(fd,(struct sockaddr*)&cliaddr,&len);
    if(newfd > 0)
	{
         if(hostname)
			 strcpy(hostname,inet_ntoa(cliaddr.sin_addr));
	}
    return newfd;
}

int lt_acceptEx(int fd,struct sockaddr_in * pClientAddr)
{
	int newfd;
    unsigned int len = sizeof(struct sockaddr_in);
   
	newfd = accept(fd,(struct sockaddr*)pClientAddr,&len);
    return newfd;
}

/*           lt_connect
   Initiate communication with a remote server.
   parameter:
       addr = the server name of the remote machine
       port = well-known port on remote server
   return the file descriptor used for commuincation or -1 if error
*/

int lt_connect(char *addr,unsigned short port)
{
    struct sockaddr_in server;
    int retval;
	int sock;
    unsigned long on;

    on = 1;
    if((lt_ignore_sigpipe() !=0)||
        ((sock = socket(AF_INET,SOCK_STREAM,0))<0))
        return -1;
#ifndef WIN32
    setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,&on,sizeof(on));
#endif
   
    BZERO((char*)&server,sizeof(struct sockaddr_in));
    server.sin_addr.s_addr = inet_addr(addr);
    server.sin_port = htons(port);
    server.sin_family = AF_INET;
    retval = connect(sock,(struct sockaddr *)&server,sizeof(struct sockaddr));
    if(retval == -1)
    {
        shutdown(sock,2);
#ifdef __WIN__
        closesocket(sock);
#else
		close(sock);
#endif
       
        return -1;
    }
   
    return sock;
}

int lt_connectwait(char *addr,unsigned short port,struct timeval timeout)
{
    struct sockaddr_in server;
    int retval;
	int sock;
    unsigned long on;
	fd_set writeset;
	int error;
	int len;
	int ret = 0;

    on = 1;
    if((lt_ignore_sigpipe() !=0)||
        ((sock = socket(AF_INET,SOCK_STREAM,0))<0))
        return -1;
#ifndef __WIN__
    setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,&on,sizeof(on));
#endif
   
	lt_setblock(sock,0);
    lt_setlinger(sock,1,0);

    BZERO((char*)&server,sizeof(struct sockaddr_in));
    server.sin_addr.s_addr = inet_addr(addr);
    server.sin_port = htons(port);
    server.sin_family = AF_INET;
    retval = connect(sock,(struct sockaddr *)&server,sizeof(struct sockaddr));
    if(retval < 0)
    {
		FD_ZERO(&writeset);
		FD_SET(sock,&writeset);

		if(select(FD_SETSIZE,NULL,&writeset,NULL,&timeout) > 0)
		{
#ifndef __WIN__
			getsockopt(sock,SOL_SOCKET,SO_ERROR,(char *)&error,&len);
			if(error == 0)
				ret = 0;
			else
				ret = -1;
#else
			ret = 0;
#endif
		}
		else
			ret = -1;

		if(ret == -1)
		{
            shutdown(sock,2);
#ifdef __WIN__
            
			closesocket(sock);
#else
			close(sock);
#endif
			return -1;
		}
       
        return sock;
    }
   
    return sock;
}

/*          lt_close
    Close commnuication for the given file descriptor.
    parameter: 
          socket = file descriptor of socket connection to be closed
    returns:
          a negative value indicates an error occurred
*/

int lt_close(int fd)
{
	shutdown(fd,2);
#ifdef __WIN__
	closesocket(fd);
#else
    close(fd);
#endif

    return 0;
   
}

int lt_readblock(int fd, void *buf, int size,struct timeval timeout)
{
    char *bufp;
    int bytesread;
    int bytestoread;
    int totalbytes;
	
	fd_set readset;
	fd_set exset;
    int retval;
	
	retval = 0;
	for (bufp = buf, bytestoread = size, totalbytes = 0;
	bytestoread > 0;
	bufp += bytesread, bytestoread -= bytesread) 
	{
		
        FD_ZERO(&readset);
		FD_SET(fd,&readset);
		FD_ZERO(&exset);
		FD_SET(fd,&exset);
		bytesread = 0;
		while (((retval = select(FD_SETSIZE, &readset, NULL, &exset, 
			((timeout.tv_sec == 0) && (timeout.tv_usec == 0)) ? NULL : &timeout)) == -1)
#ifdef __WIN__
			&& (WSAGetLastError() == WSAEINTR) )
#else
			&& (errno == EINTR) )
#endif
		{
			FD_ZERO(&readset);
			FD_SET(fd, &readset);
			FD_ZERO(&exset);
			FD_SET(fd,&exset);
		}
		
		if(retval > 0)
		{
			if(FD_ISSET(fd,&readset))
			{
#ifdef __WIN__
				bytesread = recv(fd,bufp,bytestoread,0);
#else
				bytesread = read(fd, bufp, bytestoread);
#endif
				if(bytesread > 0)
				{
					totalbytes += bytesread;
					continue;
				}
				
				if (bytesread == 0)
					return -1;
				
				if ((bytesread < 0) 
#ifdef __WIN__
					&& (WSAGetLastError() != WSAEWOULDBLOCK) )
#else
					&& (errno != EWOULDBLOCK) )
#endif
					return -1;
				if (bytesread < 0)
					bytesread = 0;
				continue;
				
			}
			
			if(FD_ISSET(fd,&exset))
				return -1;
		}
		
		if (retval < 0) 
			return -1;
		
		if(retval == 0)
		{
			if((bytestoread > 0) && (bytestoread < size))
				continue;
			return -2;
		}
	}
    return 0;
}

int lt_read(int fd,void *buf,int size)
{
	int bytesread;
	while(1)
	{
#ifdef __WIN__
		bytesread = recv(fd,buf,size,0);
#else
		bytesread = read(fd, bufp, bytestoread);
#endif
		if(bytesread > 0)
			return bytesread;
		if(bytesread == 0)
			return -1;
		if ((bytesread < 0) 
#ifdef __WIN__
			&& (WSAGetLastError() != WSAEWOULDBLOCK) )
#else
			&& (errno != EWOULDBLOCK) )
#endif
			return -1;
	if (bytesread < 0)
		bytesread = 0;
	continue;
	}

	return 0;
}

/*
  return : 0: success
           -1: disconnect
           -2: timeout
*/
int lt_readwait(int fd,void *buf,int size,struct timeval timeout)
{
	int bytesread;
	fd_set readset;
	fd_set exset;
    int retval;
	int err = 0;

	FD_ZERO(&readset);
	FD_SET(fd,&readset);
	FD_ZERO(&exset);
	FD_SET(fd,&exset);
	bytesread = 0;
	while (((retval = select(FD_SETSIZE, &readset, NULL, &exset, 
		((timeout.tv_sec == 0) && (timeout.tv_usec == 0)) ? NULL : &timeout)) == -1)
#ifdef __WIN__
		&& ((err = WSAGetLastError()) == WSAEINTR) )
#else
		&& (errno == EINTR) )
#endif
	{
		FD_ZERO(&readset);
		FD_SET(fd, &readset);
		FD_ZERO(&exset);
		FD_SET(fd,&exset);
	}

	if (retval < 0) 
		return -1;

	if(retval == 0)
		return -2;

	if(retval > 0)
	{
#ifdef __WIN__
		bytesread = recv(fd,buf,size,0);
#else
		bytesread = read(fd, bufp, bytestoread);
#endif
		if(bytesread > 0)
			return bytesread;
		if(bytesread == 0)
			return -1;
		if ((bytesread < 0) 
#ifdef __WIN__
			&& (WSAGetLastError() != WSAEWOULDBLOCK) )
#else
			&& (errno != EWOULDBLOCK) )
#endif
			return -1;
	if (bytesread < 0)
		return 0;
	
	}

	return 0;
}

int lt_readline(int fd, void * buf, int size, struct timeval timeout)
{
    int ret = 0;
    int i = 0;
    char * p = (char *)buf;
    
    while((ret = lt_readblock(fd,p,1,timeout)) >= 0)
    {
        if((p - (char *)buf) > 0)
        {
            if(( *p == '\n') && (*(p - 1) == '\r')) 
                return p - (char *)buf + 1;
        }
        p++;
    }

    return ret;
}


int lt_writeblock(int fd, void *buf, int size,struct timeval timeout)
{
    char *bufp;
    int bytestowrite;
    int byteswritten;
    int totalbytes;
	
	fd_set writeset;
	fd_set exset;
    int retval;
    int nSelect;
	
    retval = 0;
	nSelect = 0;
    for (bufp = buf, bytestowrite = size, totalbytes = 0;
	bytestowrite > 0;
	bufp += byteswritten, bytestowrite -= byteswritten) 
	{
		FD_ZERO(&writeset);
		FD_SET(fd,&writeset);
		FD_ZERO(&exset);
		FD_SET(fd,&exset);
		byteswritten = 0;
		
		if(nSelect == 1)
		{
			while (((retval = select(FD_SETSIZE, NULL,&writeset, &exset,
				(timeout.tv_sec == 0)?NULL:&timeout)) == -1)
#ifdef __WIN__
				&& (WSAGetLastError() == WSAEINTR) )
#else
				&& (errno == EINTR) )
#endif
			{
				FD_ZERO(&writeset);
				FD_SET(fd, &writeset);
				FD_ZERO(&exset);
				FD_SET(fd,&exset);
			}
		}
		
		if(retval > 0 || nSelect == 0)
		{
			if(FD_ISSET(fd,&writeset) || nSelect == 0)
			{
#ifdef __WIN__
				byteswritten = send(fd,bufp,bytestowrite,0);
#else
				byteswritten = write(fd, bufp, bytestowrite);
#endif
				if(byteswritten > 0)
				{
					totalbytes += byteswritten;
					nSelect = 0;
					continue;
				}
				if (byteswritten == 0)
					return -1;
				if ((byteswritten == -1) 
#ifdef __WIN__
					&& (WSAGetLastError() != WSAEWOULDBLOCK) )
#else
					&& (errno != EWOULDBLOCK) )
#endif
					return -1;
				if (byteswritten == -1)
				{
					nSelect = 1;
					byteswritten = 0;
				}
				continue;
			}
			if(FD_ISSET(fd,&exset))
				return -1;
		}
		if (retval < 0) 
			return -1;
		if(retval == 0)
			return -2;
	}
    return 0;
}


int lt_canread(int fd,struct timeval timeout)
{
	int ret;
    fd_set readset;
	fd_set exset;
    int block;

	block = 0;
    if((timeout.tv_sec == 0) && (timeout.tv_usec == 0))
        block = 1;

    FD_ZERO(&readset);
    FD_SET(fd,&readset);
	FD_ZERO(&exset);
	FD_SET(fd,&exset);
    while(((ret = 
		select(FD_SETSIZE,&readset,NULL,&exset,block?NULL:&timeout)) == -1)
#ifdef __WIN__
		&& (WSAGetLastError() == WSAEINTR) )
#else
		&& (errno == EINTR) )
#endif
	{
		FD_ZERO(&readset);
        FD_SET(fd,&readset);
		FD_ZERO(&exset);
	    FD_SET(fd,&exset);
	}
	
	if(FD_ISSET(fd,&exset))
		return -1;

	if(ret > 0)
	{
		if(FD_ISSET(fd,&readset))
			return 0;
		return -1;
	}
	if(ret == 0)
		return -2; /*timeout*/

	return -1;
}


int GetLocalAddr(int fd,char *addr, short *port, unsigned long *ip)
{
	struct sockaddr_in addrLocal;
	char *tmp;
	unsigned int len = sizeof(struct sockaddr_in);
	if(getsockname(fd,(struct sockaddr*)&addrLocal,&len)== -1)
		return 0;
	
	tmp = inet_ntoa(addrLocal.sin_addr);
	if(!tmp)
		return 0;
	if(addr)
		strcpy(addr,tmp);
	if(port)
		*port = ntohs(addrLocal.sin_port);
	if(ip)
		*ip = addrLocal.sin_addr.s_addr;
	return 0;
}

int lt_setblock(int fd,int nBlock)
{
    unsigned long flag;
#ifdef __WIN__
	 if(nBlock)
		 flag = 0;
	 else
		 flag = 1;
	 ioctlsocket(fd,FIONBIO,(void*) &flag);
#else
    flag = fcntl(fd,F_GETFL,(long)0);

    if(nBlock)
        flag &= ~O_NONBLOCK;
    else
        flag |= O_NONBLOCK;
    fcntl(fd,F_SETFL,flag);
#endif
    return 0;
}

int lt_setnodelay(int fd)
{
    int optval,optlen;

    optval = 1;
    optlen = sizeof( optval );

    if( setsockopt( fd, IPPROTO_TCP, TCP_NODELAY,
                    (char *)&optval, optlen ) < 0 )
        return -1;

    return 0;
}

int lt_setlinger( int fd, int onoff, int lingertime)
{
    struct linger linger;

    linger.l_onoff = (unsigned short)onoff;
    linger.l_linger = (unsigned short)lingertime;

    if( setsockopt( fd, SOL_SOCKET, SO_LINGER,
                    (char *)&linger, sizeof(linger) ) < 0 )
        return -1;

    return -1;
}


int lt_TcpSetKeepAlive(int sock,int iCkTime)
{
	int var;
	int iRetCode;
   
	
	var=1;
	iRetCode=setsockopt(sock,SOL_SOCKET,SO_KEEPALIVE,(const char *)&var,sizeof(int));
	if(iRetCode<0)
		return -1;
	var=iCkTime;
	iRetCode=setsockopt(sock,IPPROTO_TCP,SO_KEEPALIVE,(const char *)&var,sizeof(var));

    /*
    int                 keepIdle = 6;
    int                 keepInterval = 5;
    int                 keepCount = 3;

    setsockopt(sock, SOL_TCP, TCP_KEEPIDLE, (void *)&keepIdle, sizeof(keepIdle));

    setsockopt(sock, SOL_TCP,TCP_KEEPINTVL, (void *)&keepInterval, sizeof(keepInterval));

    setsockopt(sock,SOL_TCP, TCP_KEEPCNT, (void *)&keepCount, sizeof(keepCount)); 
*/
	if(iRetCode<0)
		return -1;
	return 0;
} 


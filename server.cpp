//TCP服务器

/*

 用法:./server ip port

 说明:该流式套接字服务器程序工作于多线程模式，处理客户端发来的请求命令，只有查询命令，服务器才会回复

 */

#include <stdio.h>

#include <unistd.h>

#include <stdlib.h>

#include <errno.h>

#include <time.h>

#include <sys/socket.h>

#include <sys/types.h>

#include <arpa/inet.h>

#include <pthread.h>

#include <string.h>

#include<syslog.h>

#include<sys/types.h>

#include<sys/stat.h>

#include <signal.h>

#include "printertomysql.h"

#define BUFSIZE 1024

#define BACKLOG 128

int g_iUseSysLog = 1;

enum COMMAND
{
	INSERT = 1,
	SELECT = 2,
	UPDATE = 3,
	DELETE = 4,
};
static void bail(const char *on_what)
{
	if(g_iUseSysLog)
	{
		//char out_buf[100];
		//snprintf(out_buf, strlen(on_what), "%s",on_what);
		syslog(LOG_INFO,"%s", on_what);
	}
	else
	{

   		 fputs(strerror(errno),stderr);

   		 fputs(": ",stderr);

   		 fputs(on_what, stderr);

   		 fputc('\n',stderr);

	}

	exit(1);
}

time_t td;

struct tm tm;

PrinterToMySQL * printerToMySQL;

struct TmpSockt
{

    int connfd;

    sockaddr_in client;

};

void *start_routine(void *arg)
{

    char reqBuf[BUFSIZE];
	
	char *p;

    char dtfmt[BUFSIZE];

	char sql[BUFSIZE];
	
	string name("");
	string passwd("");
	string strval("");
	int count = 0;
	
	int id = 0,command = 0;
	
    struct TmpSockt *temp=(TmpSockt*)arg;

    while (1)
    {

       //读取客户端发来的请求，若客户端没有发送请求，则服务器将阻塞
        long  bytes=read(temp->connfd, reqBuf,sizeof(reqBuf));
        if(bytes<0)
            syslog(LOG_INFO,"read()");
		
       //服务器检查客户端是否关闭了套接字，此时read操作返回0(EOF)，

       //如果客户端关闭了其套接字，则服务器将执行close结束此连接，然后开始接收下一个客户端的连接请求

        if(bytes==0)
        {

	    if(g_iUseSysLog == 0)
	    {
            	printf("The %ld thread exit...\n",(long)pthread_self());

	    }
	    else
	    {
		syslog(LOG_INFO,"The %ld thread exit...\n",(long)pthread_self() );
	    }
	    close(temp->connfd);

            delete temp;

            pthread_exit(NULL);

            break;

        }
		//提取command和key
		
		/*
		  command   name   passwd
		*/
		p = reqBuf;
		while(*p != 0)
		{
			if(p == reqBuf)
			{
				command = atoi(p);
				p++;
				continue;
			}

			if(*p == ' ')
			{
				p++;
				count++;
				continue;
			}
			if(count < 4 && count > 0)
			{
				name += *p;
			}
			else
			{
				passwd += *p;
			}
			p++;
		}
		
		switch(command)
		{
			case INSERT:
						//插入
						memset(sql,0,BUFSIZE);
						sprintf(sql,"insert into 3d_user(name,passwd) values('%s','%s')",name.c_str(),passwd.c_str());

						if(printerToMySQL->InsertData(sql) == 0)
						{
							if(g_iUseSysLog == 0)
							{
								fprintf(stderr,"插入成功\n");
							}
							else
							{
								syslog(LOG_INFO,"插入成功\n");
							}
						}
						else
						{
							if(g_iUseSysLog == 0)
							{
								fprintf(stderr,"插入失败\n");
							}
							else
							{
								syslog(LOG_INFO,"插入失败\n");	
							}
						}
						strval = "ok";
						
						break;
				
			case SELECT:
						//查询
						memset(sql,0,BUFSIZE);
						sprintf(sql,"SELECT * FROM 3d_user where name = '%s'",name.c_str());
						
						strval = printerToMySQL->SelectData(sql,3);

						break;
				
			case UPDATE:
						//更新
						memset(sql,0,BUFSIZE);
						sprintf(sql,"update 3d_user set name = '%s',passwd='%s' where id = '%d'",name.c_str(),passwd.c_str(),id);

						if(printerToMySQL->UpdateData(sql) == 0)
						{
							if(g_iUseSysLog == 0)
							{
								fprintf(stderr,"更新成功\n");
							}
							else
							{
								syslog(LOG_INFO,"更新成功\n");
							}
						}
						else
						{
							if(g_iUseSysLog == 0)
							{
								fprintf(stderr,"更新失败\n");
							}
							else
							{
								syslog(LOG_INFO,"更新失败\n");
							}
						}
						strval = "ok";
						
						break;
				
			case DELETE:
						//删除
						memset(sql,0,BUFSIZE);
						sprintf(sql,"delete from 3d_user where id = '%d'",id);
						
						if(printerToMySQL->DeleteData(sql) == 0)
						{
							if(g_iUseSysLog == 0)
							{
								fprintf(stderr,"删除成功\n");
							}
							else
							{
								syslog(LOG_INFO,"删除成功\n");
							}
						}
						else
						{
							if(g_iUseSysLog == 0)
							{
								fprintf(stderr,"删除失败\n");
							}
							else
							{
								syslog(LOG_INFO,"删除失败\n");
							}
						}
						
						strval = "ok";
						
						break;
			default:
                               syslog(LOG_INFO,"command:%d name:%s passwd:%s",command,name.c_str(),passwd.c_str());
			      ;
		}
		
		//向请求字符串尾添加NULL字符构成完整的请求结果
        reqBuf[bytes]=0;
		
		//将格式化结果发送给客户端
		bytes=write(temp->connfd, strval.c_str(),strval.length());

        if (bytes<0)
            syslog(LOG_INFO,"write()");
    }

    return NULL;
}

void sig_term(int signo)
{ 
	if(signo == SIGTERM)
	/* catched signal sent by kill(1) command */
	{ 
		syslog(LOG_INFO, "program terminated.");
		closelog(); 
		exit(0); 
	}
}


int main(int argc,char **argv)
{
    int val = daemon(0,0);
    if(val != 0)
	    return -1;
    char host[]="127.0.0.1";
    char user[]="momo";
    char port[] ="3306";
    char passwd[]="momo12";
    char dbname[]="3d"; 
    char charset[] = "GBK";//支持中文
#if 0
    int sockfd,connectfd; 
	
	//描述符

    pthread_t thread;

    TmpSockt *sockt;

    int portnumber; 
	//端口号

    struct sockaddr_in server;//服务器地址信息

    struct sockaddr_in client;//客户端地址信息

    socklen_t sin_size;

    openlog("3d_server", LOG_PID, LOG_DAEMON);
    syslog(LOG_INFO, "program started.");
    signal(SIGTERM, sig_term); /* arrange to catch the signal */

    if ((portnumber=atoi(argv[2]))==-1)
    {

	if(g_iUseSysLog == 0)
	{
        	fprintf(stderr,"格式错误");
	}
	else
	{
		syslog(LOG_INFO,"格式错误");
	}
        exit(1);
    }

    if ((sockfd=socket(AF_INET,SOCK_STREAM,0))==-1)
    {

	if(g_iUseSysLog == 0)
	{
        	fprintf(stderr,"socket error %s",strerror(errno));
	}
	else
	{
		syslog(LOG_INFO,"socket error %s",strerror(errno));
	}

        exit(1);

    }

    

    //设置套接字选项为SO_REUSEADDR

    int opt=SO_REUSEADDR;

    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    bzero(&server, sizeof(server));

    server.sin_family=AF_INET;

    server.sin_port=htons(portnumber);

    if (!inet_aton(argv[1], &server.sin_addr))
    {

        syslog(LOG_INFO,"bad address");

    }

   //绑定套接字到相应地址

    if (bind(sockfd, (struct sockaddr*)&server,sizeof(server))==-1)
    {
	if(g_iUseSysLog == 0)
	{
        	fprintf(stderr,"bind error:%s\a\n",strerror(errno));
	}
	else
	{
		syslog(LOG_INFO,"bind error:%s\a\n",strerror(errno));
	}
        exit(1);

    }

   //监听网络连接

    if (listen(sockfd,BACKLOG)==-1)
    {

	if(g_iUseSysLog == 0)
	{
       		fprintf(stderr,"Listen error:%s\a\n",strerror(errno));
	}
	else
	{
		syslog(LOG_INFO,"Listen error:%s\a\n",strerror(errno));
	}
        exit(1);

    }
#endif

	//初始化
    printerToMySQL = new PrinterToMySQL;
	printerToMySQL->InitMySQL();
    if(printerToMySQL->ConnMySQL(host,port,dbname,user,passwd,charset) == 0)
    {
	    if(g_iUseSysLog == 0)
	    {
          	    fprintf(stderr,"连接成功\n");
	    }
	    else
	    {
		    syslog(LOG_INFO,"连接成功\n");
	    }
    }
    else
    {
	    if(g_iUseSysLog == 0)
	    {
           	    fprintf(stderr,"连接失败");
	    }
	    else
	    {
		    syslog(LOG_INFO,"连接失败");
	    }
    }

#if 0
    sin_size=sizeof(struct sockaddr_in);


    syslog(LOG_INFO,"waiting for the client's request...\n");
#if 1
    while (true)
    {
#if 1
        if ((connectfd=accept(sockfd, (struct sockaddr*)&client, &sin_size))==-1)
        {

		if(g_iUseSysLog == 0)
		{
           		 fprintf(stderr,"accept error:%s\n",strerror(errno));
		}
		else
		{
			syslog(LOG_INFO,"accept error:%s\n",strerror(errno));
		}

            	exit(1);

        }
	if(g_iUseSysLog == 0)
	{
        	fprintf(stdout,"Server got connection from %s\n",inet_ntoa(client.sin_addr));
	}
	else
	{
		syslog(LOG_INFO,"Server got connection from %s\n",inet_ntoa(client.sin_addr));
	}

        

        sockt=new TmpSockt;

        sockt->connfd=connectfd;

        memcpy((void *)&sockt->client, &client,sizeof(client));

       //产生线程,并执行线程函数，处理客服端请求
        if (pthread_create(&thread,NULL,start_routine, (void*)sockt)==-1)
        {
		if(g_iUseSysLog == 0)
		{
            		fprintf(stderr,"pthread_creat() error:%s\a\n",strerror(errno));
		}
		else
		{
			syslog(LOG_INFO,"pthread_creat() error:%s\a\n",strerror(errno));
		}

            exit(1);

        }
	
       sleep(1);
#endif
    }

#endif
    close(sockfd); 
#endif
        printerToMySQL->CloseMySQLConn();
	//关闭监听
        syslog(LOG_INFO,"program stop!!!");

#if 0
	while(true)
	{
		sleep(1);

	}
#endif
	syslog(LOG_INFO,"program stop");
    return 0;
}

//TCP客户端

/*

 

 用法：./client hostname port

 

 说明：本程序使用TCP连接和TCP服务器通信，当连接建立后，向服务器发送如下格式字符串

 

 格式字符串示例：

 (1) %D

 (2) %A %D %H:%M:%S

 (3) %A

 (4) %H:%M:%S

 (5)...

 
 */

#include <stdio.h>

#include <stdlib.h>

#include <unistd.h>

#include <errno.h>

#include <string.h>

#include <netdb.h>

#include <sys/types.h>

#include <time.h>

#include <sys/socket.h>

#include <arpa/inet.h>

#define BUFSIZE 1024

#define backlog 128 
//等待队列大小

static void bail(const char *on_what)
{

    fputs(strerror(errno),stderr);

    fputs(": ",stderr);

    fputs(on_what, stderr);

    fputc('\n',stderr);

    exit(1);

}

int main(int argc,char *argv[])
{

    int sockfd;//客户端套接字

    char buf[BUFSIZE];

    struct sockaddr_in server_addr;

    int portnumber;

    long nbytes;

    long z;

    char reqBuf[BUFSIZE];
	char rg = 0x06;//行隔开
    char cg = {0x05};//字段隔开
	
	/*
	string str("");
	str += command;
	str += rg;
	str += name;
	str += rg;
	str += passwd;
	str += '\n';
	*/

    if (argc!=3)
    {

        printf("输入格式错误\n");

        exit(1);

    }

    if ((portnumber=atoi(argv[2]))<0)
    {

        fprintf(stderr,"Usage:%s hostname portnumber\a\n",argv[0]);

        exit(1);

    }
	

   //创建客户端套接字
   
    if ((sockfd=socket(PF_INET,SOCK_STREAM,0))==-1)
    {

        fprintf(stderr,"Socket error:%s\a\n",strerror(errno));

        exit(1);

    }

	
   //创建服务器地址

    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family=AF_INET;

    server_addr.sin_port=htons(portnumber);

    if (!inet_aton(argv[1], &server_addr.sin_addr))
    {

        bail("bad address");

    }

	
   //连接服务器

    if (connect(sockfd, (struct sockaddr*)(&server_addr),sizeof(server_addr))==-1)
    {

        fprintf(stderr,"connect error:%s\a\n",strerror(errno));

        exit(1);

    }

    printf("connected to server %s\n",inet_ntoa(server_addr.sin_addr));

	
   //客户端主循环输入 “quit”退出

    for (; ; )
    {

       //提示输入命令

        fputs("\nEnter fotmat string(^D or 'quit' to exit):",stdout);

        if (!fgets(reqBuf,sizeof(reqBuf),stdin))
        {

            printf("\n");
			
            break;
        }

		
       //为日期时间请求字符串添加NULL字符作为结尾，另外同时去掉末尾的换行符

        z=strlen(reqBuf);

        if (z>0 && reqBuf[--z]=='\n')

            reqBuf[z]=0;

 
        if (z==0)//客户端仅键入Enter

            continue;

  
        //输入‘quit’退出

        if(!strcasecmp(reqBuf,"QUIT"))//忽略大小写比较
        {

            printf("press any key to end client.\n");

            getchar();

            break;

        }

       //发送日期时间请求字符串到服务器，注意请求信息中去掉了NULL字符

        z=write(sockfd, reqBuf, sizeof(reqBuf));

        printf("client has sent '%s' to the sever\n",reqBuf);

        if (z<0)
		{
            bail("write()");
		}


       //从客户端套接字中读取服务器发回的应答

        if ((nbytes=read(sockfd,buf,sizeof(buf)))==-1)
        {

            fprintf(stderr,"read error:%s\n",strerror(errno));

            exit(1);

        }

		
       //若服务器由于某种原因关闭了连接，则客户端需要处理此事件

        if(nbytes==0)
        {

            printf("server hs closed the socket.\n");

            printf("press any key to exit...\n");

            getchar();

            break;

        }

        buf[nbytes]='\0';


        //输出日期时间结果

        printf("result from %s port %u:\n\t'%s'\n",inet_ntoa(server_addr.sin_addr),(unsigned)ntohs(server_addr.sin_port),buf);

    }

    close(sockfd);

    return 0;

}
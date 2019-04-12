/*
 * CThread.cc
 *
 *  Created on: Mar 4, 2013
 *      Author: yaowei
 */

#include "worker_threads.h"
#include "global_settings.h"
#include "utils.h"
#include "socket_wrapper.h"
#include <syslog.h>
#include "printertomysql.h"
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>

#define BUFSIZE 1024 

extern PrinterToMySQL * printerToMySQL;

int CWorkerThread::init_count_ = 0;
pthread_mutex_t	CWorkerThread::init_lock_ = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  CWorkerThread::init_cond_ = PTHREAD_COND_INITIALIZER;

int CWorkerThread::freetotal_ = 0;
int CWorkerThread::freecurr_  = 0;
boost::mutex CWorkerThread::mutex_;
std::vector<CONN*> CWorkerThread::vec_freeconn_;

int g_iUseSysLog = 1;

enum COMMAND
{
	INSERT = 1,
	SELECT = 2,
	UPDATE = 3,
	DELETE = 4,
};

void CWorkerThread::start_routine(struct bufferevent *bev, void *arg) 
{
	char sql[BUFSIZE];
	char readbuf[BUFSIZE];
//	char tempbuf[BUFSIZE];
	memset(readbuf,0,BUFSIZE);
	memset(readbuf,0,BUFSIZE);

	std::string strval;
	enum COMMAND command;

        CONN* conn = static_cast<CONN*>(arg);
	assert(conn != NULL);
	char key = 0xff;
	long bytes = 0;
        do 	
	{

                if((bytes = bufferevent_read(bev, readbuf, BUFSIZE)) > 0)
		{
#if 1
			 for(int i = 0; i < bytes; i++)
			 {
				readbuf[i] ^= key; 
			 }
#endif
			 //conn->rlen = conn->rlen + bytes;
			 //syslog(LOG_INFO,"read :%s",readbuf);
			 //防止恶意连接，进行token校验，不满足校验条件的为恶意连接，直接关闭

			 size_t outsize = 0;
#if 0
			 on_read_cb(tempbuf, (size_t)bytes,readbuf,outsize,conn->client);
			 if(ssl_client_want_write(&(conn->client)))
			 {
			 
				 do_sock_write(conn->client);
			 }
#endif
#if 1
			 if (bytes >= TOKEN_LENGTH && conn->isVerify == false)
			 {
				 syslog(LOG_INFO,"test");
				 conn->isVerify = true;
				 std::string str_verify(readbuf, TOKEN_LENGTH);
				 if (str_verify.compare(std::string(TOKEN_STR)) != 0)
				 {
					 syslog(LOG_INFO,"CWorkerThread::ClientTcpReadCb DDOS. str=%s",str_verify.c_str());
					 CloseConn(conn, bev);
					 return;
				 }
			 }
#endif

		}

		std::string str_recv(readbuf, bytes);
		std::vector<std::string> vec_command;
		syslog(LOG_INFO,"read :%s",str_recv.c_str());
#if 1
		if (utils::FindCRLF(str_recv) && utils::FindSPACE(str_recv))
		{
			/* 有可能同时收到多条信息 */
			std::vector<std::string> vec_str;
			utils::SplitData(str_recv, CRLF, vec_str);
			for (unsigned int i = 0; i < vec_str.size(); ++i)
			{
				vec_str[i].erase(0,TOKEN_LENGTH);
				utils::SplitData(vec_str[i], "   ", vec_command);
			
		

				//提取command和key
		 		/*
		  			* command   name   passwd
		  		*/
				if(vec_command.size() == 6 || vec_command.size() == 3)
				{
					string id;
					std::string name;
					std::string passwd;
					std::string device_id;
					std::string p2p_id;
					std::string ip;
					struct sockaddr_in addr_in;
					socklen_t addrlen = sizeof(sockaddr_in);

					if(vec_command.size() == 6)
					{
		 				command = (enum COMMAND)atoi(vec_command[0].c_str());
		 				name    = vec_command[1];
						passwd  = vec_command[2];
						device_id = vec_command[3];
						p2p_id = vec_command[4];
					        getpeername(conn->sfd, (struct sockaddr*)&addr_in, &addrlen);
						ip = inet_ntoa(addr_in.sin_addr);
					}
					else if(vec_command.size() == 3)
					{
						command = (enum COMMAND)atoi(vec_command[0].c_str());
						name    = vec_command[1];
						passwd  = vec_command[2];
					}

		 			switch(command)
		 			{
			 			case INSERT:
								{
				 					memset(sql,0,BUFSIZE);
									boost::mutex::scoped_lock Lock(mutex_);
									string str;
									if(name != "0000" && device_id != "0000")
									{
									   sprintf(sql,"insert delayed into device_info(name,device_id,p2p_id,ip_address) values('%s','%s','%s','%s')",name.c_str(),device_id.c_str(),p2p_id.c_str(),ip.c_str());
									}
									else 
									{
									 	sprintf(sql,"SELECT * FROM user_info where name = '%s'",name.c_str());
									 	strval = printerToMySQL->SelectData(sql,3);
									 	if(strval.size() == 0) //user is in the table?
										{
									 		sprintf(sql,"insert delayed into user_info(name,passwd) values('%s','%s')",name.c_str(),passwd.c_str());
										}
										else
										{
											syslog(LOG_INFO,"用户名存在，插入失败\n");
											str="e1:fail";
											for(int i = 0; i < str.length(); i++)
											{
											
												str[i] ^= key;
											}
											SocketOperate::WriteSfd(conn->sfd, str.c_str(),str.length());
											break;
										}
									}
									if(printerToMySQL->InsertData(sql) == 0)
									{	
										syslog(LOG_INFO,"插入成功\n");
										str="f0:ok";
									}
									else
									{
										syslog(LOG_INFO,"插入失败\n");
										str="e2:fail";
									}	
									strval = "ok";
#if 1
									for(int i = 0; i < str.length(); i++)
									{
										str[i] ^=key;
									}
									SocketOperate::WriteSfd(conn->sfd, str.c_str(),str.length());
#endif
									//SSL_write(ssl, str.c_str(), str.length());
									break;
								}
					
						case SELECT:
								{
                                        				memset(sql,0,BUFSIZE);
									boost::mutex::scoped_lock Lock(mutex_);
									sprintf(sql,"SELECT * FROM user_info where name='%s'",name.c_str());
									syslog(LOG_INFO,"%s\n",name.c_str());
									strval = printerToMySQL->SelectData(sql,3);
									string str;
									string new_passwd;
									if(strval.size() != 0)
									{
										str = "f0:";
									  	const	char * p = strval.c_str();
									  	syslog(LOG_INFO,"%s\n",strval.c_str());
										p += strlen(p) - 3;
                                                                                while(*p != 0x06)
										{
											p--;
										}
										//str += (p + 1);
										//str.erase(str.end() - 1);
										//str.erase(str.end() - 1);
										new_passwd = (p + 1);
										new_passwd.erase(new_passwd.end() - 1);
										new_passwd.erase(new_passwd.end() - 1);

										syslog(LOG_INFO,"old_passwd = %s,new_passwd = %s\n",passwd.c_str(),new_passwd.c_str());
										if(new_passwd.compare(passwd.c_str()))
										{
											str = "e3:fail";
										}
										else
										{
											str = "f0:ok";
										}
									        	
									}
									else
									{
									
										str = "e1:fail";
									}
#if 1
                                                                        for(int i = 0; i < str.length(); i++)
                                                                        {
																													                                                                                str[i] ^=key;
									} 
         																																						SocketOperate::WriteSfd(conn->sfd, str.c_str(),str.length());
#endif
									break;
								}

						case UPDATE:
								{
									memset(sql,0,BUFSIZE);
									boost::mutex::scoped_lock Lock(mutex_);
									sprintf(sql,"update user_info set passwd='%s' where name = '%s'",passwd.c_str(),name.c_str());
									string str;
									if(printerToMySQL->UpdateData(sql) == 0)
									{	
										syslog(LOG_INFO,"更新成功\n");
										str="f0:ok";
									}
									else
									{
										syslog(LOG_INFO,"更新失败\n");
										str="e3:fail";
									}

#if 1
                                                                        for(int i = 0; i < str.length(); i++)
                                                                        {
                                                                                str[i] ^=key;
                                                                        }
                                                                        SocketOperate::WriteSfd(conn->sfd, str.c_str(),str.length());
#endif
									//SSL_write(ssl, str.c_str(), str.length());

									strval = "ok";
									break;
								}

						case DELETE:
								{
									memset(sql,0,BUFSIZE);
									boost::mutex::scoped_lock Lock(mutex_);
									sprintf(sql,"delete from user_info where device_id = '%s'",id.c_str());
                                                                        string str;

									if(printerToMySQL->DeleteData(sql) == 0)
									{
										str="1:success";
										syslog(LOG_INFO,"删除成功\n");
									}
									else
									{
										str="1:fail";
										syslog(LOG_INFO,"删除失败\n");
									}
									strval = "ok";
#if 0
                                                                        for(int i = 0; i < str.length(); i++)
                                                                        {
                                                                                str[i] ^=key;
                                                                        }
                                                                        SocketOperate::WriteSfd(conn->sfd, str.c_str(),str.length());
#endif
									//SSL_write(ssl, str.c_str(), str.length());

									break;
								}
						default:
								{
								syslog(LOG_INFO,"指令无法识别\n");
                                        			syslog(LOG_INFO,"command:%d name:%s passwd:%s",command,name.c_str(),passwd.c_str());
								string str="e3:fail";
								for(int i = 0; i < str.length(); i++)
								{
									str[i] ^=key;
								}
								SocketOperate::WriteSfd(conn->sfd, str.c_str(),str.length());
								}
					}//switch	
					vec_command.clear();

				}//if vec_command.size
				else
				{
					syslog(LOG_INFO,"内容格式不正确\n");
					string str="e3:fail";
				        for(int i = 0; i < str.length(); i++)
				        {
					       str[i] ^= key;
				        }
					SocketOperate::WriteSfd(conn->sfd, str.c_str(),str.length());
					vec_command.clear();
				}

			}//for

		}//if
#endif
		 //向请求字符串尾添加NULL字符构成完整的请求结果
		 //reqBuf[bytes]=0;

		 //将格式化结果发送给客户端

		/* if(!SocketOperate::WriteSfd(conn->sfd, strval.c_str(),strval.length()))
		 {
		        syslog(LOG_INFO, "CWorkerThread::ClientTcpReadCb:send sfd .error = %s" , strerror(errno));
		 }*/
	}while(0);
	return ;
}

CWorkerThread::CWorkerThread()
{
	last_thread_ = -1;
}

CWorkerThread::~CWorkerThread()
{
}

/* 初始化worker线程池 */
bool CWorkerThread::InitThreads(struct event_base* main_base)
{

	InitFreeConns();

	syslog(LOG_INFO, "Initializes worker threads...");

	for(unsigned int i=0; i<utils::G<CGlobalSettings>().thread_num_; ++i)
	{
		LIBEVENT_THREAD* libevent_thread_ptr = new LIBEVENT_THREAD;
		/* 建立每个worker线程和主监听线程通信的管道 */
		int fds[2];
		if (pipe(fds) != 0)
		{
			syslog(LOG_INFO, "CThread::InitThreads:Can't create notify pipe");
			return false;
		}
		libevent_thread_ptr->notify_receive_fd = fds[0];
		libevent_thread_ptr->notify_send_fd	   = fds[1];

		if(!SetupThread(libevent_thread_ptr))
		{
			utils::SafeDelete(libevent_thread_ptr);
			syslog(LOG_INFO, "CThread::InitThreads:SetupThread failed.");
			return false;
		}

		vec_libevent_thread_.push_back(libevent_thread_ptr);
	}

	for (unsigned int i = 0; i < utils::G<CGlobalSettings>().thread_num_; i++)
	{
		CreateWorker(WorkerLibevent, vec_libevent_thread_.at(i));
	}

	 /* 等待所有线程都已经启动完毕. */
	WaitForThreadRegistration(utils::G<CGlobalSettings>().thread_num_);

	syslog(LOG_INFO, "Create threads success. we hava done all the libevent setup.");

	return true;
}

void CWorkerThread::CreateWorker(void *(*func)(void *), void *arg)
{
	pthread_t thread;
	pthread_attr_t attr;
	int ret;

	pthread_attr_init(&attr);

	if ((ret = pthread_create(&thread, &attr, func, arg)) != 0)
	{
		syslog(LOG_INFO, "CWorkerThread::CreateWorker:Can't create thread:%s" , strerror(ret));
		exit(1);
	}
}


void *CWorkerThread::WorkerLibevent(void *arg)
{
	LIBEVENT_THREAD *me = static_cast<LIBEVENT_THREAD *>(arg);

	me->thread_id = pthread_self();

	RegisterThreadInitialized();

	event_base_dispatch(me->base);

	return NULL;
}

bool CWorkerThread::SetupThread(LIBEVENT_THREAD* me)
{
	me->base = event_base_new();
	assert(me != NULL);

	/* 通过每个worker线程的读管道监听来自master的通知 */
	me->notify_event = *event_new(me->base, me->notify_receive_fd, EV_READ|EV_PERSIST, ReadPipeCb, (void*)me);
	assert(&me->notify_event != NULL);

	if (event_add(&me->notify_event, NULL) == -1)
	{
		int error_code = EVUTIL_SOCKET_ERROR();
		syslog(LOG_INFO, "CWorkerThread::SetupThread:event_add errorCode = %d,  description = %s" ,error_code
								                                                        ,evutil_socket_error_to_string(error_code));
		return false;
	}

	return true;
}

void CWorkerThread::ReadPipeCb(int fd, short event, void* arg)
{

	LIBEVENT_THREAD *libevent_thread_ptr = static_cast<LIBEVENT_THREAD*>(arg);
	assert(libevent_thread_ptr != NULL);

	/* read from master-thread had write, a byte 代表一个客户端连接 */
	char buf[1];
	if (read(fd, buf, 1) != 1)
	{
		syslog(LOG_INFO, "CWorkerThread::ThreadLibeventProcess:Can't read from libevent pipe.");
		return;
	}

	/* 将主线程塞到队列中的连接pop出来 */
	CONN_INFO connInfo;
	if(!libevent_thread_ptr->list_conn.pop_front(connInfo))
	{
		syslog(LOG_INFO, "CWorkerThread::ThreadLibeventProcess:list_conn.pop_front NULL.");
		return;
	}

	/*初始化新连接，将连接事件注册入libevent */
	if(connInfo.sfd != 0)
	{
		CONN* conn = InitNewConn(connInfo, libevent_thread_ptr);
		if(NULL == conn)
		{
			syslog(LOG_INFO, "CWorkerThread::ReadPipeCb:Can't listen for events on sfd = %d"  ,connInfo.sfd);
			close(connInfo.sfd);
		}
		syslog(LOG_INFO, "CWorkerThread::ReadPipeCb thread id = %lu" ,conn->thread->thread_id);
	}
}

CONN* CWorkerThread::InitNewConn(const CONN_INFO& conn_info, LIBEVENT_THREAD* libevent_thread_ptr)
{
	CONN* conn = GetConnFromFreelist();
	if (NULL == conn)
	{
		conn = new CONN;
		if (NULL == conn)
		{
			syslog(LOG_INFO, "CWorkerThread::InitNewConn:new conn error.");
			return NULL;
		}

		try
		{
			conn->rBuf = new char[DATA_BUFFER_SIZE];
			conn->wBuf = new char[DATA_BUFFER_SIZE];
		} catch (std::bad_alloc &)
		{
			FreeConn(conn);
			syslog(LOG_INFO, "CWorkerThread::InitNewConn:new buf error.");
			return NULL;
		}
	}

	conn->sfd = conn_info.sfd;
	conn->rlen = 0;
	conn->wlen = 0;
	conn->thread = libevent_thread_ptr;
	//ssl_client_init(&(conn->client),conn_info.sfd,SSLMODE_SERVER);

	/* 将新连接加入此线程libevent事件循环 */
	int flag = EV_READ | EV_PERSIST;
	struct bufferevent *client_tcp_event = bufferevent_socket_new(libevent_thread_ptr->base, conn->sfd, BEV_OPT_CLOSE_ON_FREE);
	if (NULL == client_tcp_event)
	{
		if(!AddConnToFreelist(conn))
		{
			FreeConn(conn);
		}
		int error_code = EVUTIL_SOCKET_ERROR();
		syslog(LOG_INFO,
				"CWorkerThread::conn_new:bufferevent_socket_new errorCode = %d, escription = %s" , error_code ,evutil_socket_error_to_string(error_code));

		return NULL;
	}
	bufferevent_setcb(client_tcp_event, start_routine, NULL, ClientTcpErrorCb, (void*) conn);

	/* 利用客户端心跳超时机制处理半开连接 */
	struct timeval heartbeat_sec;
	heartbeat_sec.tv_sec = utils::G<CGlobalSettings>().client_heartbeat_timeout_;
	heartbeat_sec.tv_usec= 0;
	bufferevent_set_timeouts(client_tcp_event, &heartbeat_sec, NULL);

	bufferevent_enable(client_tcp_event, flag);

	return conn;
}


void CWorkerThread::ClientTcpReadCb(struct bufferevent *bev, void *arg)
{
	CONN* conn = static_cast<CONN*>(arg);
	assert(conn != NULL);

	int recv_size = 0;
	if ((recv_size = bufferevent_read(bev, conn->rBuf + conn->rlen, DATA_BUFFER_SIZE - conn->rlen)) > 0)
	{
		conn->rlen = conn->rlen + recv_size;
		//防止恶意连接，进行token校验，不满足校验条件的为恶意连接，直接关闭
		if (conn->rlen >= TOKEN_LENGTH && conn->isVerify == false)
		{
			conn->isVerify = true;
			std::string str_verify(conn->rBuf, TOKEN_LENGTH);
			if (str_verify.compare(std::string(TOKEN_STR)) != 0)
			{
				syslog(LOG_INFO, "CWorkerThread::ClientTcpReadCb DDOS. str = %s" , str_verify.c_str());
				CloseConn(conn, bev);
				return;
			} else
			{
				conn->rlen = conn->rlen - TOKEN_LENGTH;
				memmove(conn->rBuf, conn->rBuf + TOKEN_LENGTH, conn->rlen);
			}
		}
	}

	std::string str_recv(conn->rBuf, conn->rlen);
	if (utils::FindCRLF(str_recv))
	{
		/* 有可能同时收到多条信息 */
		std::vector<std::string> vec_str;
		utils::SplitData(str_recv, CRLF, vec_str);

		for (unsigned int i = 0; i < vec_str.size(); ++i)
		{
			if(!SocketOperate::WriteSfd(conn->sfd, vec_str.at(i).c_str(), vec_str.at(i).length()))
			{
				syslog(LOG_INFO, "CWorkerThread::ClientTcpReadCb:send sfd .error = %s" , strerror(errno));
			}
		}

		int len = str_recv.find_last_of(CRLF) + 1;
		memmove(conn->rBuf, conn->rBuf + len, DATA_BUFFER_SIZE - len);
		conn->rlen = conn->rlen - len;
	}
}

void CWorkerThread::ClientTcpErrorCb(struct bufferevent *bev, short event, void *arg)
{
	CONN* conn = static_cast<CONN*>(arg);

	if (event & BEV_EVENT_TIMEOUT)
	{
		syslog(LOG_INFO, "CWorkerThread::ClientTcpErrorCb:TimeOut.");
	}
	else if (event & BEV_EVENT_EOF)
	{
	}
	else if (event & BEV_EVENT_ERROR)
	{
		int error_code = EVUTIL_SOCKET_ERROR();
		syslog(LOG_INFO,
				"CWorkerThread::ClientTcpErrorCb:some other errorCode = %d,  description = %s" ,error_code ,evutil_socket_error_to_string(error_code));
	}

	CloseConn(conn, bev);
}

void CWorkerThread::DispatchSfdToWorker(int sfd)
{
	/* Round Robin*/
	int tid = (last_thread_ + 1) % utils::G<CGlobalSettings>().thread_num_;
	static int count = 0;
	LIBEVENT_THREAD *libevent_thread_ptr = vec_libevent_thread_.at(tid);
	last_thread_ = tid;

	/* 将新连接的加入此worker线程连接队列 */
	CONN_INFO connInfo;
	connInfo.sfd = sfd;
	libevent_thread_ptr->list_conn.push_back(connInfo);

	/* 通知此worker线程有新连接到来，可以读取了 */
	char buf[1];
	buf[0] = 'c';
	if (write(libevent_thread_ptr->notify_send_fd, buf, 1) != 1)
	{
		syslog(LOG_INFO, "CWorkerThread::DispatchSfdToWorker:Writing to thread notify pipe");
	}
	syslog(LOG_INFO, "CWorkerThread::DispatchSfdToWorker:Writing to thread notify pipe.sfd:%d",count++);
}

void CWorkerThread::RegisterThreadInitialized(void)
{
    pthread_mutex_lock(&init_lock_);
    init_count_++;
    if(init_count_ == int(utils::G<CGlobalSettings>().thread_num_))
    {
    	pthread_cond_signal(&init_cond_);
    }
    pthread_mutex_unlock(&init_lock_);
}

void CWorkerThread::WaitForThreadRegistration(int nthreads)
{
	pthread_mutex_lock(&init_lock_);
    pthread_cond_wait(&init_cond_, &init_lock_);
    pthread_mutex_unlock(&init_lock_);
}

void CWorkerThread::InitFreeConns()
{
	freetotal_ 	= 200;
	freecurr_	= 0;

	vec_freeconn_.resize(freetotal_);
}

CONN* CWorkerThread::GetConnFromFreelist()
{
	CONN *conn = NULL;

	boost::mutex::scoped_lock Lock(mutex_);
	if(freecurr_ > 0)
	{
		conn = vec_freeconn_.at(--freecurr_);
	}

	return conn;
}

bool CWorkerThread::AddConnToFreelist(CONN* conn)
{
	bool ret = false;
	boost::mutex::scoped_lock Lock(mutex_);
	if (freecurr_ < freetotal_)
	{
		vec_freeconn_.at(freecurr_++) = conn;
		ret = true;
	}
	else
	{
		/* 增大连接内存池队列 */
		size_t newsize = freetotal_ * 2;
		vec_freeconn_.resize(newsize);
		freetotal_ = newsize;
		vec_freeconn_.at(freecurr_++) = conn;
		ret = true;
	}

	return ret;
}

void CWorkerThread::FreeConn(CONN* conn)
{
	if (conn)
	{
		utils::SafeDeleteArray(conn->rBuf);
		utils::SafeDeleteArray(conn->wBuf);
		utils::SafeDelete (conn);
	}
}

void CWorkerThread::CloseConn(CONN* conn, struct bufferevent* bev)
{
	assert(conn != NULL);

	//ssl_client_cleanup(&(conn->client));
	/* 清理资源：the event, the socket and the conn */
	bufferevent_free(bev);

	syslog(LOG_INFO, "CWorkerThread::conn_close sfd = %u" ,conn->sfd);

	/* if the connection has big buffers, just free it */
	if (!AddConnToFreelist (conn))
	{
		FreeConn(conn);
	}

	return;
}

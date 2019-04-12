#include <signal.h>
#include <syslog.h>
#include "defines.h"
#include "config_file.h"
#include "global_settings.h"
#include "master_thread.h"
#include "init_configure.h"
#include "utils.h"
#include "printertomysql.h"
#include "ssl.h"

static void InitConfigure();
static void SettingsAndPrint();
static void Run();
static void SigUsr(int signo);
static void sig_term(int signo);

PrinterToMySQL * printerToMySQL;

int main(void)
{
	char host[]="127.0.0.1";
        char user[]="momo";
	char port[] ="3306";
	char passwd[]="momo12";
	char dbname[]="3d";
	char charset[] = "GBK";//支持中文

	system("ulimit -n 20000");
	openlog("3d_server", LOG_PID, LOG_DAEMON);
	syslog(LOG_INFO, "program started.");

	printerToMySQL = new PrinterToMySQL;
	printerToMySQL->InitMySQL();

	if(printerToMySQL->ConnMySQL(host,port,dbname,user,passwd,charset) == 0)
	{
		syslog(LOG_INFO,"连接成功\n");
	}
	else
	{
		syslog(LOG_INFO,"连接失败");
	}

	InitConfigure();

	SettingsAndPrint();
	//ssl_init("server.crt", "server.key");

#if 1 
        int val = daemon(0,0);
        if(val != 0)
                return -1;

#endif
	if (signal(SIGUSR1, SigUsr) == SIG_ERR )
	{
		syslog(LOG_INFO, "Configure signal failed.");
		exit(EXIT_FAILURE);
	}

	signal(SIGTERM, sig_term); /* arrange to catch the signal */
	//ssl_init("server.crt", "server.key");

	Run();

	return EXIT_SUCCESS;
}

void Run()
{
	CMasterThread masterThread;
	if(!masterThread.InitMasterThread())
	{
		syslog(LOG_INFO, "InitNetCore failed.");
		exit(EXIT_FAILURE);
	}

	masterThread.Run();
}

void sig_term(int signo)
{
        if(signo == SIGTERM)
        /* catched signal sent by kill(1) command */
        {
               syslog(LOG_INFO, "program terminated.");
	       closelog();
	       if(printerToMySQL != NULL)
	       {
	       		delete printerToMySQL;
			printerToMySQL = NULL;
	       }
	       exit(0);
        }
}

void SigUsr(int signo)
{
	if(signo == SIGUSR1)
	{
		/* 重新加载应用配置文件（仅仅是连接超时时间），log4cxx日志配置文件*/
		InitConfigure();
		SettingsAndPrint();
		syslog(LOG_INFO, "reload configure.");
		return;
	}
}

void InitConfigure()
{
	CInitConfig initConfObj;
	std::string current_path;
	if(!utils::GetCurrentPath(current_path))
	{
		syslog(LOG_INFO, "GetCurrentPath failed.");
		exit(EXIT_FAILURE);
	}
	initConfObj.SetConfigFilePath(std::string(current_path));
	initConfObj.InitLog4cxx("RRMode");
	if (!initConfObj.LoadConfiguration())
	{
		syslog(LOG_INFO, "LoadConfiguration failed.");
		exit(EXIT_FAILURE);
	}
}

void SettingsAndPrint()
{
	utils::G<CGlobalSettings>().remote_listen_port_ = utils::G<ConfigFile>().read<int> ("remote.listen.port", 12006);
	utils::G<CGlobalSettings>().thread_num_ = utils::G<ConfigFile>().read<int> ("worker.thread.num", 4);
	utils::G<CGlobalSettings>().client_heartbeat_timeout_ = utils::G<ConfigFile>().read<int>("client.heartbeat.timeout.s", 70);

	syslog(LOG_INFO, "******remote.listen.port=%d ******", utils::G<CGlobalSettings>().remote_listen_port_);
	syslog(LOG_INFO, "******worker.thread.num =%u ******", utils::G<CGlobalSettings>().thread_num_);
	syslog(LOG_INFO, "******client.heartbeat.timeout.s =%d ******", utils::G<CGlobalSettings>().client_heartbeat_timeout_);
}

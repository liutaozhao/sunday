/*
 * Init_log4cxx.cc
 *
 *  Created on: 2012-9-12
 *      Author: yaowei
 */

#include "init_configure.h"
#include "config_file.h"
#include "utils.h"
#include<syslog.h>

#define LOG_CON_FILE        "/conf/log.conf"
#define CONF_FILE        	"/conf/config.ini"

//LoggerPtr g_logger;


CInitConfig::CInitConfig()
{
}

CInitConfig::~CInitConfig()
{
}

void CInitConfig::InitLog4cxx(std::string project_name)
{
	PropertyConfigurator::configure(config_file_path_ + LOG_CON_FILE);
	//g_logger = Logger::getLogger(project_name);
        syslog(LOG_INFO, "Run...");
}

bool CInitConfig::LoadConfiguration()
{
	std::locale old_locale = std::locale::global(std::locale(""));
	std::string file_path = config_file_path_ + CONF_FILE;
	std::ifstream conf_file(file_path.c_str());
	std::locale::global(old_locale);
	if (!conf_file)
	{
		syslog(LOG_INFO, "CInitConfig::LoadConfiguration failed.");
		return false;
	}
	conf_file >> utils::G<ConfigFile>();
	return true;
}



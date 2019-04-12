#include "printertomysql.h"
#include "stdio.h"
#include<cstring>
#include<syslog.h>
extern int g_iUseSysLog;

PrinterToMySQL::PrinterToMySQL()
{

}

PrinterToMySQL::~PrinterToMySQL()
{
	CloseMySQLConn();
}
//初始化mysql
int PrinterToMySQL::InitMySQL()
{
       if( mysql_init(&mysql) == NULL )
       {
            if(g_iUseSysLog == 1)
	    {
	    	      syslog(LOG_INFO,"Error:%s,%s,%d",mysql_error(&mysql),__FUNCTION__,__LINE__);
	    }
	    else
	    {
            	      fprintf(stderr,"Error:%s,%s,%d",mysql_error(&mysql),__FUNCTION__,__LINE__);
	    }
            return 1;
       } 
       return 0;
}

//初始化数据
int PrinterToMySQL::ConnMySQL(char *host,char * port ,char * Db,char * user,char* passwd,char * charset)
{  
       mysql_options(&mysql,MYSQL_OPT_RECONNECT,"gbk"); 
       if (mysql_real_connect(&mysql,host,user,passwd,Db,0,NULL,0) == NULL)
       {
	       if(g_iUseSysLog == 1)
	       {
		       syslog(LOG_INFO,"Error:%s,%s,%d",mysql_error(&mysql),__FUNCTION__,__LINE__);
	       }
	       else
	       {
              	       fprintf(stderr,"Error:%s,%s,%d",mysql_error(&mysql),__FUNCTION__,__LINE__);
	       }
              return 1;
       }    
       if(mysql_set_character_set(&mysql,"GBK") != 0)
       {
	       if(g_iUseSysLog == 1)
	       {
		       syslog(LOG_INFO,"Error:%s,%s,%d",mysql_error(&mysql),__FUNCTION__,__LINE__);
	       }
	       else
	       {
              	       fprintf(stderr,"Error:%s,%s,%d",mysql_error(&mysql),__FUNCTION__,__LINE__);
	       }
              return 1;
       }
       return 0;
}

//查询数据
string PrinterToMySQL::SelectData(char * SQL,int Cnum)
{
    MYSQL_ROW m_row;
    MYSQL_RES *m_res;
    char sql[2048];
    sprintf(sql,"%s",SQL);
    int rnum = 0;
    char rg = 0x06;//行隔开
    char cg = {0x05};//字段隔开
    if(mysql_query(&mysql,sql) != 0)
    {
	    if(g_iUseSysLog == 1)
	    {
		    syslog(LOG_INFO,"Error:%s,%s,%d",mysql_error(&mysql),__FUNCTION__,__LINE__);
	    }
	    else
	    {
	            fprintf(stderr,"Error:%s,%s,%d",mysql_error(&mysql),__FUNCTION__,__LINE__);
	    }
            return "";
    }
    m_res = mysql_store_result(&mysql);
    if(m_res == NULL)
    {
	    if(g_iUseSysLog == 1)
	    {	
		    syslog(LOG_INFO,"Error:%s,%s,%d",mysql_error(&mysql),__FUNCTION__,__LINE__);
	    }
	    else
	    {
		    fprintf(stderr,"Error:%s,%s,%d",mysql_error(&mysql),__FUNCTION__,__LINE__);
	    }
            return "";
    }
    string str("");
    m_row = mysql_fetch_row(m_res);
    while(m_row != NULL)
    {
        for(int i = 0;i < Cnum;i++)
        {
            str += m_row[i];
            str += rg;
        }
        str += cg;             
        rnum++;
	m_row = mysql_fetch_row(m_res);
    }
    mysql_free_result(m_res);
    return str;
}

//插入数据
int PrinterToMySQL::InsertData(char * SQL)
{
    char sql[2048];
    sprintf(sql,"%s",SQL);
    if(mysql_query(&mysql,SQL) != 0)
    {
	    if(g_iUseSysLog == 1)
	    {
		    syslog(LOG_INFO,"Error:%s,%s,%d",mysql_error(&mysql),__FUNCTION__,__LINE__);
	    }
	    else
	    {
        	    fprintf(stderr,"Error:%s,%s,%d",mysql_error(&mysql),__FUNCTION__,__LINE__);
	    }
            return 1;
    }
    return 0;
}

 

//更新数据
int PrinterToMySQL::UpdateData(char * SQL)
{
    char sql[2048];
    sprintf(sql,"%s",SQL);
    if(mysql_query(&mysql,sql) != 0)
    {
	    if(g_iUseSysLog == 1)
	    {
		    syslog(LOG_INFO,"Error:%s,%s,%d",mysql_error(&mysql),__FUNCTION__,__LINE__);
	    }
	    else
	    {
        	    fprintf(stderr,"Error:%s,%s,%d",mysql_error(&mysql),__FUNCTION__,__LINE__);
	    }
            return 1;
    }
    return 0;
}

//删除数据
int PrinterToMySQL::DeleteData(char * SQL)
{
    char sql[2048];
    sprintf(sql,"%s",SQL);
    if(mysql_query(&mysql,sql) != 0)
    {
	    if(g_iUseSysLog == 1)
	    {
		    syslog(LOG_INFO,"Error:%s,%s,%d",mysql_error(&mysql),__FUNCTION__,__LINE__);
	    }
	    else
	    {
                    fprintf(stderr,"Error:%s,%s,%d",mysql_error(&mysql),__FUNCTION__,__LINE__);
	    }
            return 1;
    }
    return 0;
}

 
//关闭数据库连接
void PrinterToMySQL::CloseMySQLConn()
{
    mysql_close(&mysql);
}

#if 0
int main(int argc, char* argv[])
{
    char host[]="192.168.229.130";
    char user[]="momo";
    char port[] ="3306";
    char passwd[]="momo12";
    char dbname[]="3d"; 
    char charset[] = "GBK";//支持中文
    char SQL[1000];
	
    //初始化
    PrinterToMySQL * printerToMySQL = new PrinterToMySQL;
	printerToMySQL->InitMySQL();
    if(printerToMySQL->ConnMySQL(host,port,dbname,user,passwd,charset) == 0)
           fprintf(stderr,"连接成功/r/n");
    else
           fprintf(stderr,"连接失败");
	   
	//插入
	//memset(SQL,0,1000);
	//memcpy(SQL,"insert into 3d_user(id,name,passwd) values(3,'我的','123210')",strlen("insert into 3d_user(id,name,passwd) values(3,'我的','123210')"));
	char sql0[]="insert into 3d_user(name,passwd) values('my','123210')";
    if(printerToMySQL->InsertData(sql0) == 0)
           fprintf(stderr,"插入成功/r/n");
	else
		fprintf(stderr,"插入失败");
   
    //查询
	//memset(SQL,0,1000);
	//memcpy(SQL,"SELECT id,name,passwd FROM 3d_user",strlen("SELECT id,name,passwd FROM 3d_user"));
	char sql1[] = "SELECT id,name,passwd FROM 3d_user";
    string str = printerToMySQL->SelectData(sql1,3);
    if( str.length() > 0 )
    {
           fprintf(stderr,"查询成功/n");
           fprintf(stderr,"/n");
    }
    else
    {
           fprintf(stderr,"查询失败");
    }

    //更新
	//memset(SQL,0,1000);
	//memcpy(SQL,"update 3d_user set name = '修改了',passwd='2345' where id = 3 ",strlen("update 3d_user set name = '修改了',passwd='2345' where id = 3 "));
	char sql2[] = "update 3d_user set name = 'fixed',passwd='2345' where id = '3'";
    if(printerToMySQL->UpdateData(sql2) == 0)
           fprintf(stderr,"更新成功/r/n");

	
    //删除
	memset(SQL,0,1000);
	memcpy(SQL,"delete from 3d_user where id = '3' ",strlen("delete from 3d_user where id = '3' "));

    if(printerToMySQL->DeleteData(SQL) == 0)
           fprintf(stderr,"删除成功/r/n");

    printerToMySQL->CloseMySQLConn();
	
    return 0;

}
#endif

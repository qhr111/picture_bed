#ifndef _API_COMMON_H_
#define _API_COMMON_H_
#include <string>
#include "redis_keys.h"
#include "Common.h"
#include "json/json.h"
#include "Logging.h"
#include "DBPool.h"
#include "CachePool.h"
#include "redis_keys.h"

extern string s_dfs_path_client;
extern string s_web_server_ip;
extern string s_web_server_port;
extern string s_storage_web_server_ip;
extern string s_storage_web_server_port;
using std::string;
;
int ApiInit();
//获取用户文件个数
int CacheSetCount(CacheConn *pCacheConn, string key, int64_t count);
int CacheGetCount(CacheConn *pCacheConn, string key, int64_t &count);
int CacheIncrCount(CacheConn *pCacheConn, string key);
int CacheDecrCount(CacheConn *pCacheConn, string key);
int DBGetUserFilesCountByUsername(CDBConn *pDBConn, string user_name, int &count);
int DBGetShareFilesCount(CDBConn *pDBConn, int &count);
int DBGetSharePictureCountByUsername(CDBConn *pDBConn, string user_name, int &count);
int RemoveFileFromFastDfs(const char *fileid);
#endif
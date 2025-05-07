#include "ApiCommon.h"



string s_dfs_path_client;
string s_web_server_ip;
string s_web_server_port;
string s_storage_web_server_ip;
string s_storage_web_server_port;

// 比如加载mysql相关的记录到redis
// https://blog.csdn.net/lxw1844912514/article/details/121528746
/*对于 Redis 在内存中的数据，需要定期的同步到磁盘中，但对于 Redis 异常重启，就没有办法了。比如在 Redis 中插入后，Redis 重启
，数据没有持久化到硬盘。这时可以在重启 Redis 后，从数据库执行下 count(*) 操作,然后更新到 Redis 中。一次全表扫描还是可行的。*/
template <typename... Args>
static std::string formatString(const std::string &format, Args... args)
{
    auto size = std::snprintf(nullptr, 0, format.c_str(), args...) + 1; // Extra space for '\0'
    std::unique_ptr<char[]> buf(new char[size]);
    std::snprintf(buf.get(), size, format.c_str(), args...);
    return std::string(buf.get(), buf.get() + size - 1); // We don't want the '\0' inside
}

int ApiInit()
{
    CDBManager *pDBManager = CDBManager::getInstance();
    CDBConn *pDBConn = pDBManager->GetDBConn("tuchuang_slave");
    AUTO_REL_DBCONN(pDBManager, pDBConn);
    CacheManager *pCacheManager = CacheManager::getInstance();
    CacheConn *pCacheConn = pCacheManager->GetCacheConn("token");
    AUTO_REL_CACHECONN(pCacheManager, pCacheConn);


    int ret = 0;
    // 共享文件的数量 统计加载到redis
    int count = 0;
    ret = DBGetShareFilesCount(pDBConn, count);
    if(ret < 0) 
    {
        LOG_ERROR << "GetShareFilesCount failed";
        return -1;
    }
    // 加载到redis
    ret = CacheSetCount(pCacheConn, FILE_PUBLIC_COUNT,(int64_t) count);
    if(ret < 0)
    {
        LOG_ERROR << "CacheSetCount failed";
        return -1;
    }

    // 对于个人的文件数量 和图片分享数量，在登录的时候 读取到redis

    return 0;
}


int CacheSetCount(CacheConn *pCacheConn, string key, int64_t count)
{
    string ret = pCacheConn->set(key, std::to_string(count));
    if(!ret.empty())
    {
        return 0;
    }
    else
    {
        return -1;
    }
}

int CacheGetCount(CacheConn *pCacheConn, string key, int64_t &count)
{
    count = 0;
    string str_count = pCacheConn->get(key);
    if(!str_count.empty())
    {
        count = atoll(str_count.c_str());
        return 0;
    }
    else
    {
        return -1;
    }
}

int CacheIncrCount(CacheConn *pCacheConn, string key)
{
    int64_t count = 0;
    int ret = pCacheConn->incr(key, count);
    if(ret < 0) 
    {
        return -1;
    }
    LOG_INFO << key << " - " << count;
    return 0;
}
// 这里最小我们只允许为0
int CacheDecrCount(CacheConn *pCacheConn, string key)
{
    int64_t count = 0;
    int ret = pCacheConn->decr(key, count);
    if(ret < 0) 
    {
        return -1;
    }
    LOG_INFO << key << " - " << count;
    if(count < 0)
    {
        LOG_ERROR << key << " 请检测你的逻辑"<< "decr  count < 0  -> " << count;
        ret = CacheSetCount(pCacheConn, key, 0);
        if(ret < 0) 
        {
            return -1;
        }
    }
    return 0;
}

//获取用户文件个数
int DBGetUserFilesCountByUsername(CDBConn *pDBConn, string user_name, int &count)
{
    count = 0;
    int ret = 0;
    // 先查看用户是否存在
    string str_sql;

    str_sql = formatString("select count(*) from user_file_list where user='%s'", user_name.c_str());
    LOG_INFO << "执行: " << str_sql;
    CResultSet *pResultSet = pDBConn->ExecuteQuery(str_sql.c_str());
    if (pResultSet && pResultSet->Next())
    {
        // 存在在返回
        count = pResultSet->GetInt("count(*)");
        LOG_INFO << "count: " << count;
        ret = 0;
        delete pResultSet;
    }
    else if (!pResultSet)
    { // 操作失败
        LOG_ERROR << str_sql << " 操作失败";
        ret = -1;
    }
    else
    {
        // 没有记录则初始化记录数量为0
        ret = 0;
        LOG_INFO << "没有记录: count: " << count;
        
    }
    return ret;
}

//获取用户共享图片格式
int DBGetSharePictureCountByUsername(CDBConn *pDBConn, string user_name, int &count)
{
    count = 0;
    int ret = 0;
    // 先查看用户是否存在
    string str_sql;

    str_sql = formatString("select count(*) from share_picture_list where user='%s'", user_name.c_str());
    LOG_INFO << "执行: " << str_sql;
    CResultSet *pResultSet = pDBConn->ExecuteQuery(str_sql.c_str());
    if (pResultSet && pResultSet->Next())
    {
        // 存在在返回
        count = pResultSet->GetInt("count(*)");
        LOG_INFO << "count: " << count;
        ret = 0;
        delete pResultSet;
    }
    else if (!pResultSet)
    { // 操作失败
        LOG_ERROR << str_sql << " 操作失败";
        ret = -1;
    }
    else
    {
        // 没有记录则初始化记录数量为0
        ret = 0;
        LOG_INFO << "没有记录: count: " << count;
        
    }
    return ret;
}

//获取共享文件的数量
int DBGetShareFilesCount(CDBConn *pDBConn, int &count)
{
    count = 0;
    int ret = 0;
    
    // 先查看用户是否存在
    string str_sql = "select count(*) from share_file_list";
    CResultSet *pResultSet = pDBConn->ExecuteQuery(str_sql.c_str());
    if (pResultSet && pResultSet->Next())
    {
        // 存在在返回
        count = pResultSet->GetInt("count(*)");
        LOG_INFO << "count: " << count;
        ret = 0;
        delete pResultSet;
    }
    else if (!pResultSet)
    {
        // 操作失败
        LOG_ERROR << str_sql << " 操作失败";
        ret = -1;
    } else {
        ret = 0;
        LOG_INFO << "没有记录: count: " << count;
    }
     
    return ret;
}


//从storage删除指定的文件，参数为文件id
int RemoveFileFromFastDfs(const char *fileid)
{
    int ret = 0;

    char cmd[1024 * 2] = {0};
    sprintf(cmd, "fdfs_delete_file %s %s", s_dfs_path_client.c_str(), fileid);

    ret = system(cmd);
    LOG_INFO << "RemoveFileFromFastDfs ret = " << ret;

    return ret;
}
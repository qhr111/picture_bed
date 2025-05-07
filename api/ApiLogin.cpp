#include "ApiLogin.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <time.h>
#include "ApiCommon.h"

#define LOGIN_RET_OK 0   // 成功
#define LOGIN_RET_FAIL 1 // 失败

// 解析登录信息
int decodeLoginJson(const std::string &str_json, string &user_name, string &pwd)
{
    bool res;
    Json::Value root;
    Json::Reader jsonReader;
    res = jsonReader.parse(str_json, root);
    if (!res)
    {
        LOG_ERROR << "parse reg json failed ";
        return -1;
    }
    // 用户名
    if (root["user"].isNull())
    {
        LOG_ERROR << "user null\n";
        return -1;
    }
    user_name = root["user"].asString();

    //密码
    if (root["pwd"].isNull())
    {
        LOG_ERROR << "pwd null\n";
        return -1;
    }
    pwd = root["pwd"].asString();

    return 0;
}

// 封装登录结果的json
int encodeLoginJson(int ret, string &token, string &str_json)
{
    Json::Value root;
    root["code"] = ret;
    if (ret == 0)
    {
        root["token"] = token; // 正常返回的时候才写入token
    }
    Json::FastWriter writer;
    str_json = writer.write(root);
    return 0;
}

template <typename... Args>
std::string formatString1(const std::string &format, Args... args)
{
    auto size = std::snprintf(nullptr, 0, format.c_str(), args...) + 1; // Extra space for '\0'
    std::unique_ptr<char[]> buf(new char[size]);
    std::snprintf(buf.get(), size, format.c_str(), args...);
    return std::string(buf.get(), buf.get() + size - 1); // We don't want the '\0' inside
}
/* -------------------------------------------*/
/**
 * @brief  判断用户登陆情况
 *
 * @param username 		用户名
 * @param pwd 		密码
 *
 * @returns
 *      成功: 0
 *      失败：-1
 */
/* -------------------------------------------*/
int verifyUserPassword(string &user_name, string &pwd)
{
    int ret = 0;
    CDBManager *pDBManager = CDBManager::getInstance();
    CDBConn *pDBConn = pDBManager->GetDBConn("tuchuang_slave");
    AUTO_REL_DBCONN(pDBManager, pDBConn);

    // 先查看用户是否存在
    string strSql;

    strSql = formatString1("select password from user_info where user_name='%s'", user_name.c_str());
    CResultSet *pResultSet = pDBConn->ExecuteQuery(strSql.c_str());
    uint32_t nowtime = time(NULL);
    if (pResultSet && pResultSet->Next())
    {
        // 存在在返回
        string password = pResultSet->GetString("password");
        LOG_INFO << "mysql-pwd: " << password << ", user-pwd: " << pwd;
        if (pResultSet->GetString("password") == pwd)
            ret = 0;
        else
            ret = -1;
    }
    else
    { // 如果不存在则注册
        ret = -1;
    }

    delete pResultSet;

    return ret;
}

/* -------------------------------------------*/
/**
 * @brief  生成token字符串, 保存redis数据库
 *
 * @param username 		用户名
 * @param token     生成的token字符串
 *
 * @returns
 *      成功: 0
 *      失败：-1
 */
/* -------------------------------------------*/
int setToken(string &user_name, string &token)
{
    int ret = 0;
    CacheManager *pCacheManager = CacheManager::getInstance();
    CacheConn *pCacheConn = pCacheManager->GetCacheConn("token");
    AUTO_REL_CACHECONN(pCacheManager, pCacheConn);

    token = RandomString(32);   // 随机32个字母

    if (pCacheConn)
    {
        //用户名：token, 86400有效时间为24小时
        pCacheConn->setex(user_name, 86400, token); // redis做超时
    }
    else
    {
        ret = -1;
    }

    return ret;
}

int loadMyfilesCountAndSharepictureCount(string &user_name)
{
    int64_t redis_file_count = 0;
    int mysq_file_count = 0;
    CDBManager *pDBManager = CDBManager::getInstance();
    CDBConn *pDBConn = pDBManager->GetDBConn("tuchuang_slave");
    AUTO_REL_DBCONN(pDBManager, pDBConn);
    CacheManager *pCacheManager = CacheManager::getInstance();
    CacheConn *pCacheConn = pCacheManager->GetCacheConn("token");
    AUTO_REL_CACHECONN(pCacheManager, pCacheConn);

    // 从mysql加载
    if(DBGetUserFilesCountByUsername(pDBConn, user_name, mysq_file_count) < 0)
    {
        LOG_ERROR << "DBGetUserFilesCountByUsername failed";
        return -1;
    }
    redis_file_count = (int64_t)mysq_file_count;
    if(CacheSetCount(pCacheConn, FILE_USER_COUNT+user_name, redis_file_count) <0) // 失败了那下次继续从mysql加载
    {
        LOG_ERROR << "DBGetUserFilesCountByUsername failed";
        return -1;
    }
    LOG_INFO << "FILE_USER_COUNT: " << redis_file_count;

    // 从mysql加载
    if(DBGetSharePictureCountByUsername(pDBConn, user_name, mysq_file_count) < 0)
    {
        LOG_ERROR << "DBGetUserFilesCountByUsername failed";
        return -1;
    }    
 
    redis_file_count = (int64_t)mysq_file_count;
    if(CacheSetCount(pCacheConn, SHARE_PIC_COUNT+user_name, redis_file_count) <0) // 失败了那下次继续从mysql加载
    {
        LOG_ERROR << "DBGetUserFilesCountByUsername failed";
        return -1;
    }
    LOG_INFO << "SHARE_PIC_COUNT: " << redis_file_count;
    
    return 0;
}

int ApiUserLogin(string &url, string &post_data, string &str_json)
{
    UNUSED(url);
    string user_name;
    string pwd;
    string token;
    
    
    // 判断数据是否为空
    if (post_data.empty())
    {
        return -1;
    }
    // 解析json
    if (decodeLoginJson(post_data, user_name, pwd) < 0)
    {
        LOG_ERROR << "decodeRegisterJson failed";
        encodeLoginJson(1, token, str_json);
        return -1;
    }

    // 验证账号和密码是否匹配
    if (verifyUserPassword(user_name, pwd) < 0)
    {
        LOG_ERROR << "verifyUserPassword failed";
        encodeLoginJson(1, token, str_json);
        return -1;
    }

    // 生成token
    if (setToken(user_name, token) < 0)
    {
        LOG_ERROR << "setToken failed";
        encodeLoginJson(1, token, str_json);
        return -1;
    }

    // 加载 我的文件数量  我的分享图片数量
    
    if (loadMyfilesCountAndSharepictureCount(user_name) < 0)
    {
        LOG_ERROR << "loadMyfilesCountAndSharepictureCount failed";
        encodeLoginJson(1, token, str_json);
        return -1;
    }
    // 注册账号
    // ret = registerUser(user_name, nick_name, pwd, phone, email);
    encodeLoginJson(0, token, str_json);
    return 0;
}
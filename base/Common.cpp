#include "Common.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <memory>
#include "CachePool.h"
#include "Logging.h"

/**
 * @brief  去掉一个字符串两边的空白字符
 *
 * @param inbuf确保inbuf可修改
 *
 * @returns
 *      0 成功
 *      -1 失败
 */
int TrimSpace(char *inbuf)
{
    int i = 0;
    int j = strlen(inbuf) - 1;

    char *str = inbuf;

    int count = 0;

    if (str == NULL)
    {
        return -1;
    }

    while (isspace(str[i]) && str[i] != '\0')
    {
        i++;
    }

    while (isspace(str[j]) && j > i)
    {
        j--;
    }

    count = j - i + 1;

    strncpy(inbuf, str + i, count);

    inbuf[count] = '\0';

    return 0;
}
 

/**
 * @brief  解析url query 类似 abc=123&bbb=456 字符串
 *          传入一个key,得到相应的value
 * @returns
 *          0 成功, -1 失败
 */
int QueryParseKeyValue(const char *query, const char *key, char *value, int *value_len_p)
{
    char *temp = NULL;
    char *end = NULL;
    int value_len = 0;

    //找到是否有key
    temp = (char *)strstr(query, key);
    if (temp == NULL)
    {
        return -1;
    }

    temp += strlen(key); //=
    temp++;              // value

    // get value
    end = temp;

    while ('\0' != *end && '#' != *end && '&' != *end)
    {
        end++;
    }

    value_len = end - temp;

    strncpy(value, temp, value_len);
    value[value_len] = '\0';

    if (value_len_p != NULL)
    {
        *value_len_p = value_len;
    }

    return 0;
}

//通过文件名file_name， 得到文件后缀字符串, 保存在suffix 如果非法文件后缀,返回"null"
int GetFileSuffix(const char *file_name, char *suffix)
{
    const char *p = file_name;
    int len = 0;
    const char *q = NULL;
    const char *k = NULL;

    if (p == NULL)
    {
        return -1;
    }

    q = p;

    // mike.doc.png
    //              ↑

    while (*q != '\0')
    {
        q++;
    }

    k = q;
    while (*k != '.' && k != p)
    {
        k--;
    }

    if (*k == '.')
    {
        k++;
        len = q - k;

        if (len != 0)
        {
            strncpy(suffix, k, len);
            suffix[len] = '\0';
        }
        else
        {
            strncpy(suffix, "null", 5);
        }
    }
    else
    {
        strncpy(suffix, "null", 5);
    }

    return 0;
}
 
//验证登陆token，成功返回0，失败-1
int VerifyToken(string &user_name, string &token)
{    
    int ret = 0;
    CacheManager *pCacheManager = CacheManager::getInstance();
    // increase message count
    CacheConn *pCacheConn = pCacheManager->GetCacheConn("token");
    AUTO_REL_CACHECONN(pCacheManager, pCacheConn);

    if (pCacheConn)
    {
        string tmp_token = pCacheConn->get(user_name);
        if (tmp_token == token)
        {
            ret = 0;
        }
        else
        {
            ret = -1;
        }
    }
    else
    {
        ret = -1;
    }

    return ret;
}

string RandomString(const int len) /*参数为字符串的长度*/
{
    /*初始化*/
    string str; /*声明用来保存随机字符串的str*/
    char c;     /*声明字符c，用来保存随机生成的字符*/
    int idx;    /*用来循环的变量*/
    /*循环向字符串中添加随机生成的字符*/
    for (idx = 0; idx < len; idx++)
    {
        /*rand()%26是取余，余数为0~25加上'a',就是字母a~z,详见asc码表*/
        c = 'a' + rand() % 26;
        str.push_back(c); /*push_back()是string类尾插函数。这里插入随机字符c*/
    }
    return str; /*返回生成的随机字符串*/
}

template <typename... Args>
std::string formatString(const std::string &format, Args... args)
{
    auto size = std::snprintf(nullptr, 0, format.c_str(), args...) + 1; // Extra space for '\0'
    std::unique_ptr<char[]> buf(new char[size]);
    std::snprintf(buf.get(), size, format.c_str(), args...);
    return std::string(buf.get(), buf.get() + size - 1); // We don't want the '\0' inside
}


//处理数据库查询结果，结果集保存在count，如果要读取count值则设置为0，如果设置为-1则不读取
//返回值： 0成功并保存记录集，1没有记录集，2有记录集但是没有保存，-1失败
int GetResultOneCount(CDBConn *pDBConn, char *sql_cmd, int &count)
{
    int ret = -1;
    CResultSet *pResultSet = pDBConn->ExecuteQuery(sql_cmd);

    if (!pResultSet)
    {
        ret = -1;
    }

    if (count == 0)
    {
        // 读取
        if (pResultSet->Next())
        {
            ret = 0;
            // 存在在返回
            count = pResultSet->GetInt("count");
            LOG_DEBUG << "count: " << count;
        }
        else
        {
            ret = 1; // 没有记录
        }
    }
    else
    {
        if (pResultSet->Next())
        {
            ret = 2;
        }
        else
        {
            ret = 1; // 没有记录
        }
    }

    delete pResultSet;

    return ret;
}

int CheckwhetherHaveRecord(CDBConn *pDBConn, char *sql_cmd)
{
    int ret = -1;
    CResultSet *pResultSet = pDBConn->ExecuteQuery(sql_cmd);

    if (!pResultSet)
    {
        ret = -1;
    }
    else if (pResultSet && pResultSet->Next())
    {
        ret = 1;
    }
    else
    {
        ret = 0;
    }

    delete pResultSet;

    return ret;
}

int GetResultOneStatus(CDBConn *pDBConn, char *sql_cmd, int &shared_status)
{
    int ret = 0;
    CResultSet *pResultSet = pDBConn->ExecuteQuery(sql_cmd);

    if (!pResultSet)
    {
        LOG_ERROR << "!pResultSet faled";
        ret = -1;
    }
    
    if (pResultSet->Next())
    {
        ret = 0;
        // 存在在返回
        shared_status = pResultSet->GetInt("shared_status");
        LOG_INFO << "shared_status: " << shared_status;
    }
    else
    {
        LOG_ERROR << "pResultSet->Next()";
        ret = -1;
    }

    delete pResultSet;

    return ret;
}

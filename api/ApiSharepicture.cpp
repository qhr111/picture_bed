#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/time.h>
#include "ApiCommon.h"
#include <time.h>

//解析的json包
int decodeSharePictureJson(string &str_json, string &user_name, string &token, string &md5, string &filename)
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

    if (root["token"].isNull())
    {
        LOG_ERROR << "token null\n";
        return -1;
    }
    token = root["token"].asString();

    if (root["user"].isNull())
    {
        LOG_ERROR << "user null\n";
        return -1;
    }
    user_name = root["user"].asString();

    if (root["md5"].isNull())
    {
        LOG_ERROR << "md5 null\n";
        return -1;
    }
    md5 = root["md5"].asString();

    if (root["filename"].isNull())
    {
        LOG_ERROR << "filename null\n";
        return -1;
    }
    filename = root["filename"].asString();

    return 0;
}
int encodeSharePictureJson(int ret, string urlmd5, string &str_json)
{
    Json::Value root;
    root["code"] = ret;
    if (HTTP_RESP_OK == ret)
        root["urlmd5"] = urlmd5;
    Json::FastWriter writer;
    str_json = writer.write(root);
    return 0;
}

//解析的json包
int decodePictureListJson(string &str_json, string &user_name, string &token, int &start, int &count)
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

    if (root["token"].isNull())
    {
        LOG_ERROR << "token null\n";
        return -1;
    }
    token = root["token"].asString();

    if (root["user"].isNull())
    {
        LOG_ERROR << "user null\n";
        return -1;
    }
    user_name = root["user"].asString();

    if (root["start"].isNull())
    {
        LOG_ERROR << "start null\n";
        return -1;
    }
    start = root["start"].asInt();

    if (root["count"].isNull())
    {
        LOG_ERROR << "count null\n";
        return -1;
    }
    count = root["count"].asInt();

    return 0;
}

int decodeCancelPictureJson(string &str_json, string &user_name, string &urlmd5)
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

    if (root["user"].isNull())
    {
        LOG_ERROR << "user null\n";
        return -1;
    }
    user_name = root["user"].asString();

    if (root["urlmd5"].isNull())
    {
        LOG_ERROR << "urlmd5 null\n";
        return -1;
    }
    urlmd5 = root["urlmd5"].asString();

    return 0;
}

int encodeCancelPictureJson(int ret, string &str_json)
{
    Json::Value root;
    root["code"] = ret;
    Json::FastWriter writer;
    str_json = writer.write(root);
    return 0;
}

int decodeBrowsePictureJson(string &str_json, string &urlmd5)
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

    if (root["urlmd5"].isNull())
    {
        LOG_ERROR << "urlmd5 null\n";
        return -1;
    }
    urlmd5 = root["urlmd5"].asString();

    return 0;
}

int encodeBrowselPictureJson(int ret, int pv, string url, string user, string time, string &str_json)
{
    Json::Value root;
    root["code"] = ret;
    if (ret == 0)
    {
        root["pv"] = pv;
        root["url"] = url;
        root["user"] = user;
        root["time"] = time;
    }
    Json::FastWriter writer;
    str_json = writer.write(root);
    return 0;
}

//获取共享图片个数
int getSharePicturesCount(CDBConn *pDBConn, CacheConn *pCacheConn, string &user_name, int &count)
{
    int ret = 0;
    int64_t file_count = 0;
    
    // 先查看用户是否存在
    string str_sql;

    // 1. 先从redis里面获取，如果数量为0则从mysql查询确定是否为0
    if(CacheGetCount(pCacheConn, SHARE_PIC_COUNT+user_name, file_count) < 0)
    {
        LOG_ERROR << "CacheGetCount failed";
       ret = -1;
    }

    if(file_count == 0)
    {
        // 从mysql加载
        if(DBGetSharePictureCountByUsername(pDBConn, user_name, count) < 0)
        {
            LOG_ERROR << "DBGetSharePictureCountByUsername failed";
            return -1;
        }
       
        file_count = (int64_t)count;
        if(CacheSetCount(pCacheConn, SHARE_PIC_COUNT+user_name, file_count) < 0) // 失败了那下次继续从mysql加载
        {
            LOG_ERROR << "CacheSetCount failed";
            return -1;
        }

        ret = 0;
    }
    count = file_count;
    
    return ret;
}
 

//获取共享文件列表
//获取用户文件信息 127.0.0.1:80/sharepicture&cmd=normal
void handleGetSharePicturesList(const char *user, int start, int count, string &str_json)
{
    int ret = 0;
    char sql_cmd[SQL_MAX_LEN] = {0};
    CResultSet *pResultSet;
    int total = 0;
    int file_count = 0;
    Json::Value root;
    CDBManager *pDBManager = CDBManager::getInstance();
    CDBConn *pDBConn = pDBManager->GetDBConn("tuchuang_slave");
    AUTO_REL_DBCONN(pDBManager, pDBConn);
    CacheManager *pCacheManager = CacheManager::getInstance();
    CacheConn *pCacheConn = pCacheManager->GetCacheConn("token");
    AUTO_REL_CACHECONN(pCacheManager, pCacheConn);


    total = 0;
    string temp_user = user;
    ret = getSharePicturesCount(pDBConn, pCacheConn, temp_user, total);
    if (ret < 0)
    {
        LOG_ERROR << "getSharePicturesCount failed";
        ret = -1;
        goto END;
    }
    if (total == 0)
    {
        LOG_INFO << "getSharePicturesCount count = 0";
        ret = 0;
        goto END;
    }

    // sql语句
    sprintf(sql_cmd, "select share_picture_list.user, share_picture_list.filemd5, share_picture_list.file_name,share_picture_list.urlmd5, share_picture_list.pv, \
        share_picture_list.create_time, file_info.size from file_info, share_picture_list where share_picture_list.user = '%s' and  \
        file_info.md5 = share_picture_list.filemd5 limit %d, %d",  user, start, count); // 如果原始文件被删除后，没有同时删除共享文件则会导致共享文件不同步
    LOG_INFO << "执行: " << sql_cmd;
    pResultSet = pDBConn->ExecuteQuery(sql_cmd);
    if (pResultSet)
    {
        // 遍历所有的内容
        // 获取大小
        Json::Value files;

        while (pResultSet->Next())
        {
            Json::Value file;
            file["user"] = pResultSet->GetString("user");
            file["filemd5"] = pResultSet->GetString("filemd5");
            file["file_name"] = pResultSet->GetString("file_name");
            file["urlmd5"] = pResultSet->GetString("urlmd5");
            file["pv"] = pResultSet->GetInt("pv");
            file["create_time"] = pResultSet->GetString("create_time");
            file["size"] = pResultSet->GetInt("size");
            files[file_count] = file;
            file_count++;
        }
        if(file_count >0 )
        root["files"] = files;

        ret = 0;
        delete pResultSet;
    }
    else
    {
        ret = -1;
    }

END:
    if (ret != 0)
    {
        Json::Value root;
        root["code"] = 1;
    }
    else
    {
        root["code"] = 0;
        root["count"] = file_count;
        root["total"] = total;
    }
    str_json = root.toStyledString();
    LOG_INFO << "str_json:" << str_json;

    return;
}

//取消分享文件
void handleCancelSharePicture(const char *user, const char *urlmd5, string &str_json)
{
    int ret = 0;
    char sql_cmd[SQL_MAX_LEN] = {0};
    int line = 0;
    int ret2;
    string filemd5;
    int count = 0;
    string fileid;
    CDBManager *pDBManager = CDBManager::getInstance();
    CDBConn *pDBConn = pDBManager->GetDBConn("tuchuang_slave");
    AUTO_REL_DBCONN(pDBManager, pDBConn);
    CacheManager *pCacheManager = CacheManager::getInstance();
    CacheConn *pCacheConn = pCacheManager->GetCacheConn("token");
    AUTO_REL_CACHECONN(pCacheManager, pCacheConn);
    CResultSet *pResultSet;

    LOG_WARN << "into";

    // sql语句
    sprintf(sql_cmd, "select * from share_picture_list where user = '%s' and urlmd5 = '%s'", user, urlmd5);
    LOG_INFO << "执行: " << sql_cmd;
    pResultSet = pDBConn->ExecuteQuery(sql_cmd);
    if (!pResultSet)
    {
        LOG_ERROR << sql_cmd << " 操作失败";
        ret = -1;
        goto END;
    }
    delete pResultSet;
    // mysql_num_rows接受由mysql_store_result返回的结果结构集，并返回结构集中的行数
    line = pDBConn->GetRowNum();
    LOG_INFO << "GetRowNum = " << line;
    if (line == 0)
    {
        LOG_ERROR << "没有记录";
        ret = 0;
        goto END;
    }

    // 获取文件md5
    LOG_INFO << "urlmd5: " << urlmd5;
    // 1. 先从分享图片列表查询到文件信息
    sprintf(sql_cmd, "select filemd5 from share_picture_list where urlmd5 = '%s'", urlmd5);
    LOG_DEBUG << "执行: " << sql_cmd;
    pResultSet = pDBConn->ExecuteQuery(sql_cmd);
    if (pResultSet && pResultSet->Next())
    {
        filemd5 = pResultSet->GetString("filemd5");
        delete pResultSet;
    }
    else
    {
        if (pResultSet)
            delete pResultSet;
        ret = -1;
        goto END;
    }

    // 这个位置要开始考虑事务了， 如果删除文件记录，那一定要保证文件引用计数-1
    
    // pDBConn->StartTransaction(); // 开启事务的处理
    // 这里文件引用计数减少
    //文件信息表(file_info)的文件引用计数count，减去1
     //查询引用计数和文件id
    sprintf(sql_cmd, "select count, file_id from file_info where md5 = '%s' for update", filemd5.c_str());  // 排他锁
    LOG_INFO << "执行: " << sql_cmd;
    pResultSet = pDBConn->ExecuteQuery(sql_cmd);
    if (pResultSet && pResultSet->Next())
    {
        fileid = pResultSet->GetString("file_id");
        count = pResultSet->GetInt("count");
        delete pResultSet;
    }
    else
    {
        if(pResultSet)
            delete pResultSet;
        LOG_ERROR << sql_cmd << " 操作失败";
        ret = -1;
       // pDBConn->Rollback();
        goto END;
    }
    

    if (count > 0)
    {
        count -= 1;
        sprintf(sql_cmd, "update file_info set count=%d where md5 = '%s'", count, filemd5.c_str());
        LOG_INFO << "执行: " << sql_cmd;
        if (!pDBConn->ExecuteUpdate(sql_cmd))
        {
            LOG_ERROR << sql_cmd << " 操作失败";
            ret = -1;
           // pDBConn->Rollback();
            goto END;
        }
    }

    //删除在共享图片列表的数据
    sprintf(sql_cmd, "delete from share_picture_list where user = '%s' and urlmd5 = '%s'", user, urlmd5);
    LOG_INFO << "执行: " << sql_cmd;
    if (!pDBConn->ExecutePassQuery(sql_cmd))
    {
        LOG_ERROR << sql_cmd << " 操作失败";
         ret = -1;
       // pDBConn->Rollback();
        goto END;
    }
    // pDBConn->Commit();

    // 是否要删除文件，再次读取数据库

    if (count == 0) //说明没有用户引用此文件，需要在storage删除此文件， 这里再次去查询
    {
        //删除文件信息表中该文件的信息
        sprintf(sql_cmd, "delete from file_info where md5 = '%s'", filemd5.c_str());
        LOG_INFO << "执行: " << sql_cmd;
        if (!pDBConn->ExecuteDrop(sql_cmd))
        {
            LOG_WARN << sql_cmd << " 操作失败";
            ret = -1;
            goto END;
        }
        LOG_WARN << "RemoveFileFromFastDfs";
        //从storage服务器删除此文件，参数为为文件id
        ret2 = RemoveFileFromFastDfs(fileid.c_str());
        if (ret2 != 0)
        {
            LOG_INFO << "RemoveFileFromFastDfs err: " << ret2;
            ret = -1;
            goto END;
        }        
    }

    //事务的结束

    // 共享图片数量-1
    if(CacheDecrCount(pCacheConn, SHARE_PIC_COUNT+string(user)) < 0)
    {
        LOG_ERROR << "CacheDecrCount failed";   // 即使失败 也可以下次从mysql加载计数
    }
    LOG_WARN << "leave";
    ret = 0;
END:
    /*
    取消分享：
        成功：{"code": 0}
        失败：{"code": 1}
    */
    if (0 == ret)
        encodeCancelPictureJson(HTTP_RESP_OK, str_json);
    else
        encodeCancelPictureJson(HTTP_RESP_FAIL, str_json);

    // return;
}

//分享文件
int handleSharePicture(const char *user, const char *filemd5, const char *file_name, string &str_json)
{
    char key[5] = {0};
    int count = 0;
    // 获取数据库连接
    CDBManager *pDBManager = CDBManager::getInstance();
    CDBConn *pDBConn = pDBManager->GetDBConn("tuchuang_slave");
    AUTO_REL_DBCONN(pDBManager, pDBConn);
    CacheManager *pCacheManager = CacheManager::getInstance();
    CacheConn *pCacheConn = pCacheManager->GetCacheConn("token");
    AUTO_REL_CACHECONN(pCacheManager, pCacheConn);
    /*
    1. 图床分享：
    （1）图床分享数据库，user file_name filemd5 urlmd5  pv浏览次数 create_time
    （2）生成提取码4位，
         生成要返回的url md5（根据用户名+文件md5+随机数）
         文件md5对应的文件加+1；
         插入图床表单
    （3）返回提取码和md5
    2. 我的图床：
         返回图床的信息。
    3. 浏览请求，解析参数urlmd5和提取码key，校验成功返回下载地址
     4. 取消图床
         删除对应的行信息，并将文件md5对应的文件加-1；
    */
    int ret = 0;
    char sql_cmd[SQL_MAX_LEN] = {0};
    char create_time[TIME_STRING_LEN];
    string urlmd5;
    urlmd5 = RandomString(32); // 这里我们先简单的，直接使用随机数代替 MD5的使用

    LOG_INFO << "urlmd5: " << urlmd5;

    // 1. 生成urlmd5，生成提取码
    time_t now;
    //获取当前时间
    now = time(NULL);
    strftime(create_time, TIME_STRING_LEN - 1, "%Y-%m-%d %H:%M:%S", localtime(&now));

    // 3. 插入share_picture_list
    //图床分享图片的信息，额外保存在share_picture_list保存列表
    /*
        -- user	文件所属用户
        -- filemd5 文件本身的md5
        -- urlmd5 图床url md5，同一文件可以对应多个图床分享
        -- create_time 文件共享时间
        -- file_name 文件名字
        -- pv 文件下载量，默认值为1，下载一次加1
    */
    sprintf(sql_cmd, "insert into share_picture_list (user, filemd5, file_name, urlmd5, `key`, pv, create_time) values ('%s', '%s', '%s', '%s', '%s', %d, '%s')",
            user, filemd5, file_name, urlmd5.c_str(), key, 0, create_time);
    LOG_INFO << "执行: " << sql_cmd;
    if (!pDBConn->ExecuteCreate(sql_cmd))
    {
        LOG_ERROR << sql_cmd << " 操作失败";
        ret = -1;
        goto END;
    }


    // 这里文件引用计数增加
    //文件信息表，查找该文件的计数器  
    sprintf(sql_cmd, "select count from file_info where md5 = '%s'", filemd5);
    count = 0;
    ret = GetResultOneCount(pDBConn, sql_cmd, count); //执行sql语句
    if (ret != 0)
    {
        LOG_ERROR << sql_cmd << " 操作失败";
        ret = -1;
        goto END;
    }
    // 1、修改file_info中的count字段，+1 （count 文件引用计数）
    sprintf(sql_cmd, "update file_info set count = %d where md5 = '%s'", count + 1, filemd5);
    if (!pDBConn->ExecuteUpdate(sql_cmd))
    {
        LOG_ERROR << sql_cmd << " 操作失败";
        ret = -1;
        goto END;
    }

    // 4 增加分享图片计数
    if(CacheIncrCount(pCacheConn, SHARE_PIC_COUNT+string(user))< 0)
    {
        LOG_ERROR << " CacheIncrCount 操作失败";
    }
 
   
    ret = 0;
END:

    // 5. 返回urlmd5 和提取码key, 现在没有做提取码(如果做了就类似百度云，需要输入提取码才能获取图片地址)
    if (ret == 0)
    {
        encodeSharePictureJson(HTTP_RESP_OK, urlmd5, str_json);
    }
    else
    {
        encodeSharePictureJson(HTTP_RESP_FAIL, urlmd5, str_json);
    }

    return ret;
}

int handleBrowsePicture( const char *urlmd5, string &str_json)
{
    int ret = 0;
    char sql_cmd[SQL_MAX_LEN] = {0};
    string picture_url;
    string file_name;
    string user;
    string filemd5;
    string create_time;
    int pv = 0;

    CDBManager *pDBManager = CDBManager::getInstance();
    CDBConn *pDBConn = pDBManager->GetDBConn("tuchuang_slave");
    AUTO_REL_DBCONN(pDBManager, pDBConn);
    CacheManager *pCacheManager = CacheManager::getInstance();
    CacheConn *pCacheConn = pCacheManager->GetCacheConn("token");
    AUTO_REL_CACHECONN(pCacheManager, pCacheConn);
    CResultSet *pResultSet = NULL;

    LOG_INFO << "urlmd5: " << urlmd5;
    // 1. 先从分享图片列表查询到文件信息
    sprintf(sql_cmd, "select user, filemd5, file_name, pv, create_time from share_picture_list where urlmd5 = '%s'", urlmd5);
    LOG_DEBUG << "执行: " << sql_cmd;
    pResultSet = pDBConn->ExecuteQuery(sql_cmd);
    if (pResultSet && pResultSet->Next())
    {
        user = pResultSet->GetString("user");
        filemd5 = pResultSet->GetString("filemd5");
        file_name = pResultSet->GetString("file_name");
        pv = pResultSet->GetInt("pv");
        create_time = pResultSet->GetString("create_time");
        delete pResultSet;
    }
    else
    {
        if (pResultSet)
            delete pResultSet;
        ret = -1;
        goto END;
    }

    // 2. 通过文件的MD5查找对应的url地址
    sprintf(sql_cmd, "select url from file_info where md5 ='%s'", filemd5.c_str());
    LOG_INFO << "执行: " << sql_cmd;
    pResultSet = pDBConn->ExecuteQuery(sql_cmd);
    if (pResultSet && pResultSet->Next())
    {
        picture_url = pResultSet->GetString("url");
        delete pResultSet;
    }
    else
    {
        if (pResultSet)
            delete pResultSet;
        ret = -1;
        goto END;
    }

    // 3. 更新浏览次数
    sprintf(sql_cmd, "update share_picture_list set pv = %d where urlmd5 = '%s'", pv + 1, urlmd5);
    LOG_DEBUG << "执行: " << sql_cmd;
    if (!pDBConn->ExecuteUpdate(sql_cmd))
    {
        LOG_ERROR << sql_cmd << " 操作失败";
        ret = -1;
        goto END;
    }
    ret = 0;
END:
    // 4. 返回urlmd5 和提取码key
    if (ret == 0)
    {
        encodeBrowselPictureJson(HTTP_RESP_OK, pv, picture_url, user, create_time, str_json);
    }
    else
    {
        encodeBrowselPictureJson(HTTP_RESP_FAIL, pv, picture_url, user, create_time, str_json);
    }

    return 0;
}

int ApiSharepicture(string &url, string &post_data, string &str_json)
{
    char cmd[20];
    string user_name; //用户名
    string md5;       //文件md5码
    string urlmd5;
    string filename; //文件名字
    string token;
    int ret = 0;
    //解析命令
    QueryParseKeyValue(url.c_str(), "cmd", cmd, NULL);
    LOG_INFO << "cmd = " << cmd;

    

    if (strcmp(cmd, "share") == 0) //分享文件
    {
        ret = decodeSharePictureJson(post_data, user_name, token, md5, filename); //解析json信息
        if (ret == 0)
        {
            handleSharePicture(user_name.c_str(), md5.c_str(), filename.c_str(), str_json);
        }
        else
        {
            // 回复请求格式错误
            encodeSharePictureJson(HTTP_RESP_FAIL, urlmd5, str_json);
        }
    }
    else if (strcmp(cmd, "normal") == 0) //文件下载标志处理
    {
        int start = 0;
        int count = 0;
        ret = decodePictureListJson(post_data, user_name, token, start, count);
        if (ret == 0)
        {
            handleGetSharePicturesList(user_name.c_str(), start, count, str_json);
        }
        else
        {
              // 回复请求格式错误
            encodeSharePictureJson(HTTP_RESP_FAIL, urlmd5, str_json);
        }
    }
    else if (strcmp(cmd, "cancel") == 0) //取消分享文件
    {
        ret = decodeCancelPictureJson(post_data, user_name, urlmd5);
        if (ret == 0)
        {
            handleCancelSharePicture(user_name.c_str(), urlmd5.c_str(), str_json);
        }
        else
        {
            // 回复请求格式错误
            encodeCancelPictureJson(1, str_json);
        }
    }
    else if (strcmp(cmd, "browse") == 0) //取消分享文件
    {
        ret = decodeBrowsePictureJson(post_data, urlmd5);
        LOG_INFO << "post_data: " << post_data << ", urlmd5: " << urlmd5;
        if (ret == 0)
        {
            handleBrowsePicture(urlmd5.c_str(), str_json);
        }
        else
        {
             // 回复请求格式错误
            encodeSharePictureJson(HTTP_RESP_FAIL, urlmd5, str_json);
        }
    } 
    else
    {
        // 回复请求格式错误
        encodeSharePictureJson(HTTP_RESP_FAIL, urlmd5, str_json);
    }

    return 0;
}

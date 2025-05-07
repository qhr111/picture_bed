#include "ApiUpload.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>

#include "fdfs_client.h"
#include "ApiCommon.h"



/* -------------------------------------------*/
/**
 * @brief  将一个本地文件上传到 后台分布式文件系统中
 * 对应 fdfs_upload_file /etc/fdfs/client.conf  完整文件路径
 *
 * @param file_path  (in) 本地文件的路径
 * @param fileid    (out)得到上传之后的文件ID路径
 *
 * @returns
 *      0 succ, -1 fail
 */
/* -------------------------------------------*/
int uploadFileToFastDfs(char *file_path, char *fileid)
{
    int ret = 0;

    pid_t pid;
    int fd[2];

    //无名管道的创建
    if (pipe(fd) < 0) // fd[0] → r； fd[1] → w  获取上传后返回的信息 fileid
    {
        LOG_ERROR << "pipe error";
        ret = -1;
        goto END;
    }

    //创建进程
    pid = fork(); //
    if (pid < 0)  //进程创建失败
    {
        LOG_ERROR << "fork error";
        ret = -1;
        goto END;
    }

    if (pid == 0) //子进程
    {
        //关闭读端
        close(fd[0]);
        //将标准输出 重定向 写管道
        dup2(fd[1], STDOUT_FILENO); // 往标准输出写的东西都会重定向到fd所指向的文件, 当fileid产生时输出到管道fd[1]
        // fdfs_upload_file /etc/fdfs/client.conf 123.txt
        // printf("fdfs_upload_file %s %s %s\n", fdfs_cli_conf_path, filename, file_path);
        //通过execlp执行fdfs_upload_file  如果函数调用成功,进程自己的执行代码就会变成加载程序的代码,execlp()后边的代码也就不会执行了.
        execlp("fdfs_upload_file", "fdfs_upload_file", s_dfs_path_client.c_str(), file_path, NULL);


        //执行失败
       LOG_ERROR << "execlp fdfs_upload_file error";

        close(fd[1]);
    }
    else //父进程
    {
        //关闭写端
        close(fd[1]);

        //从管道中去读数据
        read(fd[0], fileid, TEMP_BUF_MAX_LEN); // 等待管道写入然后读取

        LOG_INFO << "fileid1: " << fileid;
        //去掉一个字符串两边的空白字符
        TrimSpace(fileid);

        if (strlen(fileid) == 0)
        {
            LOG_ERROR << "upload failed";
            ret = -1;
            goto END;
        }
        LOG_INFO << "fileid2: " << fileid;

        wait(NULL); //等待子进程结束，回收其资源
        close(fd[0]);
    }

END:
    return ret;
}

/* -------------------------------------------*/
/**
 * @brief  封装文件存储在分布式系统中的 完整 url
 *
 * @param fileid        (in)    文件分布式id路径
 * @param fdfs_file_url (out)   文件的完整url地址
 *
 * @returns
 *      0 succ, -1 fail
 */
/* -------------------------------------------*/
int getFullurlByFileid(char *fileid, char *fdfs_file_url)
{
    int ret = 0;

    char *p = NULL;
    char *q = NULL;
    char *k = NULL;

    char fdfs_file_stat_buf[TEMP_BUF_MAX_LEN] = {0};
    char fdfs_file_host_name[HOST_NAME_LEN] = {0}; // storage所在服务器ip地址

    pid_t pid;
    int fd[2];

    //无名管道的创建
    if (pipe(fd) < 0)
    {
        LOG_ERROR << "pipe error";
        ret = -1;
        goto END;
    }

    //创建进程
    pid = fork();
    if (pid < 0) //进程创建失败
    {
        LOG_ERROR << "fork error";
        ret = -1;
        goto END;
    }

    if (pid == 0) //子进程
    {
        //关闭读端
        close(fd[0]);

        //将标准输出 重定向 写管道
        dup2(fd[1], STDOUT_FILENO); // dup2(fd[1], 1);

        execlp("fdfs_file_info", "fdfs_file_info", s_dfs_path_client.c_str(), fileid, NULL);

        //执行失败
        LOG_ERROR << "execlp fdfs_file_info error";

        close(fd[1]);
    }
    else //父进程
    {
        //关闭写端
        close(fd[1]);

        //从管道中去读数据
        read(fd[0], fdfs_file_stat_buf, TEMP_BUF_MAX_LEN);
        ;

        wait(NULL); //等待子进程结束，回收其资源
        close(fd[0]);
        LOG_INFO << "fdfs_file_stat_buf: " << fdfs_file_stat_buf;
        //拼接上传文件的完整url地址--->http://host_name/group1/M00/00/00/D12313123232312.png
        p = strstr(fdfs_file_stat_buf, "source ip address: ");

        q = p + strlen("source ip address: ");
        k = strstr(q, "\n");

        strncpy(fdfs_file_host_name, q, k - q);
        fdfs_file_host_name[k - q] = '\0';      // 这里这个获取回来只是局域网的ip地址，在讲fastdfs原理的时候再继续讲这个问题

        LOG_INFO << "host_name:" << s_storage_web_server_ip << ", fdfs_file_host_name: " << fdfs_file_host_name;

        // storage_web_server服务器的端口

        strcat(fdfs_file_url, "http://");
        strcat(fdfs_file_url, s_storage_web_server_ip.c_str());
        strcat(fdfs_file_url, ":");
        strcat(fdfs_file_url, s_storage_web_server_port.c_str());
        strcat(fdfs_file_url, "/");
        strcat(fdfs_file_url, fileid);

        LOG_INFO << "fdfs_file_url:" << fdfs_file_url;
    }

END:
    return ret;
}

int storeFileinfo(CDBConn *pDBConn, CacheConn *pCacheConn, char *user, char *filename, char *md5, long size, char *fileid, char *fdfs_file_url)
{
    int ret = 0;
    time_t now;
    char create_time[TIME_STRING_LEN];
    char suffix[SUFFIX_LEN];
    char sql_cmd[SQL_MAX_LEN] = {0};

    //得到文件后缀字符串 如果非法文件后缀,返回"null"
    GetFileSuffix(filename, suffix); // mp4, jpg, png

    // sql 语句
    /*
       -- =============================================== 文件信息表
       -- md5 文件md5
       -- file_id 文件id
       -- url 文件url
       -- size 文件大小, 以字节为单位
       -- type 文件类型： png, zip, mp4……
       -- count 文件引用计数， 默认为1， 每增加一个用户拥有此文件，此计数器+1
       */
    sprintf(sql_cmd, "insert into file_info (md5, file_id, url, size, type, count) values ('%s', '%s', '%s', '%ld', '%s', %d)",
            md5, fileid, fdfs_file_url, size, suffix, 1);
    LOG_INFO << "执行: " << sql_cmd;
    if (!pDBConn->ExecuteCreate(sql_cmd)) //执行sql语句
    {
        LOG_ERROR << sql_cmd << " 操作失败";
        ret = -1;
        goto END;
    }

    //获取当前时间
    now = time(NULL);
    strftime(create_time, TIME_STRING_LEN - 1, "%Y-%m-%d %H:%M:%S", localtime(&now));

    /*
       -- =============================================== 用户文件列表
       -- user 文件所属用户
       -- md5 文件md5
       -- create_time 文件创建时间
       -- file_name 文件名字
       -- shared_status 共享状态, 0为没有共享， 1为共享
       -- pv 文件下载量，默认值为0，下载一次加1
       */
    // sql语句
    sprintf(sql_cmd, "insert into user_file_list(user, md5, create_time, file_name, shared_status, pv) values ('%s', '%s', '%s', '%s', %d, %d)", user, md5, create_time, filename, 0, 0);
    LOG_INFO << "执行: " << sql_cmd;
    if (!pDBConn->ExecuteCreate(sql_cmd))
    {
        LOG_ERROR << sql_cmd << "  操作失败";
        ret = -1;
        goto END;
    }

    // 询用户文件数量+1
    if(CacheIncrCount(pCacheConn, string(user)) < 0)
    {
        LOG_ERROR << " CacheIncrCount 操作失败";
    }

END:
    return ret;
}

int ApiUploadInit(char *dfs_path_client, char *web_server_ip, char *web_server_port, char *storage_web_server_ip, char *storage_web_server_port)
{
    s_dfs_path_client = dfs_path_client;
    s_web_server_ip = web_server_ip;
    s_web_server_port = web_server_port;
    s_storage_web_server_ip = storage_web_server_ip;
    s_storage_web_server_port = storage_web_server_port;
    return 0;
}
int ApiUpload(string &url, string &post_data, string &str_json)
{
    UNUSED(url);

    char suffix[SUFFIX_LEN] = {0};
    char fileid[TEMP_BUF_MAX_LEN] = {0};    //文件上传到fastDFS后的文件id
    char fdfs_file_url[FILE_URL_LEN] = {0}; //文件所存放storage的host_name
    int ret = 0;
    char boundary[TEMP_BUF_MAX_LEN] = {0}; //分界线信息
    char file_name[128] = {0};
    char file_content_type[128] = {0};
    char file_path[128] = {0};
    char new_file_path[128] = {0};
    char file_md5[128] = {0};
    char file_size[32] = {0};
    long long_file_size = 0;
    char user[32] = {0};
    char *begin = (char *)post_data.c_str();
    char *p1, *p2;

    Json::Value value;

    // 获取数据库连接
    CDBManager *pDBManager = CDBManager::getInstance();
    CDBConn *pDBConn = pDBManager->GetDBConn("tuchuang_slave");
    AUTO_REL_DBCONN(pDBManager, pDBConn);
    CacheManager *pCacheManager = CacheManager::getInstance();
    CacheConn *pCacheConn = pCacheManager->GetCacheConn("token");
    AUTO_REL_CACHECONN(pCacheManager, pCacheConn);


    LOG_INFO << "post_data: " << post_data;

    // 1. 解析boundary
    // Content-Type: multipart/form-data; boundary=----WebKitFormBoundaryjWE3qXXORSg2hZiB
    // 找到起始位置
    p1 = strstr(begin, "\r\n"); // 作用是返回字符串中首次出现子串的地址
    if (p1 == NULL)
    {
        LOG_ERROR << "wrong no boundary!";
        ret = -1;
        goto END;
    }
    //拷贝分界线
    strncpy(boundary, begin, p1 - begin); // 缓存分界线, 比如：WebKitFormBoundary88asdgewtgewx
    boundary[p1 - begin] = '\0';          //字符串结束符
    LOG_INFO << "boundary: " << boundary;

    // 查找文件名file_name
    begin = p1 + 2;
    p2 = strstr(begin, "name=\"file_name\""); //找到file_name字段
    if (!p2)
    {
        LOG_ERROR << "wrong no file_name!";
        ret = -1;
        goto END;
    }
    p2 = strstr(begin, "\r\n"); // 找到file_name下一行
    p2 += 4;                    //下一行起始
    begin = p2; // config.ini
    p2 = strstr(begin, "\r\n");
    strncpy(file_name, begin, p2 - begin);
    LOG_INFO << "file_name: " << file_name;

    // 查找文件类型file_content_type
    begin = p2 + 2;
    p2 = strstr(begin, "name=\"file_content_type\""); //
    if (!p2)
    {
        LOG_ERROR << "wrong no file_content_type!";
        ret = -1;
        goto END;
    }
    p2 = strstr(p2, "\r\n");
    p2 += 4;
    begin = p2;
    p2 = strstr(begin, "\r\n");
    strncpy(file_content_type, begin, p2 - begin);
    LOG_INFO << "file_content_type: " << file_content_type;
   
    // 查找文件file_path
    begin = p2 + 2;
    p2 = strstr(begin, "name=\"file_path\""); //
    if (!p2)
    {
        LOG_ERROR << "wrong no file_path!";
        ret = -1;
        goto END;
    }
    p2 = strstr(p2, "\r\n");
    p2 += 4;
    begin = p2;
    p2 = strstr(begin, "\r\n");
    strncpy(file_path, begin, p2 - begin);
    LOG_INFO << "file_path: " << file_path;

    // 查找文件file_md5
    begin = p2 + 2;
    p2 = strstr(begin, "name=\"file_md5\""); //
    if (!p2)
    {
        LOG_ERROR << "wrong no file_md5!";
        ret = -1;
        goto END;
    }
    p2 = strstr(p2, "\r\n");
    p2 += 4;
    begin = p2;
    p2 = strstr(begin, "\r\n");
    strncpy(file_md5, begin, p2 - begin);
    LOG_INFO << "file_md5: " << file_md5;

    // 查找文件file_size
    begin = p2 + 2;
    p2 = strstr(begin, "name=\"file_size\""); //
    if (!p2)
    {
        LOG_ERROR << "wrong no file_size!";
        ret = -1;
        goto END;
    }
    p2 = strstr(p2, "\r\n");
    p2 += 4;
    begin = p2;
    p2 = strstr(begin, "\r\n");
    strncpy(file_size, begin, p2 - begin);
    LOG_INFO << "file_size: " << file_size;
    long_file_size = strtol(file_size, NULL, 10); //字符串转long

    // 查找user
    begin = p2 + 2;
    p2 = strstr(begin, "name=\"user\""); //
    if (!p2)
    {
        LOG_ERROR << "wrong no user!";
        ret = -1;
        goto END;
    }
    p2 = strstr(p2, "\r\n");
    p2 += 4;
    begin = p2;
    p2 = strstr(begin, "\r\n");
    strncpy(user, begin, p2 - begin);
    LOG_INFO << "user: " << user;

    // 获取文件名后缀
    GetFileSuffix(file_name, suffix); // mp4, jpg, png
    strcat(new_file_path, file_path);
    strcat(new_file_path, ".");
    strcat(new_file_path, suffix);
    // 重命名 修改文件名
    ret = rename(file_path, new_file_path); /// 
    if (ret < 0)
    {
        LOG_ERROR << "rename " << file_path << " to " << new_file_path << "failed";
        ret = -1;
        goto END;
    }
    //===============> 将该文件存入fastDFS中,并得到文件的file_id <============
    LOG_INFO << "uploadFileToFastDfs, file_name:" << file_name << ", new_file_path:" << new_file_path;
    if (uploadFileToFastDfs(new_file_path, fileid) < 0)
    {
        LOG_ERROR << "uploadFileToFastDfs failed "; 
        ret = -1;
        goto END;
    }
    //================> 删除本地临时存放的上传文件 <===============
    LOG_INFO << "unlink: " << new_file_path;
    ret = unlink(new_file_path);
    if (ret != 0)
    {
       LOG_WARN << "unlink: " << new_file_path <<   " failed";      // 删除失败则需要有个监控重新清除过期的临时文件，比如过期两天的都删除
    }

    //================> 得到文件所存放storage的host_name <=================
    // 拼接出完整的http地址
    LOG_INFO << "getFullurlByFileid, fileid: " << fileid;
    if (getFullurlByFileid(fileid, fdfs_file_url) < 0)
    {
        LOG_ERROR << "getFullurlByFileid failed ";
        ret = -1;
        goto END;
    }

    //===============> 将该文件的FastDFS相关信息存入mysql中 <======
    LOG_INFO << "storeFileinfo, url: " << fdfs_file_url;
    // 把文件写入file_info
    if (storeFileinfo(pDBConn, pCacheConn, user, file_name, file_md5, long_file_size, fileid, fdfs_file_url) < 0)
    {
        LOG_ERROR << "storeFileinfo failed ";
        ret = -1;
        // 严谨而言，这里需要删除 已经上传的文件
        goto END;
    }

    
    value["code"] = 0;
    str_json = value.toStyledString(); // json序列化, 直接用writer是紧凑方式，这里toStyledString是格式化更可读方式

    return 0;
END:
    value["code"] = 1;
    str_json = value.toStyledString(); // json序列化

    return -1;
}
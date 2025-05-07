#include "ApiRegister.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/time.h>
#include <time.h>
#include "Common.h"

#include "DBPool.h"

#include "Logging.h"
#include "json/json.h"

//解析用户注册信息的json包
/*json数据如下
    {
        userName:xxxx,
        nickName:xxx,
        firstPwd:xxx,
        phone:xxx,
        email:xxx
    }
    */
int decodeRegisterJson(const std::string &str_json, string &user_name, string &nick_name, string &pwd, string &phone, string &email)
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
    if (root["userName"].isNull())
    {
        LOG_ERROR << "userName null\n";
        return -1;
    }
    user_name = root["userName"].asString();

    // 昵称
    if (root["nickName"].isNull())
    {
        LOG_ERROR << "nickName null\n";
        return -1;
    }
    nick_name = root["nickName"].asString();

    //密码
    if (root["firstPwd"].isNull())
    {
        LOG_ERROR << "firstPwd null\n";
        return -1;
    }
    pwd = root["firstPwd"].asString();

    //电话  非必须
    if (root["phone"].isNull())
    {
        LOG_WARN << "phone null\n";
    }
    else
    {
        phone = root["phone"].asString();
    }

    //邮箱 非必须
    if (root["email"].isNull())
    {
        LOG_WARN << "email null\n";
    }
    else
    {
        email = root["email"].asString();
    }

    return 0;
}

// 封装注册用户的json
int encodeRegisterJson(int ret, string &str_json)
{
    Json::Value root;
    root["code"] = ret;
    Json::FastWriter writer;
    str_json = writer.write(root);
    return 0;
}

template <typename... Args>
std::string formatString2(const std::string &format, Args... args)
{
    auto size = std::snprintf(nullptr, 0, format.c_str(), args...) + 1; // Extra space for '\0'
    std::unique_ptr<char[]> buf(new char[size]);
    std::snprintf(buf.get(), size, format.c_str(), args...);
    return std::string(buf.get(), buf.get() + size - 1); // We don't want the '\0' inside
}
int registerUser(string &user_name, string &nick_name, string &pwd, string &phone, string &email)
{
    int ret = 0;
    uint32_t user_id;
    CDBManager *pDBManager = CDBManager::getInstance();
    CDBConn *pDBConn = pDBManager->GetDBConn("tuchuang_slave");
    AUTO_REL_DBCONN(pDBManager, pDBConn);
    // 先查看用户是否存在
    string strSql;
    // strSql =  "select * from user_info where user_name=" +  \' + user_name +  "/"";
    strSql = formatString2("select * from user_info where user_name='%s'", user_name.c_str());
    CResultSet *pResultSet = pDBConn->ExecuteQuery(strSql.c_str());
    if (pResultSet && pResultSet->Next())
    { // 检测是否存在用户记录
        // 存在在返回
        LOG_WARN << "id: " << pResultSet->GetInt("id") << ", user_name: " << pResultSet->GetString("user_name") << " 已经存在";
        delete pResultSet;
        ret = 2;
    }
    else
    { // 如果不存在则注册
        time_t now;
        char create_time[TIME_STRING_LEN];
        //获取当前时间
        now = time(NULL);
        strftime(create_time, TIME_STRING_LEN - 1, "%Y-%m-%d %H:%M:%S", localtime(&now));
        strSql = "insert into user_info (`user_name`,`nick_name`,`password`,`phone`,`email`,`create_time`) values(?,?,?,?,?,?)";
        LOG_INFO << "执行: " << strSql;
        // 必须在释放连接前delete CPrepareStatement对象，否则有可能多个线程操作mysql对象，会crash
        CPrepareStatement *stmt = new CPrepareStatement();
        if (stmt->Init(pDBConn->GetMysql(), strSql))
        {
            uint32_t index = 0;
            string c_time = create_time;
            stmt->SetParam(index++, user_name);
            stmt->SetParam(index++, nick_name);
            stmt->SetParam(index++, pwd);
            stmt->SetParam(index++, phone);
            stmt->SetParam(index++, email);
            stmt->SetParam(index++, c_time);
            bool bRet = stmt->ExecuteUpdate();
            if (bRet)
            {
                ret = 0;
                user_id = pDBConn->GetInsertId();
                LOG_INFO << "insert user " << user_id;
            }
            else
            {
                LOG_ERROR << "insert user_info failed. " << strSql;
                ret = 1;
            }
        }
        delete stmt;
    }

    return ret;
}

int ApiRegisterUser(string &url, string &post_data, string &str_json)
{
    UNUSED(url);
    int ret = 0;
    string user_name;
    string nick_name;
    string pwd;
    string phone;
    string email;

    // 判断数据是否为空
    if (post_data.empty())
    {
        return -1;
    }
    // 解析json
    if (decodeRegisterJson(post_data, user_name, nick_name, pwd, phone, email) < 0)
    {
        LOG_ERROR << "decodeRegisterJson failed";
        encodeRegisterJson(1, str_json);
        return -1;
    }

    // 注册账号
    ret = registerUser(user_name, nick_name, pwd, phone, email);
    encodeRegisterJson(ret, str_json);

    return ret;
}
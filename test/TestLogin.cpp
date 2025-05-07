#include "ApiLogin.h"
#include "util.h"
#include "Common.h"
#define TEST_LOGIN_COUNT 200

void SingeThreadTestLogin()
{
    g_logLevel = Logger::WARN;
    // 设置传递要解析的json数据
    string post_data = "{\"user\": \"qingfuliao\", \"pwd\": \"e10adc3949ba59abbe56e057f20f883e\"}";
    string url = "/api/login";
    string str_json;

    // 获取开始时间    
    uint64_t cur_time = GetTickCount();  // 单位ms
    for(int i = 0; i < TEST_LOGIN_COUNT; i++)
    {
        ApiUserLogin(url, post_data, str_json);
        LOG_INFO << "str_json: " << str_json;
    } 
    // 获取结束时间
    uint64_t end_time = GetTickCount(); // 单位ms
    double   one_login_time =  (end_time - cur_time)*1.0/TEST_LOGIN_COUNT;
    LOG_WARN <<  "Login " <<  TEST_LOGIN_COUNT  << " times need the time: " << end_time - cur_time << "ms, average time: " << one_login_time << "ms, qps: " << long(1000/one_login_time);
}



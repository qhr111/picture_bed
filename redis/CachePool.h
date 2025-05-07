/*
 * @Author: your name
 * @Date: 2019-12-07 10:54:57
 * @LastEditTime : 2020-01-10 16:35:13
 * @LastEditors  : Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \src\cache_pool\CachePool.h
 */
#ifndef CACHEPOOL_H_
#define CACHEPOOL_H_

#include <iostream>
#include <vector>
#include <map>
#include <list>
#include <mutex>
#include <condition_variable>

#include "hiredis.h"

using std::list;
using std::map;
using std::string;
using std::vector;

#define REDIS_COMMAND_SIZE 300			/* redis Command 指令最大长度 */
#define FIELD_ID_SIZE 100				/* redis hash表field域字段长度 */
#define VALUES_ID_SIZE 1024				/* redis        value域字段长度 */
typedef char (*RFIELDS)[FIELD_ID_SIZE]; /* redis hash表存放批量field字符串数组类型 */

//数组指针类型，其变量指向 char[1024]
typedef char (*RVALUES)[VALUES_ID_SIZE]; /* redis 表存放批量value字符串数组类型 */

class CachePool;

class CacheConn
{
public:
	CacheConn(const char *server_ip, int server_port, int db_index, const char *password,
			  const char *pool_name = "");
	CacheConn(CachePool *pCachePool);
	virtual ~CacheConn();

	int Init();
	void DeInit();
	const char *GetPoolName();
	// 通用操作
	// 判断一个key是否存在
	bool isExists(string &key);
	// 删除某个key
	long del(string key);

	// ------------------- 字符串相关 -------------------
	string get(string key);
	string set(string key, string value);
	string setex(string key, int timeout, string value);

	// string mset(string key, map);
	//批量获取
	bool mget(const vector<string> &keys, map<string, string> &ret_value);
	//原子加减1
	int incr(string key, int64_t &value);
	int decr(string key, int64_t &value);

	// ---------------- 哈希相关 ------------------------
	long hdel(string key, string field);
	string hget(string key, string field);
	int hget(string key, char *field, char *value);
	bool hgetAll(string key, map<string, string> &ret_value);
	long hset(string key, string field, string value);

	long hincrBy(string key, string field, long value);
	long incrBy(string key, long value);
	string hmset(string key, map<string, string> &hash);
	bool hmget(string key, list<string> &fields, list<string> &ret_value);

	// ------------ 链表相关 ------------
	long lpush(string key, string value);
	long rpush(string key, string value);
	long llen(string key);
	bool lrange(string key, long start, long end, list<string> &ret_value);

	// zset 相关
	int ZsetExit(string key, string member);
	int ZsetAdd(string key, long score, string member);
	int ZsetZrem(string key, string member);
	int ZsetIncr(string key, string member);
	int ZsetZcard(string key);
	int ZsetZrevrange(string key, int from_pos, int end_pos, RVALUES values, int &get_num);
	int ZsetGetScore(string key, string member);

	bool flushdb();

private:
	CachePool *m_pCachePool;
	redisContext *m_pContext; // 每个redis连接 redisContext redis客户端编程的对象
	uint64_t m_last_connect_time;
	uint16_t m_server_port;
	string m_server_ip;
	string m_password;
	uint16_t m_db_index;
	string m_pool_name;
};

class CachePool
{
public:
	// db_index和mysql不同的地方
	CachePool(const char *pool_name, const char *server_ip, int server_port, int db_index,
			  const char *password, int max_conn_cnt);
	virtual ~CachePool();

	int Init();
	// 获取空闲的连接资源
	CacheConn *GetCacheConn(const int timeout_ms = 0);
	// Pool回收连接资源
	void RelCacheConn(CacheConn *pCacheConn);

	const char *GetPoolName() { return m_pool_name.c_str(); }
	const char *GetServerIP() { return m_server_ip.c_str(); }
	const char *GetPassword() { return m_password.c_str(); }
	int GetServerPort() { return m_server_port; }
	int GetDBIndex() { return m_db_index; }

private:
	string m_pool_name;
	string m_server_ip;
	string m_password;
	int m_server_port;
	int m_db_index; // mysql 数据库名字， redis db index

	int m_cur_conn_cnt;
	int m_max_conn_cnt;
	list<CacheConn *> m_free_list;

	std::mutex m_mutex;
	std::condition_variable m_cond_var;
	bool m_abort_request = false;
};

class CacheManager
{
public:
	virtual ~CacheManager();

	static CacheManager *getInstance();

	int Init();
	CacheConn *GetCacheConn(const char *pool_name);
	void RelCacheConn(CacheConn *pCacheConn);

private:
	CacheManager();

private:
	static CacheManager *s_cache_manager;
	map<string, CachePool *> m_cache_pool_map;
};

class AutoRelCacheCon
{
public:
	AutoRelCacheCon(CacheManager *manger, CacheConn *conn) : manger_(manger), conn_(conn) {}
	~AutoRelCacheCon()
	{
		if (manger_)
		{
			manger_->RelCacheConn(conn_);
		}
	} //在析构函数规划
private:
	CacheManager *manger_ = NULL;
	CacheConn *conn_ = NULL;
};

#define AUTO_REL_CACHECONN(m, c) AutoRelCacheCon autorelcacheconn(m, c)

#endif /* CACHEPOOL_H_ */

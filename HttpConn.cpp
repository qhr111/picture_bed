/*
 * HttpConn.cpp
 *  Modify on: 2022-10-30
 * 		Author: darren
 *  Created on: 2013-9-29
 *      Author: ziteng@mogujie.com
 */

#include "HttpConn.h"
#include "HttpParserWrapper.h"
#include "ThreadPool.h"
#include "Common.h"
#include "ApiRegister.h"
#include "ApiLogin.h"
#include "ApiMyfiles.h"
#include "ApiSharefiles.h"
#include "ApiDealfile.h"
#include "ApiDealsharefile.h"
#include "ApiSharepicture.h"
#include "ApiMd5.h"
#include "ApiUpload.h"
static HttpConnMap_t g_http_conn_map;

extern string strMsfsUrl;
extern string strDiscovery;

// conn_handle 从0开始递增，可以防止因socket handle重用引起的一些冲突
static uint32_t g_conn_handle_generator = 0;

typedef hash_map<uint32_t, CHttpConn*> UserMap_t;
static UserMap_t g_uuid_conn_map;

ThreadPool g_thread_pool;
uint32_t CHttpConn::s_uuid_alloctor = 0;

std::mutex CHttpConn::s_resp_mutex;
std::list<ResponsePdu_t*>	CHttpConn::s_response_pdu_list;



CHttpConn *FindHttpConnByHandle(uint32_t conn_handle)
{
	CHttpConn *pConn = NULL;
	HttpConnMap_t::iterator it = g_http_conn_map.find(conn_handle);
	if (it != g_http_conn_map.end())
	{
		pConn = it->second;
	}

	return pConn;
}


CHttpConn* get_http_conn_by_uuid(uint32_t uuid)
{
	CHttpConn* pConn = NULL;
	UserMap_t::iterator it = g_uuid_conn_map.find(uuid);
	if (it != g_uuid_conn_map.end()) {
		pConn = (CHttpConn *)it->second;
	}

	return pConn;
}
void httpconn_callback(void *callback_data, uint8_t msg, uint32_t handle, uint32_t uParam, void *pParam)
{
	NOTUSED_ARG(uParam);
	NOTUSED_ARG(pParam);

	// convert void* to uint32_t, oops
	uint32_t conn_handle = *((uint32_t *)(&callback_data));
	CHttpConn *pConn = FindHttpConnByHandle(conn_handle);
	if (!pConn)
	{
		return;
	}

	switch (msg)
	{
	case NETLIB_MSG_READ:
		pConn->OnRead();
		break;
	case NETLIB_MSG_WRITE:
		pConn->OnWrite();
		break;
	case NETLIB_MSG_CLOSE:
		pConn->OnClose();
		break;
	default:
		LOG_ERROR << "!!!httpconn_callback error msg:" << msg;
		break;
	}
}

void http_conn_timer_callback(void *callback_data, uint8_t msg, uint32_t handle, void *pParam)
{
	UNUSED(pParam);
	CHttpConn *pConn = NULL;
	HttpConnMap_t::iterator it, it_old;
	uint64_t cur_time = GetTickCount();

	for (it = g_http_conn_map.begin(); it != g_http_conn_map.end();)
	{
		it_old = it;
		it++;

		pConn = it_old->second;
		pConn->OnTimer(cur_time);
	}
}

void init_http_conn()
{
	netlib_register_timer(http_conn_timer_callback, NULL, 1000);
}

//////////////////////////
CHttpConn::CHttpConn()
{
	m_busy = false;
	m_sock_handle = NETLIB_INVALID_HANDLE;
	m_state = CONN_STATE_IDLE;

	m_last_send_tick = m_last_recv_tick = GetTickCount();
	m_conn_handle = ++g_conn_handle_generator;
	if (m_conn_handle == 0)
	{
		m_conn_handle = ++g_conn_handle_generator;
	}

	m_uuid = ++CHttpConn::s_uuid_alloctor;
	if (m_uuid == 0) {
		m_uuid = ++CHttpConn::s_uuid_alloctor;
	}
	g_uuid_conn_map.insert(make_pair(m_uuid, this));
	// LOG_INFO << "CHttpConn, handle = " << m_conn_handle;
	// LOG_INFO << "conn_uuid: " << m_uuid << ", pConn: " << this;
}

CHttpConn::~CHttpConn()
{
	// LOG_INFO << "~CHttpConn, handle = " << m_conn_handle;
}

int CHttpConn::Send(void *data, int len)
{
	m_last_send_tick = GetTickCount();

	if (m_busy)
	{
		m_out_buf.Write(data, len);
		return len;
	}

	int ret = netlib_send(m_sock_handle, data, len);
	if (ret < 0)
		ret = 0;

	if (ret < len)
	{
		m_out_buf.Write((char *)data + ret, len - ret);
		m_busy = true;
		LOG_INFO << "not send all, remain=" << m_out_buf.GetWriteOffset();
	}
	else
	{
		OnWriteComlete();
	}

	return len;
}

void CHttpConn::Close()
{
	m_state = CONN_STATE_CLOSED;

	g_http_conn_map.erase(m_conn_handle);
	g_uuid_conn_map.erase(m_uuid);		// 移除uuid
	netlib_close(m_sock_handle);

	ReleaseRef();
}

void CHttpConn::OnConnect(net_handle_t handle)
{
	// LOG_INFO << "CHttpConn, handle = " << handle;
	m_sock_handle = handle;
	m_state = CONN_STATE_CONNECTED;
	g_http_conn_map.insert(make_pair(m_conn_handle, this));

	netlib_option(handle, NETLIB_OPT_SET_CALLBACK, (void *)httpconn_callback);
	netlib_option(handle, NETLIB_OPT_SET_CALLBACK_DATA, reinterpret_cast<void *>(m_conn_handle));
	netlib_option(handle, NETLIB_OPT_GET_REMOTE_IP, (void *)&m_peer_ip);
}

void CHttpConn::OnRead() // CHttpConn业务层面的OnRead
{
	for (;;)
	{
		uint32_t free_buf_len = m_in_buf.GetAllocSize() - m_in_buf.GetWriteOffset();
		if (free_buf_len < READ_BUF_SIZE + 1)
			m_in_buf.Extend(READ_BUF_SIZE + 1);

		int ret = netlib_recv(m_sock_handle, m_in_buf.GetBuffer() + m_in_buf.GetWriteOffset(), READ_BUF_SIZE);
		if (ret <= 0)
			break;

		m_in_buf.IncWriteOffset(ret);

		m_last_recv_tick = GetTickCount();
	}

	// 每次请求对应一个HTTP连接，所以读完数据后，不用在同一个连接里面准备读取下个请求
	char *in_buf = (char *)m_in_buf.GetBuffer();
	uint32_t buf_len = m_in_buf.GetWriteOffset();
	in_buf[buf_len] = '\0';

	// 如果buf_len 过长可能是受到攻击，则断开连接
	// 正常的url最大长度为2048，我们接受的所有数据长度不得大于2K
	if (buf_len > 2048)
	{
		LOG_ERROR << "get too much data: " << in_buf;
		Close();
		return;
	}

	LOG_DEBUG << "buf_len: " << buf_len << ", m_conn_handle: " << m_conn_handle << ", in_buf: " << in_buf;
	// 解析http数据
	m_cHttpParser.ParseHttpContent(in_buf, buf_len);
	if (m_cHttpParser.IsReadAll())
	{
		string url = m_cHttpParser.GetUrl();
		string content = m_cHttpParser.GetBodyContent();
		LOG_INFO << "url: " << url; // for debug
		if (strncmp(url.c_str(), "/api/reg", 8) == 0)
		{ // 注册
			_HandleRegisterRequest(url, content);
		}
		else if (strncmp(url.c_str(), "/api/login", 10) == 0)
		{ // 登录
			auto f1 = g_thread_pool.exec(std::bind(&CHttpConn::_HandleLoginRequest, this,
				 std::placeholders::_1, std::placeholders::_2),  url, content);
			// _HandleLoginRequest(url, content);
		}
		else if (strncmp(url.c_str(), "/api/myfiles", 10) == 0)
		{ //
			_HandleMyfilesRequest(url, content);
		}
		else if (strncmp(url.c_str(), "/api/sharefiles", 15) == 0)
		{ //
			_HandleSharefilesRequest(url, content);
		}
		else if (strncmp(url.c_str(), "/api/dealfile", 13) == 0)
		{ //
			_HandleDealfileRequest(url, content);
		}
		else if (strncmp(url.c_str(), "/api/dealsharefile", 18) == 0)
		{ //
			_HandleDealsharefileRequest(url, content);
		}
		else if (strncmp(url.c_str(), "/api/sharepic", 13) == 0)
		{											  //
			_HandleSharepictureRequest(url, content); // 处理
		}
		else if (strncmp(url.c_str(), "/api/md5", 8) == 0)
		{									 //
			_HandleMd5Request(url, content); // 处理
		}
		else if (strncmp(url.c_str(), "/api/upload", 11) == 0)
		{ // 上传
			_HandleUploadRequest(url, content);
		}
		else
		{
			LOG_ERROR << "url unknown, url= " << url;
			Close();
		}
	}
}

void CHttpConn::OnWrite()
{
	if (!m_busy)
		return;

	// LOG_INFO << "send: " << m_out_buf.GetWriteOffset();
	int ret = netlib_send(m_sock_handle, m_out_buf.GetBuffer(), m_out_buf.GetWriteOffset());
	if (ret < 0)
		ret = 0;

	int out_buf_size = (int)m_out_buf.GetWriteOffset();

	m_out_buf.Read(NULL, ret);

	if (ret < out_buf_size)
	{
		m_busy = true;
		LOG_INFO << "not send all, remain = " << m_out_buf.GetWriteOffset();
	}
	else
	{
		OnWriteComlete();
		m_busy = false;
	}
}

void CHttpConn::OnClose()
{
	Close();
}

void CHttpConn::OnTimer(uint64_t curr_tick)
{
	if (curr_tick > m_last_recv_tick + HTTP_CONN_TIMEOUT)
	{
		LOG_WARN  << "HttpConn timeout, handle=" << m_conn_handle;
		Close();
	}
}
/*
OnRead, buf_len=1321, conn_handle=2, POST /api/upload HTTP/1.0
Host: 127.0.0.1:8081
Connection: close
Content-Length: 722
Accept: application/json, text/plain,
User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/106.0.0.0 Safari/537.36 Edg/106.0.1370.52
Content-Type: multipart/form-data; boundary=----WebKitFormBoundaryjWE3qXXORSg2hZiB
Origin: http://114.215.169.66
Referer: http://114.215.169.66/myFiles
Accept-Encoding: gzip, deflate
Accept-Language: zh-CN,zh;q=0.9,en;q=0.8,en-GB;q=0.7,en-US;q=0.6
Cookie: userName=qingfuliao; token=e4252ae6e49176d51a5e87b41b6b9312

------WebKitFormBoundaryjWE3qXXORSg2hZiB
Content-Disposition: form-data; name="file_name"

config.ini
------WebKitFormBoundaryjWE3qXXORSg2hZiB
Content-Disposition: form-data; name="file_content_type"

application/octet-stream
------WebKitFormBoundaryjWE3qXXORSg2hZiB
Content-Disposition: form-data; name="file_path"

/root/tmp/5/0034880075
------WebKitFormBoundaryjWE3qXXORSg2hZiB
Content-Disposition: form-data; name="file_md5"

10f06f4707e9d108e9a9838de0f8ee33
------WebKitFormBoundaryjWE3qXXORSg2hZiB
Content-Disposition: form-data; name="file_size"

20
------WebKitFormBoundaryjWE3qXXORSg2hZiB
Content-Disposition: form-data; name="user"

qingfuliao
------WebKitFormBoundaryjWE3qXXORSg2hZiB--
*/
// Add By Lanhu 2014-12-19 通过登陆IP来优选电信还是联通IP

int CHttpConn::_HandleUploadRequest(string &url, string &post_data)
{
	string str_json;
	int ret = ApiUpload(url, post_data, str_json);
	char *szContent = new char[HTTP_RESPONSE_HTML_MAX];
	uint32_t nLen = str_json.length();
	snprintf(szContent, HTTP_RESPONSE_HTML_MAX, HTTP_RESPONSE_HTML, nLen, str_json.c_str());
	ret = Send((void *)szContent, strlen(szContent));   // 返回值暂时不做处理
	delete [] szContent;
	return 0;
}

// 账号注册处理
int CHttpConn::_HandleRegisterRequest(string &url, string &post_data)
{
	string str_json;
	int ret = ApiRegisterUser(url, post_data, str_json); // 处理请求的json、返回封装的json
	char *szContent = new char[HTTP_RESPONSE_HTML_MAX];
	uint32_t nLen = str_json.length();
	snprintf(szContent, HTTP_RESPONSE_HTML_MAX, HTTP_RESPONSE_HTML, nLen, str_json.c_str());
	LOG_INFO << "szContent: " << szContent;
	ret = Send((void *)szContent, strlen(szContent));
	delete [] szContent;
	return 0;
}
// 账号登陆处理 /api/login
// int CHttpConn::_HandleLoginRequest(string &url, string &post_data)
// {
// 	string str_json;
// 	int ret = ApiUserLogin(url, post_data, str_json);
// 	char *szContent = new char[HTTP_RESPONSE_HTML_MAX];
// 	uint32_t nLen = str_json.length();
// 	snprintf(szContent, HTTP_RESPONSE_HTML_MAX, HTTP_RESPONSE_HTML, nLen, str_json.c_str());
// 	ret = Send((void *)szContent, strlen(szContent));

// 	return 0;
// }

int CHttpConn::_HandleLoginRequest(string &url, string &post_data)
{
	LOG_INFO << "into";
	string str_json;
	int ret = ApiUserLogin(url, post_data, str_json);
	char *szContent = new char[HTTP_RESPONSE_HTML_MAX];
	uint32_t nLen = str_json.length();
	snprintf(szContent, HTTP_RESPONSE_HTML_MAX, HTTP_RESPONSE_HTML, nLen, str_json.c_str());
	CHttpConn::AddResponseData(m_uuid, szContent);
	delete szContent;

	return 0;
}

//
int CHttpConn::_HandleDealfileRequest(string &url, string &post_data)
{
	string str_json;
	int ret = ApiDealfile(url, post_data, str_json);
	char *szContent = new char[HTTP_RESPONSE_HTML_MAX];
	uint32_t nLen = str_json.length();
	snprintf(szContent, HTTP_RESPONSE_HTML_MAX, HTTP_RESPONSE_HTML, nLen, str_json.c_str());
	ret = Send((void *)szContent, strlen(szContent));
	delete [] szContent;
	return 0;
}
//
int CHttpConn::_HandleDealsharefileRequest(string &url, string &post_data)
{
	string str_json;
	int ret = ApiDealsharefile(url, post_data, str_json);
	char *szContent = new char[HTTP_RESPONSE_HTML_MAX];
	uint32_t nLen = str_json.length();
	snprintf(szContent, HTTP_RESPONSE_HTML_MAX, HTTP_RESPONSE_HTML, nLen, str_json.c_str());
	ret = Send((void *)szContent, strlen(szContent));
	delete [] szContent;
	return 0;
}
//
int CHttpConn::_HandleMd5Request(string &url, string &post_data)
{
	string str_json;
	int ret = ApiMd5(url, post_data, str_json);
	char *szContent = new char[HTTP_RESPONSE_HTML_MAX]; // 注意buffer的长度
	uint32_t nLen = str_json.length();
	LOG_INFO << "json size:" << str_json.size();
	snprintf(szContent, HTTP_RESPONSE_HTML_MAX, HTTP_RESPONSE_HTML, nLen, str_json.c_str());
	ret = Send((void *)szContent, strlen(szContent));
	delete [] szContent;
	return 0;
}
//
int CHttpConn::_HandleMyfilesRequest(string &url, string &post_data)
{
	string str_json;
	int ret = ApiMyfiles(url, post_data, str_json);
	char *szContent = new char[HTTP_RESPONSE_HTML_MAX]; // 注意buffer的长度
	uint32_t nLen = str_json.length();
	// LOG_INFO << "json size:" << str_json.size();
	snprintf(szContent, HTTP_RESPONSE_HTML_MAX, HTTP_RESPONSE_HTML, nLen, str_json.c_str());
	ret = Send((void *)szContent, strlen(szContent));
	delete [] szContent;
	return 0;
}
//
int CHttpConn::_HandleSharefilesRequest(string &url, string &post_data)
{
	string str_json;
	int ret = ApiSharefiles(url, post_data, str_json);
	char *szContent = new char[HTTP_RESPONSE_HTML_MAX];
	uint32_t nLen = str_json.length();
	snprintf(szContent, HTTP_RESPONSE_HTML_MAX, HTTP_RESPONSE_HTML, nLen, str_json.c_str());
	ret = Send((void *)szContent, strlen(szContent));
	delete [] szContent;
	return 0;
}
//
int CHttpConn::_HandleSharepictureRequest(string &url, string &post_data)
{
	string str_json;
	int ret = ApiSharepicture(url, post_data, str_json);
	char *szContent = new char[HTTP_RESPONSE_HTML_MAX];
	uint32_t nLen = str_json.length();
	snprintf(szContent, HTTP_RESPONSE_HTML_MAX, HTTP_RESPONSE_HTML, nLen, str_json.c_str());
	ret = Send((void *)szContent, strlen(szContent));
	delete [] szContent;
	return 0;
}

void CHttpConn::OnWriteComlete()
{
	LOG_INFO  << "write complete";
	Close();
}

void CHttpConn::AddResponseData(uint32_t conn_uuid, string resp_data)
{
	LOG_INFO << "into";
	ResponsePdu_t* pResp = new ResponsePdu_t;
	pResp->conn_uuid = conn_uuid;
	pResp->resp_data = std::move(resp_data);

	s_resp_mutex.lock();
	s_response_pdu_list.push_back(pResp);
	s_resp_mutex.unlock();
}

void CHttpConn::SendResponseDataList()
{
	// LOG_INFO << "into";
	// 发送数据
	s_resp_mutex.lock();
	while (!s_response_pdu_list.empty()) {
		ResponsePdu_t* pResp = s_response_pdu_list.front();
		s_response_pdu_list.pop_front();
		s_resp_mutex.unlock();
		CHttpConn* pConn = get_http_conn_by_uuid(pResp->conn_uuid);  // 该连接有可能已经被释放，如果被释放则返回NULL
		LOG_INFO << "conn_uuid: " << pResp->conn_uuid << ", pConn: " << pConn;
		if (pConn) {
			LOG_INFO << "send: " << pResp->resp_data;
			pConn->Send((void *)pResp->resp_data.c_str(), pResp->resp_data.size());
		}
		delete pResp;

		s_resp_mutex.lock();
	}

	s_resp_mutex.unlock();
}

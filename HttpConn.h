/*
 * HttpConn.h
 *
 *  Created on: 2013-9-29
 *      Author: ziteng
 */

#ifndef __HTTP_CONN_H__
#define __HTTP_CONN_H__

#include <list>
#include <mutex>
#include "netlib.h"
#include "util.h"
#include "HttpParserWrapper.h"
#include "ThreadPool.h"
#define HTTP_CONN_TIMEOUT			60000

#define READ_BUF_SIZE	2048


#define HTTP_RESPONSE_HTML          "HTTP/1.1 200 OK\r\n"\
                                    "Connection:close\r\n"\
                                    "Content-Length:%d\r\n"\
                                    "Content-Type:application/json;charset=utf-8\r\n\r\n%s"
                                    
#define HTTP_RESPONSE_HTML_MAX      4096


extern ThreadPool g_thread_pool;
enum {
    CONN_STATE_IDLE,
    CONN_STATE_CONNECTED,
    CONN_STATE_OPEN,
    CONN_STATE_CLOSED,
};

typedef struct {
	uint32_t	conn_uuid;      // 用于查找connection
	string		resp_data;      // 要回发的数据
} ResponsePdu_t;


class CHttpConn : public CRefObject
{
public:
	CHttpConn();
	virtual ~CHttpConn();

	uint32_t GetConnHandle() { return m_conn_handle; }
	char* GetPeerIP() { return (char*)m_peer_ip.c_str(); }

	int Send(void* data, int len);

    void Close();
    void OnConnect(net_handle_t handle);
    void OnRead();
    void OnWrite();
    void OnClose();
    void OnTimer(uint64_t curr_tick);
    void OnWriteComlete();

    static void AddResponseData(uint32_t conn_uuid, string resp_data);	// 工作线程调用
	static void SendResponseDataList();	// 主线程调用
private:
    // 文件上传处理
    int _HandleUploadRequest(string& url, string& post_data);
    // 账号注册处理
    int _HandleRegisterRequest(string& url, string& post_data);
    // 账号登陆处理
    int _HandleLoginRequest(string& url, string& post_data);
    //  
    int _HandleDealfileRequest(string& url, string& post_data);
    // 
    int _HandleDealsharefileRequest(string& url, string& post_data);
    //
    int _HandleMd5Request(string& url, string& post_data);
    // 
    int _HandleMyfilesRequest(string& url, string& post_data);
    // 
    int _HandleSharefilesRequest(string& url, string& post_data);
    // 
    int _HandleSharepictureRequest(string& url, string& post_data);
protected:
	net_handle_t	m_sock_handle;
	uint32_t		m_conn_handle;
	bool			m_busy;

    uint32_t        m_state;
	std::string		m_peer_ip;
	uint16_t		m_peer_port;
	CSimpleBuffer	m_in_buf;
	CSimpleBuffer	m_out_buf;

	uint64_t		m_last_send_tick;
	uint64_t		m_last_recv_tick;
    
    CHttpParserWrapper m_cHttpParser;

    static uint32_t	s_uuid_alloctor;  // uuid分配
	uint32_t		m_uuid;  // 自己的uuid

    static std::mutex s_resp_mutex;
    static std::list<ResponsePdu_t*>	s_response_pdu_list;	// 主线程发送回复消息
};

typedef hash_map<uint32_t, CHttpConn*> HttpConnMap_t;

CHttpConn* FindHttpConnByHandle(uint32_t handle);
void init_http_conn();
CHttpConn* get_http_conn_by_uuid(uint32_t uuid);



#endif /* IMCONN_H_ */

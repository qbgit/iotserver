
#ifdef WIN32
#include <WinSock2.h>
#include <windows.h>
#pragma comment(lib,"ws2_32.lib")
#pragma comment(lib,"psapi.lib")
#pragma comment(lib,"Iphlpapi.lib")
#pragma comment(lib,"Userenv.lib")
#endif


#include "uv.h"
#include "protocol.h"
#include "protocol2.h"
#include "tcp_server.h"
#include "util.h"
#include <map>
#include <iostream>
//#include <iomanip>
//#include <stdlib.h> 



using namespace std;

#define lock_     uv_mutex_lock(&_mutex_1)
#define unlock_   uv_mutex_unlock(&_mutex_1)
#define initlock_ uv_mutex_init(&_mutex_1)
class tcp_server1 :public tcp_server
{
	//所有客戶端，用四字节整形数作为hash key
	std::map<uint32_t, client_t*>  _map_c;
	uv_mutex_t  _mutex_1;
public:
	tcp_server1(){}
	~tcp_server1(){}
protected:
	uint32_t getdeviceid(client_t *client)
	{
		tcp_settings *cnf = client->config;
		char * idpos = &client->head[0] + cnf->idoffset;
		uint32_t deviceid =htonl( *((uint32_t*)idpos));
		///((deviceid >> 16) & 0xff00) | deviceid >> 24

		return deviceid;
	}
public:



	int on_headers_complete(void *param) {
		client_t * pclient = (client_t*)param;

		//printf("the header len is %d\n", pclient->recvlen);
		printf("the deviceid is %02x\n", getdeviceid(pclient));
		return 0;
	}
	//该函数没有进入线程池
	int on_message_complete(void *param) {
		client_t * pclient = (client_t*)param;
		tcp_unit * data = pclient->buffer_data;
		char * buf = data->data;
		int len = data->tlen;
		//printf("the total len is %d\n", pclient->buffer_data->tlen);
		return 0;
	}
	//该函数进入线程池
	int on_data(tcp_unit * data) {
		//printf("the thread pid is %d\n", _getpid());
#ifdef _DEBUG
		int hl = data->headlen;
		//printf("the hl is %d\n", hl);
		for (size_t i = hl; i < hl+8; i++) {
			printf("%02x ", data->data[i]);
		}
		printf("\n");
#endif



#if 0
		//给客户端回送信息

		tcp_unit_w * unitw = (tcp_unit_w*)malloc(sizeof(tcp_unit_w));
		unitw->delay = 0;
		unitw->data = (char*)malloc(sizeof(char) * 18);
		memcpy(unitw->data, buf, 18);
		unitw->len = 18;
		unitw->next = NULL;
		unitw->type = enum_sc;
		client_send(rb->id, unitw);
		//解析成为json发送到相应的接口
		string response;
		int timeout = 2; //2秒
						 //post是同步的

						 /*	bool ret = client.Post("http://127.0.0.1:9069/sensor_data/8052", buf, len, response, timeout);
						 if (ret == false) {
						 printf("error!:%s\n", client.geterror().c_str());
						 }
						 printf("response:%s\n", response.c_str());*/
#endif
		return 0;
	}

	
};


static const char *http_content =
"HTTP/1.1 %s\r\nContent-Type: %s\r\nConnection: keep-alive\r\n"
"Content-Length: %d\r\n"
"Access-Control-Allow-Origin: *\r\n"
"\r\n";

int get_http_content_baselen()
{
	return strlen(http_content)-6;
}

int main(int argc, char* argv[]) {

	config c;
	cout<<c.readdefault()<<endl;
	return 0;

	uint16_t port = 8051;
	uv_loop_t *uv_loop = uv_default_loop();
	tcp_server1 tcpserver;
	tcpserver.tcp_init("./config_flv.json", uv_loop);
	tcpserver.start("0.0.0.0", port);
	printf("the server is at %d running!\n", port);
	uv_run(uv_loop, UV_RUN_DEFAULT);
	return 0;
}
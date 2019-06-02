#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uv.h>

#define DEFAULT_IP "127.0.0.1"
#define DEFAULT_PORT 7001

struct sockaddr_in dest;
uv_timer_t*  timer;
uv_loop_t* loop;
uv_write_t write_req;

void write_cb(uv_write_t* req, int status)
{
	if (status < 0) {
		printf("%s\n", uv_strerror(status));
		return;
	}
	free(req);
}

void timer_cb(uv_timer_t* timer)
{
	uv_buf_t buff = uv_buf_init("hellohappynihaowoshi", 20);

	uv_connect_t* connect = (uv_connect_t*)timer->data;
	uv_stream_t* tcp = connect->handle;

	int buf_count = 1;

	uv_write(&write_req, tcp, &buff, buf_count, write_cb);
}

void on_connect(uv_connect_t* connect, int status)
{
	if (status < 0) {
		printf("connect error: %s!\n", uv_strerror(status));
		return;
	}
	printf("connect success!");

	timer = (uv_timer_t*)calloc(sizeof(uv_timer_t), 1);
	timer->data = connect;
	uv_timer_init(loop, timer);

	uv_timer_start(timer, timer_cb, 0, 300);

}

typedef struct client_context
{
	uv_loop_t * loop = NULL;
	uv_tcp_t  socket;
	uv_connect_t connect;
	//const char * ip;
	//uint16_t port;
	struct sockaddr_in dest;
	uv_write_t write_req;
}client_context;


int send(uint8_t * data, size_t len)
{
	return 0;
}
void init_client_context(client_context * cc,const char * ip,uint16_t port)
{
	if (cc == NULL)
		return;
	cc->loop = uv_default_loop();
	uv_tcp_init(cc->loop, &cc->socket);
	//cc->ip = ip;
	//cc->port = port;
	uv_ip4_addr(ip, port, &dest);
}

int connect(client_context * cc)
{
	if (cc == NULL)
		return -1;
	int ret = uv_tcp_connect(&cc->connect, &cc->socket, (const struct sockaddr*)&dest, on_connect);
	if (ret) {
		fprintf(stderr, "Connect error: %s\n", uv_strerror(ret));
		return 0;
	}
	return uv_run(loop, UV_RUN_DEFAULT);

}

int main1(int argc, char **argv) {




	loop = uv_default_loop();
	uv_tcp_t  socket;
	uv_tcp_init(loop, &socket);

	uv_connect_t connect;

	uv_ip4_addr(DEFAULT_IP, DEFAULT_PORT, &dest);
	int ret = uv_tcp_connect(&connect, &socket, (const struct sockaddr*)&dest, on_connect);
	if (ret) {
		fprintf(stderr, "Connect error: %s\n", uv_strerror(ret));
		return 0;
	}
	return uv_run(loop, UV_RUN_DEFAULT);
}
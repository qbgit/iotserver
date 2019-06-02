#pragma once

#include "uv.h"
#include "protocol.h"
#include "util.h"
#include <map>
#include <iostream>

using namespace std;
#define MAX_WRITE_HANDLES 1000


class tcp_server
{
	//包头和包体的
	tcp_settings _set;
	//tcpserver
	uv_tcp_t * _server = NULL;
public:
	tcp_server(){}
	~tcp_server() {}
protected:
	//客户端断开连接
	static void on_close(uv_handle_t* handle) {
		//客户端下线
		client_t* client = (client_t*)handle->data;
		client->clean();
		delete client;
		//client_offline(client->deviceid);
	}
	static void alloc_cb(uv_handle_t * handle, size_t suggested_size, uv_buf_t* buf) {
		int buflen = 0;
		int headlen = 0;
		int bodylen = 0;
		client_t* client = (client_t*)handle->data;
		char *head = &(client->head[0]);     //数据接收的头部
		char *pos = head + client->recvlen; //位置指向数据已经接收的下一个字节
		//得到头部长度
		headlen = get_headlen(client->config);
		if (client->status == enum_head) //接收头部字节
		{
			buflen = headlen - client->recvlen;
			*buf = uv_buf_init(pos, buflen);
		}
		else  //接收数据部分字节
		{
			//得到包体长度
			bodylen = get_bodylen(client->config, head); //包体长度不包含包头
			//printf("the body len is %d\n", bodylen);
			if (bodylen > 0) {
				if (client->buffer_data == NULL) {
					//总长度
					//printf("create memory\n");
					client->buffer_data = new tcp_unit();
					//加上头部长度
					client->buffer_data->headlen = headlen;
					client->buffer_data->data = new char[bodylen + headlen];
					client->buffer_data->tlen = bodylen + headlen;
					//数据接收的长度加上头部的长度，开始接收数据体
					client->buffer_data->recvlen = headlen;
					//拷贝头部
					memcpy(client->buffer_data->data, head, headlen);
					*buf = uv_buf_init(client->buffer_data->data + headlen, bodylen);
				}
				else {
					//前面加了头部
					char * pos = client->buffer_data->data + client->buffer_data->recvlen;
					buflen = client->buffer_data->tlen - client->buffer_data->recvlen;
					*buf = uv_buf_init(pos, buflen);
				}
			}
			else { //否则没有包体，只有包头
				client->buffer_data->tlen = bodylen;
				client->buffer_data->recvlen = 0;
			}
		}

	}
	//


	
	static void worker(uv_work_t* req) {		
		thread_work * rb = (thread_work *)req->data;
		tcp_unit *tu = rb->data;
		//uint8_t *buf =(uint8_t*)tu->data;
		//int len = tu->tlen;
		//int headlen = tu->headlen;

		tcp_server *server =(tcp_server*)rb->client->data;
		server->on_data(tu);
	}
	static void after_worker(uv_work_t* req,int status) {

		thread_work *work = static_cast<thread_work *>(req->data);
		tcp_unit * tu = work->data;
		free(tu->data);
		free(tu);
		free(work);
		
	}
	static int tcp_parser_execute(client_t* client, char *data, int size)
	{
		int headlen = get_headlen(client->config);
		if (client->status == enum_head)
		{
			client->recvlen += size;
			//如果头部字节已经接收完毕
			if (headlen == client->recvlen)
			{
				client->status = enum_body; //开始接收包体数据
				//头部已经接收完毕则发生事件
				//继承加入可以发生事件，如加入列表
				tcp_server * server = (tcp_server *)client->data;
				server->on_headers_complete(client);
			}
		}
		else if (client->status == enum_body)
		{
			client->buffer_data->recvlen += size;
			if (client->buffer_data->tlen == client->buffer_data->recvlen) {//数据已经接收完毕
																			//包头数据初始化
				client->recvlen = 0;
				client->status = enum_head; //开始重新接收包头
				if (client->buffer_data->tlen > 0) {
					//发生事件
					tcp_server * server = (tcp_server *)client->data;
					server->on_message_complete(client);
#if 0 
					//you can let thread work go and let it happen ,then no uv_queue_work
					//client->clean();
#endif
#if 1
					thread_work * work = new thread_work(client,client->buffer_data);
					work->id = client->deviceid;
					client->buffer_data = NULL;

					//消息体接收结束,交给线程池处理
					int status = uv_queue_work(client->config->uv_loop,
						&work->request,
						worker,
						after_worker);
					CHECK(status, "uv_queue_work");
#endif
				}
			}
		}
		return client->recvlen;

	}

	static void on_read(uv_stream_t* tcp, ssize_t nread, const uv_buf_t * buf) {
		ssize_t parsed;
		client_t* client = (client_t*)tcp->data;
		if (nread >= 0) {
			parsed = (ssize_t)tcp_parser_execute(
				client, buf->base, nread);
			if (parsed < 0) {
				LOG_ERROR("parse error");
				//tcp_parser *parser = &client->parser;
				
				uv_close((uv_handle_t*)&client->tcp, on_close);
			}
		}
		else {
			if (nread != UV_EOF) {
				UVERR(nread, "read");
			}
			uv_close((uv_handle_t*)&client->tcp, on_close);
		}

	}

	static void on_connect(uv_stream_t* server_handle, int status) {
		CHECK(status, "connect");
		printf("connected!\n");
		tcp_server * server  = (tcp_server *)server_handle->data;
		client_t* client = (client_t*)calloc(1, sizeof(client_t));
		client->config   = &(server->_set);
		client->data     = server;
		uv_loop_t * uv_loop = client->config->uv_loop;

		uv_tcp_init(uv_loop, &client->tcp);

		client->tcp.data = client;

		int r = uv_accept(server_handle, (uv_stream_t*)&client->tcp);
		if (r == 0) {
			uv_read_start((uv_stream_t*)&client->tcp, alloc_cb, on_read);
		}
		else {
			CHECK(r, "accept");
			uv_close((uv_handle_t*)(&client->tcp), on_close);
		}
	}



public:
	//头部接收完毕
	virtual int on_headers_complete(void *param) {
		return 0;
	}

	virtual int on_message_complete(void *param) {
		return 0;
	}
	//收到完整一帧数据
	virtual int on_data(tcp_unit * data) {
		return 0;
	}
	int start(const char * ip, uint16_t port) {
		int r = uv_tcp_init(_set.uv_loop, _server);
		//保存用户数据，是tcp server 本身指针
		_server->data = this; // &_set;
		CHECK(r, "tcp_init");
		r = uv_tcp_keepalive(_server, 1, 60);
		CHECK(r, "tcp_keepalive");
		struct sockaddr_in address;
		r = uv_ip4_addr(ip, port, &address);
		CHECK(r, "ip4_addr");
		r = uv_tcp_bind(_server, (const struct sockaddr*)&address, 0);
		CHECK(r, "tcp_bind");
		r = uv_listen((uv_stream_t*)_server, MAX_WRITE_HANDLES, on_connect);
		CHECK(r, "uv_listen");

		//tcp_servers.push(port, server);
		return 0;

	}
	int  tcp_init(const char * configfile,
		uv_loop_t * uv_loop
		)
	{
		_server = new uv_tcp_t();
		_set.uv_loop = uv_loop;

		if (read_config_protocol(configfile, _set) != 0)
		{
			cout << "error read config file "<<configfile << endl;
			uninit();
			return -1;
		}
		//
		_set.hlen_calc = get_headlen(&_set);
		printf("the head len is %d\n", _set.hlen_calc);
		return 0;
	}

	void uninit()
	{
		free(_server);
	}

	//得到所有的客戶端ip地址和流量等数据
	int getclients()
	{
		return 0;
	}
	client_t *getclient(uint32_t deviceid) {
		return NULL;
	}

};


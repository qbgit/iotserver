#pragma once


#include <string>
#include <sstream>

#include <uv.h>
using namespace std;


//注意一个发送一次，不要重用
class http_base
{
#define BUF_LEN  4096
	char  recvbody[BUF_LEN] ;
	//接收的位置
	ssize_t recvlen = 0;
	uv_loop_t *loop = NULL;
	uv_tcp_t  *socket = NULL;
	string postcontent;
	int postcontentlen = 0;
	uv_write_t write_req;
	uv_buf_t buf;
public:



	static void alloc_buffer(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf) {
		http_base *hb = (http_base *)handle->data;
		ssize_t pos = hb->recvlen;
		buf->base = &hb->recvbody[pos]; // (char*)malloc(suggested_size);
		buf->len = (ULONG)(BUF_LEN - pos);
	}



	static void on_read(uv_stream_t *client, ssize_t nread, const uv_buf_t *buf) {
		http_base *hb = (http_base *)client->data;
		if (nread < 0) {
			if (nread != UV_EOF)
				fprintf(stderr, "Read error %s\n", uv_err_name((int)nread));
			uv_close((uv_handle_t*)client, NULL);
			hb->recvbody[hb->recvlen] = '\0';
			printf("the recv is %Id: %s\n",hb->recvlen, hb->recvbody);
			return;
		}
		else if (nread == 0)
		{
			//已经读完
			uv_close((uv_handle_t*)client, NULL);
			//printf("the recv is %s\n", hb->recvbody);
		}
		else
			hb->recvlen += nread;


	}

	static void after_write_data(uv_write_t* req, int status) {

		if (status < 0) {
			fprintf(stderr, "connect failed error %s\n", uv_err_name(status));
			free(req);
			return;
		}
		
		uv_read_start((uv_stream_t*)req->handle, alloc_buffer, on_read);
		//uv_buf_t *resbuf = (uv_buf_t *)(req + 1);
		//free(resbuf->base);
		//free(req);
	}

	static void on_connect(uv_connect_t *req, int status) {

		if (status < 0) {
			fprintf(stderr, "connect failed error %s\n", uv_err_name(status));
			free(req);
			return;
		}
		http_base * hb = (http_base*)req->data;


		hb->buf.base = (char*)hb->postcontent.c_str();
		hb->buf.len = (ULONG)(hb->postcontentlen);
		uv_stream_t *client = req->handle;
		//指针传递
		client->data = hb;

		uv_write(&hb->write_req, client, &hb->buf, 1, after_write_data);
	}



	static void on_resolved(uv_getaddrinfo_t *resolver, int status, struct addrinfo *res)
	{
		if (status < 0) {
			fprintf(stderr, "getaddrinfo callback error %s\n", uv_err_name(status));
			return;
		}
		http_base * hb = (http_base*)resolver->data;
		char addr[17] = { '\0' };
		uv_ip4_name((struct sockaddr_in*) res->ai_addr, addr, 16);
		fprintf(stderr, "%s\n", addr);
		uv_connect_t *connect_req = (uv_connect_t*)malloc(sizeof(uv_connect_t));
		//指针传递
		connect_req->data = hb;
		hb->socket = (uv_tcp_t*)malloc(sizeof(uv_tcp_t));
		uv_tcp_init(hb->loop, hb->socket);
		uv_tcp_connect(connect_req, hb->socket, (const struct sockaddr*) res->ai_addr, on_connect);
		uv_freeaddrinfo(res);
	}
	int postdata1(uv_loop_t *loop, const char* host, unsigned short port, const char* path, const char* post_content)
	{

		std::stringstream stream;
		stream << "POST " << path;
		stream << " HTTP/1.0\r\n";
		stream << "Host: " << host << "\r\n";
		stream << "User-Agent: qb IOT http client\r\n";
		stream << "Content-Type:application/x-www-form-urlencoded\r\n";
		stream << "Content-Length:" << strlen(post_content) << "\r\n";
		stream << "Connection:close\r\n\r\n";
		stream << post_content;

		struct sockaddr_in address;
		this->loop = loop;
		this->postcontent = stream.str();
		this->postcontentlen = (int)strlen(this->postcontent.c_str());
		uv_ip4_addr(host, port, &address);

		//server = gethostbyname(host);
		//memcpy((char *)&address.sin_addr.s_addr, (char*)server->h_addr, server->h_length);
		uv_connect_t *connect_req = (uv_connect_t*)malloc(sizeof(uv_connect_t));
		//指针传递
		connect_req->data = this;
		this->socket = (uv_tcp_t*)malloc(sizeof(uv_tcp_t));
		uv_tcp_init(this->loop, this->socket);
		uv_tcp_connect(connect_req, this->socket, (const struct sockaddr*) &address, on_connect);
		return 0;
	}


	int postdata(uv_loop_t *loop,const char* host, const char *port, const char* path, const char* post_content)
	{
		this->loop = loop;
		postcontent = post_content;
		struct addrinfo hints;
		hints.ai_family = PF_INET;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_protocol = IPPROTO_TCP;
		hints.ai_flags = 0;
		uv_getaddrinfo_t resolver;
		resolver.data = this;
		int r = uv_getaddrinfo(loop, &resolver, on_resolved, host, port, &hints);
		return 0;
	}

	int getdata(uv_loop_t *loop, const char* host, unsigned short port, const char* path, const char* get_content)
	{
		//GET请求方式
		this->loop = loop;
		std::stringstream stream;
		if (strlen(get_content) > 0)
			stream << "GET " << path << "?" << get_content;
		else
			stream << "GET " << path;
		stream << " HTTP/1.0\r\n";
		stream << "Host: " << host << "\r\n";
		stream << "User-Agent: Mozilla/5.0 (Windows; U; Windows NT 5.1; zh-CN; rv:1.9.2.3) Gecko/20100401 Firefox/3.6.3\r\n";
		stream << "Connection:close\r\n\r\n";
		
		struct sockaddr_in address;
		this->loop = loop;
		this->postcontent = stream.str();
		this->postcontentlen = (int)strlen(this->postcontent.c_str());
		uv_ip4_addr(host, port, &address);

		uv_connect_t *connect_req = (uv_connect_t*)malloc(sizeof(uv_connect_t));
		//指针传递
		connect_req->data = this;
		this->socket = (uv_tcp_t*)malloc(sizeof(uv_tcp_t));
		uv_tcp_init(this->loop, this->socket);
		uv_tcp_connect(connect_req, this->socket, (const struct sockaddr*) &address, on_connect);
		
	}

};


//http协议的get post 请求
class http_connect
{
public:

	string _data;
	//char * _buf
public:

	http_connect::http_connect()
	{
#ifdef WIN32
		//此处一定要初始化一下，否则gethostbyname返回一直为空
		WSADATA wsa = { 0 };
		WSAStartup(MAKEWORD(2, 2), &wsa);
#endif
	}

	http_connect::~http_connect()
	{
		WSACleanup();
	}
	int http_connect::socketHttp(const char* host, unsigned short port, const char *request)
	{
		SOCKET sockfd;
		struct sockaddr_in address;
		struct hostent *server;

		sockfd = socket(AF_INET, SOCK_STREAM, 0);
		address.sin_family = AF_INET;
		address.sin_port = htons(port);
		server = gethostbyname(host);
		memcpy((char *)&address.sin_addr.s_addr, (char*)server->h_addr, server->h_length);



		if (-1 == connect(sockfd, (struct sockaddr *)&address, sizeof(address))) {
#ifdef WIN32 
			closesocket(sockfd);
#endif
			cout << "connection error!" << std::endl;
			return -1;
		}



#ifdef WIN32
		int ret = 0;
		int timeout = 2000; //2s
		ret = setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout, sizeof(timeout));
		ret = setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
		int iRes = send(sockfd, request, (int)strlen(request), 0);
		if (iRes == SOCKET_ERROR) {
			printf("send failed: %d\n", WSAGetLastError());
			closesocket(sockfd);
			return -1;
		}

#else
		struct timeval timeout = { 2,0 };//2s
		int ret = setsockopt(sock_fd, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout, sizeof(timeout));
		int ret = setsockopt(sock_fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
		write(sockfd, request, strlen(request));
#endif
		char * buf = new char[1024 * 1024];


		int offset = 0;
		int rc;

		do {

#ifdef WIN32
	
			rc = recv(sockfd, buf + offset, 1024 - offset, 0);
#else
			rc = read(sockfd, buf + offset, 1024);
#endif
			if (rc > 0) {
				offset += rc;
				printf("Bytes received: %d\n", rc);
			}
			else if (rc == 0)
				printf("Connection closed\n");
			else
				printf("recv failed: %d\n",rc);
		} while (rc > 0);



#ifdef WIN32
		closesocket(sockfd);
#else
		close(sockfd);
#endif
		buf[offset] = 0;
		_data = buf;
		delete[]buf;

		return 0;
		size_t r = _data.rfind("\r\n\r\n");
		cout << "r is " << r << endl;
		//cout << _data << std::endl;
		string re = _data.substr(409 + 4);
		cout << "the re is " << re << endl;
	}

	int http_connect::postData(const char* host, unsigned short port, const char* path, const char* post_content)
	{
		//POST请求方式
		std::stringstream stream;
		stream << "POST " << path;
		stream << " HTTP/1.0\r\n";
		stream << "Host: " << host << "\r\n";
		stream << "User-Agent: Mozilla/5.0 (Windows; U; Windows NT 5.1; zh-CN; rv:1.9.2.3) Gecko/20100401 Firefox/3.6.3\r\n";
		stream << "Content-Type:application/x-www-form-urlencoded\r\n";
		stream << "Content-Length:" << strlen(post_content) << "\r\n";
		stream << "Connection:close\r\n\r\n";
		stream << post_content ;
		string temp = stream.str();
		//cout << "temp is :" << temp << endl;

		return socketHttp(host, port, temp.c_str());
	}

	int http_connect::getData(const char* host, unsigned short port, const char* path, const char* get_content)
	{
		//GET请求方式
		std::stringstream stream;
		if (strlen(get_content) > 0)
			stream << "GET " << path << "?" << get_content;
		else
			stream << "GET " << path;
		stream << " HTTP/1.0\r\n";
		stream << "Host: " << host << "\r\n";
		stream << "User-Agent: Mozilla/5.0 (Windows; U; Windows NT 5.1; zh-CN; rv:1.9.2.3) Gecko/20100401 Firefox/3.6.3\r\n";
		stream << "Connection:close\r\n\r\n";
		string temp = stream.str();
		//cout << "len is " << temp.size() << endl;
		return socketHttp(host, port, temp.c_str());
	}
};



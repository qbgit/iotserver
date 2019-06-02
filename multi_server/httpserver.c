
/*
  author:qianbo
  email:418511899@qq.com
*/

#include "httpserver.h"
#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>


#include "uv.h"
#include "http_parser.h"
#include "util.h"

static int request_num = 1;
static uv_loop_t* uv_loop;
static uv_tcp_t server;
static http_parser_settings parser_settings;

static const char * content_type_html = "text/html";
static const char * content_type_css = "text/css";
static const char * content_type_javascript = "application/javascript";
static const char * content_type_plain = "text/plain";
static const char * response_code_200 = "200 OK";
static const char * response_code_404 = "404 Not Found";
static const char * result_noaccess = "no access";
static const char * result_failed = "no file";

typedef struct client_t {
	uv_tcp_t handle;
	http_parser parser;
	uv_write_t write_req;
	int request_num;
	//url 长度
	char path[1024];
	int keepalive;
}client_t;

void on_close(uv_handle_t* handle) {
	client_t* client = (client_t*)handle->data;
	LOGF("[ %5d ] connection closed\n\n", client->request_num);
	free(client);
}

void alloc_cb(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf) {
	*buf = uv_buf_init((char*)malloc(suggested_size), (int)suggested_size);
}

void on_read(uv_stream_t* tcp, ssize_t nread, const uv_buf_t * buf) {
	ssize_t parsed;

	client_t* client = (client_t*)tcp->data;
	if (nread >= 0) {
		parsed = (ssize_t)http_parser_execute(
			&client->parser, &parser_settings, buf->base, nread);
		if (client->parser.upgrade) {
			LOG_ERROR("parse error: cannot handle http upgrade");
			uv_close((uv_handle_t*)&client->handle, on_close);
		}
		else if (parsed < nread) {
			LOG_ERROR("parse error");
			uv_close((uv_handle_t*)&client->handle, on_close);
		}
	}
	else {
		if (nread != UV_EOF) {
			
			//UVERR(nread, "read");
		}
		uv_close((uv_handle_t*)&client->handle, on_close);
	}
	free(buf->base);
}

typedef struct work_thread {
	
	client_t* client;
	uv_work_t request;
	//result 指向缓冲区地址
	uv_buf_t result_b[2];
	//http 头部
	char* head;
	size_t headlen;
	//http body内容在
	char* result;
	//缓冲区result长度
	size_t resultlen;
	const char *response_code;// [32];
	const char *content_type ;
	int error;
}work_thread;

void after_write(uv_write_t* req, int status) {
	CHECK(status, "write");
	if (!uv_is_closing((uv_handle_t*)req->handle))
	{
		work_thread* work = req->data;
		if (work->response_code != response_code_404)
			free(work->result);
		free(work->head);
		free(work);
		uv_close((uv_handle_t*)req->handle, on_close);
	}
}




void route(const char * path)
{
	
	//解析路径
	//client->path
	//读取配置文件，路由
	printf("no file,route now:%s",path);//, but path is : %s",client->path);
	return;
}

void render(uv_work_t* req) {
	char filepath[260];
	char indexpath[260];
	int has_index = 0;
	work_thread* work = req->data;
	client_t* client = (client_t*)work->client;
	work->response_code = response_code_200;
	work->content_type = content_type_plain; // "text/plain";
	sprintf(filepath, ".%s", client->path);
	
	sprintf(indexpath, ".%s%s", client->path, "index.html");

	//这里要判断是否有上传文件


	 has_index = _access(indexpath, R_OK) != -1;
	if (/*!has_index &&*/ filepath[strlen(filepath)- 1] == '/') {
		size_t len = 16;
		char * pos = NULL;
		uv_fs_t scandir_req;
		int r = uv_fs_scandir(uv_loop, &scandir_req, filepath, 0, NULL);
		uv_dirent_t dent;
		work->content_type = content_type_html;
		work->result = malloc(1024 * 1024);
		strcpy(work->result ,"<html><body><ul>");
		//注意 这里超过1M 字节会跳过后面的文件，fix me 钱波
		while (UV_EOF != uv_fs_scandir_next(&scandir_req, &dent)) {
			len = strlen(work->result);
			if (len + 260 > 1024 * 1024)
				break;
			pos = &(work->result[len]);
			if (dent.type == UV_DIRENT_DIR) {
				sprintf(pos, "<li><a href='%s'>%s/</a></li>\n",dent.name, dent.name);
			}
			else
				sprintf(pos, "<li><a href='%s'>%s</a></li>\n",dent.name,dent.name);
		}
		len = strlen(work->result);
		pos = &(work->result[len]);
		strcpy(pos, "</ul></body></html>");
		work->resultlen = len + 19;
		uv_fs_req_cleanup(&scandir_req);
	}
	else {
		const char * file_to_open = filepath;
		//if (has_index) {
		//	file_to_open = indexpath;
		//}

		int exists = (_access(file_to_open, R_OK) != -1);
		if (!exists) { //如果文件不存在则开始路由
			route(client->path);
			work->result = malloc(1024*20); // "no access";
			char * pp = "<html><body><div>route : %s,but it not work now</div></body></html>";
			sprintf(work->result, pp, client->path);
			work->resultlen = strlen(work->result);
			work->content_type = content_type_html;
			work->response_code = response_code_200;// "";// "404 Not Found";

			return;
		}
		//文件存在，打开文件返回文件内容
		//注意这里可以增加缓存系统， fix me 钱波
		FILE * f = fopen(file_to_open , "rb");
		if (f) {
			fseek(f, 0, SEEK_END);
			unsigned size = ftell(f);
			fseek(f, 0, SEEK_SET);
			work->result = malloc(size);
			work->resultlen = size;
			fread(&work->result[0], size, 1, f);
			fclose(f);
			if (endswith(file_to_open, "html")) {
				work->content_type = content_type_html; // "text/html");
			}
			else if (endswith(file_to_open, "css")) {
				work->content_type = content_type_css; // "text/css");
			}
			else if (endswith(file_to_open, "js")) {
				work->content_type = content_type_javascript; // "application/javascript";
			}
		}
		else {
			work->result =(char*)result_failed;
			work->response_code = response_code_404; // "404 Not Found";
		}
	}
}

static const char *http_content =
"HTTP/1.1 %s\r\n"
"Content-Type: %s\r\n"
"Connection: keep-alive\r\n"
"Content-Length: %d\r\n"
"Access-Control-Allow-Origin: *\r\n"
"\r\n";
size_t get_http_content_baselen()
{
	return strlen(http_content) - 6;// %s %s %d
	//int i = 0;
	//int t = 0;
	//while (strcmp(http_content[i], "\r\n") != 0)
	//{
	//	t += strlen(http_content[i]) + 2;
	//	i++;
	//}
	//t += 2;
	//t -= 6;//%s %s %d
	//return t;
}
void after_render(uv_work_t* req) {
	work_thread* work = req->data;
	client_t* client = work->client;

	LOGF("[ %5d ] after render\n", client->request_num);


	size_t len = get_http_content_baselen();
	len += strlen(work->response_code);
	len += strlen(work->content_type);
	char numbuf[12];

	_itoa((int)work->resultlen, numbuf, 10);
	len += strlen(numbuf) +1;

	work->head = malloc(len);
	work->headlen = len;

	sprintf(work->head, 
		http_content, 
		work->response_code,
		work->content_type, 
		work->resultlen);
	work->result_b[0].base = work->head;
	work->result_b[0].len =(ULONG) work->headlen;

	work->result_b[1].base = work->result;
	work->result_b[1].len =(ULONG) work->resultlen;
	uv_buf_t *resbuf = &work->result_b[0];

	client->write_req.data = work;

	int r = uv_write(&client->write_req,
		(uv_stream_t*)&client->handle,
		resbuf,
		2,
		after_write);
	CHECK(r, "write buff");
}

int on_message_begin(http_parser* parser) {
	LOGF("\n***MESSAGE BEGIN***\n");
	return 0;
}

int on_headers_complete(http_parser* parser) {
	LOGF("\n***HEADERS COMPLETE***\n");
	return 0;
}

int on_url(http_parser* parser, const char* url, size_t length) {
	client_t* client = (client_t*)parser->data;
	LOGF("[ %5d ] on_url\n", client->request_num);
	LOGF("Url: %.*s\n", (int)length, url);
	struct http_parser_url u;
	int result = http_parser_parse_url(url, length, 0, &u);
	if (result) {
		fprintf(stderr, "\n\n*** failed to parse URL %s ***\n\n", url);
		return -1;
	}
	else {
		if ((u.field_set & (1 << UF_PATH))) {
			const char * data = url + u.field_data[UF_PATH].off;
			
			memcpy(client->path, data, u.field_data[UF_PATH].len);
			//可能超过
			client->path[u.field_data[UF_PATH].len] = '\0';
			//client->path = std::string(data, u.field_data[UF_PATH].len);
		}
		else {
			fprintf(stderr, "\n\n*** failed to parse PATH in URL %s ***\n\n", url);
			return -1;
		}
	}
	return 0;
}

int on_header_field(http_parser* parser, const char* at, size_t length) {
	LOGF("Header field: %.*s\n", (int)length, at);
	return 0;
}

int on_header_value(http_parser* parser, const char* at, size_t length) {
	LOGF("Header value: %.*s\n", (int)length, at);
	return 0;
}

int on_body(http_parser* parser, const char* at, size_t length) {
	LOGF("Body: %.*s\n", (int)length, at);
	return 0;
}

int on_message_complete(http_parser* parser) {
	client_t* client = (client_t*)parser->data;
	LOGF("[ %5d ] on_message_complete\n", client->request_num);
	work_thread* work = malloc(sizeof(work_thread));
	work->client = client;
	work->request.data = work;
	int status = uv_queue_work(uv_loop,
		&work->request,
		render,
		(uv_after_work_cb)after_render);
	CHECK(status, "uv_queue_work");

	return 0;
}

void on_connect(uv_stream_t* server_handle, int status) {
	CHECK(status, "connect");
	//assert((uv_tcp_t*)server_handle == &server);

	client_t* client = malloc(sizeof(client_t));
	//设定keepalive
	client->keepalive = 0;
	client->request_num = request_num;
	request_num++;

	LOGF("[ %5d ] new connection\n", request_num);

	uv_tcp_init(uv_loop, &client->handle);
	http_parser_init(&client->parser, HTTP_REQUEST);

	client->parser.data = client;
	client->handle.data = client;

	int r = uv_accept(server_handle, (uv_stream_t*)&client->handle);
	CHECK(r, "accept");

	uv_read_start((uv_stream_t*)&client->handle, alloc_cb, on_read);
}

#define MAX_WRITE_HANDLES 1000


void test()
{
	char buf[] = "/live/123";
	//用来接收返回数据的数组。这里的数组元素只要设置的比分割后的子字符串个数大就好了。
	char *revbuf[8] = { 0 };
	//分割后子字符串的个数
	int num = 0;
	split(buf, "/", revbuf, &num);
	int i = 0;
	for (i = 0; i < num; i++) {
		printf("%s\n", revbuf[i]);
	}
}


int main() 
{
	test();
	//printf("%d\n", endswith("abc.jpg", "jpg"));
	//return 0;
	//signal(SIGPIPE, SIG_IGN);
	//int cores = sysconf(_SC_NPROCESSORS_ONLN);
	//printf("number of cores %d\n", cores);
	//char cores_string[10];
	//sprintf(cores_string, "%d", 4);
	//setenv("UV_THREADPOOL_SIZE", cores_string, 1);
	parser_settings.on_url = on_url;
	// notification callbacks
	parser_settings.on_message_begin = on_message_begin;
	parser_settings.on_headers_complete = on_headers_complete;
	parser_settings.on_message_complete = on_message_complete;
	// data callbacks
	parser_settings.on_header_field = on_header_field;
	parser_settings.on_header_value = on_header_value;
	parser_settings.on_body = on_body;
	uv_loop = uv_default_loop();
	int r = uv_tcp_init(uv_loop, &server);
	CHECK(r, "tcp_init");
	r = uv_tcp_keepalive(&server, 1, 60);
	CHECK(r, "tcp_keepalive");
	struct sockaddr_in address;
	r = uv_ip4_addr("0.0.0.0", 8000, &address);
	CHECK(r, "ip4_addr");
	r = uv_tcp_bind(&server, (const struct sockaddr*)&address, 0);
	CHECK(r, "tcp_bind");
	r = uv_listen((uv_stream_t*)&server, MAX_WRITE_HANDLES, on_connect);
	CHECK(r, "uv_listen");
	LOG("listening on port 8000");
	uv_run(uv_loop, UV_RUN_DEFAULT);
	return 0;
}
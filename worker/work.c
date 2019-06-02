#if 1


#if defined (WIN32) || defined (_WIN32)
#include <WinSock2.h>
#include <windows.h>
#pragma comment(lib,"ws2_32.lib")
#pragma comment(lib,"psapi.lib")
#pragma comment(lib,"Iphlpapi.lib")
#pragma comment(lib,"Userenv.lib")

#else
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#endif


#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uv.h>
#include "http_parser.h"
uv_loop_t *loop;
uv_pipe_t queue;



static http_parser_settings parser_settings;
typedef struct {
	uv_write_t req;
	uv_buf_t buf;
} write_req_t;



typedef struct client_t {
	//client_t() :
	//	body() {}
	http_parser parser;
	int request_num;
	uv_tcp_t tcp;
	uv_connect_t connect_req;
	uv_shutdown_t shutdown_req;
	uv_write_t write_req;
	//std::stringstream body;
}client_t;

void free_write_req(uv_write_t *req) {
	write_req_t *wr = (write_req_t*)req;
	free(wr->buf.base);
	free(wr);
}

void alloc_buffer(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf) {
	buf->base = malloc(suggested_size);
	buf->len = suggested_size;
}

void echo_write(uv_write_t *req, int status) {
	if (status) {
		fprintf(stderr, "Write error %s\n", uv_err_name(status));
	}
	free_write_req(req);
}

void echo_read(uv_stream_t *tcp, ssize_t nread, const uv_buf_t *buf) {
	if (nread > 0) {

		client_t *client = tcp->data;
		http_parser * parser = &client->parser;
		int parsed = 0;
		/*if (parser->http_errno == HPE_PAUSED) {
		LOG("PAUSED");
		return ;
		}*/
		parsed = (ssize_t)http_parser_execute(parser, &parser_settings, buf->base, nread);
		if (parser->upgrade) {
			//LOG("We do not support upgrades yet");
		}
		else if (parsed != nread) {
			//LOGF("parsed incomplete data: %ld/%ld bytes parsed\n", parsed, nread);
			//LOGF("\n*** %s ***\n",
			//	http_errno_description(HTTP_PARSER_ERRNO(parser)));
		}

		write_req_t *req = (write_req_t*)malloc(sizeof(write_req_t));
		req->buf = uv_buf_init(buf->base, nread);
		uv_write((uv_write_t*)req, tcp, &req->buf, 1, echo_write);
		return;
	}

	if (nread < 0) {
		if (nread != UV_EOF)
			fprintf(stderr, "Read error %s\n", uv_err_name(nread));
		uv_close((uv_handle_t*)tcp, NULL);
	}

	free(buf->base);
}

void on_new_connection(uv_stream_t *q, ssize_t nread, const uv_buf_t *buf) {
	if (nread < 0) {
		if (nread != UV_EOF)
			fprintf(stderr, "Read error %s\n", uv_err_name(nread));
		uv_close((uv_handle_t*)q, NULL);
		return;
	}

	uv_pipe_t *pipe = (uv_pipe_t*)q;
	if (!uv_pipe_pending_count(pipe)) {
		fprintf(stderr, "No pending count\n");
		return;
	}

	uv_handle_type pending = uv_pipe_pending_type(pipe);
	assert(pending == UV_TCP);


	client_t * client = malloc(sizeof(client_t));
	http_parser_init(&client->parser, HTTP_RESPONSE);
	uv_tcp_t *tcp = &client->tcp;
	tcp->data = client;
	//uv_tcp_t *client = (uv_tcp_t*)malloc(sizeof(uv_tcp_t));
	uv_tcp_init(loop, &client->tcp);
	if (uv_accept(q, (uv_stream_t*)tcp) == 0) {
		uv_os_fd_t fd;
		uv_fileno((const uv_handle_t*)tcp, &fd);
#ifdef WIN32
		fprintf(stderr, "Worker %d: Accepted fd %d\n", _getpid(), fd);
#else
		fprintf(stderr, "Worker %d: Accepted fd %d\n", getpid(), fd);
#endif
		uv_read_start((uv_stream_t*)tcp, alloc_buffer, echo_read);
	}
	else {
		uv_close((uv_handle_t*)tcp, NULL);
	}
}

int main() {
	loop = uv_default_loop();
	

	setenv("UV_THREADPOOL_SIZE", 4, 1);
#if 0 
	parser_settings.on_url = on_url;
	// notification callbacks
	parser_settings.on_message_begin = on_message_begin;
	parser_settings.on_headers_complete = on_headers_complete;
	parser_settings.on_message_complete = on_message_complete;
	// data callbacks
	parser_settings.on_header_field = on_header_field;
	parser_settings.on_header_value = on_header_value;
	parser_settings.on_body = on_body;
#endif
	uv_pipe_init(loop, &queue, 1 /* ipc */);
	uv_pipe_open(&queue, 0);
	uv_read_start((uv_stream_t*)&queue, alloc_buffer, on_new_connection);
	return uv_run(loop, UV_RUN_DEFAULT);
}

#endif
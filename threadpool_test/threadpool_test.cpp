// threadpool_test.cpp : 定义控制台应用程序的入口点。
//
#ifdef _WIN32
#include <WinSock2.h>
#include <windows.h>
#pragma comment(lib,"ws2_32.lib")
#pragma comment(lib,"psapi.lib")
#pragma comment(lib,"Iphlpapi.lib")
#pragma comment(lib,"Userenv.lib")
#endif
#include "uv.h"
#include "task.h"

static int work_cb_count;
static int after_work_cb_count;
static uv_work_t work_req;
static char data;


static void work_cb(uv_work_t* req) {
	ASSERT(req == &work_req);
	ASSERT(req->data == &data);
	work_cb_count++;
}


static void after_work_cb(uv_work_t* req, int status) {
	ASSERT(status == 0);
	ASSERT(req == &work_req);
	ASSERT(req->data == &data);
	after_work_cb_count++;
}


TEST_IMPL(threadpool_queue_work_simple) {
	int r;

	work_req.data = &data;
	r = uv_queue_work(uv_default_loop(), &work_req, work_cb, after_work_cb);
	ASSERT(r == 0);
	uv_run(uv_default_loop(), UV_RUN_DEFAULT);

	ASSERT(work_cb_count == 1);
	ASSERT(after_work_cb_count == 1);

	MAKE_VALGRIND_HAPPY();
	return 0;
}


TEST_IMPL(threadpool_queue_work_einval) {
	int r;

	work_req.data = &data;
	r = uv_queue_work(uv_default_loop(), &work_req, NULL, after_work_cb);
	ASSERT(r == UV_EINVAL);

	uv_run(uv_default_loop(), UV_RUN_DEFAULT);

	ASSERT(work_cb_count == 0);
	ASSERT(after_work_cb_count == 0);

	MAKE_VALGRIND_HAPPY();
	return 0;
}


int main()
{

	TEST_IMPL(threadpool_queue_work_simple);
	return 0;
}
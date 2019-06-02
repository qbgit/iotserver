#include <stdio.h>  
#include <iostream>  
#include "uv.h"  
#include "httpclient.h"
using namespace std;


uint64_t repeat = 0;

static void callback(uv_timer_t *handle) {
	repeat = repeat + 1;
	cout << "repeat count:" << repeat << endl;
}
static void callback1(uv_timer_t *handle) {
	repeat = repeat + 1;
	cout << "repeat count:" << repeat << endl;
}
static void callback2(uv_timer_t *handle) {
	repeat = repeat + 1;
	cout << "repeat count:" << repeat << endl;
}


void timetest(uv_loop_t *loop)
{
	uv_timer_t timer1;
	uv_timer_t timer2;
	uv_timer_t timer3;

	uv_timer_init(loop, &timer1);
	uv_timer_init(loop, &timer2);
	uv_timer_init(loop, &timer3);

	uv_timer_start(&timer1, callback, 500, 0);
	uv_timer_start(&timer2, callback1, 600, 0);
	uv_timer_start(&timer3, callback2, 600, 0);
}

int main_1(int argc, char* argv[]) {

	//http_connect hc;
	//hc.postData("127.0.0.1", 8080, "/data/7000/1000", "path = 123.jpg");
	printf("post data\n");

	uv_loop_t *loop = uv_default_loop();
	http_base base;
	base.postdata1(loop, "127.0.0.1", 8080, "/data/7000/1000", "path = 1234.jpg");
	
	uv_run(loop, UV_RUN_DEFAULT);
	printf("post end\n");
	return 0;
}
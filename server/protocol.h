#ifndef _PROTOCOL_H_
#define _PROTOCOL_H_

/*
author : qianbo
email  : 418511899@qq.com
func   : protocol
time   ：2018-10-4
*/
#include "stdio.h"
#include "stdint.h"
#include "uv.h"

/*
   大端还是小端
*/
typedef enum endian_{
	enum_big = 0,
	enum_little,
}endian_;
/*
  是否在线
*/
typedef enum online_
{
	enum_online = 1,
	enum_offline = 0
}online_;

typedef enum recv_status_
{
	enum_head = 0, //接收头部
	enum_body = 1  //接收包体
}recv_status;

typedef enum packtype_
{
	enum_none = 0,
	enum_cs = 1,       //接收服务器包
	enum_sc = 2,       //发送数据包
	enum_control = 3,  //控制
	enum_config  = 4,  //设置
	enum_query   = 5,  //查询
	enum_total = 6
}packtype_;


typedef struct tcp_unit
{
	//真实数据
	char * data = NULL;
	//数据总长度bodylen + headlen
	int tlen = 0;
	//接收到的数据长度
	int recvlen = 0;
	//头部长度
	int headlen = 0;
}tcp_unit;



typedef struct tcp_unit_w{
	char * data = NULL;
	int len = 0;
	packtype_ type = enum_cs;
	uv_tcp_t *tcphandle = NULL;
	//延迟多少毫秒发送
	int delay = 0;
	struct tcp_unit_w *next = NULL;
}tcp_unit_w;

typedef struct urls
{
	char * url;
	urls * next;
}urls;

//协议头部和回调函数设置
typedef struct tcp_settings
{
	urls url;
	//datatype 不同类型
	char datatype = 0;
	unsigned char head = 0x69;
	//如果有头部起始位置一定为0 一个字节如0x69
	char headoffset = 0;
	//起始头部为0 或者 1个字节 .2 .3 .4
	char headlength = 0;
	//从第一个字节开始是id，从0开始算
	char idoffset = 1;
	//4个字节为id长度，设备id的长度
	char idlength = 4;
	//命令长度
	char cmdlength = 1;
	//命令偏移量
	char cmdoffset = 5;
	//代表包长内容的只有一个字节
	char content_len = 1;
	//代表包长度的是第6个字节为起始位置，从0开始算
	char content_offset = 6;

	char includeht = 0;
	//是否包含crc校验和end
	char includecrcend = 0;
	// 0 是没有校验，2是2个字节crc16 
	char crclen = 2; 

	char headfieldslen = 0;
	
	char timestamp_offset = 0;
	char timestamp_len = 0;
	char type_offset = 0;
	char type_len = 0;
	char memo_offset = 0;
	char memo_len = 0;

	//一个结束字节，如果为零则忽略end
	char end_start = 1;
	//end_start 如果为零则忽略
	unsigned char end = 0x16;
	
	/*保留 
	big endian 0 
	little endian 1
	*/
	char bl = enum_big;

	/*计算出来的长度
	   初始化为-1,
	   否则大于0
	*/
	int hlen_calc = -1;
	
	uv_loop_t * uv_loop = NULL;
}tcp_settings;




static void free_unit(tcp_unit ** unit)
{
	if ((*unit) != NULL) {
		if ((*unit)->data != NULL) {
			free((*unit)->data);
			(*unit)->data = NULL;
		}
		free(*unit);
		*unit = NULL;
	}
}

typedef struct client_t {
	//tcp client session
	uv_tcp_t tcp;
	//解析使用
	tcp_settings * config = NULL;
	//写入
	uv_write_t write_req;
	//设备id
	uint32_t   deviceid = 0;
	//读入的内容
	tcp_unit * buffer_data = NULL;
	
	//需要写入的内容
	tcp_unit_w * buffer_data_w = NULL;
	
	//最长128字节包头
	char head[128];
	//已经接收的头部的长度
	int recvlen;

	//用户自定义数据指针 tcpserver
	void * data = NULL;
	
	//接收状态 接收头部，0 接收数据 1
	recv_status status = enum_head; 

	//是否在线
	int is_online = enum_online;

	uv_timer_t  _timer;
	uv_thread_t _thread;


	int thread_run = 0;

	int time_init() {
		if (config == NULL)
			return -1;
		uv_timer_init(config->uv_loop, &_timer);
		return 0;
	}

	client_t() {
	}

	void clean()
	{
		free_unit(&buffer_data);
	}
	int headlen() {
		return recvlen;
	}
}client_t;


typedef struct thread_work {
	thread_work(client_t* cli, tcp_unit * unit) :
		request(),
		client(cli),
		data(unit),
		error(false) {
		//保存数据指针,传到处理线程
		request.data = this;
	}
	uint32_t id =0;
	client_t* client = NULL;
	//把数据接过来进行处理
	tcp_unit * data = NULL;
	uv_work_t request;
	bool error;
}thread_work;

static int get_headlen(tcp_settings * setting)
{
	if (setting->hlen_calc <=0)
	{
		if (setting->datatype == 0) // 是普通
			setting->hlen_calc = setting->headlength + setting->idlength + setting->cmdlength + setting->content_len + setting->headfieldslen;
		else if (setting->datatype == 1)
			//最后四个是deviceid
			setting->hlen_calc = setting->content_len + setting->timestamp_len
			+ setting->cmdlength + setting->type_len + setting->memo_len + setting->idlength;
	}
	return setting->hlen_calc;
}

static int get_bodylen(tcp_settings * setting, char *head)
{
	//headlen_offset 是偏移量+1
	char *pos = head + setting->content_offset;// -1;
	int slen = setting->crclen + setting->end_start;
	int blen = 0;
	if (setting->content_len == 1){
		blen = *(pos);
	}
	else if (setting->content_len == 2){
		uint16_t* pos1 = (uint16_t*)(pos);
		if (setting->bl == enum_big)
			blen = ntohs(*pos1);
		else
			blen = *pos1;
	}
	else if (setting->content_len == 4){ //最多四个字节长度
		uint32_t * pos1 = (uint32_t*)(pos);
		if (setting->bl == enum_big)
			blen = ntohl(*pos1);
		else
			blen += *pos1;
	}
	int retlen = blen + slen;
	//得到的长度包含头部长度
	if (setting->includeht == 1){
		retlen -= get_headlen(setting);
	}
	//得到的长度包含了尾部和crc校验,减去
	if (setting->includecrcend == 1)
		retlen -= slen;
	return retlen;
}




#endif
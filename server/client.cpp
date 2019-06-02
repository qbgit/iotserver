/*
  author:钱波
  hash表存储客户端
  */
//g++ - std = c++0x - I / usr / include tcpServer.cpp - L / usr / lib / -luv - o tcpsrv

#include "client.h"
#include <map>

using namespace std;



#define lock_     uv_mutex_lock(&_mutex_1)
#define unlock_   uv_mutex_unlock(&_mutex_1)
#define initlock_ uv_mutex_init(&_mutex_1)
uv_mutex_t  _mutex_1;
map<uint32_t, client_t*> map_;


static void timer_callback(uv_timer_t *handle);

static void after_write_data(uv_write_t* req, int status) {
	uv_buf_t *resbuf = (uv_buf_t *)(req + 1);
	free(resbuf->base);
	free(req);
}
static int write_data(tcp_unit_w * unit)
{
	uv_stream_t *stream = (uv_stream_t *)unit->tcphandle;
	if (uv_is_writable(stream)) {

		uv_write_t *write_req = (uv_write_t *)malloc(sizeof(*write_req) + sizeof(uv_buf_t));
		uv_buf_t *resbuf = (uv_buf_t *)(write_req + 1);

		resbuf->base = unit->data;
		resbuf->len = unit->len;
		int r = uv_write(write_req,
			(uv_stream_t*)stream,
			resbuf,
			1,
			after_write_data);
		return r;
	}
	return -1;
	
}


//传感器上线 定时开始发送数据
static void timer_callback(uv_timer_t *handle) {
	tcp_unit_w * unit = (tcp_unit_w*)handle->data;
	uint32_t id = *((uint32_t*)(handle + 1));
	write_data(unit);
	free(handle);
	write_buffer_packet(id);
}
//从控制包以，设置和查询包的链表的头部开始

/*
客户端一旦上线，将缓存中的包发出去
id: 传感器id号
*/
int write_buffer_packet(uint32_t id){
	tcp_unit_w * unit = NULL;
	lock_;
	auto iter = map_.find(id);
	if (iter != map_.end()){
		client_t * cli = iter->second;
		unit = cli->buffer_data_w;
		if (unit != NULL){
			if (cli ->is_online == enum_online) {
				cli->data = unit->next;
				unit->tcphandle = &(cli->tcp);
			}
			else
				unit = NULL;
		}
	}
	unlock_;
	if (unit == NULL){
		return 0;
	}

	if (unit->delay > 0){
		uv_timer_t * timer = (uv_timer_t*)malloc(sizeof(uv_timer_t) + sizeof(uint32_t));
		timer->data = unit;
		uint32_t *pid = (uint32_t *)(timer + 1);
		*pid = id;
		uv_timer_start(timer, timer_callback, unit->delay, 0);
	}
	else{ //不延时，直接写
		write_data(unit);
		write_buffer_packet(id);
	}
	return 0;
}

int client_push(uint32_t key, client_t *obj){
	//client_t *oldobj = NULL;
	lock_;
	auto iter = map_.find(key);
	if (iter == map_.end()){
		//cli_buf  * cobj = new cli_buf();
		//cobj->client = obj;
		//cobj->is_online = enum_online;
		map_.insert(pair<uint32_t, client_t*>(key, obj));
		//写缓存到客户端,控制包,配置包,查询包
		unlock_;
		return 0;
	}
	else{//并非新的客户端，清除以前的
		client_t * oldobj = iter->second;
		//oldobj->is_online = enum_online;
		//oldobj = cobj->client;
		//cobj->client = obj;
		unlock_;
		//关闭连接 删除数据
		if (oldobj != NULL)
		{
			uv_close((uv_handle_t*)&oldobj->tcp, NULL);
			oldobj->clean();
			free(oldobj);
		}
		return 1;
	}
}

int client_exists(uint32_t key){
	lock_;
	auto iter = map_.find(key);// == map_.end())
	unlock_;
	return iter == map_.end() ? false : true;
}
int client_offline(uint32_t key)
{
	client_t * client = NULL;
	lock_;
	auto iter = map_.find(key);
	if (iter != map_.end()){
		client = iter->second;
		//仅仅断开连接，并不清除内存
		uv_close((uv_handle_t*)&client->tcp, NULL);
		iter->second->is_online = enum_offline;
	}
	unlock_;
	if (client != NULL){
		client->clean();
	}
	return true;
}

size_t client_size(){
	return map_.size();
}
void client_clean(){
	lock_;
	auto iter = map_.begin();
	while (iter != map_.end()){
		//删除所有挂接的缓存
		client_t *client = iter->second;
		uv_close((uv_handle_t*)&client->tcp, NULL);
		client->clean();
		free(client);
		free(iter->second);
		iter++;
	}
	map_.clear();
	unlock_;
}
int client_send(uint32_t deviceid, tcp_unit_w *unit)
{
	if (unit == NULL)
		return -1;
	int ret = -1;
	unit->tcphandle = NULL;
	lock_;
	auto iter = map_.find(deviceid);
	if (iter != map_.end())
	{
		ret = 0;
		client_t * cli = iter->second;
		//client_t * cli = cbuf->client;
		if (cli != NULL && cli->is_online == enum_online)
		{
			unit->tcphandle = &cli->tcp;
		}
		else //not online ,insert into our buffer
		{
			if (unit->type == enum_sc) //如果是数据包是不缓存的
				return -1;
			printf("device not online , inert into buffer\n");
			tcp_unit_w *tmp = cli->buffer_data_w;
			if (tmp == NULL){
				cli->buffer_data_w = unit;
			}
			else {//to the end of the link
				for (; tmp != NULL && tmp->next != NULL; tmp = tmp->next);
				tmp = unit;
			}
		}//else 
	}
	else{
		printf("device never online , please wait until it online once\n");
	}
	unlock_;
	if (unit->tcphandle != NULL)
		write_data(unit);
	else
		free(unit);


	return ret;

}

void client_init(){
	initlock_;
}

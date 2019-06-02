#ifndef _CLIENT_H_
#define _CLIENT_H_

#include <stdint.h>
#include <stdio.h>
#include <string>
#include "protocol.h"

using namespace std;



//删除掉的时候必须调用的函数
//typedef void(*uv_clean_call)(client_t * handle,
//	char *buffer, size_t size);

void client_init();
void client_clean();
int  client_send(uint32_t deviceid, tcp_unit_w *unit);
size_t client_size();
int client_offline(uint32_t key);
int client_exists(uint32_t key);
int client_push(uint32_t key, client_t *obj);
int write_buffer_packet(uint32_t id);




#endif
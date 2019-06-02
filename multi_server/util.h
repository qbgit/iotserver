#pragma once
#ifndef _H_UTIL_H_
#define _H_UTIL_H_

#include <string.h>
#include "cJSON.h"
#ifdef WIN32
#else
#include<dlfcn.h>
#endif

static void split(char *src, const char *separator, char **dest, int *num) {
	char *pNext;
	int count = 0;
	if (src == NULL || strlen(src) == 0)
		return;
	if (separator == NULL || strlen(separator) == 0)
		return;
	pNext = strtok(src, separator);
	while (pNext != NULL) {
		*dest++ = pNext;
		++count;
		pNext = strtok(NULL, separator);

	}
	*num = count;
}
//查找
static int endswith(const char * value, const char * search)
{
	//扫描字符串里面有没有.
	//找出文件后缀名称
	size_t len, len1 = 0;
	len = strlen(value);
	len1 = strlen(search);
	if (len1 < len) {
		//abc.jpg
		//jpg
		char * pos = (char*)value + len - len1 - 1;
		if (*pos == '.')
		{
			return strcmp(++pos, search);
		}
	}

	return -1;

}

static int analyse_http_json(const char * json)
{
	//char *s = "{\"name\":\"xiao hong\",\"age\":10}";
	cJSON *root = cJSON_Parse(json);
	if (!root) {
		printf("get root faild !\n");
		return -1;
	}
	cJSON *name = cJSON_GetObjectItem(root, "name");
	if (!name) {
		printf("No name !\n");
		return -1;
	}
	printf("name type is %d\n", name->type);
	printf("name is %s\n", name->valuestring);
	cJSON *age = cJSON_GetObjectItem(root, "age");
	if (!age) {
		printf("no age!\n");
		return -1;
	}
}
20 printf("age type is %d\n", age->type);
21 printf("age is %d\n", age->valueint);

//加载linux动态链接库


typedef int(*PFUNC_CALL)(int, int);
typedef int(*PFUNC_SHOW)(int);        //给函数指针定义一个别名   PFUNC_SHOW = int(*)(int)

static int load()
{
	return 0;
}


int main()
{

	void *handle = dlopen("./libmath.so", RTLD_LAZY);  //加载动态库
	if (handle == NULL)
	{
		printf("dlopen失败：%s\n", dlerror());
		return -1;
	}
	//获取函数地址
	PFUNC_CALL add = (PFUNC_CALL)dlsym(handle, "add");
	if (add == NULL)
	{
		printf("dlsym失败:%s\n", dlerror());
		return -1;
	}


	int(*show)(int) = dlsym(handle, "show");              //int(*show)(int) = PFUNC_SHOW
	if (add == NULL)                                       // 函数原型  int show(int)
	{
		printf("dlsym失败:%s\n", dlerror());
		return -1;
	}

	show(add(9, 5));

	dlclose(handle);
	return 0;

}
//加载windows动态链接库


#endif
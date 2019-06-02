#ifndef _UTIL_H_
#define _UTIL_H_


#include <uv.h>
#include <iostream>
#include <fstream>

#include <string>
#include <stdarg.h>
#include "protocol.h"
#include <ctime>
#include "rapidjson/document.h"     // rapidjson's DOM-style API
#include "rapidjson/prettywriter.h" // for stringify JSON
#include "rapidjson/filestream.h"   // wrapper of C stream for prettywriter as output

using namespace rapidjson;
using namespace std;


#if (defined DEBUG) || (defined _DEBUG)
#define CHECK(status, msg) \
  if (status != 0) { \
    fprintf(stderr, "%s: %s\n", msg, uv_err_name(status)); \
    exit(1); \
      }
#define UVERR(err, msg) fprintf(stderr, "%s: %s\n", msg, uv_err_name(err))
#define LOG_ERROR(msg) puts(msg);
#define LOG(msg) puts(msg);
#define LOGF(...) printf(__VA_ARGS__);
#else
#define CHECK(status, msg)
#define UVERR(err, msg)
#define LOG_ERROR(msg)
#define LOG(msg)
#define LOGF(...)
#endif


static void sleep()
{
	/*for(;;){
	test_http();
	std::this_thread::sleep_for(std::chrono::milliseconds(5*1000));
	}*/
}
/*
得到当前时间
*/
static int get_time_1() {
	std::time_t t = std::time(nullptr);
	tm * t1 = std::gmtime(&t);
	int year = t1->tm_year + 1900;
	int month = t1->tm_mon + 1;
	int day = t1->tm_mday;
	cout << t1->tm_year + 1900 << "-" << t1->tm_mon + 1 << "-" << t1->tm_mday << endl;
	if (year > 2018 || month > 1 || year < 2018)
		return -1;
	return 0;

}

/*去掉空格*/
static void trim(string & ret)
{
	size_t i = 0;
	size_t len = 0;// ret.length();
	while (1) {
		len = ret.length();
		if (len <= 0)
			break;
		if (ret[0] == ' ')
			ret.erase(0);
		else
			break;
	}
	while (1) {
		len = ret.length();
		if (len <= 0)
			break;
		if (ret[len - 1] == ' ')
			ret.erase(len - 1);
		else
			break;
	}


}
//解析json，到一个map表
static int json_analyse(string & ret)
{
	trim(ret);
	size_t len = ret.length();
	string info;
	if (len > 3)
	{
		if (ret[0] == '{' && ret[len - 1] == '}')
		{
			size_t t = ret.find("\"info\":");
			info = ret.substr(t + 7);
		}
		else
			return -1;
	}
}


static string Format(const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	char buf[256];
	//sprintf_s(buf, 255, fmt, args);
	vsprintf_s(buf, 255, fmt, args);
	//wvnsprintf()
	//wvnsprintf(buf, 256, fmt, args);
	va_end(args);
	return string(buf);
}

static char* InReadConfig(const char *filename, Document *doc)
{
	char * buffer = NULL;
	int ret = -1;
	long size;
	string pname = "" ;
	char exebuffer[256];
	size_t len = 256;
	ret = uv_exepath(exebuffer, &len);
	//printf(exebuffer);
	//GetAppExePath(pname);
	if (filename == NULL)
		pname += "./config.json";
	else
		pname += filename;
	ifstream in(pname.c_str(), ios::in | ios::binary | ios::ate);
	size = (long)in.tellg();
	in.seekg(0, ios::beg);
	buffer = new char[size+1];
	in.read(buffer, size);
	in.close();
	buffer[size] = '\0';

	//Document document;  
	// Default template parameter uses UTF8 and MemoryPoolAllocator.
	if (doc->ParseInsitu(buffer).HasParseError()){

		delete[]buffer;
		return NULL;
	}
	return buffer;
}



static bool member_exists(Document &object, const char *member){
#define MEMBER_ITER(iter) rapidjson::Value::ConstMemberIterator iter 
#define MEMBER_FIND(x) MEMBER_ITER(i) = object.FindMember(x);if(i != object.MemberEnd())
	MEMBER_FIND(member)
		return true;
	return false;
}
static bool members_exists(Document &object, const char *member, const char * childmember){
	MEMBER_FIND(member)
	{
		return object[member].FindMember(childmember) != object.MemberEnd();
	}
	return false;
}
static int read_config_protocol(const char * filename, tcp_settings &setting)
{
#define value_extst(x,y) if(members_exists(object,x,y))
#define value_extst_1(x) if(member_exists(object,x))
	Document object;
	char * buffer = NULL;
	if ((buffer = InReadConfig(filename, &object)) == NULL)
	{
		return -1;
	}
	//group.server = object["server"].GetString();
	try{
		int groupsize = object["urls"].Size();
		//if ()
		value_extst_1("head")
		{
			//setting.head = object["head"]["offset"].GetUint();
			setting.headlength = object["head"]["len"].GetInt();
		}
		value_extst_1("datatype")
			setting.datatype = object["datatype"].GetInt();
		value_extst("deviceid", "offset")
			setting.idoffset = object["deviceid"]["offset"].GetInt();
		value_extst("cmd", "len")
			setting.cmdlength = object["cmd"]["len"].GetInt();
		value_extst("contentlen", "len")
			setting.content_len = object["contentlen"]["len"].GetInt();
		value_extst("contentlen", "offset")
			setting.content_offset = object["contentlen"]["offset"].GetInt();
		value_extst("contentlen","includeht")
			setting.includeht = object["contentlen"]["includeht"].GetInt();
		value_extst_1("includecrcend")
			setting.includecrcend = object["includecrcend"].GetInt();
		value_extst_1("crclen")
			setting.crclen = object["crclen"].GetInt();
		else
			setting.crclen = 0;
		

		value_extst_1("headfields")
			setting.headfieldslen = object["headfields"]["len"].GetInt();
		value_extst_1("end_start")
		{
			setting.end_start = object["end_start"].GetInt();
			setting.end = object["end"].GetInt();
		}
		else{
			setting.end_start = 0;
			setting.end = 0;
		}
		value_extst_1("headfields"){
			setting.headfieldslen = object["headfields"]["len"].GetInt();
		}
		value_extst("timestamp", "offset") {
			setting.timestamp_offset = object["timestamp"]["offset"].GetInt();
		}
		value_extst("timestamp","len"){
			setting.timestamp_len = object["timestamp"]["len"].GetInt();
		}
		value_extst("type", "offset") {
			setting.type_offset = object["type"]["offset"].GetInt();
		}
		value_extst("type", "len") {
			setting.type_len =  object["type"]["len"].GetInt();
		}
		value_extst("memo", "offset") {
			setting.memo_offset = object["memo"]["offset"].GetInt();
		}
		value_extst("memo", "len") {
			setting.memo_len = object["memo"]["len"].GetInt();
		}

		//0 bigendian 1 little endian
		value_extst_1("b1")
			setting.bl = object["bl"].GetInt();
		if (buffer != NULL) {
			free(buffer);
			buffer = NULL;
		}
	} 
	catch(...)
	{
		if (buffer != NULL)
			free(buffer);
		cout << "error! parse the json config file " << endl;
		return -1;
	}
	return 0;
}



#endif
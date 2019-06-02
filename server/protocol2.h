#pragma once
/*
author : qianbo
email  : 418511899@qq.com
func   : protocol2
time   ：2019-01-23
*/
#include "stdio.h"
#include "stdint.h"
#include "uv.h"
#include <string>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
#include <map>
using namespace std;
//第二代数据结构
typedef struct unit
{
	//string  name;
	char  offset;
	char  len;
	int   content;
}unit;

typedef struct tcp_set
{
	vector<string> urls;
	map<string, unit> settings;
	char datatype;
	int hlen_calc = -1;
	uv_loop_t * uv_loop = NULL;
}tcp_set;



class config
{
	char * buffer = NULL;
protected:
	string exepath() {
		char exebuffer[256];
		size_t len = 256;
		uv_exepath(exebuffer, &len);
		return string(exebuffer);
	}
public:
	config()
	{}
	~config()
	{
		if (buffer != NULL)
			delete[]buffer;
	}
	
public:
	string read_config_prorocol2(const char *filename, tcp_set *setting)
	{
		ifstream in(filename , ios::in | ios::binary | ios::ate);
		//long size = (long)in.tellg();
		in.seekg(0, ios::beg);

		string test;
		ostringstream ostr;
		if (!in.bad()) {

			string str;
			//const int LINE_LENGTH = 256;
			//char str[LINE_LENGTH];
			vector<string> s1;
			while (getline(in, str))
			{
				s1.push_back(str);
				ostr << str << ",";
			}

			for (int i = 0; i < s1.size(); i++)
			{
				//str += s1[i];
				cout << s1[i] << endl;
			}
			cout << str << endl;
			//cout << ostr.str()<< endl;
			//string str;
			//while(in >> str )
			//while (std::getline(in, str))
			//{
				//cout << str << " ";
				//str.replace()
				//test += str;
			//	cout << test;
			//}
		}
		return test;
	}
	string readdefault()
	{
		return read_config_prorocol2("config_flv.json", NULL);
	}

};

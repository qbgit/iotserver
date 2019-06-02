#ifndef _PROTOCOL_H_
#define _PROTOCOL_H_

/*
author : qianbo
email  : 418511899@qq.com
func   : protocol
time   ��2018-10-4
*/
#include "stdio.h"
#include "stdint.h"
#include "uv.h"

/*
   ��˻���С��
*/
typedef enum endian_{
	enum_big = 0,
	enum_little,
}endian_;
/*
  �Ƿ�����
*/
typedef enum online_
{
	enum_online = 1,
	enum_offline = 0
}online_;

typedef enum recv_status_
{
	enum_head = 0, //����ͷ��
	enum_body = 1  //���հ���
}recv_status;

typedef enum packtype_
{
	enum_none = 0,
	enum_cs = 1,       //���շ�������
	enum_sc = 2,       //�������ݰ�
	enum_control = 3,  //����
	enum_config  = 4,  //����
	enum_query   = 5,  //��ѯ
	enum_total = 6
}packtype_;


typedef struct tcp_unit
{
	//��ʵ����
	char * data = NULL;
	//�����ܳ���bodylen + headlen
	int tlen = 0;
	//���յ������ݳ���
	int recvlen = 0;
	//ͷ������
	int headlen = 0;
}tcp_unit;



typedef struct tcp_unit_w{
	char * data = NULL;
	int len = 0;
	packtype_ type = enum_cs;
	uv_tcp_t *tcphandle = NULL;
	//�ӳٶ��ٺ��뷢��
	int delay = 0;
	struct tcp_unit_w *next = NULL;
}tcp_unit_w;

typedef struct urls
{
	char * url;
	urls * next;
}urls;

//Э��ͷ���ͻص���������
typedef struct tcp_settings
{
	urls url;
	//datatype ��ͬ����
	char datatype = 0;
	unsigned char head = 0x69;
	//�����ͷ����ʼλ��һ��Ϊ0 һ���ֽ���0x69
	char headoffset = 0;
	//��ʼͷ��Ϊ0 ���� 1���ֽ� .2 .3 .4
	char headlength = 0;
	//�ӵ�һ���ֽڿ�ʼ��id����0��ʼ��
	char idoffset = 1;
	//4���ֽ�Ϊid���ȣ��豸id�ĳ���
	char idlength = 4;
	//�����
	char cmdlength = 1;
	//����ƫ����
	char cmdoffset = 5;
	//����������ݵ�ֻ��һ���ֽ�
	char content_len = 1;
	//��������ȵ��ǵ�6���ֽ�Ϊ��ʼλ�ã���0��ʼ��
	char content_offset = 6;

	char includeht = 0;
	//�Ƿ����crcУ���end
	char includecrcend = 0;
	// 0 ��û��У�飬2��2���ֽ�crc16 
	char crclen = 2; 

	char headfieldslen = 0;
	
	char timestamp_offset = 0;
	char timestamp_len = 0;
	char type_offset = 0;
	char type_len = 0;
	char memo_offset = 0;
	char memo_len = 0;

	//һ�������ֽڣ����Ϊ�������end
	char end_start = 1;
	//end_start ���Ϊ�������
	unsigned char end = 0x16;
	
	/*���� 
	big endian 0 
	little endian 1
	*/
	char bl = enum_big;

	/*��������ĳ���
	   ��ʼ��Ϊ-1,
	   �������0
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
	//����ʹ��
	tcp_settings * config = NULL;
	//д��
	uv_write_t write_req;
	//�豸id
	uint32_t   deviceid = 0;
	//���������
	tcp_unit * buffer_data = NULL;
	
	//��Ҫд�������
	tcp_unit_w * buffer_data_w = NULL;
	
	//�128�ֽڰ�ͷ
	char head[128];
	//�Ѿ����յ�ͷ���ĳ���
	int recvlen;

	//�û��Զ�������ָ�� tcpserver
	void * data = NULL;
	
	//����״̬ ����ͷ����0 �������� 1
	recv_status status = enum_head; 

	//�Ƿ�����
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
		//��������ָ��,���������߳�
		request.data = this;
	}
	uint32_t id =0;
	client_t* client = NULL;
	//�����ݽӹ������д���
	tcp_unit * data = NULL;
	uv_work_t request;
	bool error;
}thread_work;

static int get_headlen(tcp_settings * setting)
{
	if (setting->hlen_calc <=0)
	{
		if (setting->datatype == 0) // ����ͨ
			setting->hlen_calc = setting->headlength + setting->idlength + setting->cmdlength + setting->content_len + setting->headfieldslen;
		else if (setting->datatype == 1)
			//����ĸ���deviceid
			setting->hlen_calc = setting->content_len + setting->timestamp_len
			+ setting->cmdlength + setting->type_len + setting->memo_len + setting->idlength;
	}
	return setting->hlen_calc;
}

static int get_bodylen(tcp_settings * setting, char *head)
{
	//headlen_offset ��ƫ����+1
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
	else if (setting->content_len == 4){ //����ĸ��ֽڳ���
		uint32_t * pos1 = (uint32_t*)(pos);
		if (setting->bl == enum_big)
			blen = ntohl(*pos1);
		else
			blen += *pos1;
	}
	int retlen = blen + slen;
	//�õ��ĳ��Ȱ���ͷ������
	if (setting->includeht == 1){
		retlen -= get_headlen(setting);
	}
	//�õ��ĳ��Ȱ�����β����crcУ��,��ȥ
	if (setting->includecrcend == 1)
		retlen -= slen;
	return retlen;
}




#endif
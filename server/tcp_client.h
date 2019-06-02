#pragma once
class tcp_client
{
public:
	tcp_client();
	~tcp_client();
};


class TCPClient
{
	//ֱ�ӵ���connect/connect6���������  
public:
	TCPClient(uv_loop_t* loop = uv_default_loop());
	virtual ~TCPClient();
	static void StartLog(const char* logpath = nullptr);//������־�����������Ż�������־  
public:
	//��������  
	virtual bool connect(const char* ip, int port);//����connect�̣߳�ѭ���ȴ�ֱ��connect���  
	virtual bool connect6(const char* ip, int port);//����connect�̣߳�ѭ���ȴ�ֱ��connect���  
	virtual int  send(const char* data, std::size_t len);
	virtual void setrecvcb(client_recvcb cb, void* userdata);////���ý��ջص�������ֻ��һ��  
	void close();

	//�Ƿ�����Nagle�㷨  
	bool setNoDelay(bool enable);
	bool setKeepAlive(int enable, unsigned int delay);

	const char* GetLastErrMsg() const {
		return errmsg_.c_str();
	};
protected:
	//��̬�ص�����  
	static void AfterConnect(uv_connect_t* handle, int status);
	static void AfterClientRecv(uv_stream_t *client, ssize_t nread, const uv_buf_t* buf);
	static void AfterSend(uv_write_t *req, int status);
	static void onAllocBuffer(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf);
	static void AfterClose(uv_handle_t *handle);

	static void ConnectThread(void* arg);//������connect�߳�  
	static void ConnectThread6(void* arg);//������connect�߳�  

	bool init();
	bool run(int status = UV_RUN_DEFAULT);
private:
	enum {
		CONNECT_TIMEOUT,
		CONNECT_FINISH,
		CONNECT_ERROR,
		CONNECT_DIS,
	};
	uv_tcp_t client_;//�ͻ�������  
	uv_loop_t *loop_;
	uv_write_t write_req_;//дʱ����  
	uv_connect_t connect_req_;//����ʱ����  
	uv_thread_t connect_threadhanlde_;//�߳̾��  
	std::string errmsg_;//������Ϣ  
	uv_buf_t readbuffer_;//�������ݵ�buf  
	uv_buf_t writebuffer_;//д���ݵ�buf  
	uv_mutex_t write_mutex_handle_;//����write,����ǰһwrite��ɲŽ�����һwrite  

	int connectstatus_;//����״̬  
	client_recvcb recvcb_;//�ص�����  
	void* userdata_;//�ص��������û�����  
	std::string connectip_;//���ӵķ�����IP  
	int connectport_;//���ӵķ������˿ں�  
	bool isinit_;//�Ƿ��ѳ�ʼ��������close�������ж�  
};

}

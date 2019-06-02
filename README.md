# iot_tcpserver

    从json配置文件中读取不同的tcp 头部设置，可以适应不同的协议
    中达物联网tcpserver,非业务层,高并发接受数据传送到node
    it can work now
    已经验证 tcpClient_8051 

## 2018-10-07 更新
  
    增加httpserver 的httpparse<br>

## 2018-10-07 更新
 
    获取设备id号，增加httpclient post 和 get协议，不用加载其他httpclient
    使用libuv直接发送http，接收不能超过4096字节，后期优化





# 项目介绍

  ## 物联网接收从各个传感器与摄像头的数据

  ### 1传感器数据
 
    可以配置为传感器数据的服务器，接收传感器4g modbus协议或者自定义协议 <br>
  
  ### 2摄像头数据
    
    准备接收推流flv，手机端，android和ios端两种硬编码数据流。两个手机端将逐步开放
  
  ### 3iot_tcpserver 本身也将可以配置成为流媒体server。
    
    作为httpflv和fmp4流媒体server

# 软件架构
 
    ## cs 与 bs 架构结合
  
    架构说明将会上传word文档和ppt

## 安装教程

    1 把config_805x.json 放入到bin文件夹中 <br>
    2. 程序读取配置 不同的协议头部使用不同的config文件 <br>
    3. 启动等待数据 <br>

## 类说明：
    
    从tcp_server 继承 得到三个基本事件<br>
    1 on_headers_complete <br>
    2 on message_complete <br>
    3 on_data <br>
      和message_complete 功能类似，但是有直接的数据指针和长度 <br>
    

    class tcp_server1 :public tcp_server
    {
       ---//所有客戶端，用四字节整形数作为hash key
	    std::map<uint32_t, client_t*>  _map_c;
	    public:
	    int on_headers_complete(void *param) {
		  client_t * pclient = (client_t*)param;
		  printf("the header len is %d\n", pclient->recvlen);
		  return 0;
	  }
	  int on_message_complete(void *param) {
		client_t * pclient = (client_t*)param;
		tcp_unit * data = pclient->buffer_data;
		char * buf = data->data;
		int len = data->tlen;
		printf("the total len is %d\n", pclient->buffer_data->tlen);

	#ifdef _DEBUG
		for (size_t i = 0; i < len; i++) {
			printf("%02x ",(uint8_t) buf[i]);
		}
		printf("\n");
	#endif

		return 0;
	}

	int on_data(uint8_t * data, int len) {
		printf("the thread pid is %d\n", _getpid());

		return 0;
       }
    };
  
	
    
  <br>  
    程序单个线程接收后将使用线程池处理数据，后期会加入多进程方式处理数据


  <br>  
	   程序初步完成测试

    启动服务器后，测试程序 tcpClient_8051.js
    使用 node  tcpClient_8051.js 启动客户端程序，服务器端打印相应的数据信息。
    启动文件说明：

      {
        "urls": [
		{ "url": "http://192.168.1.23/postdata" },
		{ "url": "http://192.168.1.223/postdata" }
        ],
        "data": { "test": "1" },
	    "head":{"offset":1,"len":1, content:"0x69"},
	    "deviceid": { "offset": 1, "len": 4 },
	    "cmd": { "offset": 5, "len": 1 },
	    "contentlen": { "offset": 6, "len": 1, "includeht": 0 },
	    "crclen": 2,
	    "end_start": 1,
	    "end": 22,
	    "bl": 0
      }
      说明:
        urls: 发送数据的httpserver地址
		data：回送数据
		head：接收的头部字节  offset 以0 为开始
		deviceid ：设备id
		cmd :命令
		contentlen: 表明内容长度字节 ， includeht，长度是否包含头部字节
		crclen: 是否有crc校验 0 为无，有则为crc校验长度
		end_start:是否有结尾字节
		end:结尾字节是多少，22 为0x16 ，end_start 为0时跳过 

# todolist
   
    1 视频httpflv     下一步
    2 协议为字符，且\r\n为结束等方式的物联网发送 下一步
	  3 验证8052 8053   正在进行
	  4 数据库存储      正在进行
    5 http协议发送，  正在进行
	  6 多进程方式      规划中
	  7 分布式分发服务  规划中
	  8 存储服务        规划中
	
# 参与贡献

    qianbo
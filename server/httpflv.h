#pragma once
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "TThreadRunable.h"
#include "TTime.h"
typedef struct sl_buffer {
	uint8_t * buf = NULL;
	size_t blen = 0;
}sl_buffer;

typedef struct sp_buffer {
#define EXT_LEN sizeof(pre)

	sl_buffer _sps;
	sl_buffer _pps;
	sl_buffer _spspps;

	inline void  reset_sl_buffer(sl_buffer *sp, size_t len) {
		if (sp->buf != NULL)
			delete[]sp->buf;
		sp->buf = new uint8_t[len];
		sp->blen = len;
	}
	sl_buffer *decode_sps() {
		uint8_t pre[] = { 0x00, 0x00, 0x01 };
		if (_spspps.blen > 0)
			return &_spspps;
		size_t len = _sps.blen + _pps.blen;
		if (len == 0)
			return NULL;
		if (_sps.buf != NULL && _pps.buf != NULL) {
			sl_buffer * sp = &_spspps;
			reset_sl_buffer(sp, len + EXT_LEN * 2);
			uint8_t *pos = _spspps.buf;
			memcpy(pos, &pre[0], EXT_LEN);
			pos += EXT_LEN;
			memcpy(pos, _sps.buf, _sps.blen);
			pos += _sps.blen;
			memcpy(pos, &pre[0], EXT_LEN);
			pos += EXT_LEN;
			memcpy(pos, _pps.buf, _pps.blen);
			return sp;
		}
		return NULL;
	}
	void slbuf_set(sl_buffer *buf, uint8_t *c, size_t len) {
		reset_sl_buffer(buf, len);
		memcpy(buf->buf, c, len);
		buf->blen = len;
	}

	void sps_set(uint8_t *sps, size_t len) {
		slbuf_set(&_sps, sps, len);
	}
	void pps_set(uint8_t *pps, size_t len) {
		slbuf_set(&_pps, pps, len);
	}
	int sp_status = 0;
	sl_buffer *sps() {
		return &_sps;
	}
	sl_buffer *pps() {
		return &_pps;
	}
	sp_buffer() {}
	~sp_buffer() {
#define DEL_SL_BUF(x) if(x.buf!=NULL) delete []x.buf;
		DEL_SL_BUF(_sps);
		DEL_SL_BUF(_pps);
		DEL_SL_BUF(_spspps);
	}
}sp_buffer;

#if 0
class h5client
{
public:
	int writebufferto(const char *buffer, size_t size)
	{
		return 0;
	}
};

class httpflv :public TThreadRunable
{
private:
	sp_buffer _spbuffer;
	sl_buffer _flvhead;
	sl_buffer _flvdst;
	//stru_rr_param _param;
	//stru_rr_param * _pm = nullptr;
	//rtspflv_live * _rtsp = nullptr;

	int v_headlen = 0;
	//audio
	//TCapSoundSender _cap_sound_sender;
	//FFSimpleEncoder _audio_encoder2;
	//音频编码线程
	//TEnCodeThread _aencode_thread;
	//TQPacket *_audiopkt = nullptr;
	//send to server
	h5client _h5client;
	//这里要增加flv websocket server
	uint32_t _recv_stamp = 0;
	uint32_t _first_stamp = 0;

	std::mutex  _mutex;
	//int _video_ok = 0;

	uint32_t _first_key = 0;
	uint32_t _flvheader = 0;

public:

	class Lock {
	private:
		std::unique_lock<std::mutex> _lock;
	public:
		inline Lock(httpflv* parent) : _lock(parent->_mutex) {}
	};
	void sps_pps_callback(int nalu, uint8_t * data, size_t size)
	{
		if (nalu == 0x07)
		{
			++_spbuffer.sp_status;
			_spbuffer.sps_set(data, size);
		}
		else if (nalu == 0x08)
		{
			++_spbuffer.sp_status;
			_spbuffer.pps_set(data, size);
		}
		if (_spbuffer.sp_status == 2) {
			sl_buffer * sps = _spbuffer.sps();
			sl_buffer * pps = _spbuffer.pps();
			writespspps(sps->buf, sps->blen, pps->buf, pps->blen, 0);
		}
	}

#if 0
	int CallbackAudio_pcm(void *frame)
	{
		if (_rtmpOut.GetConnectStatus() == 0)
			return -1;
		//you can process the audio here
		_aencode_thread.Push((TQFrame*)frame);
		return 0;
	}

	int CallbackAudio_Encode(void *frame)
	{
		//int ret = -1;
		TQFrame * revframe = (TQFrame*)frame;
		if (_audiopkt == nullptr)
			_audiopkt = TQPacket::TPacketAlloc(NULL, 4096, AUDIO_PACKET);
		uint8_t *dest = (uint8_t*)_audiopkt->avdata;
		int len = 4096;
		if (_audio_encoder2.Encode2(revframe->_buffer, revframe->_len, dest, len) != -1)
		{
			_audiopkt->len = len;
			_audiopkt->pts = GetTimestamp32() - _first_stamp; // pkt->pts 
			if (_video_ok == 0)
				return -1;
			Lock lock(this);
			return _rtmpOut.SendPacketAudioFrame(_audiopkt);
		}
		else
		{
			printf("audio encoder error\n");
			return -1;
		}
	}
#endif
#define FLV_TAG_HEAD_LEN 11
#define FLV_PRE_TAG_LEN 4
#define SOCKRET(x) if(x<0) return -1
	int send_data(uint8_t *content, size_t size) {
		int sockret = -1;

		sockret = _h5client.writebufferto((const char*)(&size), 4);
		SOCKRET(sockret);
		sockret = _h5client.writebufferto((const char*)content, size);
		SOCKRET(sockret);

		printf("send data %d\n", size);
		return 0;
	}

	int writespspps(uint8_t * sps, uint32_t spslen, uint8_t * pps, uint32_t ppslen, uint32_t timestamp)
	{
		//if (isfirst){
		uint8_t flv_header[13] = { 0x46, 0x4c, 0x56, 0x01, 0x01, 0x00, 0x00, 0x00, 0x09, 0x00, 0x00, 0x00, 0x00 };
		uint32_t body_len = spslen + ppslen + 16;
		uint32_t output_len = 13 + body_len + FLV_TAG_HEAD_LEN + FLV_PRE_TAG_LEN;
		uint8_t * output = (uint8_t*)malloc(output_len);
		uint32_t offset = 0;
		memcpy(output, flv_header, 13);
		offset = 13;
		// flv tag header
		output[offset++] = 0x09; //tagtype video
		output[offset++] = (uint8_t)(body_len >> 16); //data len
		output[offset++] = (uint8_t)(body_len >> 8); //data len
		output[offset++] = (uint8_t)(body_len); //data len
		output[offset++] = (uint8_t)(timestamp >> 16); //time stamp
		output[offset++] = (uint8_t)(timestamp >> 8); //time stamp
		output[offset++] = (uint8_t)(timestamp); //time stamp
		output[offset++] = (uint8_t)(timestamp >> 24); //time stamp
		output[offset++] = 0x00; //stream id 0
		output[offset++] = 0x00; //stream id 0
		output[offset++] = 0x00; //stream id 0

								 //flv VideoTagHeader
		output[offset++] = 0x17; //key frame, AVC
		output[offset++] = 0x00; //avc sequence header
		output[offset++] = 0x00; //composit time ??????????
		output[offset++] = 0x00; // composit time
		output[offset++] = 0x00; //composit time

								 //flv VideoTagBody --AVCDecoderCOnfigurationRecord
		output[offset++] = 0x01; //configurationversion
		output[offset++] = sps[1]; //avcprofileindication
		output[offset++] = sps[2]; //profilecompatibilty
		output[offset++] = sps[3]; //avclevelindication
		output[offset++] = 0xff; //reserved + lengthsizeminusone
		output[offset++] = 0xe1; //numofsequenceset
		output[offset++] = (uint8_t)(spslen >> 8); //sequence parameter set length high 8 bits
		output[offset++] = (uint8_t)(spslen); //sequence parameter set  length low 8 bits
		memcpy(output + offset, sps, spslen); //H264 sequence parameter set
		offset += spslen;
		output[offset++] = 0x01; //numofpictureset
		output[offset++] = (uint8_t)(ppslen >> 8); //picture parameter set length high 8 bits
		output[offset++] = (uint8_t)(ppslen); //picture parameter set length low 8 bits
		memcpy(output + offset, pps, ppslen); //H264 picture parameter set

											  //no need set pre_tag_size ,RTMP NO NEED
											  // flv test 
		offset += ppslen;
		uint32_t fff = body_len + FLV_TAG_HEAD_LEN;
		output[offset++] = (uint8_t)(fff >> 24); //data len
		output[offset++] = (uint8_t)(fff >> 16); //data len
		output[offset++] = (uint8_t)(fff >> 8); //data len
		output[offset++] = (uint8_t)(fff); //data len
		if (send_data(output, output_len) < 0) {
			//RTMP Send out
			free(output);
			return -1;
		}
		else {
			free(output);
		}
		return 0;
	}

	//同时写入sps和pps
	int writeavcframe_0(uint8_t * nal, uint32_t nal_len, uint32_t timestamp, bool IsIframe) {
		uint32_t body_len = nal_len + 5 + 4; //flv VideoTagHeader +  NALU length
		uint32_t output_len = body_len + FLV_TAG_HEAD_LEN + FLV_PRE_TAG_LEN;
		uint8_t * output = _flvdst.buf;
		size_t spslen = 0;
		size_t ppslen = 0;
		uint32_t offset = 0;

		if (IsIframe) { //关键帧计算长度
			spslen += _spbuffer.sps()->blen;
			//关键帧加上spspps
			ppslen += _spbuffer.pps()->blen;
			body_len += (spslen + ppslen + 8);
			output_len += (spslen + ppslen + 8); //加上两个开头
		}
		// flv tag header
		output[offset++] = 0x09; //tagtype video
		output[offset++] = (uint8_t)(body_len >> 16); //data len
		output[offset++] = (uint8_t)(body_len >> 8); //data len
		output[offset++] = (uint8_t)(body_len); //data len
		output[offset++] = (uint8_t)(timestamp >> 16); //time stamp
		output[offset++] = (uint8_t)(timestamp >> 8); //time stamp
		output[offset++] = (uint8_t)(timestamp); //time stamp
		output[offset++] = (uint8_t)(timestamp >> 24); //time stamp
		output[offset++] = 0x00; //stream id 0
		output[offset++] = 0x00; //stream id 0
		output[offset++] = 0x00; //stream id 0


								 //flv VideoTagHeader
		if (IsIframe) {
			output[offset++] = 0x17; //key frame, AVC
		}
		else
			output[offset++] = 0x27; //key frame, AVC

		output[offset++] = 0x01; //avc NALU unit
		output[offset++] = 0x00; //composit time ??????????
		output[offset++] = 0x00; // composit time
		output[offset++] = 0x00; //composit time

		if (IsIframe)
		{
			output[offset++] = (uint8_t)(spslen >> 24); //nal length 
			output[offset++] = (uint8_t)(spslen >> 16); //nal length 
			output[offset++] = (uint8_t)(spslen >> 8);  //nal length 
			output[offset++] = (uint8_t)(spslen);       //nal length 
			memcpy(output + offset, _spbuffer.sps()->buf, spslen);
			offset += spslen;

			output[offset++] = (uint8_t)(ppslen >> 24); //nal length 
			output[offset++] = (uint8_t)(ppslen >> 16); //nal length 
			output[offset++] = (uint8_t)(ppslen >> 8);  //nal length 
			output[offset++] = (uint8_t)(ppslen);       //nal length 

			memcpy(output + offset, _spbuffer.pps()->buf, ppslen);
			offset += ppslen;
		}

		//size_t nal_len1 = nal_len + spsppslen;
		output[offset++] = (uint8_t)(nal_len >> 24); //nal length 
		output[offset++] = (uint8_t)(nal_len >> 16); //nal length 
		output[offset++] = (uint8_t)(nal_len >> 8); //nal length 
		output[offset++] = (uint8_t)(nal_len); //nal length 

		memcpy(output + offset, nal, nal_len);

		//no need set pre_tag_size ,RTMP NO NEED
		offset += nal_len;
		uint32_t fff = body_len + FLV_TAG_HEAD_LEN;
		output[offset++] = (uint8_t)(fff >> 24); //data len
		output[offset++] = (uint8_t)(fff >> 16); //data len
		output[offset++] = (uint8_t)(fff >> 8); //data len
		output[offset++] = (uint8_t)(fff); //data len

		return send_data(output, output_len);
	}
	int writeavcframe(uint8_t * nal, uint32_t nal_len, uint32_t timestamp, bool IsIframe)

	{
		uint32_t body_len = nal_len + 5 + 4; //flv VideoTagHeader +  NALU length
		uint32_t output_len = body_len + FLV_TAG_HEAD_LEN + FLV_PRE_TAG_LEN;
		uint8_t * output = _flvdst.buf; // (uint8_t*)malloc(output_len);
		uint32_t offset = 0;
		// flv tag header

		output[offset++] = 0x09; //tagtype video
		output[offset++] = (uint8_t)(body_len >> 16); //data len
		output[offset++] = (uint8_t)(body_len >> 8); //data len
		output[offset++] = (uint8_t)(body_len); //data len
		output[offset++] = (uint8_t)(timestamp >> 16); //time stamp
		output[offset++] = (uint8_t)(timestamp >> 8); //time stamp
		output[offset++] = (uint8_t)(timestamp); //time stamp
		output[offset++] = (uint8_t)(timestamp >> 24); //time stamp
		output[offset++] = 0x00; //stream id 0
		output[offset++] = 0x00; //stream id 0
		output[offset++] = 0x00; //stream id 0

								 //flv VideoTagHeader
		if (IsIframe)
			output[offset++] = 0x17; //key frame, AVC
		else
			output[offset++] = 0x27; //key frame, AVC

		output[offset++] = 0x01; //avc NALU unit
		output[offset++] = 0x00; //composit time ??????????
		output[offset++] = 0x00; // composit time
		output[offset++] = 0x00; //composit time

		output[offset++] = (uint8_t)(nal_len >> 24); //nal length 
		output[offset++] = (uint8_t)(nal_len >> 16); //nal length 
		output[offset++] = (uint8_t)(nal_len >> 8); //nal length 
		output[offset++] = (uint8_t)(nal_len); //nal length 
		memcpy(output + offset, nal, nal_len);

		//no need set pre_tag_size ,RTMP NO NEED
		offset += nal_len;
		uint32_t fff = body_len + FLV_TAG_HEAD_LEN;
		output[offset++] = (uint8_t)(fff >> 24); //data len
		output[offset++] = (uint8_t)(fff >> 16); //data len
		output[offset++] = (uint8_t)(fff >> 8); //data len
		output[offset++] = (uint8_t)(fff); //data len
		return send_data(output, output_len);
	}


	int callback(const char* flag, uint8_t * data, long size) {

		//判断sps和pps
		uint8_t *rdata = data + v_headlen - 4; //rdata指向 00 00 00 01
		long nalsize = size - v_headlen;
		long rsize = nalsize + 4;
		uint8_t nalu = *(rdata + 4) & 0x1f;
		if (_spbuffer.sp_status < 2) {
			if (nalu == 0x07 || nalu == 0x08) { //sps pps
				if (_spbuffer.sp_status <2)
					sps_pps_callback(nalu, rdata + 4, rsize - 4);
			}
		}

		if (nalu == 0x05 || nalu == 0x01)
		{
			_recv_stamp = GetTimestamp32();
			if (_first_stamp == 0)
				_first_stamp = _recv_stamp;
			bool keyframe = nalu == 0x05 ? true : false;
			uint32_t ts = _recv_stamp - _first_stamp; // pkt->pts 

													  //加上头部数据,改成关键帧则加上sps和pps
			writeavcframe_0(rdata + 4, rsize - 4, ts, keyframe);
			//关键帧不加上sps和pps
			//writeavcframe(rdata + 4, rsize - 4, ts, keyframe);

		}
		return 0;
	}



	static void Init()
	{
		WSADATA wsaData;
		WSAStartup(0x202, &wsaData);


		//av_register_all();
		//avformat_network_init();
		//avdevice_register_all();
		//DeviceAudio::Init();
	}

	static void UnInit()
	{
		//DeviceAudio::UnInit();
		WSACleanup();
	}
	void init_start(stru_rr_param *param) {
		if (param == nullptr)
			return;
		_pm = param;
		v_headlen = TQPacket::TPacketHeadLen(VIDEO_PACKET);
		v_headlen += 4;
		_recv_stamp = 0;
		_first_stamp = 0;
		//_InPar.Reset();
		if (_h5client.connect(param->_sip, param->_sport, param->_flag) != 0) {
			cerr << "h5 server can not connect" << endl;
		}
		_flvhead.buf = new uint8_t[256 * 1024];
		_flvdst.buf = new uint8_t[1024 * 1024];
		if (_rtsp == nullptr) {
			_rtsp = new rtspflv_live();


			auto ptrfunc = std::bind(&TRtspMixAudio2Server::callback,
				this, std::placeholders::_1,
				std::placeholders::_2,
				std::placeholders::_3);
			//这里传入
			pro_param poparam;

			poparam.flag = param->_flag;
			poparam.headlen = v_headlen;
			poparam.logLevel = 255;
			poparam.rtpovertcp = false;
			poparam.timeout = 5;
			poparam.url = param->_url_rtsp;
			//poparam.cb = ptr_func;
			poparam.cb1 = ptrfunc;
			_rtsp->start(&poparam);
		}

		Start();
	}

	//重写stop函数
	void Stop()
	{
		TThreadRunable::Stop();
		Notify();
		//_video_ok = 0;
		Join();
		if (_rtsp != nullptr) {
			_rtsp->Stop();
			delete _rtsp;
		}
		if (_flvdst.buf != nullptr)
			delete[]_flvdst.buf;
		if (_flvhead.buf != nullptr)
			delete[] _flvhead.buf;

	}


	TRtspMixAudio2Server::TRtspMixAudio2Server() {}
	TRtspMixAudio2Server::~TRtspMixAudio2Server()
	{
		//if (_audiopkt != nullptr)
		//	TQPacket::Free1(&_audiopkt);
	}



	//断线重连
	void TRtspMixAudio2Server::Run()
	{
		int check = 0;
		uint32_t stamp = 0;
		if (_rtsp == nullptr) {
			cerr << "rtsp obj is null,exit thread" << endl;
			return;
		}
		//_rtsp->start(); //开始rtsp
		while (1)
		{
			WaitForSignal();//得到重新连接的事件
							//断线重连接
			if (IsStop())
				break;
			else {
#if 0
				if (_rtmp->GetConnectStatus() == CONN_CLOSED)
				{
					if (_rtmp->Connect(_param._url_rtmp) != 0)
					{
						cerr << "can't connect the server:" << _param._url_rtmp << endl;
					}
					std::this_thread::sleep_for(std::chrono::microseconds(50 * 1000));
				}
#endif
			}
		}
		//_rtsp->Stop();
	}




};
#endif
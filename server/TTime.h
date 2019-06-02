/*
Author:钱波
email: 418511899@qq.com
wei:   18091589062
func  :时间
time:  2018年5月30日
*/
#ifndef _T_TIME_H_
#define _T_TIME_H_

#ifdef WIN32
#include <winsock2.h>
#include <windows.h>
#include <stdint.h>
#include <sys/timeb.h>

//av_gettime()

static int gettimeofday(struct timeval* tp, int* /*tz*/) {

	static LARGE_INTEGER tickFrequency, epochOffset;

	// For our first call, use "ftime()", so that we get a time with a proper epoch.
	// For subsequent calls, use "QueryPerformanceCount()", because it's more fine-grain.
	static BOOL isFirstCall = TRUE;

	LARGE_INTEGER tickNow;
	QueryPerformanceCounter(&tickNow);

	if (isFirstCall) {
		struct timeb tb;
		ftime(&tb);
		tp->tv_sec = (long)tb.time;
		tp->tv_usec = 1000 * tb.millitm;

		// Also get our counter frequency:
		QueryPerformanceFrequency(&tickFrequency);

		// And compute an offset to add to subsequent counter times, so we get a proper epoch:
		epochOffset.QuadPart
			= tb.time*tickFrequency.QuadPart + (tb.millitm*tickFrequency.QuadPart) / 1000 - tickNow.QuadPart;

		isFirstCall = FALSE; // for next time
	}
	else {
		// Adjust our counter time so that we get a proper epoch:
		tickNow.QuadPart += epochOffset.QuadPart;

		tp->tv_sec = (long)(tickNow.QuadPart / tickFrequency.QuadPart);
		tp->tv_usec = (long)(((tickNow.QuadPart % tickFrequency.QuadPart) * 1000000L) / tickFrequency.QuadPart);
	}
	return 0;
}

#endif
//static uint32_t GetTimestamp32(int64_t begin)
//{
//	struct timeval tv;
//	gettimeofday(&tv, NULL);
//	//这是微妙值
//	int64_t timestamp_us = (tv.tv_sec * 1000000LL + tv.tv_usec - begin);
//	uint32_t n = (uint32_t)(timestamp_us* 0.001); //只取毫秒，比较精确
//	return n;
//
//}
static uint32_t GetTimestamp32()
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	//这是毫秒值
	uint32_t timestamp_ms = (tv.tv_sec * 1000L + tv.tv_usec / 1000);
	return timestamp_ms;
}
static int64_t GetTimestamp64_0()
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	//这是微妙值
	return (tv.tv_sec * 1000000LL + tv.tv_usec);
}


static uint32_t convertToRTPTimestamp(/*struct timeval tv*/)
{
	timeval tv;
	gettimeofday(&tv, NULL);
	UINT32 timestampIncrement = (90000 * tv.tv_sec);
	timestampIncrement += (UINT32)((2.0 * 90000 * tv.tv_usec + 1000000.0) / 2000000);
	//UINT32 const rtpTimestamp =  timestampIncrement;  
	return timestampIncrement;
}




#endif
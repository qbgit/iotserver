/*
Author:钱波
email: 418511899@qq.com
wei:   18091589062
func  :线程类
time:  2018年5月30日
*/

#ifndef _TTHREAD_RUN_ABLE_H_
#define _TTHREAD_RUN_ABLE_H_


#include <mutex>
#include <queue>
#include <thread>
#include <atomic>
#include <condition_variable>
using namespace std;

class TThreadRunable
{
private:

	//线程
	thread _thread;
	//等待信号
	std::mutex _signal_mutex;
	std::condition_variable _cond;
protected:
	//char  _running = false;
	char _stop = true;
	//锁定运行状态
	std::mutex _mutex;
public:
	TThreadRunable()
	{}
	virtual ~TThreadRunable()
	{}

public:
	char * status(){
		return &_stop;
	}
	
	void Join()
	{
		if (_thread.joinable())
			_thread.join();
	}
	bool  IsStop()
	{
		return _stop == 1 ? true : false;
	}
	void WaitForSignal()
	{
		std::unique_lock<std::mutex> ul(_signal_mutex);
		_cond.wait(ul);
	}
	void Notify()
	{
		_cond.notify_one();
	}

	virtual int Start()
	{
		
#if 0
		RunLock<TThreadRunable> lock(this);
		if (lock.GetStatus() == Run_Run)
			return -1;
		lock.Change2Run();
#endif
		//_running = true;
		if (_stop == 0)
			return -1;
		_stop = 0;
		_thread = std::thread(std::bind(&TThreadRunable::Run, this));
		return 0;
	}	
	
	virtual void Stop()
	{
		//_running = false;
		_stop = 1; // true;
		//RunLock<TThreadRunable> lock(this);
		//if (lock.GetStatus() == Run_Run)
		//{
		//	lock.Change2Stop();
		//	return;
		//}
	}

	virtual void Run() = 0;

};



#endif
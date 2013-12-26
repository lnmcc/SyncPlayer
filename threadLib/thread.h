/*
 * thread.h
 * 
 * Created: Oct 10, 2013
 *  Author : sijiewang
 *  Email  : lnmcc@hotmail.com
 *  Site   : lnmcc.net
 */

#ifndef THREAD_H_
#define THREAD_H_

#include <pthread.h>
#include <unistd.h>

namespace syncplayer{

class Thread {

public:
	Thread();
	virtual ~Thread();

	virtual void run() = 0;

	bool start();

	//FIXME:detached状态下不可以调用下面2个函数
	void join();
	void join(unsigned long millisTime);

	void stop();
	int getStatus();

	pthread_t getThreadID();

	void setThreadPriority(int prio);
	int getThreadPriority();

private:
	void* run1();
	static void* run0(void* args);

	void setThreadPolicy(pthread_attr_t *attr, int policy);

public:
	static const int THREAD_STATUS_RUNNING = 1;
	static const int THREAD_STATUS_EXIT = -1;

private:
	pthread_t m_tid;
	int m_threadStatus;

	pthread_attr_t m_attr;
	struct sched_param m_schedParam;
};

} //namespace

#endif /* THREAD_H_ */

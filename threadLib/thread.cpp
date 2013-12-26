/*
 * thread.cpp
 *
 *  Created: Oct 10, 2013
 *  Author : sijiewang
 *  Email  : lnmcc@hotmail.com
 *  Site   : lnmcc.net
 */

#include "thread.h"
#include <iostream>
#include <assert.h>
#include <sched.h>

namespace syncplayer {

Thread::Thread() {

	m_tid = 0;
	int ret = pthread_attr_init(&m_attr);
	assert(ret == 0);

	pthread_attr_setdetachstate(&m_attr, PTHREAD_CREATE_DETACHED);
	setThreadPolicy(&m_attr, SCHED_OTHER);
}

Thread::~Thread() {

	int ret = pthread_attr_destroy(&m_attr);
	assert(ret == 0);
}

void* Thread::run0(void* args) {

	Thread *p = (Thread*) args;
	p->run1();
	return p;
}

void* Thread::run1() {

	m_threadStatus = THREAD_STATUS_RUNNING;
	m_tid = pthread_self();
	run();
	m_threadStatus = THREAD_STATUS_EXIT;
	m_tid = 0;
	pthread_exit(NULL);
	return NULL;
}

void Thread::stop() {

}

bool Thread::start() {

	int ret = pthread_create(&m_tid, &m_attr, run0, this);
	if (0 != ret) {
		std::cerr << __FILE__ << ":" << __LINE__ << ": Thread start error!"
				<< std::endl;
		return false;
	}

	return true;
}

pthread_t Thread::getThreadID() {
	return m_tid;
}

int Thread::getStatus() {
	return m_threadStatus;
}

void Thread::join() {
	if (m_tid > 0) {
		pthread_join(m_tid, NULL);
	}
}

void Thread::join(unsigned long millisTime) {

	if (0 == m_tid) {
		return;
	}
	if (0 == millisTime) {
		join();
	} else {
		unsigned long k = 0;
		while (m_threadStatus != THREAD_STATUS_EXIT && k <= millisTime) {
			usleep(1000);
			k++;
		}
	}
}

int Thread::getThreadPriority() {

	struct sched_param param;
	int rt = pthread_attr_getschedparam(&m_attr, &param);
	assert(rt == 0);

	std::cerr << __FILE__ << ":" << __LINE__ << " : " << "prioryty = "
			<< param.__sched_priority << std::endl;

	return param.__sched_priority;
}

void Thread::setThreadPolicy(pthread_attr_t *attr, int policy) {

	int rt = pthread_attr_setschedpolicy(attr, policy);
	assert(rt == 0);
}

void Thread::setThreadPriority(int prio) {

	//获取当前调度Policy
	int policy;
	int ret = pthread_attr_getschedpolicy(&m_attr, &policy);
	assert(ret == 0);

	//获取对应Policy的最大最小优先级
	int max, min;
	max = sched_get_priority_max(policy);
	assert(max != -1);

	min = sched_get_priority_min(policy);
	assert(min != -1);

	int p = prio;
	if (p < min) {
		p = min;
	} else if (p > max) {
		p = max;
	}

	switch (policy) {
	case SCHED_FIFO:
		m_schedParam.__sched_priority = p;
		pthread_attr_setschedparam(&m_attr, &m_schedParam);
		break;
	case SCHED_RR:
		m_schedParam.__sched_priority = p;
		pthread_attr_setschedparam(&m_attr, &m_schedParam);
		break;
	case SCHED_OTHER:
		std::cerr << __FILE__ << ":" << __LINE__ << " : "
				<< "SCHEED_OTHER not support thread priority, use default"
				<< std::endl;
		break;
	default:
		std::cerr << __FILE__ << ":" << __LINE__ << " : "
				<< "Not support thread priority, Unknown policy, use default"
				<< std::endl;
		break;
	}
}

} //namespace


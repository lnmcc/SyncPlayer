/*
 * common.h
 *
 *  Created on: Oct 11, 2013
 *  Author : sijiewang
 *  Email  : lnmcc@hotmail.com
 *  Site   : lnmcc.net
 */

#ifndef COMMON_H_
#define COMMON_H_

#include <X11/Xlib.h>
#include <GL/glew.h>
#include <GL/glx.h>
#include <GL/glu.h>
#include <queue>
#include <pthread.h>
#include <string>
#include <iostream>

namespace syncplayer {

class PluginManager;
class openGlPaintPlugin;
class Event;
struct DDS_IMAGE_DATA;
class ParseEvent;

//一些功能函数

extern long long Nowus();

//一个线程安全的，带阻塞功能的队列
template<typename T>
class CondQueue {

public:
	CondQueue();
	~CondQueue();

	//提供线程调用，当容量满时会阻塞
	void push(const T &value);
	//不同于STL的pop(),这个函数不pop头元素还会删除该元素
	T& pop();
	T& front();
	unsigned int size();
	bool empty();
	bool full();
	bool busy();

private:

	unsigned int __size();

private:
	static const unsigned int maxSize = 1000;
	std::queue<T> m_queue;

	pthread_mutex_t m_mutex;
	pthread_cond_t m_cond;
};

template<typename T>
CondQueue<T>::CondQueue() {

	pthread_mutex_init(&m_mutex, NULL);
	pthread_cond_init(&m_cond, NULL);
}

template<typename T>
CondQueue<T>::~CondQueue() {

	pthread_mutex_destroy(&m_mutex);
	pthread_cond_destroy(&m_cond);
}

template<typename T>
void CondQueue<T>::push(const T &value) {

	pthread_mutex_lock(&m_mutex);

	while (__size() >= maxSize) {
		pthread_cond_wait(&m_cond, &m_mutex);
	}

	m_queue.push(value);
	pthread_cond_signal(&m_cond);
	pthread_mutex_unlock(&m_mutex);
}

template<typename T>
T& CondQueue<T>::pop() {

	pthread_mutex_lock(&m_mutex);

	while (__size() <= 0) {
		pthread_cond_wait(&m_cond, &m_mutex);
	}

	T &tmp = m_queue.front();
	m_queue.pop();
	pthread_cond_signal(&m_cond);
	pthread_mutex_unlock(&m_mutex);

	return tmp;
}

template<typename T>
T& CondQueue<T>::front() {

	pthread_mutex_lock(&m_mutex);

	while (__size() <= 0) {
		pthread_cond_wait(&m_cond, &m_mutex);
	}

	T &tmp = m_queue.front();

	pthread_cond_signal(&m_cond);
	pthread_mutex_unlock(&m_mutex);

	return tmp;
}

template<typename T>
unsigned int CondQueue<T>::size() {

	unsigned int size;

	pthread_mutex_lock(&m_mutex);
	size = m_queue.size();
	pthread_mutex_unlock(&m_mutex);

	return size;
}

template<typename T>
unsigned int CondQueue<T>::__size() {

	return m_queue.size();
}

template<typename T>
bool CondQueue<T>::empty() {

	bool result;

	pthread_mutex_lock(&m_mutex);
	result = m_queue.empty();
	pthread_mutex_unlock(&m_mutex);

	return result;
}

template<typename T>
bool CondQueue<T>::full() {

	bool result;

	pthread_mutex_lock(&m_mutex);
	result = (m_queue.size() == maxSize);
	pthread_mutex_unlock(&m_mutex);

	return result;
}

struct PicInfo {

	std::string path;
	std::string type;
	unsigned int orderID;
	DDS_IMAGE_DATA *data;
	//本图片所属的event
	Event *event;

	//左下右上的顶点坐标
	int left; //x0
	int button; //y0
	int right; //x1
	int top; //y1
	int zCoord;

	long long start;
	long long from;
	long long to;
	unsigned int duration;

	unsigned int syncs; //指示这张图片所在同步组中的图片总数量
};

class Context {

public:
	static Context *GetContext();

	void setScreenWidth(unsigned int width);
	unsigned int getScreenWidth();
	void setScreenHeight(unsigned int height);
	unsigned int getScreenHeight();
	void setScreenXOffset(int xOffset);
	int getScreenXOffset();
	void setScreenYOffset(int yOffset);
	int getScreenYOffset();

	void setDisplay(Display *display);
	Display* getDisplay();
	void setGLXContext(GLXContext *glc);
	GLXContext* getGLXContext();
	void setGLXWindow(GLXWindow *glw);
	GLXWindow* getGLXWindow();
	void setX11Window(Window *win);
	Window* getX11Window();

	void setPluginManager(PluginManager *pm);
	PluginManager* getPluginManager();

	void setOpenGlPaintPlugin(openGlPaintPlugin *opg);
	openGlPaintPlugin* getOpenGlPaintPlugin();

	void setEventParse(ParseEvent *pe);
	ParseEvent* getEventParse();

private:
	Context();
	~Context();

private:
	class Garbo {

	public:
		~Garbo();
	};

private:
	static Context *m_context;
	static Garbo m_Garbo;

	Display *m_display;
	GLXContext *m_glc;
	GLXWindow *m_glw;
	Window *m_window;

	unsigned int m_screenWidth;
	unsigned int m_screenHeight;
	int m_screenXOffset;
	int m_screenYOffset;

	PluginManager *m_pluginManager;
	openGlPaintPlugin *m_openGlPaintPlugin;
	CondQueue<Event*> m_eventQueue;

	ParseEvent* m_eventParse;
};

} //namespace
#endif /* COMMON_H_ */

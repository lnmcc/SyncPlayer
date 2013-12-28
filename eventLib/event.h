/*
 * event.h
 *
 *  Created on: Oct 14, 2013
 *  Author : sijiewang
 *  Email  : lnmcc@hotmail.com
 *  Site   : lnmcc.net
 */

#ifndef EVENT_H_
#define EVENT_H_

#include "../commonLib/common.h"
#include "../threadLib/thread.h"

#include <string>
#include <JSONCPP/json.h>
#include <queue>

namespace syncplayer {

//描述从接收到的事件
class Event {

public:
	void dump();

	int id; //事件ID
	std::string type; //事件类型
	std::string libName; //处理事件的插件名
	Json::Value deps; //事件关联文件
	long long start; //开始时间
	long long refCount; //Event 引用计数
};

class ParseEvent: public Thread {

public:
	static ParseEvent *GetParseEvent(Context *context);

	void run();
	void dump();

	//分析一个json格式的文件
	//[in] path: json文件的路径
	bool parseFile2Event(std::string path);

	//本函数在处理完string*后，会释放str内存
	bool parseString2Event(std::string* str);

	//分析队列中的event, 并处理它
	bool processEvent();

private:
	ParseEvent(Context *context);
	virtual ~ParseEvent();

private:
	class Garbo {

	public:
		~Garbo();
	};

private:
	static ParseEvent *m_parseEvent;
	static Garbo m_Garbo;

	Context *m_context;
	CondQueue<Event*> m_eventQueue;
	CondQueue<std::string*> m_jsonStrQueue;
};

//每收到一个Event都会启动一个本类来具体处理Event，调用者不需要负责删除这个类，本类会自行删除
//FIXME:鉴于这个类会自行删除，考虑要隐藏父类的一些方法
class ProcessEvent: public Thread {

public:
	ProcessEvent(Context *context, Event *event);
	~ProcessEvent();

	void run();

private:
	Event *m_event;
	Context *m_context;
};

} //namespace

#endif /* EVENT_H_ */

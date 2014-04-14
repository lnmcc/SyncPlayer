/*
 * pluginManager.h
 *
 *  Created on: Oct 14, 2013
 *  Author : sijiewang
 *  Email  : lnmcc@hotmail.com
 *  Site   : lnmcc.net
 */

#ifndef PLUGINMANAGER_H_
#define PLUGINMANAGER_H_

#include "../../commonLib/common.h"
#include "../../threadLib/thread.h"

#include <iterator>
#include <string>
#include <vector>
#include <map>

namespace syncplayer {

class Context;
class Event;
struct PicInfo;

class Plugin {

public:
	Plugin(const std::string name = "Unknow");
	virtual ~Plugin();

	virtual std::string getName();
	virtual void push(PicInfo *picInfo);
	virtual bool full();
	virtual bool empty();
	void setBusy(bool busy);
	bool getBusy();

private:
	std::string m_name;
	bool m_busy;
};

class PluginManager: public Thread {

public:
	static PluginManager *GetManager(Context *context);

	void registPlugin();
	void push(PicInfo *dataPtr);
	void run();

private:
	PluginManager(Context *context);
	~PluginManager();

	//如果没有找到相应的插件返回NULL
	Plugin *getPlugin(const std::string name);

private:
	class Garbo {

	public:
		~Garbo();
	};

private:
	static PluginManager *m_pluginManager;
	static Garbo m_Garbo;

	Context *m_context;
	std::multimap<std::string, Plugin*> m_pluginMap;
	std::multimap<Event*, Plugin*> m_event2plugin;
	CondQueue<PicInfo *> m_picInfoQueue;
};

} // namespace

#endif /* PLUGINMANAGER_H_ */

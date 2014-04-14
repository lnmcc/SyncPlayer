/*
 * pluginManager.cpp
 *
 *  Created on: Oct 14, 2013
 *  Author : sijiewang
 *  Email  : lnmcc@hotmail.com
 *  Site   : lnmcc.net
 */

#include "pluginManager.h"
#include "../bmpPlugin/bmpPlugin.h"
#include "../ddsPlugin/ddswriter.h"

namespace syncplayer {

#define MAX_PLUGIN_SIZE 10

Plugin::Plugin(const std::string name) {

	m_name = name;
	m_busy = false;
}

Plugin::~Plugin() {

}

std::string Plugin::getName() {

	return m_name;
}

void Plugin::push(PicInfo *picInfo) {

}

bool Plugin::full() {
	return false;
}

bool Plugin::empty() {
	return true;
}

void Plugin::setBusy(bool busy) {
	m_busy = busy;
}

bool Plugin::getBusy() {
	return m_busy;
}

PluginManager* PluginManager::m_pluginManager = NULL;

PluginManager::Garbo::~Garbo() {

	if (NULL != m_pluginManager) {
		delete m_pluginManager;
	}
}

PluginManager* PluginManager::GetManager(Context *context) {

	if (NULL == m_pluginManager) {
		m_pluginManager = new PluginManager(context);
	}
	return m_pluginManager;
}

PluginManager::PluginManager(Context *content) {

	m_context = content;
	registPlugin();
}

PluginManager::~PluginManager() {

}

void PluginManager::registPlugin() {

	bmpPlugin *bmpplugin = NULL;

	for (unsigned int i = 0; i < MAX_PLUGIN_SIZE; i++) {

		bmpplugin = new bmpPlugin(m_context);
		m_pluginMap.insert(
				std::pair<std::string, Plugin*>(bmpplugin->getName(),
						bmpplugin));
		bmpplugin->setBusy(false);
		bmpplugin->start();
	}
}

//FIXME:这里只处理了bmp
Plugin* PluginManager::getPlugin(const std::string name) {

	Plugin *ret = NULL;
	std::multimap<std::string, Plugin*>::iterator iter;

	std::cerr << __FILE__ << ":" << __LINE__ << "find plugin 2" << std::endl;

	iter = m_pluginMap.find(name);
	if (iter != m_pluginMap.end()) {
		unsigned int count = m_pluginMap.count(name);
		std::cerr << __FILE__ << ":" << __LINE__ << "count = " << count
				<< std::endl;

		std::cerr << __FILE__ << ":" << __LINE__ << "find plugin 3"
				<< std::endl;

		do {
			for (unsigned int i = 0; i < count; i++) {
				std::cerr << __FILE__ << ":" << __LINE__ << "find plugin 4"
						<< std::endl;

				if (!iter->second->getBusy()) {
					//找到一个可用的插件
					std::cerr << __FILE__ << ":" << __LINE__ << "find plugin"
							<< std::endl;
					ret = iter->second;
					break;
				} else {
					iter++;
				}
			}
		} while (ret == NULL);
	}

	/*
	 if (name == "bmpPlugin") {
	 bmpPlugin *bmpplugin = new bmpPlugin(m_context);
	 bmpplugin->start();
	 ret = bmpplugin;
	 }
	 */
	return ret;
}
//FIXME:这里只处理了bmp
void PluginManager::run() {

	PicInfo *picInfo = NULL;
	std::multimap<Event*, Plugin*>::iterator epIter;
	Plugin *plugin = NULL;

	while (true) {

		picInfo = m_picInfoQueue.pop();
		epIter = m_event2plugin.find(picInfo->event);

		if (epIter != m_event2plugin.end()) {
			unsigned int count = m_event2plugin.count(picInfo->event);

			for (unsigned int i = 0; i < count; i++) {
				plugin = epIter->second;
				if (plugin->getName() == "bmpPlugin") {
					plugin->push(picInfo);
				} else {
					std::cerr << "no such plugin, get one" << std::endl;

					plugin = getPlugin("bmpPlugin");
					m_event2plugin.insert(
							std::pair<Event*, Plugin*>(picInfo->event, plugin));
					plugin->setBusy(true);
					plugin->push(picInfo);
				}
			}
		} else {
			std::cerr << "no such event, get one" << std::endl;
			plugin = getPlugin("bmpPlugin");
			m_event2plugin.insert(
					std::pair<Event*, Plugin*>(picInfo->event, plugin));
			plugin->setBusy(true);
			plugin->push(picInfo);
		}
	} // end while
	/*
	 if (picInfo->type == "bmp") {
	 //一直找到一个可用的插件为止
	 do {
	 plugin = getPlugin("bmpPlugin");
	 std::cerr << __FILE__ << ":" << __LINE__ << ": want to get plugin"
	 << std::endl;
	 //FIXME:可优化
	 } while (plugin == NULL);

	 plugin->push(picInfo);
	 }
	 }*/
#if 0
	while (true) {

		picInfo = m_picInfoQueue.pop();
		epIter = m_event2plugin.find(picInfo->event);

		if (epIter != m_event2plugin.end()) {
			unsigned int count = m_event2plugin.count(picInfo->event);

			for (unsigned int i = 0; i < count; i++) {
				plugin = epIter->second;
				if (plugin->getName() == "bmpPlugin") {
					plugin->push(picInfo);
				} else {
					plugin = getPlugin("bmpPlugin");
					m_event2plugin.insert(
							std::pair<Event*, Plugin*>(picInfo->event, plugin));
					plugin->push(picInfo);
				}
			}
		} else {
			plugin = getPlugin("bmpPlugin");
			m_event2plugin.insert(
					std::pair<Event*, Plugin*>(picInfo->event, plugin));
			plugin->push(picInfo);
		}

	} // end while
#endif
}

void PluginManager::push(PicInfo *dataPtr) {

	m_picInfoQueue.push(dataPtr);
}

} // namespace


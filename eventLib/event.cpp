/*
 * event.cpp
 *
 *  Created on: Oct 14, 2013
 *  Author : sijiewang
 *  Email  : lnmcc@hotmail.com
 *  Site   : lnmcc.net
 */


#include "event.h"
#include "../bmpPlugin/bmpPlugin.h"
#include "../pluginManager/pluginManager.h"
#include "../openGlPaintPlugin/openGlPaintPlugin.h"
#include "../ddsPlugin/ddswriter.h"

#include <cassert>
#include <iostream>
#include <fstream>
#include <cstdlib>

namespace syncplayer {

void Event::dump() {

	std::cerr << "Event Information" << std::endl;
	std::cerr << "id: " << id << std::endl;
	std::cerr << "type: " << type << std::endl;
	std::cerr << "libName: " << libName << std::endl;
	std::cerr << "deps: " << std::endl;

	for (unsigned int i = 0; i < deps.size(); i++) {
		std::cerr << deps[i]["path"].asString() << " : "
				<< deps[i]["type"].asString() << std::endl;
	}
}

ParseEvent* ParseEvent::m_parseEvent = NULL;
ParseEvent::Garbo ParseEvent::m_Garbo;

ParseEvent::Garbo::~Garbo() {

	if (NULL != m_parseEvent) {
		delete m_parseEvent;
	}
}

ParseEvent* ParseEvent::GetParseEvent(Context *context) {

	if (NULL == m_parseEvent) {
		m_parseEvent = new ParseEvent(context);
	}
	return m_parseEvent;
}

ParseEvent::ParseEvent(Context *context) {

	m_context = context;
}

ParseEvent::~ParseEvent() {

	for (unsigned int i = 0; i < m_eventQueue.size(); i++) {
		delete m_eventQueue.pop();
	}
}

bool ParseEvent::parseFile2Event(std::string path) {

	std::ifstream in;
	in.open(path.c_str(), std::ifstream::in);
	assert(in.is_open());

	Json::Reader reader;
	Json::Value root;

	if (!reader.parse(in, root, false)) {
		std::cerr << __FILE__ << ":" << __LINE__ << "paser json file error!"
				<< std::endl;
		return false;
	}
	//FIXME:
	Event *event = new Event();
	event->id = root["id"].asInt();
	event->type = root["type"].asString();
	event->libName = root["libName"].asString();
	event->deps = root["deps"];
	event->start = atoll(root["start"].asString().c_str());
	event->refCount = 0;

	m_eventQueue.push(event);

	return true;
}

bool ParseEvent::parseString2Event(std::string *str) {

	Json::Reader reader;
	Json::Value root;

	if (!reader.parse(*str, root)) {
		std::cerr << __FILE__ << ":" << __LINE__ << "paser json string error!"
				<< std::endl;
		return false;
	}

	//FIXME:
	Event *event = new Event();
	event->id = root["id"].asInt();
	event->type = root["type"].asString();
	event->libName = root["libName"].asString();
	event->deps = root["deps"];
	event->start = atoll(root["start"].asString().c_str());
	event->refCount = 0;
	m_eventQueue.push(event);
	delete str;

	return true;
}

void ParseEvent::dump() {

	//FIXME
	for (unsigned int i = 0; i < m_eventQueue.size(); i++) {
		//m_eventQueue->dump();
	}
}

bool ParseEvent::processEvent() {

	//这里可能会阻塞
	Event *event = m_eventQueue.pop();
	event->dump();
	ProcessEvent *pev = new ProcessEvent(m_context, event);
	pev->start();

	return true;
}

void ParseEvent::run() {

	while (true) {
		if (processEvent()) {
			//FIXME
			std::cerr << __FILE__<< __LINE__ << " : pasrse event Done!"
					<< std::endl;
		} else {
			std::cerr << __FILE__<< __LINE__ << " : pasrse event Failed!"
					<< std::endl;
		}
	}
}

} //namespace


/*
 * main.cpp
 *
 *  Created on: Oct 10, 2013
 *  Author : sijiewang
 *  Email  : lnmcc@hotmail.com
 *  Site   : lnmcc.net
 */

#include "../commonLib/common.h"
#include "../bmpPlugin/bmpPlugin.h"
#include "../ddsPlugin/ddswriter.h"
#include "../xlibEnv/xlibEnv.h"
#include "../threadLib/thread.h"
#include "../eventLib/event.h"
#include "../pluginManager/pluginManager.h"
#include "../openGlPaintPlugin/openGlPaintPlugin.h"
#include "../socketLib/socket.h"

#include <unistd.h>
#include <iostream>

using namespace syncplayer;

int main(int argc, char **argv) {

	std::cerr << "Main begin " << std::endl;

	Context *context = Context::GetContext();
	context->setScreenWidth(800);
	context->setScreenHeight(600);
	context->setScreenXOffset(0);
	context->setScreenYOffset(0);

	xlibEnv::xlibEnv::GetXlibEnv(context);

	PluginManager *pluginMgr = PluginManager::GetManager(
			context);
	context->setPluginManager(pluginMgr);
	pluginMgr->start();

	openGlPaintPlugin *openglpaintplugin =
			new openGlPaintPlugin(context);
	context->setOpenGlPaintPlugin(openglpaintplugin);

	ParseEvent *parseEvent = ParseEvent::GetParseEvent(context);
	context->setEventParse(parseEvent);
	//parseEvent.parseFile2Event("/home/lnmcc/Documents/test.json");
	//parseEvent.parseFile2Event("/home/lnmcc/Documents/test2.json");
	//parseEvent.parseFile2Event("/home/lnmcc/Documents/test3.json");
	//parseEvent.parseFile2Event("/home/lnmcc/Documents/test4.json");

	//parseEvent->dump();
	parseEvent->start();

	Socket mysocket(context);
	mysocket.start();


	//openglpaintplugin->setThreadPriority(99);
	openglpaintplugin->start();

	pause();

	return 0;
}


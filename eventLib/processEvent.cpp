/*
 * processEvent.cpp
 *
 *  Created on: Oct 20, 2013
 *  Author : sijiewang
 *  Email  : lnmcc@hotmail.com
 *  Site   : lnmcc.net
 */

#include "event.h"
#include "../main/bmpPlugin/bmpPlugin.h"
#include "../main/pluginManager/pluginManager.h"

namespace syncplayer {

ProcessEvent::ProcessEvent(Context *context, Event *event) {

	m_event = event;
	m_context = context;
}

ProcessEvent::~ProcessEvent() {

}

void ProcessEvent::run() {

	if (m_event->type == "sync") {
		if (m_event->libName == "pic") {

			Json::Value value = m_event->deps;
			//记录前面图片所消耗的时间，以计算下一组（张）图片的开始时间
			long long lastDuration = 0;
			long long currentStart = 0;

			for (unsigned int i = 0; i < value.size(); i++) {

				PicInfo *picInfo = new PicInfo();

				picInfo->event = m_event;
				picInfo->syncs = value[i]["syncs"].asUInt();

				if (value[i]["order"].asInt() == 1) {
					picInfo->from = m_event->start + lastDuration;
					lastDuration += value[i]["duration"].asUInt() * 1000;
					currentStart = picInfo->from;
					picInfo->to = picInfo->from
							+ value[i]["duration"].asUInt() * 1000;
				} else {
					picInfo->from = currentStart;
					picInfo->to = picInfo->from
							+ value[i]["duration"].asUInt() * 1000;
				}
				//如果可能，先过滤掉一部分过期的数据
				if (picInfo->to - Nowus() < 0) {
					delete picInfo;
					continue;
				}

				if (value[i]["type"].asString() == "bmp") {
					picInfo->type = value[i]["type"].asString();
					picInfo->path = value[i]["path"].asString();
					picInfo->left = value[i]["left"].asInt();
					picInfo->button = value[i]["button"].asInt();
					picInfo->right = value[i]["right"].asInt();
					picInfo->top = value[i]["top"].asInt();
					picInfo->orderID = value[i]["order"].asInt();
					picInfo->duration = value[i]["duration"].asUInt();
					picInfo->zCoord = value[i]["z"].asInt();
					m_context->getPluginManager()->push(picInfo);
				}
			}
		}
	}
	//极其小心的处理这条语句
	delete this;
}

} //namespace

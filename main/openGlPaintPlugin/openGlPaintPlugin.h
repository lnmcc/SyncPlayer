/*
 * openGlPaintPlugin.h
 *
 *  Created on: Oct 9, 2013
 *      Author: sijiewang
 */

#ifndef OPENGLPAINTPLUGIN_H_
#define OPENGLPAINTPLUGIN_H_

#include "../../commonLib/common.h"
#include "../../threadLib/thread.h"
#include "../ddsPlugin/ddswriter.h"

#include <queue>
#include <map>
#include <vector>
#include <X11/Xlib.h>

using namespace std;

namespace syncplayer {

#define MAX_DATA_QUEUE_SIZE 1024

class openGlPaintPlugin: public Thread {

public:
	openGlPaintPlugin(Context *context);
	virtual ~openGlPaintPlugin();

	//供外部调用，绘制DDS
	//[in] dds: DDS文件在内存中的地址
	void paintDDS(PicInfo* picInfo);
	void render();
	void run();

	bool push(PicInfo *dataPtr);

private:
	//把内存中的DDS文件转换成纹理, 这个方法会把传入的picinfo->data内存销毁掉
	//[in] PicInfo*: 传入的内存dds文件指针
	//[out] texID: 转换成纹理后的ID
	bool memDDS2Tex(PicInfo* picInfo, GLuint *texID);

	void initGLX();
	void initGL();

private:
	Context *m_context;

	Display *m_dpy;
	Window *m_win;
	GLXContext *m_glc;

	GLint m_maxTexNum;
	GLuint *m_textures;

	//记录所有注册上来的队列
	typedef queue<PicInfo*> QpicItem;
	//order --> QpicItem*
	typedef map<unsigned int, QpicItem*> EventID2Order;

	map<Event*, EventID2Order*> m_registedEvent;

	pthread_mutex_t  m_mapMutex;
};

} //namespace

#endif /* OPENGLPAINTPLUGIN_H_ */

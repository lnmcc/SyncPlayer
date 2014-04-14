/*
 * openGlPaintPlugin.cpp
 *
 *  Created on: Oct 9, 2013
 *  Author : sijiewang
 *  Email  : lnmcc@hotmail.com
 *  Site   : lnmcc.net
 */
#include "openGlPaintPlugin.h"
#include "../../eventLib/event.h"

#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <iostream>
#include <iterator>

namespace syncplayer {

static PFNGLCOMPRESSEDTEXIMAGE2DARBPROC glCompressedTexImage2DARB;
static PFNGLACTIVETEXTUREARBPROC glActiveTextureARB;
static PFNGLMULTITEXCOORD2FARBPROC glMultiTexCoord2fARB;

openGlPaintPlugin::openGlPaintPlugin(Context *context) {

	m_context = context;
	m_dpy = m_context->getDisplay();
	m_win = m_context->getX11Window();
	m_glc = m_context->getGLXContext();

	pthread_mutex_init(&m_mapMutex, NULL);
}

openGlPaintPlugin::~openGlPaintPlugin() {

	glDisable(GL_TEXTURE_2D);

	glDeleteTextures(m_maxTexNum, m_textures);
	delete[] m_textures;

	pthread_mutex_destroy(&m_mapMutex);

	//FIXME: free Queue
	map<Event*, EventID2Order*>::iterator iter1;
	EventID2Order::iterator iter2;
	QpicItem *qitem;
	PicInfo *tmpPtr = NULL;

	for (iter1 = m_registedEvent.begin(); iter1 != m_registedEvent.end();
			iter1++) {

		EventID2Order* eio = iter1->second;

		for (iter2 = eio->begin(); iter2 != eio->end(); iter2++) {

			qitem = iter2->second;

			while (qitem->size() > 0) {

				tmpPtr = qitem->front();

				if (tmpPtr->data != NULL) {
					free(tmpPtr->data);
				}
				qitem->pop();
			}
			delete qitem;
		}
		delete eio;
	}

}

void openGlPaintPlugin::initGLX() {

	glCompressedTexImage2DARB =
			(PFNGLCOMPRESSEDTEXIMAGE2DARBPROC) glXGetProcAddressARB(
					(const GLubyte*) "glCompressedTexImage2DARB");

	glActiveTextureARB = (PFNGLACTIVETEXTUREARBPROC) glXGetProcAddressARB(
			(const GLubyte*) "glActiveTextureARB");

	glMultiTexCoord2fARB = (PFNGLMULTITEXCOORD2FARBPROC) glXGetProcAddressARB(
			(const GLubyte*) "glMultiTexCoord2fARB");
}

void openGlPaintPlugin::initGL() {

	int screenWidth = m_context->getScreenWidth();
	int screenHeight = m_context->getScreenHeight();

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	//gluPerspective(45.0,
	//		m_context->getScreenWidth() / m_context->getScreenHeight(), 0.0f,
	//		1000.0f);
	gluOrtho2D(-screenWidth / 2, screenWidth / 2, -screenHeight / 2,
			screenHeight / 2);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glViewport(m_context->getScreenXOffset(), m_context->getScreenYOffset(),
			m_context->getScreenWidth(), m_context->getScreenHeight());

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

	glShadeModel(GL_SMOOTH);
	//剔除多边形背面
	glCullFace(GL_BACK);
	glFrontFace(GL_CCW);
	glEnable(GL_CULL_FACE);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	//查询当前OpenGL可用于多重纹理贴图的最大纹理数目,然后立刻申请最大纹理数目
	glGetIntegerv(GL_MAX_TEXTURE_UNITS_ARB, &m_maxTexNum);
	std::cerr << "This OpenGL support " << m_maxTexNum << " textures"
			<< std::endl;
	//申请纹理对象
	m_textures = new GLuint[m_maxTexNum];
	glGenTextures(m_maxTexNum, m_textures);
}

void openGlPaintPlugin::render() {

	glXMakeContextCurrent(m_dpy, *m_win, *m_win, *m_glc);
	glXSwapBuffers(m_dpy, *m_win);

	initGLX();
	initGL();

	map<Event*, EventID2Order*>::iterator iter1;
	EventID2Order::iterator iter2;
	QpicItem *qitem;
	PicInfo *tmpPtr = NULL;
	unsigned int count = 0;

	while (true) {

		long long beg = Nowus();
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		//开始一次新的项目查找

		pthread_mutex_lock(&m_mapMutex);

		//搜索一个Event
		for (iter1 = m_registedEvent.begin(); iter1 != m_registedEvent.end();
				iter1++) {

			//如果本event还没到时间，则跳过
			if (iter1->first->start > Nowus()) {
				continue;
			}

			EventID2Order* eio = iter1->second;

			count = 0;
			//搜索Event对应的队列
			for (iter2 = eio->begin(); iter2 != eio->end(); iter2++) {

				qitem = iter2->second;
				//FIXME:

				if (qitem->empty()) {
					break;
				}

				tmpPtr = qitem->front();
				/*
				 //FIXME:
				 //如果需要同步的队列没有都准备好，不绘制任何一个,处理下一个Event
				 if(eio->size() < tmpPtr->syncs) {
				 break;
				 }
				 */

				//判断下时间是不是需要贴图，如果时间没到则保留，否则弹出并绘制
				std::cerr << __FILE__ << ":" << __LINE__ << "Now: " << Nowus()
						<< "--------" << "from: " << tmpPtr->from << "-------"
						<< "dur: " << tmpPtr->from - Nowus() << "--------"
						<< "to: " << tmpPtr->to << "-------" << "dur: "
						<< tmpPtr->to - Nowus() << std::endl;

				long long begTimeMus = 0, endTimeMus = 0;
				begTimeMus = tmpPtr->from - Nowus();
				endTimeMus = tmpPtr->to - Nowus();
#if 1
				if (begTimeMus > 0) {
					std::cerr << __FILE__ ":" << __LINE__ << "future"
							<< std::endl;
					//FIXME:优化
					continue;
				} else if (endTimeMus < 0) {
					std::cerr << __FILE__ ":" << __LINE__ << "Over"
							<< std::endl;
					qitem->pop();
					//FIXME:可以优化
					delete (tmpPtr->data);
					tmpPtr->data = NULL;
					continue;
				}
				//如果找到一个项目count++
				count++;
				std::cerr << __FILE__ ":" << __LINE__ << "match" << std::endl;
#endif

				//qitem->pop();
				//paintDDS(tmpPtr);
				memDDS2Tex(tmpPtr, &m_textures[0]);

				glEnable(GL_TEXTURE_2D);
				glBindTexture(GL_TEXTURE_2D, m_textures[0]);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
						GL_NEAREST);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
						GL_NEAREST);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

				glBegin(GL_QUADS);
				glTexCoord2f(0.0f, 0.0f);
				glVertex3f(tmpPtr->left, tmpPtr->button, tmpPtr->zCoord);

				glTexCoord2f(1.0f, 0.0f);
				glVertex3f(tmpPtr->right, tmpPtr->button, tmpPtr->zCoord);

				glTexCoord2f(1.0f, 1.0f);
				glVertex3f(tmpPtr->right, tmpPtr->top, tmpPtr->zCoord);

				glTexCoord2f(0.0f, 1.0f);
				glVertex3f(tmpPtr->left, tmpPtr->top, tmpPtr->zCoord);
				glEnd();
				glFlush();
			}
		}
		pthread_mutex_unlock(&m_mapMutex);
		//如果一个项目都没有找到，就不要刷新
		/*if(count == 0) {
		 usleep(10 * 1000);
		 continue;
		 }*/

		long long end = Nowus();
		std::cerr << "this event found : " << count << "  render used : "
				<< end - beg << " us" << std::endl;
		glFinish();
		glXSwapBuffers(m_dpy, *m_win);

		//glDisable(GL_TEXTURE_2D);
		//usleep(40 * 1000);
	}
}

void openGlPaintPlugin::paintDDS(PicInfo* picInfo) {

	memDDS2Tex(picInfo, &m_textures[0]);

}

bool openGlPaintPlugin::memDDS2Tex(PicInfo* picInfo, GLuint *texID) {

	if (NULL == picInfo || NULL == picInfo->data) {
		fprintf(stderr, "dds2Tex  load error : %s:%d\n", __FILE__, __LINE__);
		return false;
	}

	int size = 0;
	int blockSize = 8;
	int numMipmaps = 1;
	DDS_IMAGE_DATA *pData = picInfo->data;

	blockSize = (pData->format == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT) ? 8 : 16;
	numMipmaps = pData->numMipmaps;

	if (pData->numMipmaps < 2) {
		numMipmaps = 1;
	}
	size = ((pData->width + 3) / 4) * ((pData->height + 3) / 4) * blockSize;

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, *texID);
	//加载纹理 内存----> 显存
	long long beg = Nowus();

	glCompressedTexImage2DARB(GL_TEXTURE_2D, 0, pData->format, pData->width,
			pData->height, 0, size, pData->pixels);

	long long end = Nowus();
	std::cerr << "glCompressedTexImage2DARB used : " << end - beg << " us"
			<< std::endl;

	glBindTexture(GL_TEXTURE_2D, 0);
	//FIXME:
	//free(pData);
	//picInfo->data = NULL;
	return true;
}

void openGlPaintPlugin::run() {

	render();
}

bool openGlPaintPlugin::push(PicInfo *dataPtr) {

	EventID2Order *eventID2Order = NULL;

	if (m_registedEvent.find(dataPtr->event) == m_registedEvent.end()) {

		eventID2Order = new EventID2Order();

		pthread_mutex_lock(&m_mapMutex);
		m_registedEvent.insert(
				pair<Event*, EventID2Order*>(dataPtr->event, eventID2Order));
		pthread_mutex_unlock(&m_mapMutex);

	} else {
		eventID2Order = m_registedEvent.find(dataPtr->event)->second;
	}

	QpicItem *qpicItem = NULL;

	if (eventID2Order->find(dataPtr->orderID) == eventID2Order->end()) {

		qpicItem = new QpicItem();
		pthread_mutex_lock(&m_mapMutex);
		eventID2Order->insert(
				pair<unsigned int, QpicItem*>(dataPtr->orderID, qpicItem));
		pthread_mutex_unlock(&m_mapMutex);

	} else {
		qpicItem = eventID2Order->find(dataPtr->orderID)->second;
	}

	//pthread_mutex_lock(&m_mapMutex);
	qpicItem->push(dataPtr);
	//pthread_mutex_unlock(&m_mapMutex);


	return true;
}

} //namespace

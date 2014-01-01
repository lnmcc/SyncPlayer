/*
 * xlibEnv.cpp
 *
 *  Created on: Oct 9, 2013
 *  Author : sijiewang
 *  Email  : lnmcc@hotmail.com
 *  Site   : lnmcc.net
 */

#include "xlibEnv.h"
#include <stdio.h>
#include <iostream>

namespace syncplayer {

xlibEnv* xlibEnv::m_xlibEnvInstance = NULL;
xlibEnv::Garbo xlibEnv::m_Garbo;

xlibEnv::Garbo::~Garbo() {

	if (NULL != m_xlibEnvInstance) {
		delete m_xlibEnvInstance;
	}
}

xlibEnv* xlibEnv::GetXlibEnv(Context *context) {

	if (NULL == m_xlibEnvInstance) {
		m_xlibEnvInstance = new xlibEnv(context);
	}
	return m_xlibEnvInstance;
}

xlibEnv::xlibEnv(Context *context) {

	m_context = context;

	m_dpy = NULL;
	m_vi = NULL;
	m_fc = NULL;

	m_screenWidth = m_context->getScreenWidth();
	m_screenHeight = m_context->getScreenHeight();
	m_screenXOffset = m_context->getScreenXOffset();
	m_screenYOffset = m_context->getScreenYOffset();

	//FIXME:应该做一些错误处理
	bool re = createWindow(m_screenWidth, m_screenHeight, m_screenXOffset,
			m_screenYOffset);
	if (!re) {
		std::cerr << __FILE__<< __LINE__ << ": create OpenGL Window failed!"
				<< std::endl;
	} else {
		std::cerr << __FILE__<< __LINE__ << ": create OpenGL Window success!"
				<< std::endl;

		m_context->setDisplay(m_dpy);
		m_context->setX11Window(&m_win);
		m_context->setGLXContext(&m_glc);
		m_context->setGLXWindow(&m_glw);
	}
}

xlibEnv::~xlibEnv() {

	glXMakeContextCurrent(m_dpy, None, None, NULL);
	glXDestroyContext(m_dpy, m_glc);

	if (NULL != m_vi)
		XFree(m_vi);

	if (NULL != m_fc)
		XFree(m_fc);

	XDestroyWindow(m_dpy, m_win);
	XCloseDisplay(m_dpy);
}

bool xlibEnv::createWindow(const unsigned int width, const unsigned int height,
		const int xOffset, const int yOffset) {

	XInitThreads();

	int nelements;
	int attr[] = { GLX_RENDER_TYPE, GLX_RGBA_BIT, GLX_DOUBLEBUFFER, True,
			GLX_DEPTH_SIZE, 24, None };

	m_dpy = XOpenDisplay(NULL);
	if (NULL == m_dpy) {
		std::cerr << __FILE__<< __LINE__ << ": Open Display error!"
				<< std::endl;
		return false;
	}

	m_fc = glXChooseFBConfig(m_dpy, 0, attr, &nelements);
	if (NULL == m_fc) {
		std::cerr << __FILE__<< __LINE__ << ": No appropriate framebuffer config found!"
				<< std::endl;
		return false;
	}

	m_vi = glXGetVisualFromFBConfig(m_dpy, *m_fc);
	if (NULL == m_vi) {
		std::cerr << __FILE__<< __LINE__ << ": No appropriate visual found!"
				<< std::endl;
		return false;
	}

	m_rootWin = DefaultRootWindow(m_dpy);
	m_cmap = XCreateColormap(m_dpy, m_rootWin, m_vi->visual, AllocNone);
	m_swa.colormap = m_cmap;
	m_swa.event_mask = ExposureMask | KeyPressMask;

	m_win = XCreateWindow(m_dpy, m_rootWin, xOffset, yOffset, width, height, 0,
			m_vi->depth, InputOutput, m_vi->visual, CWColormap | CWEventMask,
			&m_swa);
	XMapWindow(m_dpy, m_win);

	m_glc = glXCreateNewContext(m_dpy, *m_fc, GLX_RGBA_TYPE, NULL, GL_TRUE);

	return true;
}

void xlibEnv::initGL() {

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glClearColor(1.0f, 0.0f, 0.0f, 1.0f);

	glShadeModel(GL_SMOOTH);
	//剔除多边形背面
	glCullFace(GL_BACK);
	glFrontFace(GL_CCW);
	glEnable(GL_CULL_FACE);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
	glViewport(m_screenXOffset, m_screenYOffset, m_screenWidth, m_screenHeight);
}

} //namespace


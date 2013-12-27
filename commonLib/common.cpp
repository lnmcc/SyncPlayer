/*
 * common.cpp
 *
 *  Created on: Oct 11, 2013
 *  Author : sijiewang
 *  Email  : lnmcc@hotmail.com
 *  Site   : lnmcc.net
 */

#include "common.h"
#include <sys/time.h>

namespace syncplayer {

long long Nowus() {

    struct timeval t;
    gettimeofday(&t, NULL);
    return ((long long)t.tv_sec) * 1000000 + ((long long)t.tv_usec);
}

Context* Context::m_context = NULL;
Context::Garbo Context::m_Garbo;

Context::Garbo::~Garbo() {

	if (NULL != m_context) {
		delete m_context;
	}
}

Context* Context::GetContext() {

	if (NULL == m_context) {
		m_context = new Context();
	}
	return m_context;
}

Context::Context() {

	m_display = NULL;
	m_glc = NULL;
	m_glw = NULL;
	m_window = NULL;
	m_screenWidth = 800;
	m_screenHeight = 600;
	m_screenXOffset = 0;
	m_screenYOffset = 0;
	m_pluginManager = NULL;
	m_openGlPaintPlugin = NULL;
}

Context::~Context() {
//FIXME
}

//FIXME:
void Context::setScreenWidth(unsigned int width) {
	m_screenWidth = width;
}

unsigned int Context::getScreenWidth() {
	return m_screenWidth;
}

void Context::setScreenHeight(unsigned int height) {
	m_screenHeight = height;
}

unsigned int Context::getScreenHeight() {
	return m_screenHeight;
}

void Context::setScreenXOffset(int xOffset) {
	m_screenXOffset = xOffset;
}

int Context::getScreenXOffset() {
	return m_screenXOffset;
}

void Context::setScreenYOffset(int yOffset) {
	m_screenYOffset = yOffset;
}

int Context::getScreenYOffset() {
	return m_screenYOffset;
}

void Context::setDisplay(Display *display) {
	m_display = display;
}

Display* Context::getDisplay() {
	return m_display;
}

void Context::setGLXContext(GLXContext *glc) {
	m_glc = glc;
}

GLXContext* Context::getGLXContext() {
	return m_glc;
}

void Context::setGLXWindow(GLXWindow *glw) {
	m_glw = glw;
}

GLXWindow* Context::getGLXWindow() {
	return m_glw;
}

void Context::setX11Window(Window *win) {
	m_window = win;
}

Window* Context::getX11Window() {
	return m_window;
}

void Context::setPluginManager(PluginManager *pm) {
	m_pluginManager = pm;
}

PluginManager* Context::getPluginManager() {
	return m_pluginManager;
}

void Context::setOpenGlPaintPlugin(openGlPaintPlugin *opg) {
	m_openGlPaintPlugin = opg;
}

openGlPaintPlugin* Context::getOpenGlPaintPlugin() {
	return m_openGlPaintPlugin;
}

void Context::setEventParse(ParseEvent *pe) {
	m_eventParse = pe;
}

ParseEvent* Context::getEventParse() {
	return m_eventParse;
}

} //namespace

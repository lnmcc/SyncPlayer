/*
 * xlibEnv.h
 *
 *  Created on: Oct 9, 2013
 *  Author : sijiewang
 *  Email  : lnmcc@hotmail.com
 *  Site   : lnmcc.net
 */

#ifndef XLIBENV_H_
#define XLIBENV_H_

#include "../commonLib/common.h"
#include <stdio.h>
#include <X11/Xlib.h>
#include <GL/glx.h>

namespace syncplayer {

class xlibEnv {

public:
	static xlibEnv *GetXlibEnv(Context *context);

private:
	xlibEnv(Context *context);
	~xlibEnv();

	//创建窗口并提供opengl环境
	//[in] width: 窗口宽度
	//[in] height: 窗口高度
	//[in] xOffset: 窗口左上角的x偏移量
	//[in] yOffset: 窗口左上角的y偏移量
	bool createWindow(const unsigned int width, const unsigned int height,
			const int xOffset, const int yOffset);

	void initGL();

private:
	class Garbo {

	public:
		~Garbo();
	};

private:
	static xlibEnv *m_xlibEnvInstance;
	static Garbo m_Garbo;

	Context *m_context;
	Display *m_dpy;
	Window m_rootWin;
	Window m_win;
	XVisualInfo *m_vi;
	Colormap m_cmap;
	XSetWindowAttributes m_swa;

	GLXFBConfig *m_fc;
	GLXWindow m_glw;
	GLXContext m_glc;

	unsigned int m_screenWidth;
	unsigned int m_screenHeight;
	int m_screenXOffset;
	int m_screenYOffset;
};

} //namespace

#endif /* XLIBENV_H_ */

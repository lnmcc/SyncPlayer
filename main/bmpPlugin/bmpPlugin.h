/*
 * bmpPlugin.h
 *
 *  Created on: Oct 9, 2013
 *  Author : sijiewang
 *  Email  : lnmcc@hotmail.com
 *  Site   : lnmcc.net
 */
#ifndef BMPPLUGIN_H_
#define BMPPLUGIN_H_

#include "../../commonLib/common.h"
#include "../pluginManager/pluginManager.h"
#include "../../threadLib/thread.h"

#include <stdio.h>
#include <string>

namespace syncplayer {

class ddsWriter;

class bmpPlugin: public Plugin, public Thread {

public:
	bmpPlugin(Context *context, const std::string name = "bmpPlugin");

	~bmpPlugin();

	void run();
	void push(PicInfo* picInfo);
	bool full();
	bool empty();

	//把bmp文件加载成像素数组
	//[in] bmp: 文件路径
	//[out] pixels: 目标像素数组，这个方法会malloc内存，由调用者释放，如果读取失败则不分配内存
	//[out] width: bmp宽度
	//[out] height: bmp高度
	//[return] : 返回文件的大小， -1表示失败
	//FIXME: return value should not be int
	int file2Pix(const char *src, void **pixels, unsigned int *width,
			unsigned int *height);

	//把像素数据保存到文件
	//[in] pixels: 像素数据
	//[in] pixSize: 文件大小
	//[in] width: bmp宽度
	//[in] height: bmp高度
	//[in] destFile: 文件路径
	//[return] : true or false
	bool pix2File(const unsigned char *pixels, const int pixSize,
			const unsigned int width, const unsigned int height,
			const char *destFile);

private:
	Context *m_context;
	ddsWriter *m_ddswriter;
	CondQueue<PicInfo*> m_picInfoQueue;
};

} //namespace

#endif /* BMPPLUGIN_H_ */

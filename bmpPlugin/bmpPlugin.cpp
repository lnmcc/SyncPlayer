/*
 * bmpPlugin.cpp
 *
 *  Created on: Oct 9, 2013
 *  Author : sijiewang
 *  Email  : lnmcc@hotmail.com
 *  Site   : lnmcc.net
 */


#include "bmpPlugin.h"
#include "../ddsPlugin/ddswriter.h"
#include "../openGlPaintPlugin/openGlPaintPlugin.h"

#include <string.h>
#include <stdlib.h>
#include <iostream>

namespace xplay {

const static unsigned char bmpHeader[] = { 0x42, 0x4d, 0x3c, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x36, 0x00, 0x00, 0x00, 0x28, 0x00, 0x00, 0x00,
		0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x18, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x12, 0x0b, 0x00, 0x00,
		0x12, 0x0b, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

bmpPlugin::bmpPlugin(Context *context, const std::string name) :
		Plugin(name) {

	m_context = context;

	m_ddswriter = new ddsWriter();

	if (NULL == m_ddswriter) {
		std::cerr << __FILE__ << ":" << __LINE__ << "Cannot find ddsPlugin"
				<< std::endl;
		return;
	}
}

bmpPlugin::~bmpPlugin() {

	delete m_ddswriter;
}

int bmpPlugin::file2Pix(const char *src, void **pixels, unsigned int *width,
		unsigned int *height) {

	FILE *fp = fopen(src, "rb");
	if (NULL == fp) {
		std::cerr << __FILE__<< __LINE__ << " open file " << src << " error!"
				<< std::endl;
		pixels = NULL;
		return -1;
	}
	//bmp文件的大小信息在0x0012位置
	fseek(fp, 0x0012, SEEK_SET);
	fread(width, sizeof(*width), 1, fp);
	fread(height, sizeof(*height), 1, fp);

	//对一幅24位的bmp，一个像素为3个字节
	size_t pixSize = (*width) * (*height) * 3;
	void *p = malloc(pixSize);
	if (NULL == p) {
		std::cerr << __FILE__<< ":" << __LINE__ << " malloc error!"
				<< std::endl;
		fclose(fp);
		return -1;
	}
	*(void **) pixels = p;

	//bmp数据段在文件的0x0036位置
	fseek(fp, 0x0036, SEEK_SET);
	size_t num = fread(p, 1, pixSize, fp);
	if (num != pixSize) {
		std::cerr << __FILE__ << ":" << __LINE__ << ": read error!"
				<< std::endl;
		free(p);
		*(void**) pixels = NULL;
		return -1;
	}
	return pixSize;
}

bool bmpPlugin::pix2File(const unsigned char *pixels, const int pixSize,
		const unsigned int width, const unsigned int height,
		const char *destFile) {

	FILE *fp = fopen(destFile, "wb");
	if (NULL == fp) {
		std::cerr << __FILE__<< __LINE__ << " open file " << destFile
				<< " error!" << std::endl;
		return false;
	}

	fwrite(bmpHeader, sizeof(bmpHeader), 1, fp);
	fseek(fp, 0x0012, SEEK_SET);
	const unsigned int w = width;
	const unsigned int h = height;
	fwrite(&w, sizeof(w), 1, fp);
	fwrite(&h, sizeof(h), 1, fp);

	fclose(fp);

	return true;
}

void bmpPlugin::run() {

	PicInfo *picInfo = NULL;

	while (true) {

		unsigned int bmpWidth = 0, bmpHeight = 0;
		unsigned int ddsSize = 0;
		unsigned char *ddsMem = NULL;
		void *pixels = NULL;

		picInfo = m_picInfoQueue.pop();

		file2Pix(picInfo->path.c_str(), &pixels, &bmpWidth, &bmpHeight);

		std::cerr << "width = " << bmpWidth << " height = " << bmpHeight
				<< std::endl;

		ddsMem = m_ddswriter->pix2MemDDS((unsigned char*) pixels, bmpWidth,
				bmpHeight, &ddsSize);

		picInfo->data = m_ddswriter->loadDDSFromMem(ddsMem);

		m_context->getOpenGlPaintPlugin()->push(picInfo);
	}
}

void bmpPlugin::push(PicInfo *picInfo) {

	m_picInfoQueue.push(picInfo);
}

bool bmpPlugin::full() {
	return m_picInfoQueue.full();
}

bool bmpPlugin::empty() {
	return m_picInfoQueue.empty();
}

}


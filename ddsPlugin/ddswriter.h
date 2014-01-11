/*
 * ddsWrite.h
 *
 *  Created on: Oct 9, 2013
 *  Author : sijiewang
 *  Email  : lnmcc@hotmail.com
 *  Site   : lnmcc.net
 */

#ifndef _DDSWRITEE_H
#define  _DDSWRITEE_H

#include "../commonLib/common.h"
#include "../pluginManager/pluginManager.h"

namespace syncplayer {

#ifndef MAKEFOURCC
#define MAKEFOURCC(ch0, ch1, ch2, ch3) \
		((unsigned int)(unsigned char)(ch0) | ((unsigned int)(unsigned char)(ch1) << 8) | \
		((unsigned int)(unsigned char)(ch2) << 16) | ((unsigned int)(unsigned char)(ch3) << 24))
#endif

#define FOURCC_DXT1 MAKEFOURCC('D', 'X', 'T', '1')
#define FOURCC_DXT2 MAKEFOURCC('D', 'X', 'T', '2')
#define FOURCC_DXT3 MAKEFOURCC('D', 'X', 'T', '3')
#define FOURCC_DXT4 MAKEFOURCC('D', 'X', 'T', '4')
#define FOURCC_DXT5 MAKEFOURCC('D', 'X', 'T', '5')

#define MAX_DATA_QUEUE_SIZE 1024

typedef struct DDS_IMAGE_DATA {
	unsigned int width;
	unsigned int height;
	unsigned int numMipmaps;
	GLenum format;
	int components;
	const unsigned char *pixels;
} DDS_IMAGE_DATA;

class ddsWriter: public Plugin {

public:
	ddsWriter(Context *context = NULL, const std::string name = "ddsPlugin");
	~ddsWriter();

	bool memDDS2file(char *ddsFile, unsigned char *ddsMem, unsigned int size);

	bool pix2DDSfile(char *ddsFile, const int width, const int height,
			void *pix);

	//把像素转换成dds, 这个方法会申请内存，需要由调用者来释放
	//[in] pixs: 存放像素的内存区域
	//[in] width: 图像宽度
	//[in] height: 图像高度
	//[out] ddsSize: 生成的的dds文件大小
	//[return] 指向生存的dds文件在内存中的地址
	unsigned char *pix2MemDDS(unsigned char *pixs, const int width,
			const int height, unsigned int *ddsSize);

	//从内存中加载DDS文件，供memDDS2Tex内部调用
	//这个函数不会释放返回的内存空间，由memDDS2Tex释放
	//[in] dds: DDS文件在内存中的地址
	DDS_IMAGE_DATA* loadDDSFromMem(const unsigned char *dds);

private:
	void swap_rb(unsigned char *pixels, unsigned int n, int bpp);

	unsigned char *reverse_pixels(const unsigned char *src, int width,
			int height, unsigned int bpp);

	void convert_pixels(unsigned char *dst, unsigned char *src, int width,
			int height, int mipmaps);

private:
	Context *m_context;
};

} //namespace
#endif

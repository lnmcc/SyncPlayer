/*
 * ddsWriter.cpp
 *
 *  Created on: Oct 9, 2013
 *  Author : sijiewang
 *  Email  : lnmcc@hotmail.com
 *  Site   : lnmcc.net
 */

#include "dds.h"
#include "dxtendian.h"
#include "dxt.h"
#include "imath.h"
#include "dxt_tables.h"
#include "mipmap.h"
#include "ddswriter.h"

#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glx.h>
#include <GL/glext.h>

using namespace std;

namespace syncplayer {

ddsWriter::ddsWriter(Context* context, const std::string name) :
		Plugin(name) {

	m_context = context;
}

ddsWriter::~ddsWriter() {

}

void ddsWriter::swap_rb(unsigned char *pixels, unsigned int n, int bpp) {
	unsigned char t;

	for (unsigned int i = 0; i < n; i++) {
		t = pixels[bpp * i + 0];
		pixels[bpp * i + 0] = pixels[bpp * i + 2];
		pixels[bpp * i + 2] = t;
	}
}

/* reverse up down.                                        */
/* if you use this function, you ought to free the memory. */
unsigned char* ddsWriter::reverse_pixels(const unsigned char *src, int width,
		int height, unsigned int bpp) {
	int pxsz = 0;
	unsigned char *p = NULL;

	/* source pointer */
	const unsigned char *ps = NULL;

	/* dest pointer */
	unsigned char *pd = NULL;

	pxsz = width * height * bpp;
	p = (unsigned char*) malloc(pxsz);

	if (NULL == p) {
		fprintf(stderr, "malloc error %s:%d\n", __FILE__, __LINE__);
		return NULL;
	}
	ps = src;
	pd = p + width * bpp * (height - 1);

	for (int i = 0; i < height; i++) {
		memcpy(pd, ps, width * bpp);
		pd -= width * bpp;
		ps += width * bpp;
	}

	return p;
}

void ddsWriter::convert_pixels(unsigned char *dst, unsigned char *src,
		int width, int height, int mipmaps) {

	unsigned int num_pixels;
	unsigned char r, g, b, a;

	num_pixels = get_mipmapped_size(width, height, 1, 0, mipmaps,
			DDS_COMPRESS_NONE);

	for (unsigned int i = 0; i < num_pixels; i++) {
		b = src[4 * i + 0];
		g = src[4 * i + 1];
		r = src[4 * i + 2];
		a = src[4 * i + 3];

		dst[4 * i + 0] = b;
		dst[4 * i + 1] = g;
		dst[4 * i + 2] = r;
	}
}

/* this function will create a dds in memory.              */
/* if you use this function, you ought to free the memory. */
/* source format: BGRA .                                   */
unsigned char* ddsWriter::pix2MemDDS(unsigned char *pixs, const int width,
		const int height, unsigned int *ddsSize) {

	int compression = DDS_COMPRESS_NONE;
	/* we just support 1 mipmap*/
	unsigned int rmask = 0x00ff0000;
	unsigned int gmask = 0x0000ff00;
	unsigned int bmask = 0x000000ff;
	unsigned int amask = 0xff000000;
	unsigned int data_size = 0;
	unsigned int ddsSZ = 0;
	unsigned int fourcc = 0;

	unsigned char hdr[DDS_HEADERSIZE] = { 0 };
	int fmtbpp = 3, flags = 0, pflags = 0, caps = 0, caps2 = 0, num_mipmaps = 1,
			size = 0;
	unsigned char *pMem = NULL, *pTmp = NULL;

	if (NULL == pixs) {
		fprintf(stderr, "DDS: pixels point is NULL. %s:%d\n", __FILE__,
				__LINE__);
		return NULL;
	}

	if (!(IS_POT(width) && IS_POT(height))) {
		fprintf(stderr,
				"DDS: this image size is none power-of-2, will not be compressed.\n");
	} else {
		fprintf(stderr, "DDS: this image will  be compressed with DXT-1.\n");
		compression = DDS_COMPRESS_BC1;
	}

	PUTL32(hdr, FOURCC('D', 'D', 'S', ' '));
	PUTL32(hdr + 4, 124);
	PUTL32(hdr + 12, height);
	PUTL32(hdr + 16, width);
	PUTL32(hdr + 76, 32);
	PUTL32(hdr + 88, fmtbpp << 3);
	PUTL32(hdr + 92, rmask);
	PUTL32(hdr + 96, gmask);
	PUTL32(hdr + 100, bmask);
	PUTL32(hdr + 104, amask);

	flags = DDSD_CAPS | DDSD_PIXELFORMAT | DDSD_WIDTH | DDSD_HEIGHT;
	caps = DDSCAPS_TEXTURE;

	PUTL32(hdr + 28, num_mipmaps);
	PUTL32(hdr + 108, caps);
	PUTL32(hdr + 112, caps2);

	if (DDS_COMPRESS_NONE == compression) {
		flags |= DDSD_PITCH;
		pflags |= DDPF_RGB;
		// pflags |= DDPF_ALPHAPIXELS;
		PUTL32(hdr + 8, flags);
		PUTL32(hdr + 20, width * fmtbpp);
		PUTL32(hdr + 80, pflags);

		ddsSZ = DDS_HEADERSIZE + width * height * fmtbpp;
		pMem = (unsigned char*) malloc(ddsSZ);
		if (NULL == pMem) {
			fprintf(stderr, "DDS: Memory error: %s:%d", __FILE__, __LINE__);
			return NULL;
		}
		memset(pMem, 0, ddsSZ);
		pTmp = (unsigned char*) memcpy(pMem, hdr, DDS_HEADERSIZE);
		memcpy(pTmp + DDS_HEADERSIZE, pixs, width * height * fmtbpp);
		*ddsSize = ddsSZ;
		return pMem;

	} else {
		flags |= DDSD_LINEARSIZE;
		pflags |= DDPF_FOURCC;
		//pflags |= DDPF_ALPHAPIXELS;
		fourcc = FOURCC('D', 'X', 'T', '1');

		PUTL32(hdr + 8, flags);
		PUTL32(hdr + 80, pflags);
		/* we just support DXT1*/
		PUTL32(hdr + 84, fourcc);
		size = ((width + 3) >> 2) * ((height + 3) >> 2);
		size *= 8;
		PUTL32(hdr + 20, size);

		//      convert_pixels(dst, pixs, width, height, num_mipmaps);

		data_size = get_mipmapped_size(width, height, fmtbpp, 0, num_mipmaps,
				compression);
		ddsSZ = DDS_HEADERSIZE + data_size;
		pMem = (unsigned char*) malloc(ddsSZ);
		if (NULL == pMem) {
			fprintf(stderr, "DDS: Memory error: %s:%d", __FILE__, __LINE__);
			return NULL;
		}
		memset(pMem, 0, ddsSZ);
		pTmp = (unsigned char*) memcpy(pMem, hdr, DDS_HEADERSIZE);
		dxt_compress(pTmp + DDS_HEADERSIZE, (unsigned char*) pixs, compression,
				width, height, fmtbpp, num_mipmaps, 1, 0, 0, 0, 2.2);
		*ddsSize = ddsSZ;
		return pMem;
	}
}

/* this function just dump dds file which in memory on your disk */
bool ddsWriter::memDDS2file(char *ddsFile, unsigned char * ddsMem,
		unsigned int size) {
	char *fdest = ddsFile;
	unsigned char *pMem = ddsMem;
	unsigned int ddsSize = size;

	FILE *fp = fopen(fdest, "wb");
	if (NULL == fp) {
		fprintf(stderr, "DDS dump failed! Cannot open file: %s:%d\n", __FILE__,
				__LINE__);
		return false;
	}

	if (NULL != pMem) {
		fwrite(pMem, ddsSize, 1, fp);
	}

	fclose(fp);
	return true;
}

bool ddsWriter::pix2DDSfile(char *ddsFile, const int width, const int height,
		void *pix) {
	char *fdest = ddsFile;
	unsigned char *pMem = NULL;
	unsigned int ddsSize = 0;
	unsigned char *dst = NULL;

	FILE *fp = fopen(fdest, "wb");
	if (NULL == fp) {
		fprintf(stderr, "DDS dump failed! Cannot open file: %s:%d\n", __FILE__,
				__LINE__);
		return false;
	}

	dst = reverse_pixels((const unsigned char*) pix, width, height, 3);

	pMem = pix2MemDDS(dst, width, height, &ddsSize);

	free(dst);

	if (NULL != pMem) {
		fwrite(pMem, ddsSize, 1, fp);
		free(pMem);
	}

	fclose(fp);
	return true;
}

DDS_IMAGE_DATA* ddsWriter::loadDDSFromMem(const unsigned char *dds) {

	DDS_IMAGE_DATA *pData = NULL;
	dds_header_t hdr;
	unsigned int fourcc, iFactor, bufSZ;

	pData = (DDS_IMAGE_DATA*) malloc(sizeof(DDS_IMAGE_DATA));
	if (NULL == pData) {
		fprintf(stderr, "loadDDSFromMem memory error: %s:%d\n", __FILE__,
				__LINE__);
		return NULL;
	}
	memset(pData, 0, sizeof(DDS_IMAGE_DATA));

	memcpy(&hdr, dds, sizeof(dds_header_t));
#if	DEBUG
	fprintf(stderr, "height = %d\n", hdr.height);
	fprintf(stderr, "width = %d\n", hdr.width);
	fprintf(stderr, "num_mipmaps = %d\n", hdr.num_mipmaps);
	fprintf(stderr, "size = %d\n", hdr.size);
	fprintf(stderr, "fmt = %s\n", hdr.pixelfmt.fourcc);
	fprintf(stderr, "pitch_or_linsize = %d\n", hdr.pitch_or_linsize);
#endif
	fourcc = MAKEFOURCC(hdr.pixelfmt.fourcc[0],
			hdr.pixelfmt.fourcc[1],
			hdr.pixelfmt.fourcc[2],
			hdr.pixelfmt.fourcc[3]);

	switch (fourcc) {
	case FOURCC_DXT1:
		pData->format = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
		fprintf(stderr, "DXT1\n");
		iFactor = 2;
		break;
	case FOURCC_DXT3:
		pData->format = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
		iFactor = 4;
		break;
	case FOURCC_DXT5:
		pData->format = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
		iFactor = 4;
		break;
	default:
		fprintf(stderr, "loadDDSFromMem format error: %s:%d\n", __FILE__,
				__LINE__);
		free(pData);
		return NULL;
	}

	bufSZ = (hdr.num_mipmaps > 1) ?
			(hdr.pitch_or_linsize * iFactor) : hdr.pitch_or_linsize;
	pData->width = hdr.width;
	pData->height = hdr.height;
	pData->numMipmaps = hdr.num_mipmaps;
	pData->components = (fourcc == FOURCC_DXT1) ? 3 : 4;
	pData->pixels = dds + DDS_HEADERSIZE;

	return pData;
}

} //namespace


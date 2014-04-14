/*
 * dxt.h
 *
 *  Created on: Oct 9, 2013
 *      Author: sijiewang
 */

/*
	DDS GIMP plugin

	Copyright (C) 2004-2008 Shawn Kirst <skirst@insightbb.com>,
   with parts (C) 2003 Arne Reuter <homepage@arnereuter.de> where specified.

	This program is free software; you can redistribute it and/or
	modify it under the terms of the GNU General Public
	License as published by the Free Software Foundation; either
	version 2 of the License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
	General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; see the file COPYING.  If not, write to
	the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
	Boston, MA 02111-1307, USA.
*/

#ifndef DXT_H
#define DXT_H

//extern "C"
//{
int dxt_compress(unsigned char *dst, unsigned char *src, int format,
                 unsigned int width, unsigned int height, int bpp,
                 int mipmaps, int type, int dither, int filter,
                 int gamma_correct, float gamma);
int dxt_decompress(unsigned char *dst, unsigned char *src, int format,
                   unsigned int size, unsigned int width, unsigned int height,
                   int bpp);

int get_num_mipmaps(int width, int height);

unsigned int get_mipmapped_size(int width, int height, int bpp,
                                int level, int num, int format);

unsigned int get_volume_mipmapped_size(int width, int height,
                                       int depth, int bpp, int level,
                                       int num, int format);
float cubic_interpolate(float a, float b, float c, float d, float x);
int generate_mipmaps(unsigned char *dst, unsigned char *src,
                     unsigned int width, unsigned int height, int bpp,
                     int indexed, int mipmaps, int filter,
                     int gamma_correct, float gamma);
int generate_volume_mipmaps(unsigned char *dst, unsigned char *src,
                            unsigned int width, unsigned int height,
                            unsigned int depth, int bpp, int indexed,
                            int mipmaps, int filter,
                            int gamma_correct, float gamma);
//}
#endif

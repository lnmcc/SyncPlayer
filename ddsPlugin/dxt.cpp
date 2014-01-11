/*
 * dxt.cpp
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

/*
 * Parts of this code have been generously released in the public domain
 * by Fabian 'ryg' Giesen.  The original code can be found (at the time
 * of writing) here:  http://mollyrocket.com/forums/viewtopic.php?t=392
 *
 * For more information about this code, see the README.dxt file that
 * came with the source.
 */

#include "dds.h"
#include "dxtendian.h"
#include "mipmap.h"
#include "imath.h"
#include "dxt_tables.h"

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <glib.h>


/* extract 4x4 BGRA block */
static void extract_block(const unsigned char *src, int x, int y,
                          int w, int h, unsigned char *block)
{
   int i, j;
   int bw = MIN(w - x, 4);
   int bh = MIN(h - y, 4);
   int bx, by;
   const int rem[] =
   {
      0, 0, 0, 0,
      0, 1, 0, 1,
      0, 1, 2, 0,
      0, 1, 2, 3
   };

   for(i = 0; i < 4; ++i)
   {
      by = rem[(bh - 1) * 4 + i] + y;
      for(j = 0; j < 4; ++j)
      {
         bx = rem[(bw - 1) * 4 + j] + x;
         block[(i * 4 * 4) + (j * 4) + 0] =
            src[(by * (w * 4)) + (bx * 4) + 0];
         block[(i * 4 * 4) + (j * 4) + 1] =
            src[(by * (w * 4)) + (bx * 4) + 1];
         block[(i * 4 * 4) + (j * 4) + 2] =
            src[(by * (w * 4)) + (bx * 4) + 2];
         block[(i * 4 * 4) + (j * 4) + 3] =
            src[(by * (w * 4)) + (bx * 4) + 3];
      }
   }
}

/* pack BGR8 to RGB565 */
static inline unsigned short pack_rgb565(const unsigned char *c)
{
   //return(((c[2] >> 3) << 11) | ((c[1] >> 2) << 5) | (c[0] >> 3));
   return((mul8bit(c[2], 31) << 11) |
          (mul8bit(c[1], 63) <<  5) |
          (mul8bit(c[0], 31)      ));
}

/* unpack RGB565 to BGR */
static void unpack_rgb565(unsigned char *dst, unsigned short v)
{
   int r = (v >> 11) & 0x1f;
   int g = (v >>  5) & 0x3f;
   int b = (v      ) & 0x1f;

   dst[0] = (b << 3) | (b >> 2);
   dst[1] = (g << 2) | (g >> 4);
   dst[2] = (r << 3) | (r >> 2);
}

static void lerp_rgb(unsigned char *dst, unsigned char *a, unsigned char *b, int f)
{
   dst[0] = blerp(a[0], b[0], f);
   dst[1] = blerp(a[1], b[1], f);
   dst[2] = blerp(a[2], b[2], f);
}

static int color_distance(const unsigned char *c0,
                          const unsigned char *c1)
{
   return(((c0[0] - c1[0]) * (c0[0] - c1[0])) +
          ((c0[1] - c1[1]) * (c0[1] - c1[1])) +
          ((c0[2] - c1[2]) * (c0[2] - c1[2])));
}

static int luminance(const unsigned char *c)
{
   return((c[2] * 54 + c[1] * 182 + c[0] * 20) >> 8);
}

/* Block dithering function.  Simply dithers a block to 565 RGB.
 * (Floyd-Steinberg)
 */
static void dither_block(unsigned char *dst, const unsigned char *block)
{
   int err[8], *ep1 = err, *ep2 = err + 4, *tmp;
   int c, y;
   unsigned char *bp, *dp;
   const unsigned char *quant;

   /* process channels seperately */
   for(c = 0; c < 3; ++c)
   {
      bp = (unsigned char *)block;
      dp = dst;
      quant = (c == 1) ? quantG + 8 : quantRB + 8;

      bp += c;
      dp += c;

      memset(err, 0, sizeof(err));

      for(y = 0; y < 4; ++y)
      {
         /* pixel 0 */
         dp[ 0] = quant[bp[ 0] + ((3 * ep2[1] + 5 * ep2[0]) >> 4)];
         ep1[0] = bp[ 0] - dp[ 0];

         /* pixel 1 */
         dp[ 4] = quant[bp[ 4] + ((7 * ep1[0] + 3 * ep2[2] + 5 * ep2[1] + ep2[0]) >> 4)];
         ep1[1] = bp[ 4] - dp[ 4];

         /* pixel 2 */
         dp[ 8] = quant[bp[ 8] + ((7 * ep1[1] + 3 * ep2[3] + 5 * ep2[2] + ep2[1]) >> 4)];
         ep1[2] = bp[ 8] - dp[ 8];

         /* pixel 3 */
         dp[12] = quant[bp[12] + ((7 * ep1[2] + 5 * ep2[3] + ep2[2]) >> 4)];
         ep1[3] = bp[12] - dp[12];

         /* advance to next line */
         tmp = ep1;
         ep1 = ep2;
         ep2 = tmp;

         bp += 16;
         dp += 16;
      }
   }
}

/* Color matching function */
static unsigned int match_colors_block(const unsigned char *block,
                                       unsigned char color[4][3],
                                       int dither)
{
   unsigned int mask = 0;
   int dirb = color[0][0] - color[1][0];
   int dirg = color[0][1] - color[1][1];
   int dirr = color[0][2] - color[1][2];
   int dots[16], stops[4];
   int c0pt, halfpt, c3pt, dot;
   int i;

   for(i = 0; i < 16; ++i)
      dots[i] = block[4 * i] * dirb + block[4 * i + 1] * dirg + block[4 * i + 2] * dirr;

   for(i = 0; i < 4; ++i)
      stops[i] = color[i][0] * dirb + color[i][1] * dirg + color[i][2] * dirr;

   c0pt = (stops[1] + stops[3]) >> 1;
   halfpt = (stops[3] + stops[2]) >> 1;
   c3pt = (stops[2] + stops[0]) >> 1;

   if(!dither)
   {
      /* the version without dithering is straight-forward */
      for(i = 15; i >= 0; --i)
      {
         mask <<= 2;
         dot = dots[i];

         if(dot < halfpt)
            mask |= (dot < c0pt) ? 1 : 3;
         else
            mask |= (dot < c3pt) ? 2 : 0;
      }
   }
   else
   {
      /* with floyd-steinberg dithering (see above) */
      int err[8], *ep1 = err, *ep2 = err + 4, *tmp;
      int *dp = dots, y, lmask, step;

      c0pt <<= 4;
      halfpt <<= 4;
      c3pt <<= 4;

      memset(err, 0, sizeof(err));

      for(y = 0; y < 4; ++y)
      {
         /* pixel 0 */
         dot = (dp[0] << 4) + (3 * ep2[1] + 5 * ep2[0]);
         if(dot < halfpt)
            step = (dot < c0pt) ? 1 : 3;
         else
            step = (dot < c3pt) ? 2 : 0;

         ep1[0] = dp[0] - stops[step];
         lmask = step;

         /* pixel 1 */
         dot = (dp[1] << 4) + (7 * ep1[0] + 3 * ep2[2] + 5 * ep2[1] + ep2[0]);
         if(dot < halfpt)
            step = (dot < c0pt) ? 1 : 3;
         else
            step = (dot < c3pt) ? 2 : 0;

         ep1[1] = dp[1] - stops[step];
         lmask |= step << 2;

         /* pixel 2 */
         dot = (dp[2] << 4) + (7 * ep1[1] + 3 * ep2[3] + 5 * ep2[2] + ep2[1]);
         if(dot < halfpt)
            step = (dot < c0pt) ? 1 : 3;
         else
            step = (dot < c3pt) ? 2 : 0;

         ep1[2] = dp[2] - stops[step];
         lmask |= step << 4;

         /* pixel 3 */
         dot = (dp[3] << 4) + (7 * ep1[2] + 5 * ep2[3] + ep2[2]);
         if(dot < halfpt)
            step = (dot < c0pt) ? 1 : 3;
         else
            step = (dot < c3pt) ? 2 : 0;

         ep1[3] = dp[3] - stops[step];
         lmask |= step << 6;

         /* advance to next line */
         tmp = ep1;
         ep1 = ep2;
         ep2 = tmp;

         dp += 4;
         mask |= lmask << (y * 8);
      }
   }

   return(mask);
}

/* Special case color matching for DXT1 color blocks with non-opaque
 * alpha values.  Simple distance based color matching.  This is my
 * little hack, Fabian had no need for DXT1-alpha :)
 */
static unsigned int match_colors_block_DXT1alpha(const unsigned char *block,
                                                 unsigned char color[4][3])
{
   int i, d0, d1, d2, idx;
   unsigned int mask = 0;

   for(i = 15; i >= 0; --i)
   {
      mask <<= 2;
      d0 = color_distance(&block[4 * i], color[0]);
      d1 = color_distance(&block[4 * i], color[1]);
      d2 = color_distance(&block[4 * i], color[2]);
      if(block[4 * i + 3] < 128)
         idx = 3;
      else if(d0 < d1 && d0 < d2)
         idx = 0;
      else if(d1 < d2)
         idx = 1;
      else
         idx = 2;
      mask |= idx;
   }

   return(mask);
}

/* The color optimization function. (Clever code, part 1) */
static void optimize_colors_block(const unsigned char *block,
                                  unsigned short *max16, unsigned short *min16)
{
   static const int niterpow = 4;

   int mu[3], mn[3], mx[3];
   int i, c, r, g, b, dot, iter;
   int muv, mnv, mxv, mnd, mxd;
   int cov[6];
   unsigned char *bp, mnc[3], mxc[3];
   float covf[6], vfr, vfg, vfb, magn;
   float fr, fg, fb;

   /* determine color distribution */
   for(c = 0; c < 3; ++c)
   {
      bp = (unsigned char *)block + c;

      muv = mnv = mxv = bp[0];
      for(i = 4; i < 64; i += 4)
      {
         muv += bp[i];
         if(mnv > bp[i]) mnv = bp[i];
         if(mxv < bp[i]) mxv = bp[i];
      }

      mu[c] = (muv + 8) >> 4;
      mn[c] = mnv;
      mx[c] = mxv;
   }

   memset(cov, 0, sizeof(cov));

   /* determine covariance matrix */
   for(i = 0; i < 16; ++i)
   {
      b = block[4 * i + 0] - mu[0];
      g = block[4 * i + 1] - mu[1];
      r = block[4 * i + 2] - mu[2];

      cov[0] += r * r;
      cov[1] += r * g;
      cov[2] += r * b;
      cov[3] += g * g;
      cov[4] += g * b;
      cov[5] += b * b;
   }

   /* convert covariance matrix to float, find principal axis via power iter */
   for(i = 0; i < 6; ++i)
      covf[i] = cov[i] / 255.0f;

   vfb = mx[0] - mn[0];
   vfg = mx[1] - mn[1];
   vfr = mx[2] - mn[2];

   for(iter = 0; iter < niterpow; ++iter)
   {
      fr = vfr * covf[0] + vfg * covf[1] + vfb * covf[2];
      fg = vfr * covf[1] + vfg * covf[3] + vfb * covf[4];
      fb = vfr * covf[2] + vfg * covf[4] + vfb * covf[5];

      vfr = fr;
      vfg = fg;
      vfb = fb;
   }

   vfr = fabsf(vfr);
   vfg = fabsf(vfg);
   vfb = fabsf(vfb);

   magn = MAX(MAX(vfr, vfg), vfb);

   if(magn < 4.0) /* too small, default to luminance */
   {
      r = 148;
      g = 300;
      b = 58;
   }
   else
   {
      magn = 512.0f / magn;
      r = (int)(vfr * magn);
      g = (int)(vfg * magn);
      b = (int)(vfb * magn);
   }

   /* pick colors at extreme points */
   mnd =  0x7fffffff;
   mxd = -0x7fffffff;

   for(i = 0; i < 16; ++i)
   {
      dot = block[4 * i] * b + block[4 * i + 1] * g + block[4 * i + 2] * r;

      if(dot < mnd)
      {
         mnd = dot;
         memcpy(mnc, &block[4 * i], 3);
      }
      if(dot > mxd)
      {
         mxd = dot;
         memcpy(mxc, &block[4 * i], 3);
      }
   }

   /* reduce to 16-bit colors */
   *max16 = pack_rgb565(mxc);
   *min16 = pack_rgb565(mnc);
}

/* The refinement function (Clever code, part 2)
 * Tries to optimize colors to suit block contents better.
 * (By solving a least squares system via normal equations + Cramer's rule)
 */
static int refine_block(const unsigned char *block,
                        unsigned short *max16, unsigned short *min16,
                        unsigned int mask)
{
   static const int w1tab[4] = {3, 0, 2, 1};
   static const int prods[4] = {0x090000, 0x000900, 0x040102, 0x010402};
   /* ^ Some magic to save a lot of multiplies in the accumulating loop... */

   int akku = 0;
   int At1_r, At1_g, At1_b;
   int At2_r, At2_g, At2_b;
   unsigned int cm = mask;
   int i, step, w1, r, g, b;
   int xx, yy, xy;
   float frb, fg;
   unsigned short v, oldmin, oldmax;
   int s;

   oldmin = *min16;
   oldmax = *max16;
   if((mask ^ (mask << 2)) < 4) /* all pixels have the same index */
   {
      /* degenerate system, use optimal single-color match for average color */
      r = g = b = 8;
      for(i = 0; i < 16; ++i)
      {
         r += block[4 * i + 2];
         g += block[4 * i + 1];
         b += block[4 * i + 0];
      }

      r >>= 4;
      g >>= 4;
      b >>= 4;

      *max16 = (omatch5[r][0] << 11) | (omatch6[g][0] << 5) | omatch5[b][0];
      *min16 = (omatch5[r][1] << 11) | (omatch6[g][1] << 5) | omatch5[b][1];
      return(*min16 != oldmin || *max16 != oldmax);
   }

   At1_r = At1_g = At1_b = 0;
   At2_r = At2_g = At2_b = 0;

   for(i = 0; i < 16; ++i, cm >>= 2)
   {
      step = cm & 3;
      w1 = w1tab[step];
      r = block[4 * i + 2];
      g = block[4 * i + 1];
      b = block[4 * i + 0];

      akku  += prods[step];
      At1_r += w1 * r;
      At1_g += w1 * g;
      At1_b += w1 * b;
      At2_r += r;
      At2_g += g;
      At2_b += b;
   }

   At2_r = 3 * At2_r - At1_r;
   At2_g = 3 * At2_g - At1_g;
   At2_b = 3 * At2_b - At1_b;

   /* extract solutions and decide solvability */
   xx = akku >> 16;
   yy = (akku >> 8) & 0xff;
   xy = (akku >> 0) & 0xff;

   frb = 3.0f * 31.0f / 255.0f / (xx * yy - xy * xy);
   fg = frb * 63.0f / 31.0f;

   /* solve */
   s = (int)((At1_r * yy - At2_r * xy) * frb + 0.5f);
   if(s < 0) s = 0;
   if(s > 31) s = 31;
   v = s << 11;
   s = (int)((At1_g * yy - At2_g * xy) * fg + 0.5f);
   if(s < 0) s = 0;
   if(s > 63) s = 63;
   v |= s << 5;
   s = (int)((At1_b * yy - At2_b * xy) * frb + 0.5f);
   if(s < 0) s = 0;
   if(s > 31) s = 31;
   v |= s;
   *max16 = v;

   s = (int)((At2_r * xx - At1_r * xy) * frb + 0.5f);
   if(s < 0) s = 0;
   if(s > 31) s = 31;
   v = s << 11;
   s = (int)((At2_g * xx - At1_g * xy) * fg + 0.5f);
   if(s < 0) s = 0;
   if(s > 63) s = 63;
   v |= s << 5;
   s = (int)((At2_b * xx - At1_b * xy) * frb + 0.5f);
   if(s < 0) s = 0;
   if(s > 31) s = 31;
   v |= s;
   *min16 = v;

   return(oldmin != *min16 || oldmax != *max16);
}

/* Find min/max colors by distance */
static void get_min_max_colors_distance(const unsigned char *block,
                                        unsigned short *max16,
                                        unsigned short *min16)
{
   int i, j, dist, maxdist = -1;
   unsigned short c0 = 0, c1 = 0;

   for(i = 0; i < 64 - 4; i += 4)
   {
      for(j = i + 4; j < 64; j += 4)
      {
         dist = color_distance(&block[i], &block[j]);
         if(dist > maxdist)
         {
            maxdist = dist;
            c0 = pack_rgb565(block + i);
            c1 = pack_rgb565(block + j);
         }
      }
   }

   *max16 = MAX(c0, c1);
   *min16 = MIN(c0, c1);
}

/* Find min-max colors by luminance */
static void get_min_max_colors_luminance(const unsigned char *block,
                                         unsigned short *max16,
                                         unsigned short *min16)
{
   int i, lum, minlum = 0x7fffffff, maxlum = -0x7fffffff;
   unsigned char mn[3], mx[3];

   for(i = 0; i < 16; ++i)
   {
      lum = luminance(&block[4 * i]);
      if(lum > maxlum)
      {
         maxlum = lum;
         memcpy(mx, &block[4 * i], 3);
      }
      if(lum < minlum)
      {
         minlum = lum;
         memcpy(mn, &block[4 * i], 3);
      }
   }

   *max16 = pack_rgb565(mx);
   *min16 = pack_rgb565(mn);
}

#define INSET_SHIFT  4

/* Find min-max colors using the inset bounding box method */
static void get_min_max_colors_inset_bbox(const unsigned char *block,
                                          unsigned short *max16,
                                          unsigned short *min16)
{
   int i;
   unsigned char inset[3], mx[3], mn[3];

   mn[0] = mn[1] = mn[2] = 255;
   mx[0] = mx[1] = mx[2] = 0;

   for(i = 0; i < 16; ++i)
   {
      if(block[4 * i + 0] < mn[0]) mn[0] = block[4 * i + 0];
      if(block[4 * i + 1] < mn[1]) mn[1] = block[4 * i + 1];
      if(block[4 * i + 2] < mn[2]) mn[2] = block[4 * i + 2];
      if(block[4 * i + 0] > mx[0]) mx[0] = block[4 * i + 0];
      if(block[4 * i + 1] > mx[1]) mx[1] = block[4 * i + 1];
      if(block[4 * i + 2] > mx[2]) mx[2] = block[4 * i + 2];
   }

   inset[0] = (mx[0] - mn[0]) >> INSET_SHIFT;
   inset[1] = (mx[1] - mn[1]) >> INSET_SHIFT;
   inset[2] = (mx[2] - mn[2]) >> INSET_SHIFT;

   mn[0] = (mn[0] + inset[0] <= 255) ? mn[0] + inset[0] : 255;
   mn[1] = (mn[1] + inset[1] <= 255) ? mn[1] + inset[1] : 255;
   mn[2] = (mn[2] + inset[2] <= 255) ? mn[2] + inset[2] : 255;

   mx[0] = (mx[0] >= inset[0]) ? mx[0] - inset[0] : 0;
   mx[1] = (mx[1] >= inset[1]) ? mx[1] - inset[1] : 0;
   mx[2] = (mx[2] >= inset[2]) ? mx[2] - inset[2] : 0;

   *min16 = pack_rgb565(mn);
   *max16 = pack_rgb565(mx);
}

static void get_min_max_YCoCg(const unsigned char *block,
                              unsigned char *mincolor, unsigned char *maxcolor)
{
   int i;

   mincolor[2] = mincolor[1] = 255;
   maxcolor[2] = maxcolor[1] = 0;

   for(i = 0; i < 16; ++i)
   {
      if(block[4 * i + 2] < mincolor[2]) mincolor[2] = block[4 * i + 2];
      if(block[4 * i + 1] < mincolor[1]) mincolor[1] = block[4 * i + 1];
      if(block[4 * i + 2] > maxcolor[2]) maxcolor[2] = block[4 * i + 2];
      if(block[4 * i + 1] > maxcolor[1]) maxcolor[1] = block[4 * i + 1];
   }
}

static void scale_YCoCg(unsigned char *block,
                        unsigned char *mincolor, unsigned char *maxcolor)
{
   const int s0 = 128 / 2 - 1;
   const int s1 = 128 / 4 - 1;
   int m0, m1, m2, m3;
   int mask0, mask1, scale;
   int i;

   m0 = abs(mincolor[2] - 128);
   m1 = abs(mincolor[1] - 128);
   m2 = abs(maxcolor[2] - 128);
   m3 = abs(maxcolor[1] - 128);

   if(m1 > m0) m0 = m1;
   if(m3 > m2) m2 = m3;
   if(m2 > m0) m0 = m2;

   mask0 = -(m0 <= s0);
   mask1 = -(m0 <= s1);
   scale = 1 + (1 & mask0) + (2 & mask1);

   mincolor[2] = (mincolor[2] - 128) * scale + 128;
   mincolor[1] = (mincolor[1] - 128) * scale + 128;
   mincolor[0] = (scale - 1) << 3;

   maxcolor[2] = (maxcolor[2] - 128) * scale + 128;
   maxcolor[1] = (maxcolor[1] - 128) * scale + 128;
   maxcolor[0] = (scale - 1) << 3;

   for(i = 0; i < 16; ++i)
   {
      block[i * 4 + 2] = (block[i * 4 + 2] - 128) * scale + 128;
      block[i * 4 + 1] = (block[i * 4 + 1] - 128) * scale + 128;
   }
}

static void inset_bbox_YCoCg(unsigned char *mincolor, unsigned char *maxcolor)
{
   int inset[4], mini[4], maxi[4];

   inset[2] = (maxcolor[2] - mincolor[2]) - ((1 << (INSET_SHIFT - 1)) - 1);
   inset[1] = (maxcolor[1] - mincolor[1]) - ((1 << (INSET_SHIFT - 1)) - 1);

   mini[2] = ((mincolor[2] << INSET_SHIFT) + inset[2]) >> INSET_SHIFT;
   mini[1] = ((mincolor[1] << INSET_SHIFT) + inset[1]) >> INSET_SHIFT;

   maxi[2] = ((maxcolor[2] << INSET_SHIFT) - inset[2]) >> INSET_SHIFT;
   maxi[1] = ((maxcolor[1] << INSET_SHIFT) - inset[1]) >> INSET_SHIFT;

   mini[2] = (mini[2] >= 0) ? mini[2] : 0;
   mini[1] = (mini[1] >= 0) ? mini[1] : 0;

   maxi[2] = (maxi[2] <= 255) ? maxi[2] : 255;
   maxi[1] = (maxi[1] <= 255) ? maxi[1] : 255;

   mincolor[2] = (mini[2] & 0xf8) | (mini[2] >> 5);
   mincolor[1] = (mini[1] & 0xfc) | (mini[1] >> 6);

   maxcolor[2] = (maxi[2] & 0xf8) | (maxi[2] >> 5);
   maxcolor[1] = (maxi[1] & 0xfc) | (maxi[1] >> 6);
}

static void select_diagonal_YCoCg(const unsigned char *block,
                                  unsigned char *mincolor,
                                  unsigned char *maxcolor)
{
   unsigned char mid0, mid1, side, mask, b0, b1, c0, c1;
   int i;

   mid0 = ((int)mincolor[2] + maxcolor[2] + 1) >> 1;
   mid1 = ((int)mincolor[1] + maxcolor[1] + 1) >> 1;

   side = 0;
   for(i = 0; i < 16; ++i)
   {
      b0 = block[i * 4 + 2] >= mid0;
      b1 = block[i * 4 + 1] >= mid1;
      side += (b0 ^ b1);
   }

   mask = -(side > 8);
   mask &= -(mincolor[2] != maxcolor[2]);

   c0 = mincolor[1];
   c1 = maxcolor[1];

   c0 ^= c1;
   mask &= c0;
   c1 ^= mask;
   c0 ^= c1;

   mincolor[1] = c0;
   maxcolor[1] = c1;
}

static void eval_colors(unsigned char color[4][3],
                        unsigned short c0, unsigned short c1)
{
   unpack_rgb565(color[0], c0);
   unpack_rgb565(color[1], c1);
   if(c0 > c1)
   {
      lerp_rgb(color[2], color[0], color[1], 0x55);
      lerp_rgb(color[3], color[0], color[1], 0xaa);
   }
   else
   {
      color[2][0] = (color[0][0] + color[1][0]) >> 1;
      color[2][1] = (color[0][1] + color[1][1]) >> 1;
      color[2][2] = (color[0][2] + color[1][2]) >> 1;

      color[3][0] = color[3][1] = color[3][2] = 0;
   }
}

static void encode_color_block(unsigned char *dst,
                               const unsigned char *block,
                               int type, int dither, int dxt1_alpha)
{
   unsigned char dblock[64], color[4][3];
   unsigned short min16, max16;
   unsigned int v, mn, mx, mask;
   int i, block_has_alpha = 0;

   /* find min/max colors, determine if alpha values present in block
    * (for DXT1-alpha)
    */
   mn = mx = GETL32(block);
   for(i = 0; i < 16; ++i)
   {
      block_has_alpha = block_has_alpha || (block[4 * i + 3] < 255);
      v = GETL32(&block[4 * i]);
      mx = MAX(mx, v);
      mn = MIN(mn, v);
   }

   if(mn != mx) /* block is not a solid color, continue with compression */
   {
      /* compute dithered block for PCA if desired */
      if(dither)
         dither_block(dblock, block);

      switch(type)
      {
         case DDS_COLOR_DISTANCE:
            get_min_max_colors_distance(dither ? dblock : block, &max16, &min16);
            break;
         case DDS_COLOR_LUMINANCE:
            get_min_max_colors_luminance(dither ? dblock : block, &max16, &min16);
            break;
         case DDS_COLOR_INSET_BBOX:
            get_min_max_colors_inset_bbox(dither ? dblock : block, &max16, &min16);
            break;
         default:
            /* pca + map along principal axis */
            optimize_colors_block(dither ? dblock : block, &max16, &min16);
            if(max16 != min16)
            {
               eval_colors(color, max16, min16);
               mask = match_colors_block(block, color, dither != 0);
            }
            else
               mask = 0;

            /* refine */
            refine_block(dither ? dblock : block, &max16, &min16, mask);
            break;
      }

      if(max16 != min16)
      {
         eval_colors(color, max16, min16);
         mask = match_colors_block(block, color, dither != 0);
      }
      else
         mask = 0;
   }
   else /* constant color */
   {
      mask = 0xaaaaaaaa;
      max16 = (omatch5[block[2]][0] << 11) |
              (omatch6[block[1]][0] <<  5) |
              (omatch5[block[0]][0]      );
      min16 = (omatch5[block[2]][1] << 11) |
              (omatch6[block[1]][1] <<  5) |
              (omatch5[block[0]][1]      );
   }

   /* HACK! for DXT1 blocks which have non-opaque pixels */
   if(dxt1_alpha && block_has_alpha)
   {
      if(max16 > min16)
      {
         max16 ^= min16; min16 ^= max16; max16 ^= min16;
      }
      eval_colors(color, max16, min16);
      mask = match_colors_block_DXT1alpha(block, color);
   }

   if(max16 < min16 && !(dxt1_alpha && block_has_alpha))
   {
      max16 ^= min16; min16 ^= max16; max16 ^= min16;
      mask ^= 0x55555555;
   }

   PUTL16(&dst[0], max16);
   PUTL16(&dst[2], min16);
   PUTL32(&dst[4], mask);
}

/* write DXT3 alpha block */
static void encode_alpha_block_DXT3(unsigned char *dst,
                                    const unsigned char *block)
{
   int i, a1, a2;

   block += 3;

   for(i = 0; i < 8; ++i)
   {
      a1 = block[8 * i + 0];
      a2 = block[8 * i + 4];
      *dst++ = ((a2 >> 4) << 4) | (a1 >> 4);
   }
}

/* Write DXT5 alpha block */
static void encode_alpha_block_DXT5(unsigned char *dst,
                                    const unsigned char *block,
                                    const int offset)
{
   int i, v, mn, mx;
   int dist, bias, dist2, dist4, bits, mask;
   int a, idx, t;

   block += offset;
   block += 3;

   /* find min/max alpha pair */
   mn = mx = block[0];
   for(i = 0; i < 16; ++i)
   {
      v = block[4 * i];
      if(v > mx) mx = v;
      if(v < mn) mn = v;
   }

   /* encode them */
   *dst++ = mx;
   *dst++ = mn;

   /* determine bias and emit indices */
   dist = mx - mn;
   bias = mn * 7 - (dist >> 1);
   dist4 = dist * 4;
   dist2 = dist * 2;
   bits = 0;
   mask = 0;

   for(i = 0; i < 16; ++i)
   {
      a = block[4 * i] * 7 - bias;

      /* select index (hooray for bit magic) */
      t = (dist4 - a) >> 31; idx =  t & 4; a -= dist4 & t;
      t = (dist2 - a) >> 31; idx += t & 2; a -= dist2 & t;
      t = (dist  - a) >> 31; idx += t & 1;

      idx = -idx & 7;
      idx ^= (2 > idx);

      /* write index */
      mask |= idx << bits;
      if((bits += 3) >= 8)
      {
         *dst++ = mask;
         mask >>= 8;
         bits -= 8;
      }
   }
}

static void compress_DXT1(unsigned char *dst, const unsigned char *src,
                          int w, int h, int type, int dither, int alpha)
{
   unsigned char block[64];
   int x, y;

   for(y = 0; y < h; y += 4)
   {
      for(x = 0; x < w; x += 4)
      {
         extract_block(src, x, y, w, h, block);
         encode_color_block(dst, block, type, dither, alpha);
         dst += 8;
      }
   }
}

static void compress_DXT3(unsigned char *dst, const unsigned char *src,
                          int w, int h, int type, int dither)
{
   unsigned char block[64];
   int x, y;

   for(y = 0; y < h; y += 4)
   {
      for(x = 0; x < w; x += 4)
      {
         extract_block(src, x, y, w, h, block);
         encode_alpha_block_DXT3(dst, block);
         encode_color_block(dst + 8, block, type, dither, 0);
         dst += 16;
      }
   }
}

static void compress_DXT5(unsigned char *dst, const unsigned char *src,
                          int w, int h, int type, int dither)
{
   unsigned char block[64];
   int x, y;

   for(y = 0; y < h; y += 4)
   {
      for(x = 0; x < w; x += 4)
      {
         extract_block(src, x, y, w, h, block);
         encode_alpha_block_DXT5(dst, block, 0);
         encode_color_block(dst + 8, block, type, dither, 0);
         dst += 16;
      }
   }
}

static void compress_BC4(unsigned char *dst, const unsigned char *src,
                         int w, int h)
{
   unsigned char block[64];
   int x, y;

   for(y = 0; y < h; y += 4)
   {
      for(x = 0; x < w; x += 4)
      {
         extract_block(src, x, y, w, h, block);
         encode_alpha_block_DXT5(dst, block, -1);
         dst += 8;
      }
   }
}

static void compress_BC5(unsigned char *dst, const unsigned char *src,
                         int w, int h)
{
   unsigned char block[64];
   int x, y;

   for(y = 0; y < h; y += 4)
   {
      for(x = 0; x < w; x += 4)
      {
         extract_block(src, x, y, w, h, block);
         encode_alpha_block_DXT5(dst, block, -2);
         encode_alpha_block_DXT5(dst + 8, block, -1);
         dst += 16;
      }
   }
}

static void compress_YCoCg(unsigned char *dst, const unsigned char *src,
                           int w, int h)
{
   unsigned char block[64], colors[4][3];
   unsigned char *maxcolor, *mincolor;
   unsigned int mask;
   int c0, c1, d0, d1, d2, d3;
   int b0, b1, b2, b3, b4;
   int x0, x1, x2;
   int x, y, i;

   for(y = 0; y < h; y += 4)
   {
      for(x = 0; x < w; x += 4)
      {
         extract_block(src, x, y, w, h, block);

         encode_alpha_block_DXT5(dst, block, 0);

         maxcolor = &colors[0][0];
         mincolor = &colors[1][0];

         get_min_max_YCoCg(block, mincolor, maxcolor);
         scale_YCoCg(block, mincolor, maxcolor);
         inset_bbox_YCoCg(mincolor, maxcolor);
         select_diagonal_YCoCg(block, mincolor, maxcolor);

         lerp_rgb(&colors[2][0], maxcolor, mincolor, 0x55);
         lerp_rgb(&colors[3][0], maxcolor, mincolor, 0xaa);

         mask = 0;

         for(i = 15; i >= 0; --i)
         {
            c0 = block[4 * i + 2];
            c1 = block[4 * i + 1];

            d0 = abs(colors[0][2] - c0) + abs(colors[0][1] - c1);
            d1 = abs(colors[1][2] - c0) + abs(colors[1][1] - c1);
            d2 = abs(colors[2][2] - c0) + abs(colors[2][1] - c1);
            d3 = abs(colors[3][2] - c0) + abs(colors[3][1] - c1);

            b0 = d0 > d3;
            b1 = d1 > d2;
            b2 = d0 > d2;
            b3 = d1 > d3;
            b4 = d2 > d3;

            x0 = b1 & b2;
            x1 = b0 & b3;
            x2 = b0 & b4;

            mask <<= 2;
            mask |= (x2 | ((x0 | x1) << 1));
         }

         PUTL16(&dst[ 8], pack_rgb565(maxcolor));
         PUTL16(&dst[10], pack_rgb565(mincolor));
         PUTL32(&dst[12], mask);

         dst += 16;
      }
   }
}

int dxt_compress(unsigned char *dst, unsigned char *src, int format,
                 unsigned int width, unsigned int height, int bpp,
                 int mipmaps, int type, int dither, int filter,
                 int gamma_correct, float gamma)
{
   int i, size, w, h;
   unsigned int offset;
   unsigned char *tmp;
   int j;
   unsigned char *tmp2, *s;
   int dxt1_alpha = 0;

   if(!(IS_MUL4(width) && IS_MUL4(height)))
      return(0);

   if((mipmaps > 1) && !(IS_POW2(width) && IS_POW2(height)))
      return(0);

   size = get_mipmapped_size(width, height, bpp, 0, mipmaps,
                             DDS_COMPRESS_NONE);
   tmp = (unsigned char *)g_malloc(size);
   generate_mipmaps(tmp, src, width, height, bpp, 0, mipmaps, filter,
                    gamma_correct, gamma);

   if(bpp == 4 && format == DDS_COMPRESS_BC1)
      dxt1_alpha = 1;

   if(bpp == 1)
   {
      /* grayscale promoted to BGRA */

      size = get_mipmapped_size(width, height, 4, 0, mipmaps,
                                DDS_COMPRESS_NONE);
      tmp2 = (unsigned char *)g_malloc(size);

      for(i = j = 0; j < size; ++i, j += 4)
      {
         tmp2[j + 0] = tmp[i];
         tmp2[j + 1] = tmp[i];
         tmp2[j + 2] = tmp[i];
         tmp2[j + 3] = 255;
      }

      g_free(tmp);
      tmp = tmp2;
      bpp = 4;
   }
   else if(bpp == 2)
   {
      /* gray-alpha promoted to BGRA */

      size = get_mipmapped_size(width, height, 4, 0, mipmaps,
                                DDS_COMPRESS_NONE);
      tmp2 = (unsigned char *)g_malloc(size);

      for(i = j = 0; j < size; i += 2, j += 4)
      {
         tmp2[j + 0] = tmp[i];
         tmp2[j + 1] = tmp[i];
         tmp2[j + 2] = tmp[i];
         tmp2[j + 3] = tmp[i + 1];
      }

      g_free(tmp);
      tmp = tmp2;
      bpp = 4;
   }
   else if(bpp == 3)
   {
      size = get_mipmapped_size(width, height, 4, 0, mipmaps,
                                DDS_COMPRESS_NONE);
      tmp2 = (unsigned char *)g_malloc(size);

      for(i = j = 0; j < size; i += 3, j += 4)
      {
         tmp2[j + 0] = tmp[i + 0];
         tmp2[j + 1] = tmp[i + 1];
         tmp2[j + 2] = tmp[i + 2];
         tmp2[j + 3] = 255;
      }

      g_free(tmp);
      tmp = tmp2;
      bpp = 4;
   }

   offset = 0;
   w = width;
   h = height;
   s = tmp;

   for(i = 0; i < mipmaps; ++i)
   {
      switch(format)
      {
         case DDS_COMPRESS_BC1:
            compress_DXT1(dst + offset, s, w, h, type, dither, dxt1_alpha);
            break;
         case DDS_COMPRESS_BC2:
            compress_DXT3(dst + offset, s, w, h, type, dither);
            break;
         case DDS_COMPRESS_BC3:
            compress_DXT5(dst + offset, s, w, h, type, dither);
            break;
         case DDS_COMPRESS_BC4:
            compress_BC4(dst + offset, s, w, h);
            break;
         case DDS_COMPRESS_BC5:
            compress_BC5(dst + offset, s, w, h);
            break;
         case DDS_COMPRESS_YCOCGS:
            compress_YCoCg(dst + offset, s, w, h);
            break;
         default:
            compress_DXT5(dst + offset, s, w, h, type, dither);
            break;
      }
      s += (w * h * bpp);
      offset += get_mipmapped_size(w, h, 0, 0, 1, format);
      w = MAX(1, w >> 1);
      h = MAX(1, h >> 1);
   }

   g_free(tmp);

   return(1);
}

static void decode_color_block(unsigned char *dst, unsigned char *src,
                               int w, int h, int rowbytes, int format)
{
   int i, x, y;
   unsigned int indexes, idx;
   unsigned char *d;
   unsigned char colors[4][3];
   unsigned short c0, c1;

   c0 = GETL16(&src[0]);
   c1 = GETL16(&src[2]);

   unpack_rgb565(colors[0], c0);
   unpack_rgb565(colors[1], c1);

   if((c0 > c1) || (format == DDS_COMPRESS_BC3))
   {
      lerp_rgb(colors[2], colors[0], colors[1], 0x55);
      lerp_rgb(colors[3], colors[0], colors[1], 0xaa);
   }
   else
   {
      for(i = 0; i < 3; ++i)
      {
         colors[2][i] = (colors[0][i] + colors[1][i] + 1) >> 1;
         colors[3][i] = 255;
      }
   }

   src += 4;
   for(y = 0; y < h; ++y)
   {
      d = dst + (y * rowbytes);
      indexes = src[y];
      for(x = 0; x < w; ++x)
      {
         idx = indexes & 0x03;
         d[0] = colors[idx][2];
         d[1] = colors[idx][1];
         d[2] = colors[idx][0];
         if(format == DDS_COMPRESS_BC1)
            d[3] = ((c0 <= c1) && idx == 3) ? 0 : 255;
         indexes >>= 2;
         d += 4;
      }
   }
}

static void decode_alpha_block_DXT3(unsigned char *dst, unsigned char *src,
                                    int w, int h, int rowbytes)
{
   int x, y;
   unsigned char *d;
   unsigned int bits;

   for(y = 0; y < h; ++y)
   {
      d = dst + (y * rowbytes);
      bits = GETL16(&src[2 * y]);
      for(x = 0; x < w; ++x)
      {
         d[0] = (bits & 0x0f) * 17;
         bits >>= 4;
         d += 4;
      }
   }
}

static void decode_alpha_block_DXT5(unsigned char *dst, unsigned char *src,
                                    int w, int h, int bpp, int rowbytes)
{
   int x, y, code;
   unsigned char *d;
   unsigned char a0 = src[0];
   unsigned char a1 = src[1];
   unsigned long long bits = GETL64(src) >> 16;

   for(y = 0; y < h; ++y)
   {
      d = dst + (y * rowbytes);
      for(x = 0; x < w; ++x)
      {
         code = ((unsigned int)bits) & 0x07;
         if(code == 0)
            d[0] = a0;
         else if(code == 1)
            d[0] = a1;
         else if(a0 > a1)
            d[0] = ((8 - code) * a0 + (code - 1) * a1) / 7;
         else if(code >= 6)
            d[0] = (code == 6) ? 0 : 255;
         else
            d[0] = ((6 - code) * a0 + (code - 1) * a1) / 5;
         bits >>= 3;
         d += bpp;
      }
      if(w < 4) bits >>= (3 * (4 - w));
   }
}

int dxt_decompress(unsigned char *dst, unsigned char *src, int format,
                   unsigned int size, unsigned int width, unsigned int height,
                   int bpp)
{
   unsigned char *d, *s;
   unsigned int x, y, sx, sy;

   if(!(IS_MUL4(width) && IS_MUL4(height)))
      return(0);

   sx = (width  < 4) ? width  : 4;
   sy = (height < 4) ? height : 4;

   s = src;

   for(y = 0; y < height; y += 4)
   {
      for(x = 0; x < width; x += 4)
      {
         d = dst + (y * width + x) * bpp;
         if(format == DDS_COMPRESS_BC1)
         {
            decode_color_block(d, s, sx, sy, width * bpp, format);
            s += 8;
         }
         else if(format == DDS_COMPRESS_BC2)
         {
            decode_alpha_block_DXT3(d + 3, s, sx, sy, width * bpp);
            s += 8;
            decode_color_block(d, s, sx, sy, width * bpp, format);
            s += 8;
         }
         else if(format == DDS_COMPRESS_BC3)
         {
            decode_alpha_block_DXT5(d + 3, s, sx, sy, bpp, width * bpp);
            s += 8;
            decode_color_block(d, s, sx, sy, width * bpp, format);
            s += 8;
         }
         else if(format == DDS_COMPRESS_BC4)
         {
            decode_alpha_block_DXT5(d, s, sx, sy, bpp, width * bpp);
            s += 8;
         }
         else if(format == DDS_COMPRESS_BC5)
         {
            decode_alpha_block_DXT5(d, s + 8, sx, sy, bpp, width * bpp);
            decode_alpha_block_DXT5(d + 1, s, sx, sy, bpp, width * bpp);
            s += 16;
         }
      }
   }

   return(1);
}




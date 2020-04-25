// Avisynth v2.5.  Copyright 2002 Ben Rudiak-Gould et al.
// http://www.avisynth.org

// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA, or visit
// http://www.gnu.org/copyleft/gpl.html .
//
// Linking Avisynth statically or dynamically with other modules is making a
// combined work based on Avisynth.  Thus, the terms and conditions of the GNU
// General Public License cover the whole combination.
//
// As a special exception, the copyright holders of Avisynth give you
// permission to link Avisynth with independent modules that communicate with
// Avisynth solely through the interfaces defined in avisynth.h, regardless of the license
// terms of these independent modules, and to copy and distribute the
// resulting combined work under terms of your choice, provided that
// every copy of the combined work is accompanied by a complete copy of
// the source code of Avisynth (the version of Avisynth used to produce the
// combined work), being distributed under the terms of the GNU General
// Public License plus this exception.  An independent module is a module
// which is not derived from or based on Avisynth, such as 3rd-party filters,
// import and export plugins, or graphical user interfaces.

#include "convert_yv12.h"


/* YV12 -> YUY2 conversion */


static inline void copy_yv12_line_to_yuy2_c(const BYTE* srcY, const BYTE* srcU, const BYTE* srcV, BYTE* dstp, int width) {
  for (int x = 0; x < width / 2; ++x) {
    dstp[x*4] = srcY[x*2];
    dstp[x*4+2] = srcY[x*2+1];
    dstp[x*4+1] = srcU[x];
    dstp[x*4+3] = srcV[x];
  }
}

void convert_yv12_to_yuy2_progressive_c(const BYTE* srcY, const BYTE* srcU, const BYTE* srcV, int src_width, int src_pitch_y, int src_pitch_uv, BYTE *dstp, int dst_pitch, int height) {
  //first two lines
  copy_yv12_line_to_yuy2_c(srcY, srcU, srcV, dstp, src_width);
  copy_yv12_line_to_yuy2_c(srcY+src_pitch_y, srcU, srcV, dstp+dst_pitch, src_width);

  //last two lines. Easier to do them here
  copy_yv12_line_to_yuy2_c(
    srcY + src_pitch_y * (height-2),
    srcU + src_pitch_uv * ((height/2)-1),
    srcV + src_pitch_uv * ((height/2)-1),
    dstp + dst_pitch * (height-2),
    src_width
    );
  copy_yv12_line_to_yuy2_c(
    srcY + src_pitch_y * (height-1),
    srcU + src_pitch_uv * ((height/2)-1),
    srcV + src_pitch_uv * ((height/2)-1),
    dstp + dst_pitch * (height-1),
    src_width
    );

  srcY += src_pitch_y*2;
  srcU += src_pitch_uv;
  srcV += src_pitch_uv;
  dstp += dst_pitch*2;

  for (int y = 2; y < height-2; y+=2) {
    for (int x = 0; x < src_width / 2; ++x) {
      dstp[x*4] = srcY[x*2];
      dstp[x*4+2] = srcY[x*2+1];

      //avg(avg(a, b)-1, b)
      dstp[x*4+1] = ((((srcU[x-src_pitch_uv] + srcU[x] + 1) / 2) + srcU[x]) / 2);
      dstp[x*4+3] = ((((srcV[x-src_pitch_uv] + srcV[x] + 1) / 2) + srcV[x]) / 2);

      dstp[x*4 + dst_pitch] = srcY[x*2 + src_pitch_y];
      dstp[x*4+2 + dst_pitch] = srcY[x*2+1 + src_pitch_y];

      dstp[x*4+1 + dst_pitch] = ((((srcU[x] + srcU[x+src_pitch_uv] + 1) / 2) + srcU[x]) / 2);
      dstp[x*4+3 + dst_pitch] = ((((srcV[x] + srcV[x+src_pitch_uv] + 1) / 2) + srcV[x]) / 2);
    }
    srcY += src_pitch_y*2;
    dstp += dst_pitch*2;
    srcU += src_pitch_uv;
    srcV += src_pitch_uv;
  }
}

void convert_yv12_to_yuy2_interlaced_c(const BYTE* srcY, const BYTE* srcU, const BYTE* srcV, int src_width, int src_pitch_y, int src_pitch_uv, BYTE *dstp, int dst_pitch, int height) {
  //first four lines
  copy_yv12_line_to_yuy2_c(srcY, srcU, srcV, dstp, src_width);
  copy_yv12_line_to_yuy2_c(srcY + src_pitch_y*2, srcU, srcV, dstp + dst_pitch*2, src_width);
  copy_yv12_line_to_yuy2_c(srcY + src_pitch_y, srcU + src_pitch_uv, srcV + src_pitch_uv, dstp + dst_pitch, src_width);
  copy_yv12_line_to_yuy2_c(srcY + src_pitch_y*3, srcU + src_pitch_uv, srcV + src_pitch_uv, dstp + dst_pitch*3, src_width);

  //last four lines. Easier to do them here
  copy_yv12_line_to_yuy2_c(
    srcY + src_pitch_y * (height-4),
    srcU + src_pitch_uv * ((height/2)-2),
    srcV + src_pitch_uv * ((height/2)-2),
    dstp + dst_pitch * (height-4),
    src_width
    );
  copy_yv12_line_to_yuy2_c(
    srcY + src_pitch_y * (height-2),
    srcU + src_pitch_uv * ((height/2)-2),
    srcV + src_pitch_uv * ((height/2)-2),
    dstp + dst_pitch * (height-2),
    src_width
    );
  copy_yv12_line_to_yuy2_c(
    srcY + src_pitch_y * (height-3),
    srcU + src_pitch_uv * ((height/2)-1),
    srcV + src_pitch_uv * ((height/2)-1),
    dstp + dst_pitch * (height-3),
    src_width
    );
  copy_yv12_line_to_yuy2_c(
    srcY + src_pitch_y * (height-1),
    srcU + src_pitch_uv * ((height/2)-1),
    srcV + src_pitch_uv * ((height/2)-1),
    dstp + dst_pitch * (height-1),
    src_width
    );

  srcY += src_pitch_y * 4;
  srcU += src_pitch_uv * 2;
  srcV += src_pitch_uv * 2;
  dstp += dst_pitch * 4;

  for (int y = 4; y < height-4; y+= 2) {
    for (int x = 0; x < src_width / 2; ++x) {
      dstp[x*4] = srcY[x*2];
      dstp[x*4+2] = srcY[x*2+1];

      dstp[x*4+1] = ((((srcU[x-src_pitch_uv*2] + srcU[x] + 1) / 2) + srcU[x]) / 2);
      dstp[x*4+3] = ((((srcV[x-src_pitch_uv*2] + srcV[x] + 1) / 2) + srcV[x]) / 2);

      dstp[x*4 + dst_pitch*2] = srcY[x*2 + src_pitch_y*2];
      dstp[x*4+2 + dst_pitch*2] = srcY[x*2+1 + src_pitch_y*2];

      dstp[x*4+1 + dst_pitch*2] = ((((srcU[x] + srcU[x+src_pitch_uv*2] + 1) / 2) + srcU[x]) / 2);
      dstp[x*4+3 + dst_pitch*2] = ((((srcV[x] + srcV[x+src_pitch_uv*2] + 1) / 2) + srcV[x]) / 2);
    }

    if (y % 4 == 0) {
      //top field processed, jumb to the bottom
      srcY += src_pitch_y;
      dstp += dst_pitch;
      srcU += src_pitch_uv;
      srcV += src_pitch_uv;
    } else {
      //bottom field processed, jump to the next top
      srcY += src_pitch_y*3;
      srcU += src_pitch_uv;
      srcV += src_pitch_uv;
      dstp += dst_pitch*3;
    }
  }
}


/* YUY2 -> YV12 conversion */


void convert_yuy2_to_yv12_progressive_c(const BYTE* src, int src_width, int src_pitch, BYTE* dstY, BYTE* dstU, BYTE* dstV, int dst_pitchY, int dst_pitchUV, int height) {
  //src_width is twice the luma width of yv12 frame
  const BYTE* srcp = src;
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < src_width / 2 ; ++x) {
      dstY[x] = srcp[x*2];
    }
    dstY += dst_pitchY;
    srcp += src_pitch;
  }


  for (int y = 0; y < height / 2; ++y) {
    for (int x = 0; x < src_width / 4; ++x) {
      dstU[x] = (src[x*4+1] + src[x*4+1+src_pitch] + 1) / 2;
      dstV[x] = (src[x*4+3] + src[x*4+3+src_pitch] + 1) / 2;
    }
    dstU += dst_pitchUV;
    dstV += dst_pitchUV;
    src += src_pitch * 2;
  }
}

void convert_yuy2_to_yv12_interlaced_c(const BYTE* src, int src_width, int src_pitch, BYTE* dstY, BYTE* dstU, BYTE* dstV, int dst_pitchY, int dst_pitchUV, int height) {
  const BYTE* srcp = src;
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < src_width / 2 ; ++x) {
      dstY[x] = srcp[x*2];
    }
    dstY += dst_pitchY;
    srcp += src_pitch;
  }

  for (int y = 0; y < height / 2; y+=2) {
    for (int x = 0; x < src_width / 4; ++x) {
      dstU[x] = ((src[x*4+1] + src[x*4+1+src_pitch*2] + 1) / 2 + src[x*4+1]) / 2;
      dstV[x] = ((src[x*4+3] + src[x*4+3+src_pitch*2] + 1) / 2 + src[x*4+3]) / 2;
    }
    dstU += dst_pitchUV;
    dstV += dst_pitchUV;
    src += src_pitch;

    for (int x = 0; x < src_width / 4; ++x) {
      dstU[x] = ((src[x*4+1] + src[x*4+1+src_pitch*2] + 1) / 2 + src[x*4+1+src_pitch*2]) / 2;
      dstV[x] = ((src[x*4+3] + src[x*4+3+src_pitch*2] + 1) / 2 + src[x*4+3+src_pitch*2]) / 2;
    }
    dstU += dst_pitchUV;
    dstV += dst_pitchUV;
    src += src_pitch*3;
  }
}

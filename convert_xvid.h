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


#include "internal.h"

void yv12_to_rgb32_mmx(BYTE *dst, 
                         int dst_stride, 
                         const BYTE *y_src,
                         const BYTE *u_src,
                         const BYTE *v_src, 
                         int y_stride, int uv_stride,
                         int width, int height);


void yv12_to_rgb24_mmx(BYTE *dst, //Currently not used (buggy!)
                         int dst_stride, 
                         const BYTE *y_src,
                         const BYTE *u_src,
                         const BYTE *v_src, 
                         int y_stride, int uv_stride,
                         int width, int height);



void yv12_to_uyvy_mmx(BYTE * dst,  // Currrently not used!
				int dst_stride,
				const BYTE * y_src,
				const BYTE * u_src,
				const BYTE * v_src,
				int y_stride,
				int uv_stride,
				int width,
        int height);  


void yv12_to_yuyv_mmx(BYTE * dst,
				int dst_stride,
				const BYTE * y_src,
				const BYTE * u_src,
				const BYTE * v_src,
				int y_stride,
				int uv_stride,
				int width,
        int height);


void rgb24_to_yv12_mmx (BYTE * const y_out,
						BYTE * const u_out,
						BYTE * const v_out,
						const BYTE * const src,
						const unsigned int width,
						const unsigned int height,
            const unsigned int stride);

void rgb32_to_yv12_mmx (BYTE * const y_out,
						BYTE * const u_out,
						BYTE * const v_out,
						const BYTE * const src,
						const unsigned int width,
						const unsigned int height,
            const unsigned int stride);


void yuyv_to_yv12_mmx(BYTE * const y_out,
						BYTE * const u_out,
						BYTE * const v_out,
						const BYTE * const src,
						const int width,
						const int height,
						const int stride);

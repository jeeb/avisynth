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

/*
** Turn. version 0.1
** (c) 2003 - Ernst PechÚ
**
*/



void TurnRGB24(const unsigned char *srcp, unsigned char *dstp, const int rowsize,
				const int height, const int src_pitch, const int dst_pitch,
				const int direction)
{
	int dstp_offset;
	if (direction == -1 ) {
		for (int y = 0; y<height; y++) {
			dstp_offset = (height-y)*3;
			for (int x=0; x<rowsize/3; x++) {	
				dstp[dstp_offset+0] = srcp[(x<<1)+x+0];
				dstp[dstp_offset+1] = srcp[(x<<1)+x+1];
				dstp[dstp_offset+2] = srcp[(x<<1)+x+2];
				dstp_offset += dst_pitch;
			}
			srcp += src_pitch;
		}
	}
	else {
		for (int y=0; y<height; y++) {
			dstp_offset = y*3;
			for (int x = 0; x<rowsize/3; x++) {	
				dstp[dstp_offset+0] = srcp[rowsize-3-(x<<1)-x+0];
				dstp[dstp_offset+1] = srcp[rowsize-3-(x<<1)-x+1];
				dstp[dstp_offset+2] = srcp[rowsize-3-(x<<1)-x+2];
				dstp_offset += dst_pitch;
			}
			srcp += src_pitch;
		}
	};
}

void TurnRGB32(const unsigned char *srcp, unsigned char *dstp, const int rowsize,
			   const int height, const int src_pitch, const int dst_pitch,
			   const int direction)
{
	int dstp_offset;
	if (direction == -1) {
		for (int y=0; y<height; y++) {
			dstp_offset = (height-y)<<2;
			for (int x=0; x<rowsize>>2; x++) {	
				dstp[dstp_offset+0] = srcp[(x<<2)+0];
				dstp[dstp_offset+1] = srcp[(x<<2)+1];
				dstp[dstp_offset+2] = srcp[(x<<2)+2];
				//Alpha Channel, maybe useless
				dstp[dstp_offset+3] = srcp[(x<<2)+3];
				dstp_offset += dst_pitch;
			}
			srcp += src_pitch;
		}
	}
	else {
		for (int y = 0; y<height; y++) {
			dstp_offset = y<<2;
			for (int x=0; x<rowsize>>2; x++) {	
				dstp[dstp_offset+0] = srcp[rowsize-4-(x<<2)+0];
				dstp[dstp_offset+1] = srcp[rowsize-4-(x<<2)+1];
				dstp[dstp_offset+2] = srcp[rowsize-4-(x<<2)+2];
				//Alpha Channel, maybe useless
				dstp[dstp_offset+3] = srcp[rowsize-4-(x<<2)+3];
				dstp_offset += dst_pitch;
			}
			srcp += src_pitch;
		}
	};
}

void TurnYUY2(const unsigned char *srcp, unsigned char *dstp, const int rowsize,
			  const int height, const int src_pitch, const int dst_pitch,
			  const int direction)
{
	unsigned char u,v;
	int dstp_offset;
	if (direction == -1)
	{
		for (int y=0; y<height; y+=2)
		{
			dstp_offset = ((height-y)<<1);
			for (int x=0; x<rowsize; x+=4)
			{
				u = (srcp[x+1] + srcp[x+1+src_pitch])>>1;
				v = (srcp[x+3] + srcp[x+3+src_pitch])>>1;
				dstp[dstp_offset+0] = srcp[x+src_pitch];
				dstp[dstp_offset+1] = u;
				dstp[dstp_offset+2] = srcp[x];
				dstp[dstp_offset+3] = v;
				dstp[dstp_offset+dst_pitch+0] = srcp[x+src_pitch+2];
				dstp[dstp_offset+dst_pitch+1] = u;
				dstp[dstp_offset+dst_pitch+2] = srcp[x+2];
				dstp[dstp_offset+dst_pitch+3] = v;
				dstp_offset += dst_pitch<<1;
			}
			srcp += src_pitch<<1;
		}
	}
	else
	{
		srcp += rowsize-2;
		for (int y=0; y<height; y+=2)
		{
			dstp_offset = (y<<1);
			for (int x=0; x<rowsize; x+=4)
			{
				u = (srcp[-2-x+1] + srcp[-2-x+1+src_pitch])>>1;
				v = (srcp[-2-x+3] + srcp[-2-x+3+src_pitch])>>1;
				dstp[dstp_offset+0] = srcp[-x+2];
				dstp[dstp_offset+1] = u;
				dstp[dstp_offset+2] = srcp[-x+2+src_pitch];
				dstp[dstp_offset+3] = v;
				dstp[dstp_offset+dst_pitch+0] = srcp[-x];
				dstp[dstp_offset+dst_pitch+1] = u;
				dstp[dstp_offset+dst_pitch+2] = srcp[-x+src_pitch];
				dstp[dstp_offset+dst_pitch+3] = v;
				dstp_offset += dst_pitch<<1;
			}
			srcp += src_pitch<<1;
		}
	}
}

void TurnPlanar(const unsigned char *srcp_y, unsigned char *dstp_y,
			  const unsigned char *srcp_u, unsigned char *dstp_u,
			  const unsigned char *srcp_v, unsigned char *dstp_v,
			  const int rowsize, const int height,
			  const int rowsizeUV, const int heightUV,
			  const int src_pitch_y, const int dst_pitch_y,
			  const int src_pitch_uv, const int dst_pitch_uv,
			  const int direction)
{
	int y, x, offset;
	if (direction == -1)
	{
		for(y=0; y<height; y++)
		{
			offset = height;
			for (int x=0; x<rowsize; x++)
			{
				dstp_y[offset-y] = srcp_y[x];
				offset += dst_pitch_y;
			}
			srcp_y += src_pitch_y;
		}
		for(y=0; y<heightUV; y++)
		{
			offset = (heightUV);
			for (x=0; x<rowsizeUV; x++)
			{
				dstp_u[offset-y] = srcp_u[x];
				dstp_v[offset-y] = srcp_v[x];
				offset += dst_pitch_uv;
			}
			srcp_u += src_pitch_uv;
			srcp_v += src_pitch_uv;
		}
	}
	else
	{
		srcp_y += rowsize;
		for(y=0; y<height; y++)
		{
			offset = 0;
			for (x=0; x<rowsize; x++)
			{
				dstp_y[dst_pitch_y*x+y] = srcp_y[-1-x];
				offset += dst_pitch_y;
			}
			srcp_y += src_pitch_y;
		}
		srcp_u += (rowsizeUV);
		srcp_v += (rowsizeUV);
		for(y=0; y<heightUV; y++)
		{
			offset = 0;
			for (x=0; x<rowsizeUV; x++)
			{
				dstp_u[offset+y] = srcp_u[-1-x];
				dstp_v[offset+y] = srcp_v[-1-x];
				offset += dst_pitch_uv;
			}
			srcp_u += src_pitch_uv;
			srcp_v += src_pitch_uv;
		}
	}
}
/*
	Decimate plugin for Avisynth -- performs 1-in-N
	decimation on a stream of progressive frames, which are usually
	obtained from the output of my Telecide plugin for Avisynth.
	For each group of N successive frames, this filter deletes the
	frame that is most similar to its predecessor. Thus, duplicate
	frames coming out of Telecide can be removed using Decimate. This
	filter adjusts the frame rate of the clip as
	appropriate. Selection of the cycle size is selected by specifying
	a parameter to Decimate() in the Avisynth scipt.

	Copyright (C) 2002 Donald A. Graft

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

	The author can be contacted at:
	Donald Graft
	neuron2@attbi.com.
*/

#include "decimate.h"






/***********************************
 *******   Decimate methods   ******
 **********************************/

Decimate::Decimate( PClip _child, int _cycle, int _mode, int _threshold, bool _debug,
				            IScriptEnvironment* env ) 
  : GenericVideoFilter(_child), cycle(_cycle), mode(_mode), threshold(_threshold), debug(_debug)
{
	/* Adjust frame rate and count. */
	if (cycle < 2 || cycle > 25)
		env->ThrowError("Decimate: cycle size out of range (2-25)");
	if (mode == 0)
	{
		num_frames_hi = vi.num_frames;
		vi.num_frames = vi.num_frames * (cycle - 1) / cycle;
		vi.SetFPS(vi.fps_numerator * (cycle - 1), vi.fps_denominator * cycle);
	}
	if (debug)
	{
		char b[80];
		sprintf(b, "Decimate %s by Donald Graft, Copyright 2002\n", VERSION);
		OutputDebugString(b);
	}
}


PVideoFrame __stdcall Decimate::GetFrame(int inframe, IScriptEnvironment* env)
{
	int dropframe, useframe, w, h, x, y, pitch, dpitch;
	PVideoFrame src, next, dst;
	BYTE *srcrp, *nextrp, *dstwp;
	unsigned int p1, p2;
	int metric;

	if (mode == 0)
	{
		/* Normal decimation. Remove the frame most similar to its preceding frame. */
		/* Determine the correct frame to use and get it. */
		useframe = inframe + inframe / (cycle - 1);
		FindDuplicate((useframe / cycle) * cycle, &dropframe, &metric, env);
		if (useframe >= dropframe) useframe++;
		src = child->GetFrame(useframe, env);
		if (debug)
		{	
			char buf[80];
			sprintf(buf,"Decimate: inframe %d useframe %d\n", inframe, useframe);
			OutputDebugString(buf);
		}
	    return src;
	}
	else if (mode == 1)
	{
		/* Find the most similar frame as above but replace it with a blend of
		   the preceding and following frames. */
		num_frames_hi = vi.num_frames;
		FindDuplicate((inframe / cycle) * cycle, &dropframe, &metric, env);
		if (inframe != dropframe || (threshold && metric > threshold))
		{
			PVideoFrame src = child->GetFrame(inframe, env);
			return src;
		}
		src = child->GetFrame(inframe, env);
		if (inframe < vi.num_frames - 1)
			next = child->GetFrame(inframe + 1, env);
		else
			next = child->GetFrame(vi.num_frames - 1, env);
		dst = env->NewVideoFrame(vi);
		pitch = src->GetPitch();
		dpitch = dst->GetPitch();
		w = src->GetRowSize();
		h = src->GetHeight();
	    srcrp = (BYTE *) src->GetReadPtr();
	    nextrp = (BYTE *) next->GetReadPtr();
	    dstwp = (BYTE *) dst->GetWritePtr();
		for (y = 0; y < h; y++)
		{
			for (x = 0; x < w; x+=4)
			{
				p1 = *(int *)(srcrp+x) & 0xfefefefe;
				p2 = *(int *)(nextrp+x) & 0xfefefefe;
				*(int *)(dstwp+x) = (p1>>1) + (p2>>1);
			}
			srcrp += pitch;
			nextrp += pitch;
			dstwp += dpitch;
		}
		return dst;
	}
	else
	{
		env->ThrowError("Decimate: invalid mode option (0=discard 1=frame interpolate)");
		/* Avoid compiler warning. */
		return src;
	}
}


void __stdcall Decimate::FindDuplicate(int frame, int *chosen, int *metric, IScriptEnvironment* env)
{
	int f;
	PVideoFrame store[MAX_CYCLE_SIZE+1];
	const BYTE *storep[MAX_CYCLE_SIZE+1];
	const BYTE *prev, *curr;
	int pitch;
	int row_size;
	int height;
	int x, y, lowest_index;
	int T = 15;
	unsigned long count[MAX_CYCLE_SIZE], lowest;
	static int last_request = -1, last_result, last_metric;

	/* Only recalculate differences when a new set is needed. */
	if (frame == last_request)
	{
		*chosen = last_result;
		*metric = last_metric;
		return;
	}
	last_request = frame;

	/* Get 6 (1-in-5) or 3 (1-in-2) frames starting at the one before the asked-for one. */
	if (frame == 0)
		store[0] = child->GetFrame(0, env);
 	else if (frame >= num_frames_hi - 1)
		store[0] = child->GetFrame(num_frames_hi - 1, env);
	else
		store[0] = child->GetFrame(frame - 1, env);
	storep[0] = store[0]->GetReadPtr();

	for (f = 1; f <= cycle; f++)
	{
 		if (frame + f >= num_frames_hi - 1)
			store[f] = child->GetFrame(num_frames_hi - 1, env);
		else
			store[f] = child->GetFrame(frame + f - 1, env);
		storep[f] = store[f]->GetReadPtr();
	}

    pitch = store[0]->GetPitch();
    row_size = store[0]->GetRowSize();
    height = store[0]->GetHeight();

	/* Compare each frame to its predecessor. Subsample the frame by
	   8 in both the x and y dimensions for speed. */
	for (f = 1; f <= cycle; f++)
	{
		prev = storep[f-1];
		curr = storep[f];
		count[f-1] = 0;
		for (y = 0; y < height; y++)
		{
#ifdef DECIMATE_MMX_BUILD
			count[f-1] += asm_compare(curr, prev, row_size / 32);
#else
			for (x = 0; x < row_size;)
			{
				/* Use a threshold to give a degree of noise
				   immunity. */
				if (abs(curr[x] - prev[x]) > T)
				{
					count[f-1]++;
				}
				if (!(++x%8)) x += 24;
			}
#endif
			prev += pitch;
			curr += pitch;
		}
	}
#ifdef DECIMATE_MMX_BUILD
	__asm emms;
#endif

	/* Find the frame with the lowest difference count but
	   don't use the artificial duplicate at frame 0. */
	if (frame == 0)
	{
		lowest = count[1];
		lowest_index = 1;
	}
	else
	{
		lowest = count[0];
		lowest_index = 0;
	}
	for (x = 1; x < cycle; x++)
	{
		if (count[x] < lowest)
		{
			lowest = count[x];
			lowest_index = x;
		}
	}
	last_result = frame + lowest_index;
	last_metric = (lowest * 100000) / (height*row_size);
	*chosen = last_result;
	*metric = last_metric;

	if (debug)
	{
		char buf[80];
//		sprintf(buf,"Decimate: %d: %d %d %d %d %d [lowest frame %d]\n",
//		        frame, count[0], count[1], count[2], count[3], count[4], frame + lowest_index);
//		OutputDebugString(buf);
		if (mode == 0)
			sprintf(buf,"Decimate: dropping frame %d, metric %d\n", last_result, last_metric);
		else
		{
			if (threshold && last_metric > threshold)
				sprintf(buf,"Decimate: choosing frame %d, metric %d\n", last_result, last_metric);
			else
				sprintf(buf,"Decimate: choosing frame %d, metric %d...blending\n", last_result, last_metric);
		}
		OutputDebugString(buf);
	}
}


AVSValue __cdecl Decimate::Create(AVSValue args, void* user_data, IScriptEnvironment* env) 
{
  return new Decimate( args[0].AsClip(),			  // clip
              			   args[1].AsInt(5),			  // cycle size
					             args[2].AsInt(0),			  // mode
						           args[3].AsInt(0),			  // threshold
						           args[4].AsBool(false),		// debug
						           env );
}










/******************************
 *******   ASM Compare   ******
 *****************************/

#ifdef DECIMATE_MMX_BUILD
#pragma warning(disable:4799)
int __declspec(naked) asm_compare( const BYTE *curr, const BYTE *prev, 
                                   int row_size )
{
	__declspec(align(8)) static const __int64 Threshold = 0x1010101010101010i64;
	__declspec(align(8)) static const __int64 Ones      = 0x0101010101010101i64;
	__declspec(align(8)) static const __int64 LowByte   = 0x00000000000000FFi64;
	__asm
	{
		push		ebp

		mov			edx,[esp+4+4]	// curr
		mov			ecx,[esp+8+4]	// prev
		mov			ebp,[esp+12+4]	// row_size / 32
		pxor		mm7,mm7			// running sum
		pxor		mm6,mm6			// zero for compares
		movq		mm5,Threshold
		movq		mm4,Ones
		movq		mm3,LowByte
xloop:
		movq		mm0,[edx]
		movq		mm2,mm0
		movq		mm1,[ecx]
		psubusb		mm0,mm1
		psubusb		mm1,mm2
		por			mm0,mm1
		psubusb		mm0,mm5
		pcmpeqb		mm0,mm6
		pandn		mm0,mm4

		movq		mm1,mm0
		psrlq		mm1,32
		paddusb		mm1,mm0
		movq		mm0,mm1
		psrlq		mm0,16
		paddusb		mm0,mm1
		movq		mm1,mm0
		psrlq		mm1,8
		paddusb		mm1,mm0
		pand		mm1,mm3
		paddusw		mm7,mm1

		add			edx,32
		add			ecx,32
		dec			ebp
		jnz			xloop

		movd		eax,mm7

		pop			ebp
		ret
	};
}
#pragma warning(default:4799)
#endif

/// Avisynth v1.0 beta.  Copyright 2000 Ben Rudiak-Gould.
// http://www.math.berkeley.edu/~benrg/avisynth.html

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


#include "text-overlay.h"





/********************************************************************
***** Declare index of new filters for Avisynth's filter engine *****
********************************************************************/

AVSFunction Text_filters[] = {
  { "ShowFrameNumber", "c[scroll]b", ShowFrameNumber::Create }, // clip, scroll?
  { "ShowSMPTE", "cf", ShowSMPTE::Create },                     // clip, fps
  { "Subtitle", "cs[x]i[y]i[first_frame]i[last_frame]i[font]s[size]i[text_color]i[halo_color]i", 
    Subtitle::Create },       // see docs!
  { "Compare", "cc[channels]s[logfile]s[show_graph]b", Compare::Create },
  { 0 }
};






/******************************
 *******   Anti-alias    ******
 *****************************/

Antialiaser::Antialiaser(int width, int height, const char fontname[], int size)
 : w(width), h(height)
{
  struct {
    BITMAPINFOHEADER bih;
    RGBQUAD clr[2];
  } b;

  b.bih.biSize                    = sizeof(BITMAPINFOHEADER);
  b.bih.biWidth                   = width * 8 + 32;
  b.bih.biHeight                  = height * 8 + 32;
  b.bih.biBitCount                = 1;
  b.bih.biPlanes                  = 1;
  b.bih.biCompression             = BI_RGB;
  b.bih.biXPelsPerMeter   = 0;
  b.bih.biYPelsPerMeter   = 0;
  b.bih.biClrUsed                 = 2;
  b.bih.biClrImportant    = 2;
  b.clr[0].rgbBlue = b.clr[0].rgbGreen = b.clr[0].rgbRed = 0;
  b.clr[1].rgbBlue = b.clr[1].rgbGreen = b.clr[1].rgbRed = 255;

  hdcAntialias = CreateCompatibleDC(NULL);
  hbmAntialias = CreateDIBSection
    ( hdcAntialias,
      (BITMAPINFO *)&b,
      DIB_RGB_COLORS,
      &lpAntialiasBits,
      NULL,
      0 );
  hbmDefault = (HBITMAP)SelectObject(hdcAntialias, hbmAntialias);

  HFONT newfont = LoadFont(fontname, size, true, false);
  hfontDefault = (HFONT)SelectObject(hdcAntialias, newfont);

  SetMapMode(hdcAntialias, MM_TEXT);
  SetTextColor(hdcAntialias, 0xffffff);
  SetBkColor(hdcAntialias, 0);

  alpha_bits = new char[width*height*2];

  dirty = true;
}


Antialiaser::~Antialiaser() {
  DeleteObject(SelectObject(hdcAntialias, hbmDefault));
  DeleteObject(SelectObject(hdcAntialias, hfontDefault));
  DeleteDC(hdcAntialias);
  delete[] alpha_bits;
}


HDC Antialiaser::GetDC() {
  dirty = true;
  return hdcAntialias;
}


void Antialiaser::Apply( const VideoInfo& vi, BYTE* buf, int pitch, int textcolor, 
                         int halocolor ) 
{
  if (vi.IsRGB32())
    ApplyRGB32(buf, pitch, textcolor, halocolor);
  else if (vi.IsRGB24())
    ApplyRGB24(buf, pitch, textcolor, halocolor);
  else if (vi.IsYUY2())
    ApplyYUY2(buf, pitch, textcolor, halocolor);
}



void Antialiaser::ApplyYUY2(BYTE* buf, int pitch, int textcolor, int halocolor) {
  if (dirty) GetAlphaRect();
  int Ytext = ((textcolor>>16)&255), Utext = ((textcolor>>8)&255), Vtext = (textcolor&255);
  int Yhalo = ((halocolor>>16)&255), Uhalo = ((halocolor>>8)&255), Vhalo = (halocolor&255);
  char* alpha = alpha_bits;
  for (int y=h; y>0; --y) {
    for (int x=0; x<w; x+=2) {
      if (*(__int32*)&alpha[x*2]) {
        buf[x*2+0] = (buf[x*2+0] * (64-alpha[x*2+0]-alpha[x*2+1]) + Ytext * alpha[x*2+0] + Yhalo * alpha[x*2+1]) >> 6;
        buf[x*2+2] = (buf[x*2+2] * (64-alpha[x*2+2]-alpha[x*2+3]) + Ytext * alpha[x*2+2] + Yhalo * alpha[x*2+3]) >> 6;
        int auv1 = alpha[x*2]+alpha[x*2+2];
        int auv2 = alpha[x*2+1]+alpha[x*2+3];
        buf[x*2+1] = (buf[x*2+1] * (128-auv1-auv2) + Utext * auv1 + Uhalo * auv2) >> 7;
        buf[x*2+3] = (buf[x*2+3] * (128-auv1-auv2) + Vtext * auv1 + Vhalo * auv2) >> 7;
      }
    }
    buf += pitch;
    alpha += w*2;
  }
}


void Antialiaser::ApplyRGB24(BYTE* buf, int pitch, int textcolor, int halocolor) {
  if (dirty) GetAlphaRect();
  int Rtext = ((textcolor>>16)&255), Gtext = ((textcolor>>8)&255), Btext = (textcolor&255);
  int Rhalo = ((halocolor>>16)&255), Ghalo = ((halocolor>>8)&255), Bhalo = (halocolor&255);
  char* alpha = alpha_bits + (h-1)*w*2;
  for (int y=h; y>0; --y) {
    for (int x=0; x<w; ++x) {
      int textalpha = alpha[x*2+0];
      int haloalpha = alpha[x*2+1];
      if (textalpha | haloalpha) {
        buf[x*3+0] = (buf[x*3+0] * (64-textalpha-haloalpha) + Btext * textalpha + Bhalo * haloalpha) >> 6;
        buf[x*3+1] = (buf[x*3+1] * (64-textalpha-haloalpha) + Gtext * textalpha + Ghalo * haloalpha) >> 6;
        buf[x*3+2] = (buf[x*3+2] * (64-textalpha-haloalpha) + Rtext * textalpha + Rhalo * haloalpha) >> 6;
      }
    }
    buf += pitch;
    alpha -= w*2;
  }
}


void Antialiaser::ApplyRGB32(BYTE* buf, int pitch, int textcolor, int halocolor) {
  if (dirty) GetAlphaRect();
  int Rtext = ((textcolor>>16)&255), Gtext = ((textcolor>>8)&255), Btext = (textcolor&255);
  int Rhalo = ((halocolor>>16)&255), Ghalo = ((halocolor>>8)&255), Bhalo = (halocolor&255);
  char* alpha = alpha_bits + (h-1)*w*2;
  for (int y=h; y>0; --y) {
    for (int x=0; x<w; ++x) {
      int textalpha = alpha[x*2+0];
      int haloalpha = alpha[x*2+1];
      if (textalpha | haloalpha) {
        buf[x*4+0] = (buf[x*4+0] * (64-textalpha-haloalpha) + Btext * textalpha + Bhalo * haloalpha) >> 6;
        buf[x*4+1] = (buf[x*4+1] * (64-textalpha-haloalpha) + Gtext * textalpha + Ghalo * haloalpha) >> 6;
        buf[x*4+2] = (buf[x*4+2] * (64-textalpha-haloalpha) + Rtext * textalpha + Rhalo * haloalpha) >> 6;
      }
    }
    buf += pitch;
    alpha -= w*2;
  }
}


void Antialiaser::GetAlphaRect() {

	dirty = false;

	static BYTE	bitcnt[256],		// bit count
							bitexl[256],		// expand to left bit
							bitexr[256];		// expand to right bit
	static bool fInited = false;

	if (!fInited) {
		int i;

		for(i=0; i<256; i++) {
			BYTE b=0, l=0, r=0;

			if (i&  1) { b=1; l|=0x01; r|=0xFF; }
			if (i&  2) { ++b; l|=0x03; r|=0xFE; }
			if (i&  4) { ++b; l|=0x07; r|=0xFC; }
			if (i&  8) { ++b; l|=0x0F; r|=0xF8; }
			if (i& 16) { ++b; l|=0x1F; r|=0xF0; }
			if (i& 32) { ++b; l|=0x3F; r|=0xE0; }
			if (i& 64) { ++b; l|=0x7F; r|=0xC0; }
			if (i&128) { ++b; l|=0xFF; r|=0x80; }

			bitcnt[i] = b;
			bitexl[i] = l;
			bitexr[i] = r;
		}

		fInited = true;
	}

	int srcpitch = (w+4+3) & -4;

	char* dst = alpha_bits;
	for (int y=0; y<h; ++y) {
		BYTE* src = (BYTE*)lpAntialiasBits + ((h-y-1)*8 + 20) * srcpitch + 2;
		int wt = w;
		do {
			int alpha1, alpha2;
			int i;
			BYTE bmasks[8], tmasks[8];

			alpha1 = alpha2 = 0;

/*			BYTE tmp = 0;
			for (i=0; i<8; i++) {
				tmp |= src[srcpitch*i];
				tmp |= src[srcpitch*i-1];
				tmp |= src[srcpitch*i+1];
				tmp |= src[srcpitch*(-8+i)];
				tmp |= src[srcpitch*(-8+i)-1];
				tmp |= src[srcpitch*(-8+i)+1];
				tmp |= src[srcpitch*(8+i)];
				tmp |= src[srcpitch*(8+i)-1];
				tmp |= src[srcpitch*(8+i)+1];
			}
*/
			DWORD tmp;
			__asm {						// test if the whole area isn't just plain black
				mov edx, srcpitch
				mov	esi, src
				mov	ecx, edx
				dec esi
				shl ecx, 3
				sub esi, ecx	; src - 8*pitch - 1

				mov eax, [esi]  ; repeat 24 times
				add esi, edx
				or eax, [esi]
				add esi, edx
				or eax, [esi]
				add esi, edx
				or eax, [esi]
				add esi, edx
				or eax, [esi]
				add esi, edx
				or eax, [esi]
				add esi, edx
				or eax, [esi]
				add esi, edx
				or eax, [esi]
				add esi, edx
				or eax, [esi]
				add esi, edx
				or eax, [esi]
				add esi, edx
				or eax, [esi]
				add esi, edx
				or eax, [esi]
				add esi, edx
				or eax, [esi]
				add esi, edx
				or eax, [esi]
				add esi, edx
				or eax, [esi]
				add esi, edx
				or eax, [esi]
				add esi, edx
				or eax, [esi]
				add esi, edx
				or eax, [esi]
				add esi, edx
				or eax, [esi]
				add esi, edx
				or eax, [esi]
				add esi, edx
				or eax, [esi]
				add esi, edx
				or eax, [esi]
				add esi, edx
				or eax, [esi]
				add esi, edx
				or eax, [esi]

				and eax, 0x00ffffff
				mov tmp, eax
			}

			if (tmp != 0) {			// quick exit in a common case
				for (i=0; i<8; i++)
					alpha1 += bitcnt[src[srcpitch*i]];

				BYTE cenmask = 0, mask1, mask2;

				for(i=0; i<=8; i++) {
					cenmask |= (BYTE)(((long)-src[srcpitch*i  ])>>31);
					cenmask |= bitexl[src[srcpitch*i-1]];
					cenmask |= bitexr[src[srcpitch*i+1]];
				}

				mask1 = mask2 = cenmask;

				for(i=0; i<8; i++) {
					mask1 |= (BYTE)(((long)-src[srcpitch*(-i)])>>31);
					mask1 |= bitexl[src[srcpitch*(-8+i)-1]];
					mask1 |= bitexr[src[srcpitch*(-8+1)+1]];
					mask2 |= (BYTE)(((long)-src[srcpitch*(8+i)])>>31);
					mask2 |= bitexl[src[srcpitch*(8+i)-1]];
					mask2 |= bitexr[src[srcpitch*(8+i)+1]];

					tmasks[i] = mask1;
					bmasks[i] = mask2;
				}

				for(i=0; i<8; i++) {
					alpha2 += bitcnt[cenmask | tmasks[7-i] | bmasks[i]];
				}
			}

			dst[0] = alpha1;
			dst[1] = alpha2-alpha1;
			dst += 2;
			++src;
		} while(--wt);
	}
}












/*************************************
 *******   Show Frame Number    ******
 ************************************/

ShowFrameNumber::ShowFrameNumber(PClip _child, bool _scroll)
 : GenericVideoFilter(_child), antialiaser(vi.width, vi.height, "Arial", 192),
   scroll(_scroll) {}


PVideoFrame ShowFrameNumber::GetFrame(int n, IScriptEnvironment* env) {
  PVideoFrame frame = child->GetFrame(n, env);
  env->MakeWritable(&frame);

  HDC hdc = antialiaser.GetDC();
  SetTextAlign(hdc, TA_BASELINE|TA_LEFT);
  RECT r = { 0, 0, 32767, 32767 };
  FillRect(hdc, &r, (HBRUSH)GetStockObject(BLACK_BRUSH));
  char text[40];
  wsprintf(text, "%05d", n);
  if( scroll ) {
    int n1 = vi.field_based ? (n/2) : n;
    int y = 192 + (192*n1)%(vi.height*8);
    TextOut(hdc, child->GetParity(n) ? 32 : vi.width*8-512, y, text, strlen(text));
  } else {
    for (int y=192; y<vi.height*8; y += 192)
      TextOut(hdc, child->GetParity(n) ? 32 : vi.width*8-512, y, text, strlen(text));
  }
  GdiFlush();

  antialiaser.Apply(vi, frame->GetWritePtr(), frame->GetPitch(),
    vi.IsYUY2() ? 0xD21092 : 0xFFFF00, vi.IsYUY2() ? 0x108080 : 0);

  return frame;
}


AVSValue __cdecl ShowFrameNumber::Create(AVSValue args, void*, IScriptEnvironment* env) 
{
  return new ShowFrameNumber(args[0].AsClip(), args[1].AsBool(false));
}








/***********************************
 *******   Show SMPTE code    ******
 **********************************/

ShowSMPTE::ShowSMPTE(PClip _child, double _rate, IScriptEnvironment* env)
  : GenericVideoFilter(_child), antialiaser(vi.width, vi.height, "Arial", 192)
{
  if (_rate == 24) {
    rate = 24;
    dropframe = false;
  } 
  else if (_rate == 25) {
    rate = 25;
    dropframe = false;
  } 
  else if (_rate == 30) {
  rate = 30;
  dropframe = false;
  } 
  else if (_rate > 29.969 && _rate < 29.971) {
    rate = 30;
    dropframe = true;
  } 
  else {
    env->ThrowError("ShowSMPTE: rate argument must be 24, 25, 30, or 29.97");
  }
}


PVideoFrame __stdcall ShowSMPTE::GetFrame(int n, IScriptEnvironment* env) 
{
  PVideoFrame frame = child->GetFrame(n, env);
  env->MakeWritable(&frame);

  if (n < 0) return frame;

  if (dropframe) {
    int high = n / 17982;
    int low = n % 17982;
    if (low>=2)
      low += 2 * ((low-2) / 1798);
    n = high * 18000 + low;
  }

  int seconds = n / rate;
  int frames = n % rate;
  char text[16];
  wsprintf(text, "%02d:%02d:%02d:%02d", seconds / 3600, (seconds/60)%60, seconds%60, frames);

  HDC hdc = antialiaser.GetDC();
  SetTextAlign(hdc, TA_BASELINE|TA_CENTER);
  // RECT r = { 0, 0, 32767, 32767 };
  // FillRect(hdc, &r, (HBRUSH)GetStockObject(BLACK_BRUSH));
  TextOut(hdc, vi.width*4, vi.height*8-32, text, strlen(text));
  GdiFlush();

  antialiaser.Apply( vi, frame->GetWritePtr(), frame->GetPitch(),
                     vi.IsYUY2() ? 0xD21092 : 0xFFFF00, vi.IsYUY2() ? 0x108080 : 0 );

  return frame;
}

AVSValue __cdecl ShowSMPTE::Create(AVSValue args, void*, IScriptEnvironment* env)
{
  return new ShowSMPTE(args[0].AsClip(), args[1].AsFloat(), env);
}









/***********************************
 *******   Subtitle Filter    ******
 **********************************/


Subtitle::Subtitle( PClip _child, const char _text[], int _x, int _y, int _firstframe, 
                    int _lastframe, const char _fontname[], int _size, int _textcolor, 
                    int _halocolor )
 : GenericVideoFilter(_child), antialiaser(0), text(_text), x(_x), y(_y), 
   firstframe(_firstframe), lastframe(_lastframe), fontname(MyStrdup(_fontname)), size(_size*8)
{
  if (vi.IsYUY2()) {
    textcolor = RGB2YUV(_textcolor);
    halocolor = RGB2YUV(_halocolor);
  } else {
    textcolor = _textcolor;
    halocolor = _halocolor;
  }
}


Subtitle::~Subtitle(void) 
{
  delete antialiaser;
}


PVideoFrame Subtitle::GetFrame(int n, IScriptEnvironment* env) 
{
  PVideoFrame frame = child->GetFrame(n, env);
  env->MakeWritable(&frame);

  if (n >= firstframe && n <= lastframe) {
    if (!antialiaser)
      InitAntialiaser();
    antialiaser->Apply(vi, frame->GetWritePtr(), frame->GetPitch(), textcolor, halocolor);
  } else {
    // if we get far enough away from the frames we're supposed to
    // subtitle, then junk the buffered drawing information
    if (antialiaser && (n < firstframe-10 || n > lastframe+10)) {
      delete antialiaser;
      antialiaser = 0;
    }
  }

  return frame;
}


AVSValue __cdecl Subtitle::Create(AVSValue args, void*, IScriptEnvironment* env) 
{
    PClip clip = args[0].AsClip();
    const char* text = args[1].AsString();
    const int first_frame = args[4].AsInt(0);
    const int last_frame = args[5].AsInt(clip->GetVideoInfo().num_frames-1);
    const char* const font = args[6].AsString("Arial");
    const int size = args[7].AsInt(18);
    const int text_color = args[8].AsInt(0xFFFF00);
    const int halo_color = args[9].AsInt(0);
    const int x = args[2].AsInt(8);
    const int y = args[3].AsInt(size);
    return new Subtitle(clip, text, x, y, first_frame, last_frame, font, size, text_color, halo_color);
}


void Subtitle::InitAntialiaser() 
{
  antialiaser = new Antialiaser(vi.width, vi.height, fontname, size);

  HDC hdcAntialias = antialiaser->GetDC();

  int real_x;
  if (x == -1) {
    SetTextAlign(hdcAntialias, TA_BASELINE|TA_CENTER);
    real_x = vi.width>>1;
  } else {
    SetTextAlign(hdcAntialias, TA_BASELINE|TA_LEFT);
    real_x = x;
  }

  TextOut(hdcAntialias, real_x*8+16, y*8+16, text, strlen(text));
  GdiFlush();
}










/************************************
 *******    Compare Filter    *******
 ***********************************/


Compare::Compare(PClip _child1, PClip _child2, const char* channels, const char *fname, bool _show_graph, IScriptEnvironment* env)
	: GenericVideoFilter(_child1),
	  child2(_child2),
	  log(NULL),
	  show_graph(_show_graph),
	  antialiaser(vi.width, vi.height, "Courier New", 128)
{
	const VideoInfo& vi2 = child2->GetVideoInfo();

	if (vi.pixel_type != vi2.pixel_type || vi.width != vi2.width || vi.height != vi2.height)
		env->ThrowError("Compare: incompatible clips");

	if (channels[0] == 0) {
		if (vi.IsRGB32() || vi.IsRGB24())
			channels = "RGB";
		else if (vi.IsYUY2())
			channels = "YUV";
		else env->ThrowError("Compare: clips have unknown format");
	}

	mask = 0;
	for (int i = 0; i < strlen(channels); i++) {
		if (vi.IsRGB())	{
			switch (channels[i]) {
			case 'b':
			case 'B': mask |= 0x000000ff; break;
			case 'g':
			case 'G': mask |= 0x0000ff00; break;
			case 'r':
			case 'R': mask |= 0x00ff0000; break;
			case 'a':
			case 'A': mask |= 0xff000000; break;
			default: env->ThrowError("Compare: invalid channel: %c", channels[i]);
			}
			if (vi.IsRGB24()) mask &= 0x00ffffff;		// no alpha channel in RGB24
		} else {	// YUV
			switch (channels[i]) {
			case 'y':
			case 'Y': mask |= 0x00ff00ff; break;
			case 'u':
			case 'U': mask |= 0x0000ff00; break;
			case 'v':
			case 'V': mask |= 0xff000000; break;
			default: env->ThrowError("Compare: invalid channel: %c", channels[i]);
			}
		}
	}

	masked_bytes = 0;
	for (DWORD temp = mask; temp != 0; temp >>=8)
		masked_bytes += (temp & 1);

	if (fname[0] != 0) {
		log = fopen(fname, "wt");
		if (log) {
			fprintf(log,"Comparing channel(s) %s\n\n",channels);
			fprintf(log," Frame      MAD        MD      PSNR (dB) \n");
			fprintf(log,"-----------------------------------------\n");
		} else
			env->ThrowError("Compare: unable to create file %s", fname);
	} else {
		psnrs = new int[vi.num_frames];
		if (psnrs)
			for (int i = 0; i < vi.num_frames; i++)
				psnrs[i] = 0;
	}

}


Compare::~Compare()
{
	if (log) fclose(log);
	delete[] psnrs;
}


AVSValue __cdecl Compare::Create(AVSValue args, void*, IScriptEnvironment *env)
{
	return new Compare(	args[0].AsClip(),			// clip
						args[1].AsClip(),			// base clip
						args[2].AsString(""),		// channels
						args[3].AsString(""),		// logfile
						args[4].AsBool(true),		// show_graph
						env);
}



PVideoFrame __stdcall Compare::GetFrame(int n, IScriptEnvironment* env)
{
	PVideoFrame f1 = child->GetFrame(n, env);
	PVideoFrame f2 = child2->GetFrame(n, env);

	const BYTE* f1ptr = f1->GetReadPtr();
	const BYTE* f2ptr = f2->GetReadPtr();
	const int pitch1 = f1->GetPitch();
	const int pitch2 = f2->GetPitch();
	const int rowsize = f1->GetRowSize();
	const int height = f1->GetHeight();

	int bytecount = rowsize * height * masked_bytes / 4;
	const int incr = vi.IsRGB24() ? 3 : 4;

	int SD = 0;
	int SAD = 0;
	double SSD = 0;
	int row_SSD;

	if (((rowsize & 7) && !vi.IsRGB24()) ||					// rowsize must be a multiple or 8 for RGB32 and YUY2
		((rowsize % 6) && vi.IsRGB24()) ||					// or 6 for RGB24
		!(env->GetCPUFlags() & CPUF_INTEGER_SSE)) {			// to use the ISSE routine
		for (int y = 0; y < height; y++) {
			row_SSD = 0;
			for (int x = 0; x < rowsize; x += incr) {
				DWORD p1 = *(DWORD *)(f1ptr + x) & mask;
				DWORD p2 = *(DWORD *)(f2ptr + x) & mask;
				int d0 = (p1 & 0xff) - (p2 & 0xff);
				int d1 = ((p1 >> 8) & 0xff) - ((p2 & 0xff00) >> 8);
				int d2 = ((p1 >>16) & 0xff) - ((p2 & 0xff0000) >> 16);
				int d3 = (p1 >> 24) - (p2 >> 24);
				SD += d0 + d1 + d2 + d3;
				SAD += abs(d0) + abs(d1) + abs(d2) + abs(d3);
				row_SSD += d0 * d0 + d1 * d1 + d2 * d2 + d3 * d3;
			}
			SSD += row_SSD;
			f1ptr += pitch1;
			f2ptr += pitch2;
		}
	} else {				// ISSE version; rowsize multiple of 8 for RGB32 and YUY2; 6 for RGB24
		const _int64 mask64 = (__int64)mask << (vi.IsRGB24()? 24: 32) | mask;
		const int incr2 = incr * 2;
		__int64 iSSD;

		__asm {
			mov			esi, f1ptr
			mov			edi, f2ptr
			add			esi, rowsize
			add			edi, rowsize
			mov			ebx, height
			xor			eax, eax			; sum of squared differences low
			xor			edx, edx			; sum of squared differences high
			pxor		mm7, mm7			; sum of absolute differences
			pxor		mm6, mm6			; zero
			pxor		mm5, mm5			; sum of differences
comp_loopy:
			mov			ecx, rowsize
			neg			ecx
			pxor		mm4, mm4			; sum of squared differences (row_SSD)
comp_loopx:
			movq		mm0, [esi+ecx]
			movq		mm1, [edi+ecx]
			pand		mm0, mask64
			pand		mm1, mask64
			movq		mm2, mm0
			psubusb		mm0, mm1
			psubusb		mm1, mm2
			 movq		mm2, mm0			; SSD calculations are indented
			psadbw		mm0, mm6
			 por		mm2, mm1
			psadbw		mm1, mm6
			 movq		mm3, mm2
			 punpcklbw	mm2, mm6
			 punpckhbw	mm3, mm6
			 pmaddwd	mm2, mm2
			paddd		mm7, mm0
			paddd		mm5, mm0
			 pmaddwd	mm3, mm3
			paddd		mm7, mm1
			 paddd		mm4, mm2
			psubd		mm5, mm1
			add			ecx, incr2
			 paddd		mm4, mm3			; keep two counts at once
			jne			comp_loopx

			add			esi, pitch1
			add			edi, pitch2
			movq		mm3, mm4
			punpckhdq	mm4, mm6
			paddd		mm3, mm4
			movd		ecx, mm3
			add			eax, ecx
			adc			edx, 0
			dec			ebx
			jne			comp_loopy

			movd		SAD, mm7
			movd		SD, mm5
			mov			DWORD PTR [iSSD], eax
			mov			DWORD PTR [iSSD+4], edx
			emms
		}
		SSD = iSSD;		// convert to double
	}

	double MAD = (double)SAD / bytecount;
	double MD = (double)SD / bytecount;
	if (SSD == 0.0) SSD = 1.0;
	double PSNR = 10.0 * log10(bytecount * 255.0 * 255.0 / SSD);

	if (log) fprintf(log,"%6u  %8.4f  %+9.4f   %8.4f\n", n, MAD, MD, PSNR);
	else {
		HDC hdc = antialiaser.GetDC();
		char text[100];
		RECT r= { 32, 16, 1024, 512 };
		sprintf(text, "Frame: %u\nMAD: %8.4f\nMD: %+9.4f\nPSNR:%6.2f dB", n, MAD, MD, PSNR);
		DrawText(hdc, text, -1, &r, 0);
		GdiFlush();

		env->MakeWritable(&f1);
		BYTE* dstp = f1->GetWritePtr();
		int dst_pitch = f1->GetPitch();
		antialiaser.Apply( vi, dstp, dst_pitch, vi.IsYUY2() ? 0xD21092 : 0xFFFF00, vi.IsYUY2() ? 0x108080 : 0 );

		if (show_graph) {
			// original idea by Marc_FD
			psnrs[n] = min((int)(PSNR + 0.5), 100);
			if (vi.height > 100) {
				if (vi.IsYUY2()) {
					dstp += (vi.height - 1) * dst_pitch;
					for (int y = 0; y <= 100; y++) {						
						for (int x = max(0, vi.width - n - 1); x < vi.width; x++) {
							if (y <= psnrs[n - vi.width + 1 + x]) {
								if (y <= psnrs[n - vi.width + 1 + x] - 2) {
									dstp[x << 1] = 0x00;				// Y
									dstp[((x & -1) << 1) + 1] = 0x80;	// U
									dstp[((x & -1) << 1) + 3] = 0x80;	// V
								} else {
									dstp[x << 1] = 0xFF;				// Y
									dstp[((x & -1) << 1) + 1] = 0x80;	// U
									dstp[((x & -1) << 1) + 3] = 0x80;	// V
								}
							}
						} // for x
						dstp -= dst_pitch;
					} // for y
				} else {	// RGB
					for (int y = 0; y <= 100; y++) {
						for (int x = max(0, vi.width - n - 1); x < vi.width; x++) {
							if (y <= psnrs[n - vi.width + 1 + x]) {
								const int xx = x * incr;
								if (y <= psnrs[n - vi.width + 1 + x] -2) {
									dstp[xx] = 0x00;			// B
									dstp[xx + 1] = 0x00;		// G
									dstp[xx + 2] = 0x00;		// R
								} else {
									dstp[xx] = 0xFF;			// B
									dstp[xx + 1] = 0xFF;		// G
									dstp[xx + 2] = 0xFF;		// R
								}
							}
						} // for x
						dstp += dst_pitch;
					} // for y
				} // RGB
			} // height > 100
		} // show_graph
	} // no logfile

	return f1;
}











/************************************
 *******   Helper Functions    ******
 ***********************************/

bool GetTextBoundingBox( const char* text, const char* fontname, int size, bool bold, 
                         bool italic, int align, int* width, int* height )
{
  HFONT hfont = LoadFont(fontname, size, bold, italic);
  if (hfont == NULL)
    return false;
  HDC hdc = GetDC(NULL);
  HFONT hfontDefault = (HFONT)SelectObject(hdc, hfont);
  int old_map_mode = SetMapMode(hdc, MM_TEXT);
  UINT old_text_align = SetTextAlign(hdc, align);

  *height = *width = 8;
  bool success = true;
  RECT r = { 0, 0, 0, 0 };
  for (;;) {
    const char* nl = strchr(text, '\n');
    if (nl-text) {
      success &= !!DrawText(hdc, text, nl ? nl-text : lstrlen(text), &r, DT_CALCRECT | DT_NOPREFIX);
      *width = max(*width, int(r.right+8));
    }
    *height += r.bottom;
    if (nl) {
      text = nl+1;
      if (*text)
        continue;
      else
        break;
    } else {
      break;
    }
  }

  SetTextAlign(hdc, old_text_align);
  SetMapMode(hdc, old_map_mode);
  SelectObject(hdc, hfontDefault);
  DeleteObject(hfont);
  ReleaseDC(NULL, hdc);

  return success;
}


void ApplyMessage( PVideoFrame* frame, const VideoInfo& vi, const char* message, int size, 
                   int textcolor, int halocolor, int bgcolor, IScriptEnvironment* env ) 
{
  Antialiaser antialiaser(vi.width, vi.height, "Arial", size);
  HDC hdcAntialias = antialiaser.GetDC();
  RECT r = { 4*8, 4*8, (vi.width-4)*8, (vi.height-4)*8 };
  DrawText(hdcAntialias, message, lstrlen(message), &r, DT_NOPREFIX|DT_CENTER);
  GdiFlush();
  if (vi.IsYUY2()) {
    textcolor = RGB2YUV(textcolor);
    halocolor = RGB2YUV(halocolor);
  }
  antialiaser.Apply(vi, (*frame)->GetWritePtr(), (*frame)->GetPitch(), textcolor, halocolor);
}


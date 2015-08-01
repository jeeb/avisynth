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


#include "stdafx.h"


/* Color YUV originally by Kiraru2002(masani)
 * 
 * Adapted for AviSynth and YV12 code added by Klaus Post
*/

#include "color.h"

extern const AVSFunction Color_filters[] = {
  { "ColorYUV",
					 "c[gain_y]f[off_y]f[gamma_y]f[cont_y]f" \
					 "[gain_u]f[off_u]f[gamma_u]f[cont_u]f" \
					 "[gain_v]f[off_v]f[gamma_v]f[cont_v]f" \
					 "[levels]s[opt]s[matrix]s[showyuv]b" \
					 "[analyze]b[autowhite]b[autogain]b[conditional]b",
           Color::Create },
  { 0 }
};
  static AVSValue __cdecl Create(AVSValue args, void* user_data, IScriptEnvironment* env);

Color::Color(PClip _child, double _gain_y, double _off_y, double _gamma_y, double _cont_y,
						double _gain_u, double _off_u, double _gamma_u, double _cont_u,
						double _gain_v, double _off_v, double _gamma_v, double _cont_v,
						const char *_levels, const char *_opt, const char *_matrix, 
						bool _colorbars, bool _analyze, bool _autowhite, bool _autogain, bool _conditional,
						IScriptEnvironment* env) :
				GenericVideoFilter(_child),
				y_gain(_gain_y), y_bright(_off_y), y_gamma(_gamma_y),y_contrast(_cont_y),
				u_gain(_gain_u), u_bright(_off_u), u_gamma(_gamma_u),u_contrast(_cont_u),
				v_gain(_gain_v), v_bright(_off_v), v_gamma(_gamma_v),v_contrast(_cont_v), conditional(_conditional)
{
    try {	// HIDE DAMN SEH COMPILER BUG!!!
		if (!vi.IsYUV())
			env->ThrowError("ColorYUV: requires YUV input");

		if (!CheckParms(_levels, _matrix, _opt)) {
			if (levels < 0)		env->ThrowError("ColorYUV: parameter error : levels");
			if (matrix < 0)		env->ThrowError("ColorYUV: parameter error : matrix");
			if (opt < 0)		env->ThrowError("ColorYUV: parameter error : opt");
		}
		colorbars=_colorbars;
		analyze=_analyze;
		autowhite=_autowhite;
		autogain=_autogain;
		MakeGammaLUT();
		if (colorbars) {
			vi.height=224*2;
			vi.width=224*2;
			vi.pixel_type=VideoInfo::CS_YV12;
		}
	}
	catch (...) { throw; }
}


PVideoFrame __stdcall Color::GetFrame(int frame, IScriptEnvironment* env)
{
	PVideoFrame src;
	unsigned long *srcp;
	int pitch, w, h;
	int i,j,wby4;
	int modulo;
	PIXELDATA	pixel;
	bool updateLUT = false;

  if (colorbars) {
    PVideoFrame dst= env->NewVideoFrame(vi);
    int* pdst=(int*)dst->GetWritePtr(PLANAR_Y);
    int Y=16+abs(219-((frame+219)%438));
    Y|=(Y<<8)|(Y<<16)|(Y<<24);
    for (int i = 0;i<224*224;i++)
      pdst[i] = Y;
    unsigned char* pdstb = dst->GetWritePtr(PLANAR_U);
    {for (unsigned char y=0;y<224;y++) {
      for (unsigned char x=0;x<224;x++) {
        pdstb[x] = 16+x;
      }
      pdstb += dst->GetPitch(PLANAR_U);
    }}

    pdstb = dst->GetWritePtr(PLANAR_V);
    {for (unsigned char y=0;y<224;y++) {
      for (unsigned char x=0;x<224;x++) {
        pdstb[x] = 16+y;
      }
      pdstb += dst->GetPitch(PLANAR_U);
    }}
    return dst;
  }
	src = child->GetFrame(frame, env);
	env->MakeWritable(&src);

	srcp = (unsigned long *) src->GetWritePtr();
	pitch = src->GetPitch();
	w = src->GetRowSize();
	h = src->GetHeight();
	wby4 = w / 4;
	modulo = pitch - w;
  if (analyze||autowhite||autogain) {
    unsigned int accum_Y[256],accum_U[256],accum_V[256];
    for (i=0;i<256;i++) {
      accum_Y[i]=0;
      accum_U[i]=0;
      accum_V[i]=0;
    }
    int uvdiv=1;  //UV divider (ratio between Y and UV pixels)
    if (vi.IsPlanar()) {
      uvdiv=4;
	    BYTE* srcp2 = (BYTE*) src->GetReadPtr(PLANAR_Y);
      for (int y=0;y<h;y++) {
        for (int x=0;x<w;x++) {
          accum_Y[srcp2[x]]++;
        }
        srcp2+=pitch;
      }
      pitch = src->GetPitch(PLANAR_U);
	    srcp2 = (BYTE*) src->GetReadPtr(PLANAR_U);
      {for (int y=0;y<h/2;y++) {
        for (int x=0;x<w/2;x++) {
          accum_U[srcp2[x]]++;
        }
        srcp2+=pitch;
      }}
	    srcp2 = (BYTE*) src->GetReadPtr(PLANAR_V);
      {for (int y=0;y<h/2;y++) {
        for (int x=0;x<w/2;x++) {
          accum_V[srcp2[x]]++;
        }
        srcp2+=pitch;
      }}
      pitch = src->GetPitch(PLANAR_Y);
    } else {  // YUY2
      uvdiv=2;
      for (int y=0;y<h;y++) {
        for (int x=0;x<wby4;x++) {
          unsigned long p=srcp[x];
          accum_Y[p&0xff]++;
          accum_Y[(p>>16)&0xff]++;
          accum_U[(p>>8)&0xff]++;
          accum_V[(p>>24)&0xff]++;
        }
        srcp+=pitch/4;
      }
      srcp=(unsigned long *)src->GetReadPtr();
    }
    int pixels = vi.width*vi.height;
    float avg_u=0, avg_v=0, avg_y=0;
    int min_u=0, min_v=0, min_y=0;
    int max_u=0, max_v=0, max_y=0;
    bool hit_y=false,hit_u=false,hit_v=false;
    int Amin_u=0, Amin_v=0, Amin_y=0;
    int Amax_u=0, Amax_v=0, Amax_y=0;
    bool Ahit_miny=false,Ahit_minu=false,Ahit_minv=false;
    bool Ahit_maxy=false,Ahit_maxu=false,Ahit_maxv=false;
    int At_y2=(pixels/256); // When 1/256th of all pixels have been reached, trigger "Loose min/max"
    int At_uv2=(pixels/1024); 
   
    for (i=0;i<256;i++) {
      avg_y+=(float)accum_Y[i]*(float)i;
      avg_u+=(float)accum_U[i]*(float)i;
      avg_v+=(float)accum_V[i]*(float)i;
      if (accum_Y[i]!=0) {max_y=i;hit_y=true;} else {if (!hit_y) min_y=i+1;} 
      if (accum_U[i]!=0) {max_u=i;hit_u=true;} else {if (!hit_u) min_u=i+1;} 
      if (accum_V[i]!=0) {max_v=i;hit_v=true;} else {if (!hit_v) min_v=i+1;} 

      if (!Ahit_miny) {Amin_y+=accum_Y[i]; if (Amin_y>At_y2){Ahit_miny=true; Amin_y=i;} }
      if (!Ahit_minu) {Amin_u+=accum_U[i]; if (Amin_u>At_uv2){Ahit_minu=true; Amin_u=i;} }
      if (!Ahit_minv) {Amin_v+=accum_V[i]; if (Amin_v>At_uv2){Ahit_minv=true; Amin_v=i;} }

      if (!Ahit_maxy) {Amax_y+=accum_Y[255-i]; if (Amax_y>At_y2){Ahit_maxy=true; Amax_y=255-i;} }
      if (!Ahit_maxu) {Amax_u+=accum_U[255-i]; if (Amax_u>At_uv2){Ahit_maxu=true; Amax_u=255-i;} }
      if (!Ahit_maxv) {Amax_v+=accum_V[255-i]; if (Amax_v>At_uv2){Ahit_maxv=true; Amax_v=255-i;} }
    }

    float Favg_y=avg_y/(float)pixels;
    float Favg_u=(avg_u*(float)uvdiv)/(float)pixels;
    float Favg_v=(avg_v*(float)uvdiv)/(float)pixels;
    if (analyze) {
      char text[400];
      sprintf(text,
      "        Frame: %-8u ( Luma Y / ChromaU / ChromaV )\n"
      "      Average:      ( %7.2f / %7.2f / %7.2f )\n"
      "      Minimum:      (   %3d   /   %3d   /   %3d    )\n"
      "      Maximum:      (   %3d   /   %3d   /   %3d    )\n"
      "Loose Minimum:      (   %3d   /   %3d   /   %3d    )\n"
      "Loose Maximum:      (   %3d   /   %3d   /   %3d    )\n"
      ,
      frame,
      Favg_y,Favg_u,Favg_v,
      min_y,min_u,min_v,
      max_y,max_u,max_v,
      Amin_y,Amin_u,Amin_v,
      Amax_y,Amax_u,Amax_v
      );

      env->ApplyMessage(&src, vi, text, vi.width/4, 0xa0a0a0, 0, 0);
      if (!(autowhite||autogain)) {
        return src;
      }
    }
    if (autowhite) {
      u_bright=127-(int)Favg_u;
      v_bright=127-(int)Favg_v;
    }
    if (autogain) {
      Amax_y=min(Amax_y,236);
      Amin_y=max(Amin_y,16);  // Never scale above luma range!
      if (Amin_y!=Amax_y) {
        int y_range = Amax_y-Amin_y;
        double scale = (220.0 / y_range);
        y_gain = (int) (256.0 * scale)-256;
        y_bright = -(int)(scale * (double)(Amin_y)-16);
      }
    }
    updateLUT = true;
  }

  if (conditional) {
    if ( ReadConditionals(env) ) updateLUT = true;
  }

  if (updateLUT) {
    MakeGammaLUT();
  }

  if (vi.IsYUY2()) {
	  for (j = 0; j < h; j++)
	  {
		  for (i=0; i<wby4; i++)
		  {
			  pixel.data = *srcp;

			  pixel.yuv.y0 = LUT_Y[pixel.yuv.y0];
			  pixel.yuv.u  = LUT_U[pixel.yuv.u ];
			  pixel.yuv.y1 = LUT_Y[pixel.yuv.y1];
			  pixel.yuv.v  = LUT_V[pixel.yuv.v ];
			  *srcp++ = pixel.data;
		  }
		  srcp = (unsigned long *)((unsigned char *)srcp + modulo) ;
	  }
  } else if (vi.IsPlanar()) {
	  BYTE* srcp2 = (BYTE*) src->GetWritePtr();
    for (j = 0; j < h; j++) {
		  for (i=0; i<w; i++) {
        srcp2[i]=LUT_Y[srcp2[i]];
      }
	    srcp2 +=  pitch;
    }
	  srcp2 = (BYTE*) src->GetWritePtr(PLANAR_U);
    h=src->GetHeight(PLANAR_U);
    w=src->GetRowSize(PLANAR_U);
    pitch=src->GetPitch(PLANAR_U);
    for (j = 0; j < h; j++) {
		  for (i=0; i<w; i++) {
        srcp2[i]=LUT_U[srcp2[i]];
      }
	    srcp2 +=  pitch;
    }
	  srcp2 = (BYTE*) src->GetWritePtr(PLANAR_V);
    for (j = 0; j < h; j++) {
		  for (i=0; i<w; i++) {
        srcp2[i]=LUT_V[srcp2[i]];
      }
	    srcp2 +=  pitch;
    }
  }

	return src;
}

void Color::MakeGammaLUT(void)
{
	static const int scale = 256, shift = 2^10,
		coeff_y0 =  76309, coeff_y1 =  65536,
		coeff_u0 = 132201, coeff_u1 = 116129,
		coeff_v0 = 104597, coeff_v1 =  91881;
	int i;
	int val;
	double g,b,c,gain;
	double v;

	y_thresh1 = u_thresh1 = v_thresh1 = -1;
	y_thresh2 = u_thresh2 = v_thresh2 = 256;

	gain = ((double)y_gain + scale) / scale;
	c = ((double)y_contrast + scale) / scale;
	b = ((double)y_bright + scale) / scale;
	g = ((double)y_gamma + scale) / scale;
	if (g < 0.01)    g = 0.01;
	for (i = 0; i < 256; i++)
    {
		val = i * shift;
		switch (levels) {
			case 1:	// PC->TV
				val = (int)((val - 16 * shift) * coeff_y0 / coeff_y1 + shift / 2);
				break;
			case 2:	// TV->PC
			case 3:	// TV->PC.Y
				val = (int)(val * coeff_y1 / coeff_y0 + 16 * shift + shift / 2);
				break;
			default:	//none
				break;
		}
		val = val / shift;

		v = ((double)val) / 256;
		v = (v * gain) + ((v-0.5) * c + 0.5) - v + (b - 1);

		if (y_gamma != 0 && v > 0)
			v = pow( v, 1 / g);
		v = v * 256;
		
		v += 0.5;
		val = (int)floor(v);

		if (val > 255)
			val = 255;
		else if (val < 0)
			val = 0;

		if (val > 235) {
			if(y_thresh2 > 255)		y_thresh2 = i;
			if(opt)		val = 235;
		}
		else if (val < 16) {
			y_thresh1 = i;
			if(opt)		val = 16;
		}
		LUT_Y[i] = (unsigned char)val;
    }

	gain = ((double)u_gain + scale);
	c = ((double)u_contrast + scale);
	b = ((double)u_bright);
	for (i = 0; i < 256; i++)
    {
		val = i * shift;
		switch (levels) {
			case 1:	// PC->TV Scale
				val = (int)((val - 128 * shift) * coeff_u0 / coeff_u1 + 128 * shift + shift / 2);
				break;
			case 2:	// TV->PC Scale
				val = (int)((val - 128 * shift) * coeff_u1 / coeff_u0 + 128 * shift + shift / 2);
				break;
			default:	//none
				break;
		}
		val = val / shift;

		v = ((double)val);
		v = (v * gain / scale) + ((v-128) * c / scale + 128) - v + b;

		v += 0.5;
		val = (int)floor(v);
		
		if (val > 255)
			val = 255;
		else if (val < 0)
			val = 0;

		if (val > 240) {
			if(u_thresh2 > 255)		u_thresh2 = i;
			if(opt)		val = 240;
		}
		else if (val < 16) {
			u_thresh1 = i;
			if(opt)		val = 16;
		}
		LUT_U[i] = (unsigned char)val;
    }

	gain = ((double)v_gain + scale);
	c = ((double)v_contrast + scale);
	b = ((double)v_bright);
	for (i = 0; i < 256; i++)
    {
		val = i * shift;
		switch (levels) {
			case 1:	// PC->TV Scale
				val = (int)((val - 128 * shift) * coeff_v0 / coeff_v1 + 128 * shift + shift / 2);
				break;
			case 2:	// TV->PC Scale
				val = (int)((val - 128 * shift) * coeff_v1 / coeff_v0 + 128 * shift + shift / 2);
				break;
			default:	//none
				break;
		}
		val = val / shift;

		v = ((double)val);
		v = (v * gain / scale) + ((v-128) * c / scale + 128) - v + b;

		v += 0.5;
		val = (int)floor(v);
		
		if (val > 255)
			val = 255;
		else if (val < 0)
			val = 0;
		
		if (val > 240) {
			if(v_thresh2 > 255)		v_thresh2 = i;
			if(opt)		val = 240;
		}
		else if (val < 16) {
			v_thresh1 = i;
			if(opt)		val = 16;
		}
		LUT_V[i] = (unsigned char)val;
    }

#ifdef _DEBUG
	DumpLUT();
#endif

}

#define READ_CONDITIONAL(x,y) \
try {\
  AVSValue cv = env->GetVar("coloryuv_" x);\
  if (cv.IsFloat()) {\
    const double t = cv.AsFloat();\
    if (y != t) {\
      changed = true;\
      y = t;\
    }\
  }\
} catch (IScriptEnvironment::NotFound) {}

bool Color::ReadConditionals(IScriptEnvironment* env) {
	bool changed = false;

  READ_CONDITIONAL("gain_y", y_gain);
  READ_CONDITIONAL("gain_u", u_gain);
  READ_CONDITIONAL("gain_v", v_gain);
  READ_CONDITIONAL("off_y", y_bright);
  READ_CONDITIONAL("off_u", u_bright);
  READ_CONDITIONAL("off_v", v_bright);
  READ_CONDITIONAL("gamma_y", y_gamma);
//READ_CONDITIONAL("gamma_u", u_gamma);
//READ_CONDITIONAL("gamma_v", v_gamma);
  READ_CONDITIONAL("cont_y", y_contrast);
  READ_CONDITIONAL("cont_u", u_contrast);
  READ_CONDITIONAL("cont_v", v_contrast);

  return changed;
}

#undef READ_CONDITIONAL



bool Color::CheckParms(const char *_levels, const char *_matrix, const char *_opt)
{
	int i;
	static const char * const LevelsTbl[] = { "", "TV->PC", "PC->TV", "PC->TV.Y" };
	static const char * const MatrixTbl[] = { "", "rec.709" };
	static const char * const OptTbl[]    = { "", "coring" };

	levels = -1;
	if (_levels) {
		for (i=0; i<4 ; i++) {
			if (!lstrcmpi(_levels, LevelsTbl[i])) 
			{
				levels = i;
				break;
			}
		}
	} else {		
		levels = 0;
	}

	matrix = -1;
	if (_matrix) {
		for (i=0; i<2 ; i++) {
			if (!lstrcmpi(_matrix, MatrixTbl[i])) 
			{
				matrix = i;
				break;
			}
		}
	} else {		
		matrix = 0;
	}

	opt = -1;
	if (_opt) {
		for (i=0; i<2 ; i++) {
			if (!lstrcmpi(_opt, OptTbl[i])) 
			{
				opt = i;
				break;
			}
		}
	} else {		
		opt = 0;
	}

	if ( levels < 0 || matrix < 0 || opt < 0 )	return FALSE;
	return TRUE;
}

#ifdef _DEBUG
void Color::DumpLUT(void)
{
/*
  static const char *TitleTbl[] = {
		"Color Adjust Look-up Table : Y, luminance\n",
		"Color Adjust Look-up Table : U, Cb, Color Difference(blue)\n",
		"Color Adjust Look-up Table : V, Cr, Color Difference(red)\n"
	};
	static const BYTE *LUT[] = { (BYTE *)&LUT_Y, (BYTE *)&LUT_U, (BYTE *)&LUT_V };
	int	index,i,j;
	char buf[512];

	for(index=0; index<3; index++)
	{
		OutputDebugString( TitleTbl[index] );
		for(i=0; i<16;i++)
		{
			sprintf(buf,"%03d(%02X) : ", i * 16, i * 16);
			for(j=0;j<8;j++)
			{
				sprintf(&buf[j*4+10], "%03d ",LUT[index][i*16+j]);
			}
			sprintf(&buf[8*4+10], " - ");
			for(j=8;j<16;j++)
			{
				sprintf(&buf[j*4+13], "%03d ",LUT[index][i*16+j]);
			}
			OutputDebugString(buf);
		}
		OutputDebugString("\n");
	}
*/
}
#endif

AVSValue __cdecl Color::Create(AVSValue args, void* user_data, IScriptEnvironment* env) {
    try {	// HIDE DAMN SEH COMPILER BUG!!!
		return new Color(args[0].AsClip(),
						args[ 1].AsFloat(0.0f),		// gain_y
						args[ 2].AsFloat(0.0f),		// off_y      bright
						args[ 3].AsFloat(0.0f),		// gamma_y
						args[ 4].AsFloat(0.0f),		// cont_y
						args[ 5].AsFloat(0.0f),		// gain_u
						args[ 6].AsFloat(0.0f),		// off_u      bright
						args[ 7].AsFloat(0.0f),		// gamma_u
						args[ 8].AsFloat(0.0f),		// cont_u
						args[ 9].AsFloat(0.0f),		// gain_v
						args[10].AsFloat(0.0f),		// off_v
						args[11].AsFloat(0.0f),		// gamma_v
						args[12].AsFloat(0.0f),		// cont_v
						args[13].AsString(""),		// levels = "", "TV->PC", "PC->TV"
						args[14].AsString(""),		// opt = "", "coring"
						args[15].AsString(""),		// matrix = "", "rec.709"
						args[16].AsBool(false),		// colorbars
						args[17].AsBool(false),		// analyze
						args[18].AsBool(false),		// autowhite
						args[19].AsBool(false),		// autogain
						args[20].AsBool(false),		// conditional
						env);
	}
	catch (...) { throw; }
}

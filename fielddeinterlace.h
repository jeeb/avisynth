// Avisynth filter: YUV merge
// by Donald Graft (neuron2@interact.dk)
// adapted by Richard Berg (avisynth-dev@richardberg.net)
//
// Released under the GNU Public License
// See http://www.gnu.org/copyleft/gpl.html for details

#ifndef __FieldDeinterlace_H__
#define __FieldDeinterlace_H__

#include "internal.h"
#include "decomb.h"


/****************************************************
****************************************************/


class FieldDeinterlace: public GenericVideoFilter
{
public:
	FieldDeinterlace( PClip _child, bool _full, int _threshold, int _dthreshold, bool _blend, 
                    bool _chroma, bool _debug, IScriptEnvironment* env ); 

	PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);

  static AVSValue __cdecl Create(AVSValue args, void* user_data, IScriptEnvironment* env);

private:
	bool MotionMask_YUY2(int frame, IScriptEnvironment* env);
  
  bool full, debug, blend, chroma;
	int threshold, dthreshold;
	PVideoFrame src, fmask, dmask, final;
	const BYTE *srcp, *srcp_saved, *p, *n;
	BYTE *fmaskp, *fmaskp_saved, *dmaskp, *dmaskp_saved, *finalp;
	int pitch, dpitch, w, h;
};



#endif  // __FieldDeinterlace_H__
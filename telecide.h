// Avisynth filter: Telecide
// by Donald Graft (neuron2@interact.dk)
// adapted by Richard Berg (avisynth-dev@richardberg.net)
//
// Released under the GNU Public License
// See http://www.gnu.org/copyleft/gpl.html for details

#ifndef __Telecide_H__
#define __Telecide_H__

#include "internal.h"
#include "decomb.h"

/****************************************************
****************************************************/


class Telecide : public GenericVideoFilter
{
public:
  Telecide( PClip _child, bool _reverse, bool _swap, bool _firstlast, int _guide,
	          int _gthresh, bool _postprocess, int _threshold, int _dthreshold, bool _blend,
	          bool _chroma, int _y0, int _y1, bool _debug, IScriptEnvironment* env );
  PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);

  static AVSValue __cdecl Create(AVSValue args, void* user_data, IScriptEnvironment* env);  

private:
  bool Analyse(int frame, IScriptEnvironment* env);

  bool reverse, swap, firstlast, postprocess, blend, chroma, debug;
	int gthresh, threshold, dthreshold, y0, y1, guide;
	int pitch, pitchtimes4, dpitch, w, h;

	// Used by field matching.
	PVideoFrame fp, fc, fn, dst;
	BYTE *fprp, *fcrp, *fcrp_saved, *fnrp;
  BYTE *dstp, *dstp_saved;
	struct
	{
		int frame;
		int choice;
	} store[5];
	int predicted;

	// Used by postprocessing.
	PVideoFrame fmask, dmask, final;
	const BYTE *p, *n;
	BYTE *fmaskp, *fmaskp_saved, *dmaskp, *dmaskp_saved, *finalp;	
};



/**** ASM Routines ****/

void asm_deinterlace( const BYTE *dstp, const BYTE *p, const BYTE *n, 
                      BYTE *fmask, BYTE *dmask, int thresh, int dthresh, 
                      int row_size );

void asm_deinterlace_chroma( const BYTE *dstp, const BYTE *p,
								             const BYTE *n, BYTE *fmask,
                             BYTE *dmask, int thresh, int dthresh, 
                             int row_size );

int asm_match( const BYTE *curr, const BYTE *pprev,
							 const BYTE *pnext, int row_size );

int asm_blend( const BYTE *dstp, const BYTE *cprev,
							 const BYTE *cnext, BYTE *finalp,
								                 BYTE *dmaskp, int count );

int asm_interpolate( const BYTE *dstp, const BYTE *cprev,
								     const BYTE *cnext, BYTE *finalp,
								     BYTE *dmaskp, int count );




#endif  // __Telecide_H__
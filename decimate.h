// Avisynth filter: Decimate
// by Donald Graft (neuron2@interact.dk)
// adapted by Richard Berg (avisynth-dev@richardberg.net)
//
// Released under the GNU Public License
// See http://www.gnu.org/copyleft/gpl.html for details

#ifndef __Decimate_H__
#define __Decimate_H__

#include "internal.h"
#include "decomb.h"

#define MAX_CYCLE_SIZE 25


/****************************************************
****************************************************/



class Decimate : public GenericVideoFilter 
/**
  * Decimate 1-in-N implementation
 **/
{
public:
  Decimate( PClip _child, int _cycle, int _mode, int _threshold, bool _debug,
            IScriptEnvironment* env );
  
  PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);  
  void __stdcall FindDuplicate(int frame, int *chosen, int *metric, IScriptEnvironment* env);

  static AVSValue __cdecl Create(AVSValue args, void* user_data, IScriptEnvironment* env);

private:
	int num_frames_hi, cycle, mode, threshold;
	bool debug;
};



/**** ASM Routines ****/

int asm_compare( const BYTE *curr, const BYTE *prev, int row_size );




#endif // __Decimate_H__
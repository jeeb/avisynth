// Avisynth filter: YUV merge
// by Klaus Post (kp@interact.dk)
// adapted by Richard Berg (avisynth-dev@richardberg.net)
//
// Released under the GNU Public License
// See http://www.gnu.org/copyleft/gpl.html for details

#ifndef __Merge_H__
#define __Merge_H__

#include "internal.h"


/****************************************************
****************************************************/


class MergeChroma : public GenericVideoFilter
/**
  * Merge the chroma planes of one clip into another, preserving luma
 **/
{
public:
  MergeChroma(PClip _child, PClip _clip, float _weight, IScriptEnvironment* env);  
  PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);

  static AVSValue __cdecl Create(AVSValue args, void* user_data, IScriptEnvironment* env);

private:
  PClip clip;
  float weight;
};


class MergeLuma : public GenericVideoFilter
/**
  * Merge the luma plane of one clip into another, preserving chroma
 **/
{
public:
  MergeLuma(PClip _child, PClip _clip, float _weight, IScriptEnvironment* env);  
  PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);

  static AVSValue __cdecl Create(AVSValue args, void* user_data, IScriptEnvironment* env);

private:
  PClip clip;
  float weight;
};




/**** MMX Routines ****/

void mmx_merge_luma( unsigned int *src, unsigned int *luma, int pitch, 
                     int luma_pitch, int width, int height );
                    
void isse_weigh_luma( unsigned int *src, unsigned int *luma, int pitch, 
                     int luma_pitch, int width, int height, int weight, int invweight );

void isse_weigh_chroma( unsigned int *src,unsigned int *chroma, int pitch, 
                       int chroma_pitch, int width, int height, int weight, int invweight );

void weigh_luma( unsigned int *src, unsigned int *luma, int pitch,
                 int luma_pitch, int width, int height, int weight, int invweight);

void weigh_chroma( unsigned int *src, unsigned int *chroma, int pitch,
                   int chroma_pitch, int width, int height, int weight, int invweight);

#endif  // __Merge_H__
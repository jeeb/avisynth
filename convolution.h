// Avisynth filter: general convolution 3d
// by Richard Berg (avisynth-dev@richardberg.net)
// adapted from General Convolution 3D for VDub by Gunnar Thalin (guth@home.se)
//
// Released under the GNU Public License
// See http://www.gnu.org/copyleft/gpl.html for details

#ifndef __GeneralConvolution_H__
#define __GeneralConvolution_H__

#include "internal.h"


/*****************************************
****** General Convolution 2D filter *****
*****************************************/


class GeneralConvolution : public GenericVideoFilter 
/** This class exposes a video filter that applies general convolutions -- up to a 5x5
  * kernel -- to a clip.  Smaller (3x3) kernels have their own code path.  SIMD support forthcoming.
 **/
{
public:
    GeneralConvolution(PClip _child, int _nBias, const char * _matrix, IScriptEnvironment* _env);
    virtual ~GeneralConvolution(void);
    PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);
    static AVSValue __cdecl Create(AVSValue args, void* user_data, IScriptEnvironment* env);

protected:
    void setMatrix(const char * _matrix, IScriptEnvironment* env);
    void initBuffers(IScriptEnvironment* env);

private:                     
    int nBias;
    unsigned int nSize;
        
    // some buffers
    BYTE *pbyA, *pbyR, *pbyG, *pbyB;
    
           
    // Messy way of storing matrix, but avoids performance penalties of indirection    
    int i00;
    int i10;
    int i20;
    int i30;
    int i40;
    int i01;
    int i11;
    int i21;
    int i31;
    int i41;
    int i02;
    int i12;
    int i22;
    int i32;
    int i42;
    int i03;
    int i13;
    int i23;
    int i33;
    int i43;
    int i04;
    int i14;
    int i24;
    int i34;
    int i44;
};



#endif  // __GeneralConvolution_H__
// Avisynth filter: YUV merge
// by Klaus Post (kp@interact.dk)
// adapted by Richard Berg (avisynth-dev@richardberg.net)
//
// Released under the GNU Public License
// See http://www.gnu.org/copyleft/gpl.html for details


#include "decomb.h"
#include "telecide.h"
#include "fielddeinterlace.h"
#include "decimate.h"


/********************************************************************
***** Declare index of new filters for Avisynth's filter engine *****
********************************************************************/

AVSFunction Decomb_filters[] = {
  { "Telecide",
		"c[reverse]b[swap]b[firstlast]b[guide]i[gthresh]i[post]b[threshold]i[dthreshold]i[blend]b[chroma]b[y0]i[y1]i[debug]b",
    Telecide::Create },  // see docs!
  { "FieldDeinterlace", 
    "c[full]b[threshold]i[dthreshold]i[blend]b[chroma]b[debug]b",
    FieldDeinterlace::Create },  
  { "Decimate", "c[cycle]i[mode]i[threshold]i[debug]b", Decimate::Create },
  { 0 }
};




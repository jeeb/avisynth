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

#ifndef __Convert_helper_H__
#define __Convert_helper_H__

#include <avisynth.h>
#include <string>
#include <cstring>

typedef enum ColorRange_e {
  AVS_RANGE_FULL = 0,
  AVS_RANGE_LIMITED = 1
} ColorRange_e;

typedef enum ChromaLocation_e {
  AVS_CHROMA_LEFT = 0,
  AVS_CHROMA_CENTER = 1,
  AVS_CHROMA_TOP_LEFT = 2,
  AVS_CHROMA_TOP = 3,
  AVS_CHROMA_BOTTOM_LEFT = 4,
  AVS_CHROMA_BOTTOM = 5,
  AVS_CHROMA_DV = 6 // Special to Avisynth
} ChromaLocation_e;

typedef enum FieldBased_e {
  AVS_FIELD_PROGRESSIVE = 0,
  AVS_FIELD_BOTTOM = 1,
  AVS_FIELD_TOP = 2
} FieldBased_e;

// https://www.itu.int/rec/T-REC-H.265-202108-I
/* ITU-T H.265 Table E.5 */
typedef enum Matrix_e {
  AVS_MATRIX_RGB = 0, /* The identity matrix. Typically used for RGB, may also be used for XYZ */
  AVS_MATRIX_BT709 = 1, /* ITU-R Rec. BT.709-5 */
  AVS_MATRIX_UNSPECIFIED = 2, /* Image characteristics are unknown or are determined by the application */
  AVS_MATRIX_BT470_M = 4, // instead of AVS_MATRIX_FCC
  // FCC Title 47 Code of Federal Regulations (2003) 73.682 (a) (20)
  // Rec. ITU-R BT.470-6 System M (historical)
  AVS_MATRIX_BT470_BG = 5, /* Equivalent to 6. */
  // ITU-R Rec. BT.470-6 System B, G (historical)
  // Rec. ITU-R BT.601-7 625
  // Rec. ITU-R BT.1358-0 625 (historical)
  // Rec. ITU-R BT.1700-0 625 PAL and 625 SECAM
  AVS_MATRIX_ST170_M = 6,  /* Equivalent to 5. */
  // Rec. ITU-R BT.601-7 525
  // Rec. ITU-R BT.1358-1 525 or 625 (historical)
  // Rec. ITU-R BT.1700-0 NTSC
  // SMPTE ST 170 (2004)
  // SMPTE 170M (2004)
  AVS_MATRIX_ST240_M = 7, // SMPTE ST 240 (1999, historical)
  AVS_MATRIX_YCGCO = 8,
  AVS_MATRIX_BT2020_NCL = 9, 
  // Rec. ITU-R BT.2020 non-constant luminance system
  // Rec. ITU-R BT.2100-2 Y'CbCr
  AVS_MATRIX_BT2020_CL = 10, /* Rec. ITU-R BT.2020 constant luminance system */
  AVS_MATRIX_CHROMATICITY_DERIVED_NCL = 12, /* Chromaticity derived non-constant luminance system */
  AVS_MATRIX_CHROMATICITY_DERIVED_CL = 13, /* Chromaticity derived constant luminance system */
  AVS_MATRIX_ICTCP = 14, // REC_2100_ICTCP, Rec. ITU-R BT.2100-2 ICTCP
  AVS_MATRIX_AVERAGE = 9999, // Avisynth compatibility
} Matrix_e;

// Pre-Avisynth 3.7.1 matrix constants, with implicite PC/limited range
typedef enum Old_Avs_Matrix_e {
  AVS_OLD_MATRIX_Rec601 = 0, 
  AVS_OLD_MATRIX_Rec709 = 1,
  AVS_OLD_MATRIX_PC_601 = 2,
  AVS_OLD_MATRIX_PC_709 = 3,
  AVS_OLD_MATRIX_AVERAGE = 4,
  AVS_OLD_MATRIX_Rec2020 = 5,
  AVS_OLD_MATRIX_PC_2020 = 6
} Old_Avs_Matrix_e;

// transfer characteristics ITU-T H.265 Table E.4
typedef enum Transfer_e {
  AVS_TRANSFER_BT709 = 1,
  AVS_TRANSFER_UNSPECIFIED = 2,
  AVS_TRANSFER_BT470_M = 4,
  AVS_TRANSFER_BT470_BG = 5,
  AVS_TRANSFER_BT601 = 6,  /* Equivalent to 1. */
  AVS_TRANSFER_ST240_M = 7,
  AVS_TRANSFER_LINEAR = 8,
  AVS_TRANSFER_LOG_100 = 9,
  AVS_TRANSFER_LOG_316 = 10,
  AVS_TRANSFER_IEC_61966_2_4 = 11,
  AVS_TRANSFER_IEC_61966_2_1 = 13,
  AVS_TRANSFER_BT2020_10 = 14, /* Equivalent to 1. */
  AVS_TRANSFER_BT2020_12 = 15, /* Equivalent to 1. */
  AVS_TRANSFER_ST2084 = 16,
  AVS_TRANSFER_ARIB_B67 = 18
} Transfer_e;

// color primaries ITU-T H.265 Table E.3
typedef enum Primaries_e {
  AVS_PRIMARIES_BT709 = 1,
  AVS_PRIMARIES_UNSPECIFIED = 2,
  AVS_PRIMARIES_BT470_M = 4,
  AVS_PRIMARIES_BT470_BG = 5,
  AVS_PRIMARIES_ST170_M = 6,
  AVS_PRIMARIES_ST240_M = 7,  /* Equivalent to 6. */
  AVS_PRIMARIES_FILM = 8,
  AVS_PRIMARIES_BT2020 = 9,
  AVS_PRIMARIES_ST428 = 10,
  AVS_PRIMARIES_ST431_2 = 11,
  AVS_PRIMARIES_ST432_1 = 12,
  AVS_PRIMARIES_EBU3213_E = 22
} Primaries_e;

void matrix_parse_merge_with_props(VideoInfo &vi, const char* matrix_name, const AVSMap* props, int& _Matrix, int& _ColorRange, IScriptEnvironment* env);
void matrix_parse_merge_with_props_def(VideoInfo& vi, const char* matrix_name, const AVSMap* props, int& _Matrix, int& _ColorRange, int _Matrix_Default, int _ColorRange_Default, IScriptEnvironment* env);
void chromaloc_parse_merge_with_props(VideoInfo& vi, const char* chromaloc_name, const AVSMap* props, int& _ChromaLocation, int _ChromaLocation_Default, IScriptEnvironment* env);

void update_Matrix_and_ColorRange(AVSMap* props, int theMatrix, int theColorRange, IScriptEnvironment* env);
void update_ColorRange(AVSMap* props, int theColorRange, IScriptEnvironment* env);
void update_ChromaLocation(AVSMap* props, int theChromaLocation, IScriptEnvironment* env);

#endif  // __Convert_helper_H__

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

#include "resample_functions.h"


/*******************************************
   ***************************************
   **  Helper classes for resample.cpp  **
   ***************************************
 *******************************************/

/***************************
 ***** Point filter *****
 **************************/

double PointFilter::f(double x) {
  return 1.0;
}


/***************************
 ***** Triangle filter *****
 **************************/

double TriangleFilter::f(double x) {
  x = fabs(x);
  return (x<1.0) ? 1.0-x : 0.0;
}





/*********************************
 *** Mitchell-Netravali filter ***
 *********************************/

MitchellNetravaliFilter::MitchellNetravaliFilter (double b=1./3., double c=1./3.) {
  p0 = (   6. -  2.*b            ) / 6.;
  p2 = ( -18. + 12.*b +  6.*c    ) / 6.;
  p3 = (  12. -  9.*b -  6.*c    ) / 6.;
  q0 = (            8.*b + 24.*c ) / 6.;
  q1 = (         - 12.*b - 48.*c ) / 6.;
  q2 = (            6.*b + 30.*c ) / 6.;
  q3 = (      -     b -  6.*c    ) / 6.;
}

double MitchellNetravaliFilter::f (double x) {
  x = fabs(x);
  return (x<1) ? (p0+x*x*(p2+x*p3)) : (x<2) ? (q0+x*(q1+x*(q2+x*q3))) : 0.0;
}


/***********************
 *** Lanczos3 filter ***
 ***********************/
LanczosFilter::LanczosFilter(int t = 3) {
   taps = (double)(max( 1,min(100,t)));
}

double LanczosFilter::sinc(double value) {
  if (value != 0.0) {
    value *= M_PI;
    return sin(value) / value;
  } else {
    return 1.0;
  }
}

double LanczosFilter::f(double value) {
   value = fabs(value);

  if (value < taps) {
    return (sinc(value) * sinc(value / taps));
  } else {
    return 0.0;
  }
}


/***********************
 *** Blackman filter ***
 ***********************/
BlackmanFilter::BlackmanFilter(int t = 4) {
   taps = (double)(max( 1,min(100,t)));
   rtaps = 1.0/taps;
}

double BlackmanFilter::f(double value) {
   value = fabs(value);

  if (value < taps) {
    if (value == 0.0) {
      return 1.0;
    } else {
      value *= M_PI;
      return (sin(value) / value) * (0.42 + 0.5*cos(value*rtaps) + 0.08*cos(2*value*rtaps));
    }
  } else {
    return 0.0;
  }
}


/***********************
 *** Spline16 filter ***
 ***********************/

double Spline16Filter::f(double value) {
  value = fabs(value);

  if (value < 1.0) {
    return ( ( value - 9.0/5.0 ) * value - 1.0/5.0 ) * value + 1.0;
  } else if (value < 2.0) {
    return ( ( -1.0/3.0 * (value-1.0) + 4.0/5.0 ) * (value-1.0) - 7.0/15.0 ) * (value-1.0);
  }
  return 0.0;
}

/***********************
 *** Spline36 filter ***
 ***********************/

double Spline36Filter::f(double value) {
  value = fabs(value);

  if        (value < 1.0) {
    return ( ( 13.0/11.0  * (value    ) - 453.0/ 209.0 ) * (value    ) -   3.0/ 209.0 ) *(value    ) + 1.0;
  } else if (value < 2.0) {
    return ( ( -6.0/11.0  * (value-1.0) + 270.0/ 209.0 ) * (value-1.0) - 156.0/ 209.0 ) *(value-1.0);
  } else if (value < 3.0) {
    return  ( ( 1.0/11.0  * (value-2.0) -  45.0/ 209.0 ) * (value-2.0) +  26.0/ 209.0 ) *(value-2.0);
  }
  return 0.0;
}

/***********************
 *** Spline64 filter ***
 ***********************/

double Spline64Filter::f(double value) {
  value = fabs(value);

  if        (value < 1.0) {
    return (( 49.0/41.0 * (value    ) - 6387.0/2911.0) * (value    ) -    3.0/2911.0) * (value    ) + 1.0;
  } else if (value < 2.0) {
    return ((-24.0/41.0 * (value-1.0) + 4032.0/2911.0) * (value-1.0) - 2328.0/2911.0) * (value-1.0);
  } else if (value < 3.0) {
    return ((  6.0/41.0 * (value-2.0) - 1008.0/2911.0) * (value-2.0) +  582.0/2911.0) * (value-2.0);
  } else if (value < 4.0) {
    return ((- 1.0/41.0 * (value-3.0) +  168.0/2911.0) * (value-3.0) -   97.0/2911.0) * (value-3.0);
  }
  return 0.0;
}

/***********************
 *** Gaussian filter ***
 ***********************/

/* Solve taps from p*value*value < 9 as pow(2.0, -9.0) == 1.0/512.0 i.e 0.5 bit
                     value*value < 9/p       p = param*0.1;
                     value*value < 90/param
                     value*value < 90/{0.1, 22.5, 30.0, 100.0}
                     value*value < {900, 4.0, 3.0, 0.9}
                     value       < {30, 2.0, 1.73, 0.949}         */

GaussianFilter::GaussianFilter(double p = 30.0) {
  param = min(100.0,max(0.1,p));
}

double GaussianFilter::f(double value) {
  value = fabs(value);
	double p = param*0.1;
	return pow(2.0, - p*value*value);
}


/******************************
 **** Resampling Patterns  ****
 *****************************/

int* GetResamplingPatternRGB( int original_width, double subrange_start, double subrange_width,
                              int target_width, ResamplingFunction* func, IScriptEnvironment* env )
/**
  * This function returns a resampling "program" which is interpreted by the 
  * FilteredResize filters.  It handles edge conditions so FilteredResize    
  * doesn't have to.  
 **/
{
  double scale = double(target_width) / subrange_width;
  double filter_step = min(scale, 1.0);
  double filter_support = func->support() / filter_step;
  int fir_filter_size = int(ceil(filter_support*2));
  int* result = (int*) _aligned_malloc((1 + target_width*(1+fir_filter_size)) * 4, 64);

  int* cur = result;
  *cur++ = fir_filter_size;

  double pos_step = subrange_width / target_width;
  // the following translates such that the image center remains fixed
  double pos;

  if (original_width <= filter_support) {
    env->ThrowError("Resize: Source image too small for this resize method. Width=%d, Support=%d", original_width, (int)filter_support);
  }

  if (fir_filter_size == 1) // PointResize
    pos = subrange_start;
  else
    pos = subrange_start + ((subrange_width - target_width) / (target_width*2));

  for (int i=0; i<target_width; ++i) {
    int end_pos = int(pos + filter_support);

    if (end_pos > original_width-1)
      end_pos = original_width-1;

    int start_pos = end_pos - fir_filter_size + 1;

    if (start_pos < 0)
      start_pos = 0;

    *cur++ = start_pos;

    // the following code ensures that the coefficients add to exactly FPScale
    double total = 0.0;

    // Ensure that we have a valid position
    double ok_pos = max(0.0,min(original_width-1,pos));

    for (int j=0; j<fir_filter_size; ++j) {  // Accumulate all coefficients
      total += func->f((start_pos+j - ok_pos) * filter_step);
    }

    if (total == 0.0) {
      // Shouldn't happend for valid positions.
#ifdef _DEBUG
      env->ThrowError("Resizer: [Internal Error] Got Zero Coefficient");
#endif
      total = 1.0;
    }

    double total2 = 0.0;

    for (int k=0; k<fir_filter_size; ++k) {
      double total3 = total2 + func->f((start_pos+k - ok_pos) * filter_step) / total;
      *cur++ = int(total3*FPScale+0.5) - int(total2*FPScale+0.5);
      total2 = total3;
    }

    pos += pos_step;
  }

  return result;
}


int* GetResamplingPatternYUV( int original_width, double subrange_start, double subrange_width,
                              int target_width, ResamplingFunction* func, bool luma, BYTE *temp,
                              IScriptEnvironment* env )
/**
  * Same as with the RGB case, but with special
  * allowances for YUV-MMX code
 **/
{
  double scale = double(target_width) / subrange_width;
  double filter_step = min(scale, 1.0);
  double filter_support = func->support() / filter_step;
  int fir_filter_size = int(ceil(filter_support*2));
  int fir_fs_mmx = (fir_filter_size / 2) +1;  // number of loops in MMX code
  int target_width_a=(target_width+15)&(~15);
  int* result = luma ?
                (int*) _aligned_malloc(2*4 + target_width_a*(1+fir_fs_mmx)*8, 64) :
                (int*) _aligned_malloc(2*4 + target_width_a*(1+fir_filter_size)*8, 64);

  int* cur[2] = { result +2, result +3 };
  *result = luma ? fir_fs_mmx : fir_filter_size;

  double pos_step = subrange_width / target_width;
  // the following translates such that the image center remains fixed
  double pos;

  if (fir_filter_size == 1) // PointResize
    pos = subrange_start;
  else
    pos = subrange_start + ((subrange_width - target_width) / (target_width*2));

  if (original_width <= filter_support) {
    env->ThrowError("Resize: Source image too small for this resize method. Width=%d, Support=%d", original_width, (int)filter_support);
  }

  for (int i=0; i<target_width_a; ++i) {
    int end_pos = int(pos + filter_support);

    if (end_pos > original_width-1)  //This will ensure that the filter will not end beyond the end of the line.
      end_pos = original_width-1;

    int start_pos = end_pos - fir_filter_size + 1;  // Calculate where to start, so we don't end outside the line.

    if (start_pos < 0)  // Did we get too far back?
      start_pos = 0;

    int ii = luma ? i&1 : 0;

    *(cur[ii]) = luma ?   // Write offset of first pixel.
                 (int)(temp + (start_pos & -2) * 2) :
                 (int)(temp + start_pos * 8);

    cur[ii] += 2;

    // the following code ensures that the coefficients add to exactly FPScale
    double total = 0.0;

    // Ensure that we have a valid position
    double ok_pos = max(0.0,min(original_width-1, pos)); 

    for (int j=0; j<fir_filter_size; ++j) {  // Accumulate all coefficients
      total += func->f((start_pos + j - ok_pos) * filter_step);
    }

    if (total == 0.0) {
      // Shouldn't happend for valid positions.
#ifdef _DEBUG
      env->ThrowError("Resizer: [Internal Error] Got Zero Coefficient");
#endif
      total = 1.0;
    }

    double total2 = 0.0;
    int oldCoeff = 0;

    for (int k=0; k<fir_filter_size; ++k) {
      double total3 = total2 + func->f((start_pos+k - ok_pos) * filter_step) / total;
      int coeff = int(total3*FPScale+0.5) - int(total2*FPScale+0.5);
      total2 = total3;

      if (luma) {
        if ((k + start_pos) & 1) {
          *(cur[ii]) = (coeff << 16) + (oldCoeff & 0xFFFF);
          cur[ii] += 2;
        } else
          oldCoeff = coeff;
      } else {
        *(cur[0]) = coeff;
        cur[0] += 1;
        *(cur[0]) = coeff;
        cur[0] += 1;
      }
    }

    if (luma) {
      if ((start_pos + fir_filter_size) & 1) {
        *(cur[ii]) = 0 + (oldCoeff & 0xFFFF);
        cur[ii] += 2;
      } else
        if ((fir_filter_size & 1) == 0) {
          *(cur[ii]) = 0;
          cur[ii] += 2;
        }
    }

    pos += pos_step;
  }

  return result;
}

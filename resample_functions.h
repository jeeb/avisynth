// Avisynth v1.0 beta.  Copyright 2000 Ben Rudiak-Gould.
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

#ifndef __Resample_Functions_H__
#define __Resample_Functions_H__

#include "internal.h"

// Original value: 65536
// 2 bits sacrificed because of 16 bit signed MMX multiplication
const int FPScale = 16384; // fixed point scaler

// 09-14-2002 - Vlad59 - Lanczos3Resize - Constant added
#define M_PI 3.14159265358979323846



/*******************************************
   ***************************************
   **  Helper classes for resample.cpp  **
   ***************************************
 *******************************************/


class ResamplingFunction 
/**
  * Pure virtual base class for resampling functions
  */
{
public:
  virtual double f(double x) = 0;
  virtual double support() = 0;
};

class PointFilter : public ResamplingFunction 
/**
  * Nearest neighbour (point sampler), used in PointResize
 **/
{
public:
  double f(double x);  
  double support() { return 0.5; }  // 0.0 crashes it.
};


class TriangleFilter : public ResamplingFunction 
/**
  * Simple triangle filter, used in BilinearResize
 **/
{
public:
  double f(double x);  
  double support() { return 1.0; }
};


class MitchellNetravaliFilter : public ResamplingFunction 
/**
  * Mitchell-Netraveli filter, used in BicubicResize
 **/
{
public:
  MitchellNetravaliFilter(double b, double c);
  double f(double x);
  double support() { return 2.0; }

private:
  double p0,p2,p3,q0,q1,q2,q3;
};

// 09-14-2002 - Vlad59 - Lanczos3Resize
class Lanczos3Filter : public ResamplingFunction
/**
  * Lanczos3 filter, used in Lanczos3Resize
 **/
{
public:
	double f(double x);
	double support() { return 3.0; };

private:
	double sinc(double value);
};



int* GetResamplingPatternRGB(int original_width, double subrange_start, double subrange_width,
                                    int target_width, ResamplingFunction* func);

int* GetResamplingPatternYUV(int original_width, double subrange_start, double subrange_width,
                                    int target_width, ResamplingFunction* func, bool luma, BYTE *temp);


#endif  // __Reample_Functions_H__
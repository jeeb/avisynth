/*****************************************************************************
 * 
 * Custom variable types used in SoundTouch sound processing library.
 *
 * Author        : Copyright (c) Olli Parviainen
 * Author e-mail : oparviai @ iki.fi
 * File created  : 13-Jan-2002
 *
 * Last changed  : $Date: 2003/12/27 10:00:51 $
 * File revision : $Revision: 1.11 $
 *
 * $Id: STTypes.h,v 1.11 2003/12/27 10:00:51 Olli Exp $
 *
 * License :
 * 
 *  SoundTouch sound processing library
 *  Copyright (c) Olli Parviainen
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *****************************************************************************/

#ifndef STTypes_H
#define STTypes_H

typedef unsigned int    uint;
typedef unsigned long   ulong;

#ifndef _WINDEF_
    // if these aren't defined already by Windows headers, define now

    typedef unsigned int    BOOL;

    #define FALSE   0
    #define TRUE    1

#endif  // _WINDEF_

namespace soundtouch
{
    // Enable one of the following defines to choose either 16bit integer or
    // 32bit float sample type. If you don't have opinion, using integer samples
    // is generally faster.
    //#define INTEGER_SAMPLES       // 16bit integer samples
    #define FLOAT_SAMPLES       // 32bit float samples


    #ifdef INTEGER_SAMPLES

        // 16bit integer sample type
        typedef short SAMPLETYPE;
        // data type for sample accumulation: Use 32bit integer to prevent overflows
        typedef long  LONG_SAMPLETYPE;

        #ifdef FLOAT_SAMPLES
            // check that only one sample type is defined
            #error "conflicting sample types defined"
        #endif // FLOAT_SAMPLES

        #if WIN32 || __i386__
            // Allow MMX optimizations
            #define ALLOW_MMX
        #endif

    #else

        // floating point samples
        typedef float  SAMPLETYPE;
        // data type for sample accumulation: Use double to utilize full precision.
        typedef double LONG_SAMPLETYPE;

        #ifdef WIN32
            // Allow 3DNow! and SSE optimizations
            #define ALLOW_3DNOW
            #define ALLOW_SSE
        #endif // WIN32

    #endif  // INTEGER_SAMPLES
};

#endif

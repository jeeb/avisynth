Avisynth Plus change log
------------------------
Source: https://github.com/pinterf/AviSynthPlus/tree/MT

For a more logical (non-historical) arrangement of changes see readme.txt

20180702 r2728
--------------
- New: Expr: implement 'clip' three operand operator like in masktools2
  Description: clips (clamps) value: x minvalue maxvalue clip -> max(min(x, maxvalue), minvalue)
- New: Expr: Parameter "clamp_float"
    True: clamps 32 bit float to valid ranges, which is 0..1 for Luma or for RGB color space and -0.5..0.5 for YUV chroma UV channels
    Default false, ignored (treated as true) when scale_inputs scales float
- New: Expr: parameter "scale_inputs" (default "none")

    Autoscale any input bit depths to 8-16 bit for internal expression use, the conversion method is either full range or limited YUV range.
    
    Feature is similar to the one in masktools2 v2.2.15

    The primary reason of this feature is the "easy" usage of formerly written expressions optimized for 8 bits.

    Use
    - "int" : scales limited range videos, only integer formats (8-16bits) to 8 (or bit depth specified by 'i8'..'i16')
    - "intf": scales full range videos, only integer formats (8-16bits) to 8 (or bit depth specified by 'i8'..'i16')
    - "float" or "floatf" : only scales 32 bit float format to 8 bit range (or bit depth specified by 'i8'..'i16')
    - "all": scales videos to 8 (or bit depth specified by 'i8'..'i16') - conversion uses limited_range logic (mul/div by two's power)
    - "allf": scales videos to 8 (or bit depth specified by 'i8'..'i16') - conversion uses full scale logic (stretch)
    - "none": no magic

    Usually limited range is for normal YUV videos, full scale is for RGB or known-to-be-fullscale YUV

    By default the internal conversion target is 8 bits, so old expressions written for 8 bit videos will probably work.
    This internal working bit-depth can be overwritten by the i8, i10, i12, i14, i16 specifiers.

    When using autoscale mode, scaleb and scalef keywords are meaningless, because there is nothing to scale.

    How it works:
    - This option scales all 8-32 bit inputs to a common bit depth value, which bit depth is 8 by default and can be 
      set to 10, 12, 14 and 16 bits by the 'i10'..'i16' keywords
      For example: scale_inputs="all" converts any inputs to 8 bit range. No truncation occurs however (no precision loss), 
      because even a 16 bit data is converted to 8 bit in floating point precision, using division by 256.0 (2^16/2^8). 
      So the conversion is _not_ a simple shift-right-8 in the integer domain, which would lose precision.
    - Calculates expression (lut, lut_xy, lut_xyz, lut_xyza)
    - Scales the result back to the original video bit depth.
      Clamping (clipping to valid range) and converting to integer occurs here.

    The predefined constants such as 'range_max', etc. will behave according to the internal working bit depth

    Warning#1 
    This feature was created for easy porting earlier 8-bit-video-only lut expressions.
    You have to understand how it works internally.

    Let's see a 16bit input in "all" and "allf" mode (target is the default 8 bits)

    Limited range 16->8 bits conversion has a factor of 1/256.0 (Instead of shift right 8 in integer domain, float-division is used or else it would lose presision)

    Full range 16->8 bits conversion has a factor of 255.0/65535

    Using bit shifts (really it's division and multiplication by 2^8=256.0): 
      result = calculate_lut_value(input / 256.0) * 256.0
    Full scale 16-8-16 bit mode ('intf', 'allf')
      result = calculate_lut_value(input / 65535.0 * 255.0 ) / 255.0 * 65535.0

    Use scale_inputs = "all" ("int", "float") for YUV videos with 'limited' range e.g. in 8 bits: Y=16..235, UV=16..240).
    Use scale_inputs = "allf" (intf, floatf) for RGB or YUV videos with 'full' range e.g. in 8 bits: channels 0..255.

    When input is 32bit float, the 0..1.0 (luma) and -0.5..0.5 (chroma) channel is scaled
    to 0..255 (8 bits), 0..1023 (i10 mode), 0..4095 (i12 mode), 0..16383(i14 mode), 0..65535(i16 mode) then back.

    Warning#2
    One cannot specify different conversion methods for converting before and after the expression.
    Neither can you specify different methods for different input clips (e.g. x is full, y is limited is not supported).

- Fix: Expr: expression string order for planar RGB is properly r-g-b like in original VapourSynth version, instead of counter-intuitive g-b-r.
- Fix: Expr: check subsampling when a different output pixel format is given
- Fix: ColorYUV: round to avoid green cast on consecutive TV<>PC
- Enhanced: Limiter to work with 32 bit float clips
- Enhanced: Limiter new parameter bool 'autoscale' default false.
  If set, minimum/maximum luma/chroma values are treated as they were in 8 bit range (but non-integer values are allowed), limiter will autoscale it.
  Default: does not scale at all, parameters are used as-is. Parameters now are of float type to handle 32 bit float values.
- New: function bool VarExist(String variable_name)
  Checks if a variable exists
  Returns true if variable exists even if it holds 'Undefined', false otherwise
- Fix: RGBAdjust memory leak when used in ScriptClip
- Enhanced: RGBAdjust new parameter: conditional (like in ColorYUV)
  The global variables "rgbadjust_xxx" with xxx = r, g, b, a, rb, gb, bb, ab, rg, gg, bg, ag are read each frame, and applied. 
  It is possible to modify these variables using FrameEvaluate or ConditionalReader.
- Enhanced: RGBAdjust: support 32 bit float ('analyze' not supported, 'dither' silently ignored)
- Enhanced: AviSource to support more formats with 10+ bit depth.
  http://avisynth.nl/index.php/AviSource

  When pixel_type is not specified or set to "FULL", AviSource will try to request the formats one-by-one in the order shown in the table below.

  When a classic 'pixel_type' shares more internal formats (such as YUV422P10 first tries to request the v210 then P210 format)
  you can specify one of the specific format directly. Note that high bit-depth RGBP is prioritized against packed RGB48/64.

  The 'FourCCs for ICDecompressQuery' column means that when a codec supports the format, it will serve the frame in that one, Avisource then will convert it to the proper colorspace.

  Full support list (* = already supported): 
  'pixel_type' Avs+ Format   FourCC(s) for ICDecompressQuery
  YV24         YV24          *YV24
  YV16         YV16          *YV16 
  YV12         YV12          *YV12
  YV411        YV411         *Y41B
  YUY2         YUY2          *YUY2
  RGBP10       RGBP10        G3[0][10]  r210  R10k
  r210         RGBP10        r210
  R10k         RGBP10        R10k             
  RGBP         RGBP10        G3[0][10]  r210  R10k
               RGBP12        G3[0][12]
               RGBP14        G3[0][14]
               RGBP16        G3[0][16]
               RGBAP10       G4[0][10]
               RGBAP12       G4[0][12]
               RGBAP14       G4[0][14]
               RGBAP16       G4[0][16]
  RGB32        RGB32         *BI_RGB internal constant (0) with bitcount=32
  RGB24        RGB24         *BI_RGB internal constant (0) with bitcount=24
  RGB48        RGB48         BGR[48]    b48r 
  RGB64        RGB64         *BRA[64]   b64a
  Y8           Y8            Y800       Y8[32][32]   GREY
  Y            Y8            Y800       Y8[32][32]   GREY
               Y10           Y1[0][10]
               Y12           Y1[0][12]
               Y14           Y1[0][14]
               Y16           Y1[0][16]
  YUV422P10    YUV422P10     v210       P210
  v210         YUV422P10     v210
  P210         YUV422P10     P210
  YUV422P16    YUV422P16     P216
  P216         YUV422P16     P216
  YUV420P10    YUV420P10     P010
  P010         YUV422P10     P010
  YUV420P16    YUV420P16     P016
  P016         YUV422P16     P016
  YUV444P10    YUV444P10     v410
  v410         YUV444P10     v410
- Changed (finally): 32bit float YUV colorspaces: zero centered chroma channels. 
  U and V channels are now -0.5..+0.5 (if converted to full scale before) instead of 0..1
  Note: filters that relied on having the U and V channel center as 0.5 will fail.
  Why: the old UV 0..1 range was a very-very early decision in the high-bitdepth transition project. Also it is now
  compatible with z_XXXXX resizers (zimg image library, external plugin at the moment).
- New function: bool IsFloatUvZeroBased()
  For plugin or script writers who want to be compatible with pre r2672 Avisynth+ float YUV format:
    Check function availablity with FunctionExists("IsFloatUvZeroBased").
    When the function does not exists, the center value of 32 bit float U and V channel is 0.5
    When IsFloatUvZeroBased function exists, it will return true (always for official releases) if U and V is 0 based (+/-0.5)
- Fix: RGB64 Turnleft/Turnright (which are also used in RGB64 Resizers)
- Fix: Rare crash in FrameRegistry
- Enhanced: Allow ConvertToRGB24-32-48-64 functions for any source bit depths
- Enhanced: ConvertBits: allow fulls-fulld combinations when either clip is 32bits
  E.g. after a 8->32 bit fulls=false fulld=true: 
  Y: 16..235 -> 0..1 
  U/V: 16..240 -> -0.5..+0.5
  Note: now ConvertBits does not assume full range for YUV 32 bit float. 
  Default values of fulls and fulld are now true only for RGB colorspaces.
- Fix: couldn't see variables in avsi before plugin autoloads (colors_rgb.avsi issue)
- Fix: LoadVirtualdubPlugin: Fix crash on exit when more than one instances of a filter was used in a script
- New: LoadVirtualdubPlugin update:
  - Update from interface V6 to V20, and Filtermod version 6 (partial)
  - VirtualDub2 support with extended colorspaces
    Allow RGB24, RGB48, RGB64 besides RGB32
    AutoConvert 8 bit Planar RGB to/from RGB24, RGBPA to/from RGB32 (lossless)
    AutoConvert RGB48 and 16 bit Planar RGB(A) to/from RGB64 (lossless)
    Support YUV(A) 8 bits: YV12, YV16, YV24, YV411, YUVA420P8, YUVA422P8, YUVA444P8
    Support YUV(A) 10-16 bits (properly set "ref_x" maximum levels, no autoconvert)
  - Supports prefetchProc2 callback (API >= V14 and prefetchProc2 is defined) for multiple input frames from one input clip
    PrefetchFrameDirect and PrefetchFrame are supported. PrefetchFrameSymbolic not supported
  - Supports prefetchProc callback (API >= V12 and prefetchProc is defined)
  - Supports when filter changes frame count of the output clip
  - Extra filter parameter added at the end of filter's (unnamed) parameter list
    Imported Virtualdub filters are getting and extra named parameter to the end:
      String [rangehint]
    This parameter can tell the filter about a YUV-type clip colorspace info
    Allowed values:
      "rec601": limited range + 601
      "rec709": limited range + 709
      "PC.601": full range + 601
      "PC.709": full range + 709
      ""      : not defined (same as not given)
    Parameter will be ignored when clip is non-YUV
    How it works: the hint will _not_ change the internal VirtualDub colorspace
    constant (e.g. kPixFormat_YUV420_Planar -> kPixFormat_YUV420_Planar_709 will not happen).
    Instead the base color space is kept and colorSpaceMode and colorRangeMode will set in PixmapLayout.formatEx.
    Filter can either use this information or not, depending on supported API version and its implementation. 
    E.g. Crossfade(20,30) -> Crossfade(20,30,"rec601") though this specific filter won't use it.
- New function: BuildPixelType

  Creates a video format (pixel_type) string by giving a colorspace family, bit depth, optional chroma subsampling and/or a 
  template clip, from which the undefined format elements are inherited.
  "[family]s[bits]i[chroma]i[compat]b[oldnames]b[sample_clip]c"

  string family: YUV, YUVA, RGB, RGBA, Y
  int bits: 8, 10, 12, 14, 16, 32
  string chroma: for YUV(A) 420,422,444,411. Ignored for RGB(A) and Y
  bool compat (default false): returns packed rgb formats for 8/16 bits (RGB default: planar RGB)
  bool oldnames (default false): returns YV12/YV16/YV24 instead of YUV420P8/YUV422P8/YUV444P8
  clip sample_clip: when supported, its format is overridden by specified parameters (e.g. only change bits=10)

  Example#1: define YUV 444 P 10
   
    family = "YUV"
    bits = 10
    chroma = 444
    compat = false
    oldformat = false
    s = BuildPixelType(family, bits, chroma, compat, oldformat)
    BlankClip(width=320,height=200,length=len,pixel_type=s,color=$008080).Info()

  Example#2: Change only the bit depth of the format to 16
   
    newbits = 16
    c = last
    s = BuildPixelType(bits=newbits, sample_clip=c)
    BlankClip(width=320,height=200,length=len,pixel_type=s,color=$008080).Info()

- Source: move to c++17, 'if constexpr' requires. Use Visual Studio 2017 (or GCC 7?). CMakeLists.txt changed.
- Source: C api: AVSC_EXPORT to dllexport in capi.h for avisynth_c_plugin_init
- Source: C api: avs_is_same_colorspace VideoInfo parameters to const
- Project struct: changelog to git.
- Include current avisynth header files and def/exp file in installer, when SDK is chosen

20180328 r2664
--------------
-   Fix: YUY2 Sharpen overflow artifacts - e.g. Sharpen(0.6)
-   Fix: Levels: 32 bit float shift in luma
-   Fix: Merge sse2 for 10-14bits (regression)
-   Fix: AVX2 resizer possible access violation in extreme resizes (e.g. 600->20)
-   Fix: 32bit float PlanarRGB<->YUV conversion matrix
-   Fix: VfW: fix b64a output for OPT_Enable_b64a=true
-   Enhanced: VfW output P010 and P016 conversion to SSE2 (VfW output is used by VirtualDub for example)
-   Enhanced: ColorYUV: recalculate 8-16 bit LUT in GetFrame only when changed frame-by-frame (e.g. in autowhite)
-   Enhanced: ConvertBits 32->8 sse2/avx2 and 32->10..16 sse41/avx2 (8-15x speed)

Not included, preliminary for the near future:
-   Big change: 32 bit float YUV formats, U and V are now zero based.
    Internally YUV 32 bit float chroma center became 0.0 (the neutral value which is 128 in the 8-bit world)
    Like in VapourSynth or in avsresizer using z.lib image library.
    'Expr' changes are affecting built-in constants/operators when used in chroma plane of a 32bit clip.
    - 'cmin', 'cmax' return the zero-based shifted versions of the 16 and 240 (8 bit) values
    - For U and V planes, constant 'range_half' results in 0.0 instead of the old 0.5
    - 'scaleb' will also give zero-based result when found in an expression for chroma plane 
      (e.g. for a 32 bit float clip the '128 scaleb' will result in 0.0 instead of 128/255 for U and V planes)
      But 'scalef' when the target or source of the constant conversion is 32bits, remains independent from the plane type.
    - 'range_max' is 0.5 for 32 bit float chroma
    - new constant 'range_min', which is -0.5 for 32 bit float chroma, (0 otherwise)
    
    Additional warning: when you move 32bit float U or V plane to Y using CombinePlane, you have to be sure 
    that your filters do not rely on this new Y plane being in 0..1 range. Or else convert it by using Expr("x 0.5 +") to the 0..1 range
    Similarly: ExtractU and ExtractV will simply return the unaltered chroma planes, which are now zero-centered


20180302 r2636
--------------
-   Fix: Blur/Sharpen crashed when YUY2.width<8, RGB32.width<4, RGB64.width<2
-   ColorYUV: don't apply TV range gamma for opt="coring" when explicit "PC->TV" is given
-   ColorbarsHD: 32bit float properly zero (0.5)-centered chroma

20180301 r2632
--------------
  - Fix: IsInterleaved returned false for RGB48 and RGB64 (raffriff42)
  - Fix: SubTitle for Planar RGB/RGBA: wrong text colors (raffriff42)
  - Fix: Packed->Planar RGB conversion failed on SSE2-only computers (SSSE3 instruction used)
  - Enhanced: Blur, Sharpen
    AVX2 for 8-16 bit planar colorspaces (>1.35x speed on i7-7770)
    SSE2 for 32 bit float formats (>1.5x speed on i7-7770)
  - Fix: Resizers for 32 bit float rare random garbage on right pixels (simd code NaN issue)
  - Enhanced: Completely rewritten 16bit and float resizers, much faster (and not only with AVX2)
  - Enhanced: 8 bit resizers: AVX2 support
  - Enhanced: Speed up converting from RGB24/RGB48 to Planar RGB(A) - SSSE3, approx. doubled fps
  - New: ConvertFPS supports 10-32 bits, planar RGB(A), YUV(A)
  - New script function: int BitSetCount(int[, int, int, ...])
    Function accepts one or more integer parameters
    Returns the number of bits set to 1 in the number or the total number of '1' bits in the supplied integers.
  - Cherry-picking from StainlessS' great RT_xxxx collection/and raffriff42 utils
  - Modded script function: Hex(int , int "width"=0)
    - New "width" parameter
    - result is in uppercase
    Width is 0 to 8, the _minimum_ width of the returned string. (8 hex digit is the max of Avisynth32 bit integer)
    When width is 0 or not supplied then string length is a minimum needed.
    Function now returns hex string in uppercase, instead of lowercase.
    Example: Hex(255,4) returns "00FF".
  - Modded script function: HexValue(String, "pos"=1)
    - new pos parameter
    Returns an int conversion of the supplied hexadecimal string.
    Conversion will cease at the first non legal number base digit, without producing an error
    Added optional pos arg default=1, start position in string of the HexString, 1 denotes the string beginning.
    Will return 0 if error in 'pos' ie if pos is less than 1 or greater than string length.
  - Modded script function: ReplaceStr(String, String, String[, Boolean "sig"=false])
    - New parameter: sig for case insensitive search (Default false: exact search)
      The uppercase/lowercase rules come from the current active code page of the OS.
  - New script functions: TrimLeft, TrimRight, TrimAll for removing beginning/trailing whitespaces from a string.
    Whitespaces: Tab (9), space (32), nbsp (160)
  - New in ColorYUV:
    New parameter: bool f2c="false".
    When f2c=true, the function accepts the Tweak-like parameters for gain, gamma and contrast
    E.g. use 0/0.5/1.0/1.5/2.0/3.0 instead of -256/-128/0/128/256/512 
  - New/Fixed in ColorYUV:
    Parameter "levels" accepts "TV". (can be "TV->PC", "PC->TV", "PC->TV.Y")
    Now gamma calculation is TV-range aware when either
    - levels is "TV->PC" or
    - coring = true or
    - levels is "TV" (new - no level conversion but gamma will know proper handling)
    Previously gamma was properly calculated only for PC range.
  - New in ColorYUV: 
    32 bit float support. 
    - 32 bit float uses the Expr filter (8-16 bits is LUT-based). The expression is dynamically assembled for each plane, internal precision is float.
    - One can specify bits=32 when showyuv=true -> test clip in YUV420PS format
    For 32 bit clips "loose min" and "loose_max" (omitting the extreme 1/256 population from dark and bright pixels) statistics are computed 
    by splitting the 0..1 into 65536 uniform ranges.
 - Modded: remove "scale" parameter from ConvertBits.
   It was introduced at the very beginning of the 10+bit development, for 32bit float conversion - never used
 - Enhanced: VfW: exporting Y416 (YUV444P16) to SSE2.
 - 8-16 bit YUV chroma to 32 bit float: keep middle chroma level (e.g. 128 in 8 bits) at 0.5.
   Calculate chroma as (x-128)/255.0 + 0.5 and not x/255.0 (Note: 32 bit float chroma center will be 0.0 in the future)
 - New: Histogram parameter "keepsource"=true (raffriff42) 
   keepsource = false returns only the Histogram w/o the original picture.
   Affects "classic", "levels" and "color", "color2", ignored (n/a) for the other modes
 - New: Histogram type "color" to accept 8-32bit input and "bits"=8,9,..12 display range
 - New: Histogram parameter "markers"=true
   When markers = false:
   For "classic": no "half" level line and no invalid luma zone coloring
   For "levels":  no "half" dotted line, no coloring (neither for YUV nor for RGB)
   Ignored for the others at the moment.

20171226 r2580
----------------
  - Fix (workaround): Merge: Visual Studio 2017 15.5.1/2 generated invalid AVX2 code
    The x86 path of Merge Average was crashing due to bad code generation in the 8 bit version. Seems that thought 
    Visual Studio Team was proudly announcing optimizations in their compiler (15.5.x line), it had victims.
  - Fix: Temporalsoften 10-14 bits: an SSE 4.1 instruction was used for SSE2-only CPU-s (Illegal Instruction on Athlon XP)

20171219 r2574
----------------
  - Fix: MaskHS created inverse mask. Regression after r2173
  - Build: changed avisynth.h, strict C++ conformity with Visual Studio 2017 /permissive- flag
  - Installer (finally)
  - Expr:
    - new: Indexable source clip pixels by relative x,y positions like x[-1,1] 
    - new functions: sin cos tan asin acos atan
    - new operator: % (modulo)
    - new: Variables: uppercase letters A..Z for storing and reuse temporary results, frequently used computations.
    - new: predefined expr variables 'frameno', 'time', 'width', 'height'
    - fix: jitasm code generation at specific circumstances

20171115 r2544
  - Expr: fix "scalef" for 10-16 bits
  - Expr optimization: eliminate ^1 +0 -0 *1 /1

20171114 r2542
  - New filter: Expr
    Ported from Vapoursynth, with a lot of additional tweaks 
    e.g. AVX2, recognize constant fill, spatial absolute and relative coordinates as input, recognize plane copy, 
         no GetFrame for unused clips, converts ^2, ^3, ^4 into faster multiplication, 
         converts x^0.5 into sqrt(x), borrow specific syntax elements from masktools2 (scaling, operators - but not all of them) to have partial 
         compatibility in widely used existing expression strings (see readme.txt)

    clip Expr(clip c[,clip c2, ...], string expr [, string expr2[, string expr3[, string expr4]]] [, string format]
        [, bool optSSE2][, bool optAVX2][, bool optSingleMode])

  - Add: Levels: 32 bit float format support
  - Fix: RGB (full scale) conversion: 10-16 bits to 8 bits rounding issue; pic got darker in repeated 16<->8 bit conversion chain
  - Fix: ConvertToY: remove unnecessary clamp for Planar RGB 32 bit float
  - Fix: RGB ConvertToY when rec601, rec709 (limited range) matrix. Regression since r2266
  - Optimized: Faster RGB (full scale) 10-16 bits to 8 bits conversion when dithering
  - Other: Default frame alignment is 64 bytes (was: 32 bytes). (independently of AVX512 support)
  - Build: 
    Built with Visual Studio 2017, v141_xp toolset
    Note that DLL will work When you have VS2015 Update 3 redistributable.
  
    Download Visual Studio 2017 Redistributable from here (replaces and compatible with VS2015 redist)

    X64:
    https://go.microsoft.com/fwlink/?LinkId=746572

    x86:
    https://go.microsoft.com/fwlink/?LinkId=746571

  - Experimental x64 builds for test (internal offsets from int to size_t)
  - Source: avisynth_c.h (C interface header file) changed: 
    Optional define SIZETMOD. Experimental. Offsets are size_t instead of int (x64 is different!)
    Fix: avs_get_row_size calls into avs_get_row_size_p, instead of direct field access
    Fix: avs_get_height calls into avs_get_row_size_p, instead of direct field access.

20170629 r2508
  - Fix TemporalSoften: threshold < 255 (probably since r1576)

20170608 r2506
  - Fix CombinePlanes: feeding YV16 or YV411 target with Y8 sources

20170603 r2504
  - fix XP support broken in r2502

20170602 r2502
  - fix: (Important!) MT_SERIALIZED mode did not always protect filters (regression since r2069)
    Such filters sometimes were called in a reentrant way (like being MT_NICE_FILTER), which
    possibly resulted in using their internal buffers parallel.
  - Fix: ImageWriter crash when no '.' in provided filename
  - Fix: Overlay: correct masked blend: keep exact clip1 or clip2 pixel values for mask extremes 255 or 0. 
    Previously 0 became 1 for zero mask, similarly 255 changed into 254 for full transparency (255) mask
  - New: script functions: StrToUtf8, StrFromUtf8: Converting a 8 bit (Ansi) string to UTF8 and back.
  - New: PluginManager always throws error on finding wrong bitness DLL in the autoload directories
  - Modified: increased x64 default MemoryMax from 1GB to 4GB, but physicalRAM/4 is still limiting
  - Modified: allow conversions between RGB24/32/48/64 (8<->16 bits) w/o ConvertBits
  - Added VS2017 and v141_xp to CMakeList.txt

20170529 r2489
  - fix: memory leak in CAVIStreamSynth (e.g. feeding vdub)
  - fix: ConvertToY for RGB64 and RGB48

20170528 r2487
  - Blur, Sharpen 10-16 bits planar and RGB64: SSE2/SSE4 (2x-4x speed)
  - New script function: int GetProcessInfo([int type = 0])
    Without parameter or type==0 the current bitness of Avisynth DLL is returned (32 or 64)
    With type=1 the function can return a bit more detailed info:
    -1: error, can't establish
    0: 32 bit DLL on 32 bit OS
    1: 32 bit DLL on 64 bit OS (WoW64 process)
    2: 64 bit DLL
  - Fix: Blur width=16 (YV12 width=32)
  - Fix: Overlay Lighten: artifacts when base clip and overlay clip have different widths (regression since r2290)
  - Fix: YUY2 HorizontalReduceBy2 did nothing if target width was not mod4
  - ImageReader: 16 bit support; "pixel_type" parameter new formats "RGB48", "RGB64" and "Y16"
  - ImageWriter: 16 bit support; save RGB48, RGB64, Y16, planar RGB(A) 8 and 16 bit formats
    (note: greyscale through devIL can be corrupt with some formats, use png)
  - ImageWriter: flip greyscale images vertically (except "raw" format)
  - SubTitle: new parameter "font_filename" allows using non-installed fonts
  - (project can be compiled using gcc)
  - Allows opening unicode filenames through VfW interface (virtualdub, MPC-HC)
  - Script function Import: new parameter bool "utf8" to treat the filenames as UTF8 encoded
    (not the script text!)
  - SubTitle: new parameter bool "utf8" for drawing strings encoded in UTF8.
      Title="Cherry blossom "+CHR($E6)+CHR($A1)+CHR($9C)+CHR($E3)+CHR($81)+CHR($AE)+CHR($E8)+CHR($8A)+CHR($B1)
      SubTitle(Title,utf8=true)
  - New script functions: ScriptNameUtf8(), ScriptFileUtf8(), ScriptDirUtf8(), 
    they return variables $ScriptNameUtf8$, $ScriptFileUtf8$ and $ScriptDirUtf8$ respectively

  Knows issues: 
  - Filters with MT_SERIALIZED sometimes can get called in a reentrant way
  - Runtime Script functions under MT
  

20170316 r2455
  - Fix: IsY() script function returned IsY8() (VideoInfo::IsY was not affected)
  - New: ConvertBits, dither=1 (Floyd-Steinberg): allow any dither_bits value between 0 and 8 (0=b/w)

20170310 r2440
  - Fix Merge for float formats
  - Fix error text formatting under wine (_vsnprintf_l issue)
  - Fix Regression: YUY2 UToY copied V instead of U, since August, 2016 (v2150)

  - faster Merge: float to sse2 (both weighted and average)
  - faster ordered dither to 8bit: SSE2 (10x speed)

  - ColorBars allows any 4:2:0, 4:4:4 formats, RGB64 and all planar RGB formats
  - ColorBarsHD accepts any 4:4:4 formats
  - Dithering: Floyd-Steinberg
    Use convertBits parameter dither=1: Floyd-Steinberg (was: dither=0 for ordered dither)
  - Dithering: parameter "dither_bits"
    For dithering to lower bit depths than the target clip format
    Usage: ConvertBits(x, dither=n [, dither_bits=y])
    - ordered dither: dither_bits 2, 4, 6, ... but maximum difference between target bitdepth and dither_bits is 8
    - Floyd-Steinberg: dither_bits 1, 2, 4, 6, ... up to target bitdepth - 2
    (Avisynth+ low bitdepth, Windows 3.1 16 bit feeling I was astonished that dither_bits=6 still resulted in a quite usable image)
  - Dithering is allowed from 10-16 -> 10-16 bits (was: only 8 bit targets)
  - Dithering is allowed while keeping original bit-depth. clip10 = clip10.ConvertBits(10, dither=0, dither_bits=8)
    (you still cannot dither from 8 or 32 bit source)
  - ConditionalFilter syntax extension like Gavino's GConditional: no "=" "true" needed
  - Revert: don't give error for interlaced=true for non 4:2:0 sources (compatibility, YATTA)
  - CombinePlanes: silently autoconvert packed RGB/YUY2 inputs to planar
  - ConvertBits: show error message on YV411 conversion attempt: 8 bit only
  - ConvertBits: Don't give error message if dither=-1 (no dithering) is given for currently non-ditherable target formats
  - Script function: IsVideoFloat. returns True if clip format is 32 bit float. For convenience, same as BitsPerComponent()==32 
  - ConvertToDoubleWidth and ConvertFromDoubleWidth: RGB24<->RGB48, RGB32<->RGB64
  - New MT mode: MT_SPECIAL_MT. Specify it for MP_Pipeline like filters, even if no Prefetch is used (MP_Pipeline issue, 2 fps instead of 20)


20170202 r2420
  - CombinePlanes:
    When there is only one input clip, zero-cost BitBlt-less subframes are used, which is much faster.

    e.g.: casting YUV to RGB, shuffle RGBA to ABGR, U to Y, etc..
    Target planes that are not specified, preserve their content.

    Examples:
    combineplanes(clipRGBP, planes="RGB",source_planes="BGR") # swap R and B
    combineplanes(clipYUV, planes="GBRA",source_planes="YUVA",pixel_type="RGBAP8") # cast YUVA to planar RGBA
    combineplanes(clipYUV, planes="Y",source_planes="U",pixel_type="Y8") # extract U

  - fix: SubframePlanarA (available in IScriptEnvironment2)
  - faster: Difference-type conditional functions: Simd for 10-16 bits
  - Fix: MinMax-type conditional functions (min, max, median): return float value for float clips
  - ConvertToPlanarRGB(A): 
    PlanarRGB <-> PlanarRGBA is now allowed
  - ConvertToPlanarRGB(A):
    YUY2 source is now allowed (through automatic ConvertToRGB proxy)
  - faster: RemoveAlphaPlane (subframe instead of BitBlt copy)
  - Overlay: "Blend" native greyscale mode: process y plane only w/o conversion
  - Overlay: automatic use444=false for "blend"/"luma"/"chroma" 
    for inputs: 420/422/444 and any RGB, lossless Planar RGB intermediate for PackedRGB
    Overlay/mask auto-follows input clip format.
    For compatibility: when greymask=true (default) and mask is RGB then mask source is the B channel
  - faster: RGB48->RGB64 SSSE3 (1,6x), RGB64->RGB48 SSSE3 (1.5x speed)
  - faster: RGB24,RGB48->PlanarRGB: uses RGB32/64 intermediate clip
  - Histogram "levels": allow RGB24/32/48/64 input.
    Display R, G and B channels instead of Y, U and V
    Reminder 1: "levels" for Planar RGB was working already
    Reminder 2: Histogram "levels" and "Classic" allows bits=xx parameter, xx=8..12
                If "bits" is specified then Histogram is drawn with 9..12 bits precision. Get a wide monitor though :)
  - ConvertBits: new parameters, partially for the future.
    bool fulls, bool fulld

    For YUV and greyscale clips the bit-depth conversion uses simple bit-shifts by default.
    YUV default is fulls=false

    RGB is converted as full-stretch (e.g. 0..255->0..65535)
    RGB default is fulls=true

    If fulld is not specified, it takes the value of fulls.
    Use case: override greyscale conversion to fullscale instead of bit-shifts

    Note 1: conversion from and to float is always full-scale
    Note 2: alpha plane is always treated as full scale
    Note 3: At the moment you cannot specify fulld to be different from fulls.
   

20170119 r2397
  - TemporalSoften: Planar RGB support
  - TemporalSoften: much faster average mode (thres=255) 
    radius=1 +70%, radius=2 +45%, 
    16bit: generally 7-8x speed (SSE2/4 instead of C)
  - SeparateColumns: 10-16bit,float,RGB48/64
  - WeaveColumns: 10-16bit,float,RGB48/64,PlanarRGB(A)
  - AddAlphaPlane: fix function parameter type list, clip type did not work
  - Internals: add SubframePlanarA to IScriptEnvirontment2 for frames with alpha plane
  - SwapUV: YUVA support
  - ConvertToRGB32/64: copy alpha from YUVA
  - SeparateRows,SeparateFields: PlanarRGB(A),YUVA support
  - WeaveRows: PlanarRGB(A), YUVA
  - Weave (fields,frames): YUVA,PlanarRGB(A)
  - Crop: Fast crop possible for frames with alpha plane (subframe)
  - AddBorders missing l/r/top/bottom vs. subsampling check for YUVA
  - Fix: YUVA->PlanarRGBA and YUVA42x->444 missing alpha plane copy 
  - YUV444->RGB48/64: fast intermediate PlanarRGB(A) then RGB48/64 (not C path)
  - RGB48/64->YUV4xx target: Planar RGB intermediate (instead of C, 10x faster)

20170111 r2380
  - Overlay: Merge the source of Chroma and Luma modes into Blend mode
             Modes Blend,Luma and Chroma now support all bit depths and 444 conversionless (use444=false) mode
  - Overlay: fix SSE2 Blend for mask+opacity for 10-16 bits
  - VfW: (vdubmod14 VfW test)
    Fix: YUV444P16 or YUVA444P16 to fourcc Y416 
         if alpha channel is present, it will we copied, else filled with FFFF
    Fix: VDubPlanarHack is checked only for 8 bit YUV planar sources
20170110 r2372  (vdubmod14 VfW test)
  - New: SSE2/SSE4 for 10-16 bit <-> 10-16 bit Planar RGB (and Alpha plane) full scale conversions
    (needed for automatic planar RGB -> packed RGB VfW conversions)
  - VfW:
    Fixed: Y3[10][10],Y3[10][16] fourcc's byte order
    New: Planar RGB(A) (MagicYUV) 
         10,12,14,16 bits: G3[0][10], G4[0][10], G3[0][12], G4[0][12], G3[0][14], G4[0][14], G3[0][16], G4[0][16]
    New: YUV444P16 to fourcc Y416
    New: Automatic conversion of 12, 14 and float YUV formats to 16 bit for 4:2:0 and 4:2:2
         Note: OPT_Enable_Y3_10_16 is still valid as if format was originally 16 bits
    New: Automatic conversion of 10, 12, 14 and float YUV formats to 16 bit for 4:4:4
    New: Conversion of 10, 12, 14 and float planar RGB formats to RGB64
         when global Avisynth variable Enable_PlanarToPackedRGB is true 
    New: Conversion of 8 bit planar RGB formats to RGB24
         when global Avisynth variable Enable_PlanarToPackedRGB is true 
    New: Conversion of 8 bit planar RGBA formats to RGB32
         when global Avisynth variable Enable_PlanarToPackedRGB is true 

    Note: use OPT_VDubPlanarHack=true for YV16 and YV24 for old VirtualDub
    
    Supported formats:
    BRA[64],b64a,BGR[48],P010,P016,P210,P216,Y3[10][10],Y3[10][16],v210,Y416
    G3[0][10], G4[0][10], G3[0][12], G4[0][12], G3[0][14], G4[0][14], G3[0][16], G4[0][16]
    
    Default format FourCCs:
    RGB64: BRA[64]
    RGB48: BGR[48]
    YUV420P10: P010
    YUV420P16: P016
    YUV422P10: P210
    YUV422P16: P216
    YUV444P16 and YUVA444P16: Y416
    Planar RGB  10-16 bit: G3[0][10], G3[0][12], G3[0][14], G3[0][16]
    Planar RGBA 10-16 bit: G4[0][10], G4[0][12], G4[0][14], G4[0][16]

    Global variables to override default formats:
    OPT_Enable_V210 = true --> v210 for YUV422P10
    OPT_Enable_Y3_10_10 = true --> Y3[10][10] for YUV422P10
    OPT_Enable_Y3_10_16 = true --> Y3[10][16] for YUV422P16
    OPT_Enable_b64a = true --> b64a for RGB64
    Enable_PlanarToPackedRGB = true --> RGBP8->RGB24, RGBAP8->RGB32, all other bit depths to RGB64

20170109 r2367dev
  - VfW: BRA[64],b64a,BGR[48],P010,P016,P210,P216,Y3[10][10],Y3[10][16],v210 
    experimental!
    Default format FourCCs:
    RGB64: BRA[64]
    RGB48: BGR[48]
    YUV420P10: P010
    YUV420P16: P016
    YUV422P10: P210
    YUV422P16: P216
    Global variables to override default formats:
    OPT_Enable_V210 = true --> v210 for YUV422P10
    OPT_Enable_Y3_10_10 = true --> Y3[10][10] for YUV422P10
    OPT_Enable_Y3_10_16 = true --> Y3[10][16] for YUV422P16
    OPT_Enable_b64a = true --> b64a for RGB64
  - Overlay: blend for float format
  - Overlay: blend: SSE4 for 10-16 bit, SSE2 for float
  - AddAlphaPlane: also accepts clip with Y-only or alpha (YUVA/PRGBA/RGB32/64) for alpha source
    (was: optional int/float mask value)

20170104 r2359dev
  - Overlay:
    new parameter: bool use444 (default true for compatibility)

    Parameter is valid only for "blend" at the moment

    When set to false, and base clip/overlay clip is 420/422/Planar RGB, the overlay core 
    does not convert to and back from YV24 (4:4:4 in general)

    mask can be either greyscale or can be any planar YUV when greymask=true (default)

    Works for Planar RGB, so blending is basically lossless for this format (no YUV conversion)

    todo: support for other modes, convert packed RGB to planar RGB internally instead of YUV
  - Overlay:
    Add fast 4:2:0<->4:4:4 conversion, e.g. YV16<->YV24 (only fast YUY2 and YV12 existed so far)
    instead of invoking generic core conversion
  - Overlay:
    10-16bit SSE2/SSE4 for 420/422<->444 conversions
  - Info() made a bit more compact. 
    Bit depth info moved after color space info
    Does not display pre-SSE2 CPU flags when at least AVX is available
    Display AVX512 flags in separate line (would be too long)
  - new CPU feature constants (see cpuid.h and avisynth_c.h)
    Detect FMA4 and AVX512F,DQ,PF,ER,CD,BW,VL,IFMA,VBMI
  - new script function:
    string ReplaceStr(string s, string pattern, string replacement)
    Function is case sensitive, parameters are unnamed
  - new script function
    int NumComponents(clip)
    returns 1 for grayscale, 3 for YUVxxx, YUY2, planar RGB or RGB24/RGB48, 4 for YUVAxxx, Planar RGBA or RGB32/64
  - new script function: 
    bool HasAlpha(clip)
    returns true when clip is YUVA, Planar RGBA, or packed RGB32 or RGB64

20161222 r2347dev
  - CombinePlanes addition: setting target to default RGBP(A)/YUV(A), when inputs are greyscale and no source planes are given
    Decision is made by the target plane characters, if they are like R,G,B then target video format will be planar RGB
    Same logic applies for YUV.
    Example:
    Y1, Y2 and Y3 are greyscale clips
    Old, still valid: combineplanes(Y1, Y2, Y3, planes="RGB", source_planes="YYY", pixel_type="RGBP8")
    New:              combineplanes(Y1, Y2, Y3, planes="RGB") # result: Planar RGB
  - Fix: ScriptClip would show garbage text when internal exception occurs instead of the error message

20161211 r2343dev
  - Overlay: use y offset when greymask=true (fix)
  - Fix DV chroma positioning (UV swapped), interlaced parameter check for 4:2:0 
    (fix by IanB in classic Avisynth)
  - BitBlt in 32 bit Avisynth: 
    for processors with AVX or better ignore tricky isse memcpy replacement, trust in memcpy (test)
    (x64 is O.K., it always used memcpy)
  - Merge: use stream_load for AVX2
  - VDubFilter.dll:
    convert 'd' double and 'l' long typed parameters to 'f' float and 'i' int for poor AviSynth
    thus allowing the usage of such virtualdub filters
  - remove script array (new AVSValue schema) feature, cannot make it compatible with Invoke from v2.5 plugins
    until I figure out a workaround or such plugins would slowly distinct.

20161208 r2337dev
  - [feature temporarily removed, script arrays are incompatible until 2.5 plugins are supported]
    C interface array compatibility vol#2 (zero size arrays)
    (new AVSValue schema problem)
  - Merge, MergeChroma, MergeLuma: AVX2 (planar)
  - Possibly a bit faster text overlay

20161207 r2333dev
  - Overlay fix

20161206 r2331dev
  - YUY2 PlaneToY finally works
  - C interface compatible array-type AVSValue handling (new AVSValue schema problem)

20161205 r2327dev
  - [feature temporarily removed, script arrays are incompatible until 2.5 plugins are supported]
    BlankClip parameter "colors" accepts exact color values to use
    Color order: Y,U,V,A or R,G,B(,A)
    These color values are used as-is, not scaled or converted in any way.
    Reason: old colors parameter is int (32 bit) cannot hold three or four 16 bit or float values
    Example: BlankClip(width=1920,height=1080,length=1000,pixel_type="RGB64", colors=[64000,32768,1231,65535])
  - ExtractY, PlaneToY("Y") accepts YUY2 clip
  - ExtractR, ExtractG, ExtractB, ExtractA,
    and
    PlaneToY("R"), PlaneToY("G"), PlaneToY("B"), PlaneToY("A") 
    functions are accepting packed RGB input (RGB24/32/48/64)
    They are converted to planar RGB on-the-fly before plane extraction
  - Histogram "levels" works from Planar RGB.
    Color legends show R, G and B.
    bits=8..12 parameter is still available for finer ultra-wide histogram display

20161201 r2322dev
  - [feature temporarily removed, script arrays are incompatible until 2.5 plugins are supported]
    constant script arrays 
    (note: feature was temporarily removed in r2443)
    array_variable = [[1,2,3],[4,5,8],"hello"]
    dictionary = [["one",1],["two",2]]
    empty = []
    subarray = array_variable[0]
    val = subarray[2]
    val2 = array_variable[1,3]
    str = array_variable[2]
    n = ArraySize(array_variable) #3
    n2 = ArraySize(empty) #0
    val3 = dictionary["two"]
  - arrays as filter parameters (named and unnamed):
    new 'a' type or use '.' (any) and check AVSValue IsArray()
    todo: maybe .+ or *+ syntax?
    (note: feature was temporarily removed in r2443)
  - Planar RGB <-> YUV: SSE2 (SSE4)
  - Planar RGB <-> Packed RGB32/64: SSE2 

20161120:
  make PlanarRGB TurnLeft, TurnRight work again.
  (stricter check in PlaneToY)

20161119
  Fix: PlaneToY("Y")

20161116

  Replaced AToY8, GToY8, BToY8, RToY8
  We have ExtractX, (X = R,G,B,Y,U,V,A) instead.
  That Y8 ending was too confusing.
  See new test build

20161110 Avisynth plus additions
New functions

  AToY8 , same as PlaneToY("A") for planar RGBA or YUVA
  RToY8 , same as PlaneToY("R") for planar RGB
  GToY8 , same as PlaneToY("G") for planar RGB
  BToY8 , same as PlaneToY("B") for planar RGB

  They work the same way as UToY8 and VToY8 and ConvertToY did.
  They convert to greyscale keeping the original bit-depth, not only 8 bit! 
  Y8 naming was kept, because UToY and VToY already existed.

Extended function
  old: YToUV(clip clipU, clip clipV [, clip clipY ] ) 
  new: YToUV(clip clipU, clip clipV [, clip clipY [, clip clipA] ] ) 

  YToUV accepts optional alpha clip after Y clip

  Example
    U = source.UToY8()
    V = source.VToY8()
    Y = source.ConvertToY()
    A = source.AddAlphaPlane(128).AToY8()
    # swaps V, U and A, Y
    YToUV(V,U,A,Y).Histogram("levels").Info().RemoveAlphaPlane()
  
New function
  CombinePlanes(clip1 [,clip2, clip3, clip4], string planes [, string source_planes, string pixel_type, string sample_clip])

  Combines planes of source clip(s) into a target clip

  If sample_clip is given, target clip properties are copied from that clip
  If no sample_clip is provided, then clip1 provides the template for target clip
  An optional pixel_type string (e.g."YV24", "YUV420PS", "RGBP8") can override the base video format.

  If the source clip count is less than the given planes defined, then the last available clip is 
  used as a source.

  string planes
    the target plane order (e.g. "YVU", "YYY", "RGB")
    missing target planes will be undefined in the target
  
  string source_planes (optional)
    the source plane order, defaulting to "YUVA" or "RGBA" depending on the video format

  Examples#1
    #combine greyscale clips into YUVA clip
    U8 = source.UToY8()
    V8 = source.VToY8()
    Y8 = source.ConvertToY()
    A8 = source.AddAlphaPlane(128).AToY8()
    CombinePlanes(Y8, U8, V8, A8, planes="YUVA", source_planes="YYYY", sample_clip=source) #pixel_type="YUV444P8"
  
  Examples#2
    # Copy planes between planar RGB(A) and YUV(A) without any conversion
    # yuv 4:4:4 <-> planar rgb
    source = last.ConvertBits(32) # 4:4:4
    cast_to_planarrgb = CombinePlanes(source, planes="RGB", source_planes="YUV", pixel_type="RGBPS")
    # get back a clip identical with "source"
    cast_to_yuv = CombinePlanes(cast_to_planarrgb, planes="YUV", source_planes="RGB", pixel_type="YUV444PS")

  Examples#3
    #create a black and white planar RGB clip using Y channel
    #source is a YUV clip
    grey = CombinePlanes(source, planes="RGB", source_planes="YYY", pixel_type="RGBP8")

  Examples#4
    #copy luma from one clip, U and V from another
    #source is the template
    #sourceY is a Y or YUV clip
    #sourceUV is a YUV clip
    grey = CombinePlanes(sourceY, sourceUV, planes="YUV", source_planes="YUV", sample_clip = source)
  

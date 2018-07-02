Avisynth+ v2728 (20180702)
--------------------------

Use the installer or copy files directly
- 64 bit OS: 
  copy Avisynth.dll from x86 folder to the windows SysWOW64 folder
  copy Avisynth.dll from x64 folder to the windows System32 folder 
- 32 bit OS
  copy Avisynth.dll from x86 folder to the windows System32 folder
- All OS: Copy appropriate files from the plugins+/plugins64+ to the appropriate Avisynth+ folder

Useful links:
-------------
Source: https://github.com/pinterf/AviSynthPlus/tree/MT
Forum: https://forum.doom9.org/showthread.php?t=168856
Forum on some avs+ filters: https://forum.doom9.org/showthread.php?t=169832
Avisynth+ info page: http://avisynth.nl/index.php/AviSynth%2B
Info on Avisynth+ new color spaces: https://forum.doom9.org/showthread.php?p=1783714#post1783714
Avisynth Universal Installer by Groucho2004: https://forum.doom9.org/showthread.php?t=172124

Short info for plugin writers
-----------------------------
  Avisynth+ header (and helper headers) are available here, or choose SDK during install:
  https://github.com/pinterf/AviSynthPlus/tree/MT/avs_core/include

  Use these headers for building x86/x64 plugins, and to use Avisynth+'s high-bitdepth related VideoInfo functions
  without any concern. When a VideoInfo function is non-existant on a system that uses your plugin, 
  it won't crash, just fall back to a default behaviour. E.g. VideoInfo.BitsPerComponent() will always return 8
  when your plugin calls it on a Classic Avisynth, or pre-high bit depth Avisynth+ host.

(see readme_history.txt for details, syntax element, etc. They also appear on avisynth.nl)
20180702 r2728
--------------
- Fix: Expr: expression string order for planar RGB is properly r-g-b like in original VapourSynth version, instead of counter-intuitive g-b-r.
- Fix: Expr: check subsampling when a different output pixel format is given
- Fix: ColorYUV: round to avoid green cast on consecutive TV<>PC
- Fix: RGBAdjust memory leak when used in ScriptClip
- Fix: RGB64 Turnleft/Turnright (which are also used in RGB64 Resizers)
- Fix: Rare crash in FrameRegistry
- Fix: couldn't see variables in avsi before plugin autoloads (colors_rgb.avsi issue)
- Fix: LoadVirtualdubPlugin: Fix crash on exit when more than one instances of a filter was used in a script
- New: Expr: implement 'clip' three operand operator like in masktools2
- New: Expr: Parameter "clamp_float" (like in masktools2 2.2.15)
- New: Expr: parameter "scale_inputs" (like in masktools2 2.2.15)
- New: function bool VarExist(String variable_name)
- New function: BuildPixelType: 
  Creates a video format (pixel_type) string by giving a colorspace family, bit depth, optional chroma subsampling and/or a 
  template clip, from which the undefined format elements are inherited.
- Enhanced: Limiter to work with 32 bit float clips
- Enhanced: Limiter new parameter bool 'autoscale' default false, parameters now are of float type to handle 32 bit float values.
- Enhanced: RGBAdjust new parameter: conditional (like in ColorYUV)
  The global variables "rgbadjust_xxx" with xxx = r, g, b, a, rb, gb, bb, ab, rg, gg, bg, ag are read each frame, and applied. 
- Enhanced: RGBAdjust: support 32 bit float ('analyze' not supported, 'dither' silently ignored)
- Enhanced: AviSource to support much more formats with 10+ bit depth.
- Changed (finally): 32bit float YUV colorspaces: zero centered chroma channels. 
  U and V channels are now -0.5..+0.5 (if converted to full scale before) instead of 0..1
- New function: bool IsFloatUvZeroBased() for plugin or script writers who want to be compatible with pre r2672 Avisynth+ float YUV format:
- Enhanced: Allow ConvertToRGB24-32-48-64 functions for any source bit depths
- Enhanced: ConvertBits: allow fulls-fulld combinations when either clip is 32bits
  E.g. after a 8->32 bit fulls=false fulld=true: 
  Y: 16..235 -> 0..1 
  U/V: 16..240 -> -0.5..+0.5
  Note: now ConvertBits does not assume full range for YUV 32 bit float. 
  Default values of fulls and fulld are now true only for RGB colorspaces.
- New: LoadVirtualdubPlugin update: Update from interface V6 to V20, and Filtermod version 6 (partial)
- Source: move to c++17, 'if constexpr' requires. Use Visual Studio 2017 (or GCC 7?). CMakeLists.txt changed.
- Source: C api: AVSC_EXPORT to dllexport in capi.h for avisynth_c_plugin_init
- Source: C api: avs_is_same_colorspace VideoInfo parameters to const
- Project struct: changelog to git.
- Include current avisynth header files and def/exp file in installer, when SDK is chosen

20180328 r2664
--------------
#Fix
YUY2 Sharpen overflow artifacts - e.g. Sharpen(0.6)
Levels: 32 bit float shift in luma
Merge sse2 for 10-14bits (regression)
AVX2 resizer possible access violation in extreme resizes (e.g. 600->20)
32bit float PlanarRGB<->YUV conversion matrix
VfW: fix b64a output for OPT_Enable_b64a=true

#Enhanced
VfW output P010 and P016 conversion to SSE2 (VfW output is used by VirtualDub for example)
ColorYUV: recalculate 8-16 bit LUT in GetFrame only when changed frame-by-frame (e.g. in autowhite)
ConvertBits 32->8 sse2/avx2 and 32->10..16 sse41/avx2 (8-15x speed)

20180302 r2636
--------------
#Fix
Blur/Sharpen crashed when YUY2.width<8, RGB32.width<4, RGB64.width<2
ColorYUV: don't apply TV range gamma for opt="coring" when explicit "PC->TV" is given
ColorbarsHD: 32bit float properly zero(0.5)-centered chroma

v2632 (20180301)
----------------
#Fix
Fix: IsInterleaved returned false for RGB48 and RGB64 (raffriff42)
Fix: SubTitle for Planar RGB/RGBA: wrong text colors (raffriff42)
Fix: Packed->Planar RGB conversion failed on SSE2-only computers (SSSE3 instruction used)
Fix: Resizers for 32 bit float rare random garbage on right pixels (simd code NaN issue)

#Enhanced
Blur, Sharpen
    AVX2 for 8-16 bit planar colorspaces (>1.35x speed on i7-7770)
    SSE2 for 32 bit float formats (>1.5x speed on i7-7770)
Completely rewritten 16bit and float resizers, much faster (and not only with AVX2)
8 bit resizers: AVX2 support
Speed up converting from RGB24/RGB48 to Planar RGB(A) - SSSE3, approx. doubled fps
Enhanced: VfW: exporting Y416 (YUV444P16) to SSE2.

#New/Modded
ConvertFPS supports 10-32 bits, planar RGB(A), YUV(A)
New script function: int BitSetCount(int[, int, int, ...])
Modded script function: Hex(int , int "width"=0), new "width" parameter
Modded script function: HexValue(String, "pos"=1) new pos parameter
Modded script function: ReplaceStr(String, String, String[, Boolean "sig"=false]) New parameter: sig for case insensitive search (Default false: exact search)
New script functions: TrimLeft, TrimRight, TrimAll for removing beginning/trailing whitespaces from a string.
New in ColorYUV: New parameter: bool f2c="false". When f2c=true, the function accepts the Tweak-like parameters for gain, gamma and contrast
New/Fixed in ColorYUV: Parameter "levels" accepts "TV". (can be "TV->PC", "PC->TV", "PC->TV.Y")
New: Now gamma calculation is TV-range aware when either levels is "TV->PC" or coring = true or levels is "TV"
New in ColorYUV:  32 bit float support. 
ColorYUV: can specify bits=32 when showyuv=true -> test clip in YUV420PS format
Modded: remove obsolate "scale" parameter from ConvertBits.
Internal: 8-16 bit YUV chroma to 32 bit float: keep middle chroma level (e.g. 128 in 8 bits) at 0.5. Calculate chroma as (x-128)/255.0 + 0.5 and not x/255.0 
(Note: 32 bit float chroma center will be 0.0 in the future)
New: Histogram parameter "keepsource"=true (raffriff42) for "classic", "levels" and "color", "color2"
New: Histogram type "color" to accept 8-32bit input and "bits"=8,9,..12 display range
New: Histogram parameter "markers"=true. Markers = false disables extra markers/coloring for "classic" and "levels"

v2580 (20171226)
----------------
# Fix
Fix (workaround): Merge: Visual Studio 2017 15.5.1/2 generated invalid AVX2 code (x86 crashed)
Fix: Temporalsoften 10-14 bits: an SSE 4.1 instruction was used for SSE2-only CPU-s (Illegal Instruction on Athlon XP)

v2574 (20171219)
----------------
# Fix
Fix: MaskHS created inverse mask. Regression after r2173
Fix: jitasm code generation at specific circumstances in Expr filter

# Build
Build: changed avisynth.h, strict C++ conformity with Visual Studio 2017 /permissive- flag

# Other
Installer in two flavours: simple or full (with Microsoft Visual C++ Redistributables)

# New
Expr tweaks:
  - Indexable source clip pixels by relative x,y positions. 
  Syntax: x[a,b] where 
  'x': source clip letter a..z
  'a': horizontal shift. -width < a < width
  'b': vertical shift. -height < b < height
  'a' and 'b' should be constant. e.g.: "x[-1,-1] x[-1,0] x[-1,1] y[0,-10] + + + 4 /"
  When requested pixels come from off-screen the off-screen values are cloned from the appropriate top-bottom-left-right edge.
  Optimized version requires SSSE3 (and no AVX2 version is available). On non-SSSE3 CPUs falls back to C.
  
  - sin cos tan asin acos atan (no SSE2/AVX2 optimization, when they appear in Expr a slower C code runs the expression)
  
  - % (modulo). result = x - trunc(x/d)*d. 
  Note: internally everything is calculated as a 32 bit float. 
  A float can only hold a 24 bit integer number, don't expect 32 bit accuracy here.
  
  - Variables: uppercase letters A..Z for storing and reuse temporary results, frequently used computations.
  Store, result can still be used from stack: A@ .. Z@ 
  Store and remove from stack: A^ .. Z^
  Use: A..Z
  Example: "x y - A^ x y 0.5 + + B^ A B / C@ x +"
  
  - 'frameno' : use current frame number in expression. 0 <= frameno < clip_frame_count
  
  - 'time' : calculation: time = frameno/clip_frame_count. Use relative time position in expression. 0 <= time < frameno/clip_frame_count
  
  - 'width', 'height': currently processed plane width and height

v2542-2544 (20171114,20171115), changes since v2508
-------------------------------------
# New
  Expr filter

  Syntax ("c+s+[format]s[optAvx2]b[optSingleMode]b[optSSE2]b")
  clip Expr(clip c[,clip c2, ...], string expr [, string expr2[, string expr3[, string expr4]]] [, string format]
      [, bool optSSE2][, bool optAVX2][, bool optSingleMode])

  Clip and Expr parameters are unnamed
  'format' overrides the output video format
  'optSSE2' to disable simd optimizations (use C code)
  'optAVX2' to disable AVX2 optimizations (use SSE2 code)
  'optSingleMode' default false, to generate simd instructions for one XMM/YMM wide data instead of two. Experimental.
     One simd cycle processes 8 pixels (SSE2) or 16 pixels (AVX2) at a time by using two XMM/YMM registers as working set.
     Very-very complex expressions would use too many XMM/YMM registers which are then "swapped" to memory slots, that can be slow.
     Using optSingleMode = true may result in using less registers with no need for swapping them to memory slots.

  Expr accepts 1 to 26 clips as inputs and up to four expression strings, an optional video format overrider, and some debug parameters.
  Output video format is inherited from the first clip, when no format override.
  All clips have to match their dimensions and plane subsamplings.

  Expressions are evaluated on each plane, Y, U, V (and A) or R, G, B (,A).
  When an expression string is not specified, the previous expression is used for that plane. Except for plane A (alpha) which is copied by default.
  When an expression is an empty string ("") then the relevant plane will be copied (if the output clip bit depth is similar).
  When an expression is a single clip reference letter ("x") and the source/target bit depth is similar, then the relevant plane will be copied.
  When an expression is constant, then the relevant plane will be filled with an optimized memory fill method.
  Expressions are written in Reverse Polish Notation (RPN).

  Expressions use 32 bit float precision internally
 
  For 8..16 bit formats output is rounded and clamped from the internal 32 bit float representation to valid 8, 10, ... 16 bits range.
  32 bit float output is not clamped at all.
  
  - Clips: letters x, y, z, a, ... w. x is the first clip parameter, y is the second one, etc.
  - Math: * / + -
  - Math constant: pi
  - Functions: min, max, sqrt, abs, neg, exp, log, pow ^ (synonyms: "pow" and "^")
  - Logical: > < = >= <= and or xor not == & | != (synonyms: "==" and "=", "&" and "and", "|" and "or")
  - Ternary operator: ?
  - Duplicate stack: dup, dupN (dup1, dup2, ...)
  - Swap stack elements: swap, swapN (swap1, swap2, ...)
  - Scale by bit shift: scaleb (operand is treated as being a number in 8 bit range unless i8..i16 or f32 is specified)

  - Scale by full scale stretch: scalef (operand is treated as being a number in 8 bit range unless i8..i16 or f32 is specified)

  - Bit-depth aware constants
    ymin, ymax (ymin_a .. ymin_z for individual clips) - the usual luma limits (16..235 or scaled equivalents)

    cmin, cmax (cmin_a .. cmin_z) - chroma limits (16..240 or scaled equivalents)

    range_half (range_half_a .. range_half_z) - half of the range, (128 or scaled equivalents)

    range_size, range_half, range_max (range_size_a .. range_size_z , etc..)

  - Keywords for modifying base bit depth for scaleb and scalef: i8, i10, i12, i14, i16, f32
  
  - Spatial input variables in expr syntax:
    sx, sy (absolute x and y coordinates, 0 to width-1 and 0 to height-1)
    sxr, syr (relative x and y coordinates, from 0 to 1.0)

  Additions and differences to VS r39 version:

  ------------------------------
--------------
  (similar features to the masktools mt_lut family syntax)

  Aliases:

    introduced "^", "==", "&", "|" 
  New operator: != (not equal)

  Built-in constants

    ymin, ymax (ymin_a .. ymin_z for individual clips) - the usual luma limits (16..235 or scaled equivalents)

    cmin, cmax (cmin_a .. cmin_z) - chroma limits (16..240 or scaled equivalents)

    range_half (range_half_a .. range_half_z) - half of the range, (128 or scaled equivalents)

    range_size, range_half, range_max (range_size_a .. range_size_z , etc..)

  Autoscale helper functions (operand is treated as being a number in 8 bit range unless i8..i16 or f32 is specified)

    scaleb (scale by bit shift - mul or div by 2, 4, 6, 8...)

    scalef (scale by stretch full scale - mul or div by source_max/target_max

  Keywords for modifying base bit depth for scaleb and scalef
: i8, i10, i12, i14, i16, f32

  Built-in math constant

    pi

  Alpha plane handling
. When no separate expression is supplied for alpha, plane is copied instead of reusing last expression parameter.
  Proper clamping when storing 10, 12 or 14 bit outputs

  (Faster storing of results for 8 and 10-16 bit outputs, fixed in VS r40)
  16 pixels/cycle instead of 8 when avx2, with fallback to 8-pixel case on the right edge. Thus no need for 64 byte alignment for 32 bit float.
  (Load zeros for nonvisible pixels, when simd block size goes beyond image width, to prevent garbage input for simd calculation)
  

  Optimizations: x^0.5 is sqrt, ^1 +0 -0 *1 /1 to nothing, ^2, ^3, ^4 is done by faster and more precise multiplication
  Spatial input variables in expr syntax:
    sx, sy (absolute x and y coordinates, 0 to width-1 and 0 to height-1)
    sxr, syr (relative x and y coordinates, from 0 to 1.0)
  Optimize: recognize constant plane expression: use fast memset instead of generic simd process. Approx. 3-4x (32 bits) to 10-12x (8 bits) speedup
  Optimize: Recognize single clip letter in expression: use fast plane copy (BitBlt) 
    (e.g. for 8-16 bits: instead of load-convert_to_float-clamp-convert_to_int-store). Approx. 1.4x (32 bits), 3x (16 bits), 8-9x (8 bits) speedup

  Optimize: do not call GetFrame for input clips that are not referenced or plane-copied

  Recognize constant expression: use fast memset instead of generic simd process. Approx. 3-4x (32 bits) to 10-12x (8 bits) speedup
    Example: Expr(clip,"128","128,"128")
 
  Differences from masktools 2.2.10
  --------------------------------
-
  Up to 26 clips are allowed (x,y,z,a,b,...w). Masktools handles only up to 4 clips with its mt_lut, my_lutxy, mt_lutxyz, mt_lutxyza

  Clips with different bit depths are allowed

  Works with 32 bit floats instead of 64 bit double internally

  Less functions (e.g. no bit shifts)

  No float clamping and float-to-8bit-and-back load/store autoscale magic

  Logical 'false' is 0 instead of -1

  The ymin, ymax, etc built-in constants can have a _X suffix, where X is the corresponding clip designator letter. E.g. cmax_z, range_half_x

  mt_lutspa-like functionality is available through "sx", "sy", "sxr", "syr"

  No y= u= v= parameters with negative values for filling plane with constant value, constant expressions are changed into optimized "fill" mode

  Sample: 
  
  Average three clips:
  c = Expr(clip1, clip2, clip3, "x y + z + 3 /") 
  using spatial feature:
  c = Expr(clip1, clip2, clip3, "sxr syr 1 sxr - 1 syr - * * * 4096 scaleb *", "", "")

# Additions
  - Levels: 32 bit float format support

# Fixes
  - RGB (full scale) conversion: 10-16 bits to 8 bits rounding issue; pic got darker in repeated 16<->8 bit conversion chain
  - ConvertToY: remove unnecessary clamp for Planar RGB 32 bit float
  - RGB ConvertToY when rec601, rec709 (limited range) matrix. Regression since r2266

# Optimizations
  - Faster RGB (full scale) 10-16 bits to 8 bits conversion when dithering

# Other
  - Default frame alignment is 64 bytes (was: 32 bytes). (independently of AVX512 support)

# Source and build things.
  - Built with Visual Studio 2017, v141_xp toolset
  
    Download Visual Studio 2017 Redistributable from here (replaces and compatible with VS2015 redist)

    X64:
    https://go.microsoft.com/fwlink/?LinkId=746572

    x86:
    https://go.microsoft.com/fwlink/?LinkId=746571

  - Not for live yet - experimental - use of size_t in video frame fields

    You can now build avs+ with optional define: SIZETMOD. Type of offset-related video frame fields and return values changed from int to size_t. 
    Affects x64 where size_t is 8 bytes while int is 4 bytes.
    With Avisynth+ built with this SIZETMOD option, plugins or apps that are using C interface may be broken, when they access AVS_VideoFrameBuffer or AVS_VideoFrame fields directly.
    
    Known plugins/applications that are broken (as of 20171114): x264_64.exe
    Possible problems come from such as using field "pitch" in AVS_VideoFrame directly instead of calling avs_get_pitch_p through the interface.
    For the future: please take the warnings "DO NOT USE THIS STRUCTURE DIRECTLY" seriously.

    Plugin writers, Who are using the usual (non-C) interface, will probably see a change as frame->GetOffset which returns size_t instead of int. Nevertheless the content 
    will possibly never exceed old 32 bit limit.

    Another broken x64 plugin for SIZETMOD version: LSmashSource (as of 20171114). Possible reason: it uses CPP 2.5 (baked code) interface.
    (How much CPP 2.5 plugins exist still in 2017?)

  - avisynth_c.h (C interface header file) changed: 
    Optional define SIZETMOD. Experimental. Offsets are size_t instead of int (x64 is different!)
    Fix: avs_get_row_size calls into avs_get_row_size_p, instead of direct field access
    Fix: avs_get_height calls into avs_get_row_size_p, instead of direct field access.

v2508 (20170629), changes since v2506
-------------------------------------
# Fixes
  Fix TemporalSoften: threshold < 255

v2506 (20170608), changes since v2504
-------------------------------------
# Fixes
  Fix CombinePlanes: feeding YV16 or YV411 target with Y8 sources

v2504 (20170603), changes since v2502
-------------------------------------
# Fixes
  Fix broken XP support

v2502 (20170602), changes since v2489
-------------------------------------
# Fixes
  - (Important!) MT_SERIALIZED mode did not always protect filters (regression since r2069)
    Such filters sometimes were called in a reentrant way (like being MT_NICE_FILTER), which
    possibly resulted in using their internal buffers parallel.
  - ImageWriter crash when no '.' in provided filename
  - Overlay: correct masked blend: keep exact clip1 or clip2 pixel values for mask extremes 255 or 0. 
    Previously 0 became 1 for zero mask, similarly 255 changed into 254 for full transparency (255) mask

# other modification, additions  
  - New script functions: StrToUtf8(), StrFromUtf8(): Converting a 8 bit (Ansi) string to UTF8 and back.
  - PluginManager always throws error on finding wrong bitness DLL in the autoload directories
  - increased x64 default MemoryMax from 1GB to 4GB, but physicalRAM/4 is still limiting
  - allow conversions between RGB24/32/48/64 (8<->16 bits) w/o ConvertBits
  - Added VS2017 and v141_xp to CMakeList.txt

v2489 (20170529), changes since v2487
-------------------------------------
# Fixes
  - Memory leak in CAVIStreamSynth (e.g. feeding vdub)
  - ConvertToY for RGB64 and RGB48

v2487 (20170528), changes since v2455
-------------------------------------
# Fixes
  - Blur width=16 (YV12 width=32)
  - Overlay Lighten: artifacts when base clip and overlay clip have different widths (regression since r2290)
  - YUY2 HorizontalReduceBy2 did nothing if target width was not mod4

# optimizations
  - Blur, Sharpen 10-16 bits planar and RGB64: SSE2/SSE4 (2x-4x speed)

# other modification, additions
  - New script function: int GetProcessInfo([int type = 0])
    Without parameter or type==0 the current bitness of Avisynth DLL is returned (32 or 64)
    With type=1 the function can return a bit more detailed info:
    -1: error, can't establish
    0: 32 bit DLL on 32 bit OS
    1: 32 bit DLL on 64 bit OS (WoW64 process)
    2: 64 bit DLL
  - ImageReader: 16 bit support; "pixel_type" parameter new formats "RGB48", "RGB64" and "Y16"
  - ImageWriter: 16 bit support; save RGB48, RGB64, Y16, planar RGB(A) 8 and 16 bit formats
    (note: greyscale through devIL can be corrupt with some formats, use png)
  - ImageWriter: flip greyscale images vertically (except "raw" format)
  - SubTitle: new parameter "font_filename" allows using non-installed fonts
  - Allows opening unicode filenames through VfW interface (virtualdub, MPC-HC)
  - Script function Import: new parameter bool "utf8" to treat the filenames as UTF8 encoded
    (not the script text!)
  - SubTitle: new parameter bool "utf8" for drawing strings encoded in UTF8.
      Title="Cherry blossom "+CHR($E6)+CHR($A1)+CHR($9C)+CHR($E3)+CHR($81)+CHR($AE)+CHR($E8)+CHR($8A)+CHR($B1)
      SubTitle(Title,utf8=true)
  - New script functions: ScriptNameUtf8(), ScriptFileUtf8(), ScriptDirUtf8(), 
    they return variables $ScriptNameUtf8$, $ScriptFileUtf8$ and $ScriptDirUtf8$ respectively

v2455 (20170316), changes since v2440
-------------------------------------
# Fixes
  IsY() script function returned IsY8() (VideoInfo::IsY was not affected)

# other modification, additions
  ConvertBits, dither=1 (Floyd-Steinberg): allow any dither_bits value between 0 and 8 (0=b/w)

v2440 (20170310), changes since v2420
-------------------------------------
# Fixes
  Merge for float formats
  error text formatting under wine (_vsnprintf_l issue)
  Regression: YUY2 UToY copied V instead of U, since August, 2016 (v2150)

# optimizations
  Merge: float to sse2 (both weighted and average)
  Ordered dither to 8bit: SSE2 (10x speed)

# other modification, additions
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

v2420 (20170202), changes since v2294
--------------------------------------
# Fixes
  - MinMax-type conditional functions (min, max, median): return float value for float clips
  - ScriptClip would show garbage text when internal exception occurs instead of the error message
  - DV chroma positioning (UV swapped), interlaced parameter check for 4:2:0 
  - YUVA->PlanarRGBA and YUVA42x->444 missing alpha plane copy 

# optimizations

  - TemporalSoften: much faster average mode (thres=255) 
    radius=1 +70%, radius=2 +45%, 
    16bit: generally 7-8x speed (SSE2/4 instead of C)
  - Difference-type conditional functions: Simd for 10-16 bits
  - RemoveAlphaPlane (subframe instead of BitBlt copy)
  - RGB48->RGB64 SSSE3 (1,6x), RGB64->RGB48 SSSE3 (1.5x speed)
  - RGB24,RGB48->PlanarRGB: uses RGB32/64 intermediate clip
  - Crop: Fast crop possible for frames with alpha plane (subframe)
  - YUV444->RGB48/64: fast intermediate PlanarRGB(A) then RGB48/64 (not C path)
  - RGB48/64->YUV4xx target: Planar RGB intermediate (instead of C, 10x faster)
  - Planar RGB <-> YUV: SSE2 (SSE4)
  - Planar RGB <-> Packed RGB32/64: SSE2 
  - Merge, MergeChroma, MergeLuma: AVX2 (planar formats)
  - Possibly a bit faster text overlay
  - Overlay: 10-16bit SSE2/SSE4 for 420/422<->444 conversions
  - Overlay: blend: SSE4 for 10-16 bits, SSE2 for float
  - ConvertBits for planar RGB(A): SSE2/SSE4 for 10-16 bit <-> 10-16 bit Planar RGB (and Alpha plane) full scale conversions
    (also used internally for automatic planar RGB -> packed RGB VfW output conversions)

# other modification, additions

  - TemporalSoften: Planar RGB support
  - SeparateColumns: 10-16bit,float,RGB48/64
  - WeaveColumns: 10-16bit,float,RGB48/64,PlanarRGB(A)
  - SwapUV: YUVA support
  - ConvertToRGB32/64: copy alpha from YUVA
  - SeparateRows,SeparateFields: PlanarRGB(A),YUVA support
  - WeaveRows: PlanarRGB(A), YUVA
  - Weave (fields,frames): YUVA,PlanarRGB(A)
  - ConvertToPlanarRGB(A): 
    PlanarRGB <-> PlanarRGBA is now allowed
  - ConvertToPlanarRGB(A):
    YUY2 source is now allowed (through automatic ConvertToRGB proxy)
  - New CPU feature constants (cpuid.h and avisynth_c.h)
    Detect FMA4 and AVX512F,DQ,PF,ER,CD,BW,VL,IFMA,VBMI
    See: https://github.com/pinterf/AviSynthPlus/blob/MT/avs_core/include/avs/cpuid.h
    Constants for AVX/AVX2/FMA3/F16C... were already defined in previous releases
    Note that for live AVX and AVX2 proper OS support is needed:
      64 bit OS (Avisynth+ can be x86 or x64)
      Windows7 SP1 or newer
    On a non-supported OS AVX or better processor features are not even reported to plugins through GetCPUInfo
  - BitBlt in 32 bit Avisynth: 
    for processors with AVX or better ignore tricky isse memcpy replacement, trust in memcpy (test)
    (x64 is O.K., it always used memcpy)
  - VDubFilter.dll:
    Avisynth allows using filters written for Virtualdub (? version) with similar parameters that Avisynth uses
    New: allow parameters 'd' and 'l' types
    Avisynth+ converts 'd' double and 'l' long typed parameters to 'f' float and 'i' int as
    such types are non-existant in Avisynth
  - Internals: add SubframePlanarA to IScriptEnvirontment2 for frames with alpha plane
    General note: unlike IScriptEnvironment (that is rock solid for the time beeing), IScriptEnvironment2 is still not final.
    It is used within Avisynth+ core, but is also published in avisynth.h.
    It contains avs+ specific functions, that could not be stuffed into IScriptEnvironment without killing compatibility.
    Although it changes rarely, your plugin may not work with Avisynth+ versions after a change

#################
# VfW interface for high bit depth colorspaces
Output high bit depth formats on VfW interface.

One example is Virtualdub Deep Color mod, forum: https://forum.doom9.org/showthread.php?p=1795019
that can accept high bit depth avisynth plus script natively.

Non-existent formats will automatically converted to an existing one:
- 12, 14 and float YUV formats to 16 bit for 4:2:0 and 4:2:2
  Note: OPT_Enable_Y3_10_16 is still a valid override option
- 10, 12, 14 and float YUV formats to 16 bit for 4:4:4 (e.g. YUV444P10->YUV444P16)
  if alpha channel is present (YUVA), it will we copied, else filled with FFFF
- 10, 12, 14 and float planar RGB formats to RGB64
  Conversion of 8 bit planar RGB formats to RGB24
  Conversion of 8 bit planar RGBA formats to RGB32
  Note: Planar RGB autoconvert is triggered when global Avisynth variable Enable_PlanarToPackedRGB is true 

Note: 
    use OPT_VDubPlanarHack=true for YV16 and YV24 for older versions of VirtualDub
    when you experience swapped U and V planes
    
Supported formats:
    BRA[64],b64a,BGR[48],P010,P016,P210,P216,Y3[10][10],Y3[10][16],v210,Y416
    G3[0][10], G4[0][10], G3[0][12], G4[0][12], G3[0][14], G4[0][14], G3[0][16], G4[0][16]
    
Default format FourCCs:
    Avisynth+ will report these FourCC-s, override them with defining OPT_xxx global variables

    RGB64: BRA[64]
    RGB48: BGR[48]
    YUV420P10: P010
    YUV420P16: P016
    YUV422P10: P210
    YUV422P16: P216
    YUV444P16 and YUVA444P16: Y416
    Planar RGB  10,12,14,16 bits: G3[0][10], G3[0][12], G3[0][14], G3[0][16]
    Planar RGBA 10,12,14,16 bits: G4[0][10], G4[0][12], G4[0][14], G4[0][16]

Global variables to override default formats:
    Put them at the beginning of avs script.

    OPT_Enable_V210 = true --> v210 for YUV422P10
    OPT_Enable_Y3_10_10 = true --> Y3[10][10] for YUV422P10
    OPT_Enable_Y3_10_16 = true --> Y3[10][16] for YUV422P16
    OPT_Enable_b64a = true --> b64a for RGB64
    Enable_PlanarToPackedRGB = true --> RGBP8->RGB24, RGBAP8->RGB32, all other bit depths to RGB64

# Functions, Filters
[Overlay]
  - Conversionless "blend"/"luma"/"chroma" for Y, 4:2:0, 4:2:2, 4:4:4 and RGB clips
    Input clips are not converted automatically to 4:4:4 (YV24 for 8 bits) then back
    If you want to keep conversion, specify parameter use444=true
    Packed RGB (RGB24/32/48/64) are losslessly converted to and from planar RGB internally.
    That also means that RGB source clip is not converted to YUV for processing anymore.

    In this mode Overlay/mask automatically follows input clip format.
    For compatibility: when greymask=true (default) and mask is RGB then mask source is the B channel
  - blend for float formats

[Histogram]
    "levels" mode for RGB (Planar and RGB24/32/48/64 input)
    Color legends show R, G and B channels instead of Y, U and V

    Reminder: 
      Histogram "levels" and "Classic" allows bits=xx parameter, xx=8..12
      Original display is drawn for 8 bits wide resolution.
      Now you can draw with 9..12 bits precision. Get a wide monitor though :)

[ConvertBits]: new parameters, possibly for the future.
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
   
[AddAlphaPlane]

Full Syntax:
    AddAlphaPlane(clip c, [int mask]) or 
    AddAlphaPlane(clip c, [clip mask]) 

Adds and/or sets alpha plane
    YUV->YUVA, RGBP -> RGBAP, RGB24->RGB32, RGB48->RGB64
    If the current video format already has alpha channel and mask is provided, then the alpha plane will be overwritten with the new mask value.

Initialize alpha plane with value
    If optional mask parameter is supplied, the alpha plane is set to this value.
    Default: maximum pixel value of current bit depth (255/1023/4095/16383/65535/1.0)

Initialize alpha plane from clip
    Function can accept initializer clip with Y-only or alpha (YUVA/PRGBA/RGB32/64) for alpha source
    New alpha plane is copied from the greyscale clip or from the alpha channel.

[RemoveAlphaPlane]
    Stripes alpha plane from clip
    YUVA->YUV, RGBAP->RGBP, RGB32->RGB24, RGB64->RGB48

[Info]
    Info display was made a bit more compact. 
    Bit depth info moved after color space info
    Does not display pre-SSE2 CPU flags when at least AVX is available
    Display AVX512 flags in separate line (would be too long)

    Reminder: Info() now has more parameters than is classic Avisynth:
      Info(): c[font]s[size]f[text_color]i[halo_color]i
    Added font (default Courier New), size (default 18), text_color and halo_color parameters, similar to (but less than) e.g. in ShowFrameNumber.

[ReplaceString]
New script function:
    string ReplaceStr(string s, string pattern, string replacement)
    Function is case sensitive, parameters are unnamed

[NumComponents]
New script function:
    int NumComponents(clip)
    returns 1 for greyscale, 3 for YUVxxx, YUY2, planar RGB or RGB24/RGB48, 4 for YUVAxxx, Planar RGBA or RGB32/64
    Same as VideoInfo::NumComponents()

[HasAlpha]
New script function: 
    bool HasAlpha(clip)
    returns true when clip is YUVA, Planar RGBA, or packed RGB32 or RGB64

[Plane extraction shortcuts]
New functions: 
  ExtractR, ExtractG, ExtractB,
  ExtractY, ExtractU, ExtractV
  ExtractA

Functions extract the specified plane to a greyscale clip.

For Y,U and V the function they work the same way as UToY8 and VToY8 and ConvertToY/ConvertToY8 did.
All colorspaces are accepted.
Although packed RGB formats (RGB24/32/48/64) are not planar, they can be used.
They are converted to planar RGB(A) on-the-fly before plane extraction

[YToUV addition]
  YToUV accepts optional alpha clip after Y clip

  old: YToUV(clip clipU, clip clipV [, clip clipY ] ) 
  new: YToUV(clip clipU, clip clipV [, clip clipY [, clip clipA] ] ) 

  Example
    U = source.UToY8()
    V = source.VToY8()
    Y = source.ConvertToY()
    A = source.AddAlphaPlane(128).AToY8()
    # swaps V, U and A, Y
    YToUV(V,U,A,Y).Histogram("levels").Info().RemoveAlphaPlane()
  
[CombinePlanes]
New function for arbitrary mixing planes from up to four input clips.

  CombinePlanes(clip1 [,clip2, clip3, clip4], string planes [, string source_planes, string pixel_type, string sample_clip])

  Combines planes of source clip(s) into a target clip

  If sample_clip is given, target clip properties are copied from that clip
  If no sample_clip is provided, then clip1 provides the template for target clip
  An optional pixel_type string (e.g."YV24", "YUV420PS", "RGBP8") can override the base video format.

  If the source clip count is less than the given planes defined, then the last available clip is 
  used as a source.

  Target planes are set to default RGBP(A)/YUV(A), when all input clips are greyscale and no source planes are given
  Decision is made by the target plane characters, if they are like R,G,B then target video format will be planar RGB
  Same logic applies for YUV.
    Example:
    Y1, Y2 and Y3 are greyscale clips
    Old, still valid: combineplanes(Y1, Y2, Y3, planes="RGB", source_planes="YYY", pixel_type="RGBP8")
    New:              combineplanes(Y1, Y2, Y3, planes="RGB") # result: Planar RGB

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

  Remark:  
    When there is only one input clip, zero-cost BitBlt-less subframes are used, which is much faster.

    e.g.: casting YUV to RGB, shuffle RGBA to ABGR, U to Y, etc..
    Target planes that are not specified, preserve their content.

    Examples:
    combineplanes(clipRGBP, planes="RGB",source_planes="BGR") # swap R and B
    combineplanes(clipYUV, planes="GBRA",source_planes="YUVA",pixel_type="RGBAP8") # cast YUVA to planar RGBA
    combineplanes(clipYUV, planes="Y",source_planes="U",pixel_type="Y8") # extract U


Earlier (pre v2294) modifications and informations on behaviour of existing filter
----------------------------------------------------------------------------------
[ColorSpaceNameToPixelType]
New script function: ColorSpaceNameToPixelType()
Parameter: video colorspace string
Returns: Integer

Returns a VideoInfo::pixel_type integer from a valid colorspace string

In Avisynth+ we have way too many pixel_type's now, this function can be useful for plugins for parsing a target colorspace string parameter.

Earlier I made this function available from within avisynth core, as I made one function from the previous 3-4 different places where colorspace name parameters were parsed in a copy-paste code.

In Avisynth the existing PixelType script function returns the pixeltype name of the current clip.
This function reverses this.

It has the advantage that it returns the same (for example) YV24 constant from "YV24" or "YUV444" or "Yuv444p8", so it recognizes some possible naming variants.

csp_name = "YUV422P8"
csp_name2 = "YV16"
SubTitle("PixelType value of " + csp_name + " = " + String(ColorSpaceNameToPixelType(csp_name))\
+ " and " + csp_name2 + " = " + String(ColorSpaceNameToPixelType(csp_name2)) )

[New conditional functions]

Conditional runtime functions have 10-16 bit/float support for YUV, PlanarRGB and 16 bit packed RGB formats.

Since RGB is also available as a planar colorspace, the plane statistics functions logically were expanded.

New functions
• AverageR, AverageG AverageB like AverageLuma
• RDifference, GDifference, BDifference like LumaDifference(clip1, clip2)
• RDifferenceFromPrevious, GDifferenceFromPrevious, BDifferenceFromPrevious
• RDifferenceToNext, GDifferenceToNext, BDifferenceToNext
• RPlaneMin, GPlaneMin BPlaneMin like YPlaneMin(clip [, float threshold = 0, int offset = 0])
• RPlaneMax, GPlaneMax BPlaneMax like YPlaneMax(clip [, float threshold = 0, int offset = 0])
• RPlaneMinMaxDifference, GPlaneMinMaxDifference BPlaneMinMaxDifference like YPlaneMinMaxDifference(clip [, float threshold = 0, int offset = 0])
• RPlaneMedian, GPlaneMedian, BPlaneMedian like YPlaneMedian(clip [, int offset = 0])

For float colorspaces the Min, Max, MinMaxDifference and Median functions populate pixel counts for the internal statistics at a 16 bit resolution internally. 

[Tweak] 
See original doc: http://avisynth.nl/index.php/Tweak
The original 8 bit tweak worked with internal LUT both for luma and chroma conversion.
Chroma LUT requires 2D LUT table, thus only implemented for 10 bit clips for memory reasons.
Luma LUT is working at 16 bits (1D table)

Above these limits the calculations are realtime, and done pixel by pixel.
You can use a new parameter to force ignoring LUT usage (calculate each pixel on-the-fly)
For this purpose use realcalc=true.

[MaskHS]
Works for 10-16bit,float. 
 
MaskHS uses LUT for 10/12 bits. Above this (no memory for fast LUTs) the calculation is done realtime for each.
To override LUT for 10-12 bits use new parameter realcalc=true

[ColorKeyMask]: 
Works for RGB64, Planar RGBA 8-16,float.
ColorKeyMask color and tolerance parameters are the same as for 8 bit RGB32.
Internally they are automatically scaled to the current bit-depth

[ResetMask] 
New extension.
Accepts parameter (Mask=xxx) which is used for setting the alpha plane for a given value. 
Default value for Mask is 255 for RGB32, 65535 for RGB64 and full 16 bit, 1.0 for float. 
For 10-12-14 bit it is set to 1023, 4095 and 16383 respectively.

Parameter type is float, it can be applied to the alpha plane of a float-type YUVA or Planar RGBA clip.

[Layer] 
Layer() now works for RGB64.
Original documentation: http://avisynth.nl/index.php/Layer

By avisynth documentation: for full strength Level=257 (default) should be given.
For RGB64 this magic number is Level=65537 (this is the default when RGB64 is used)

Sample:
lsmashvideosource("test.mp4", format="YUV420P8")
x=last
x = x.Spline16Resize(800,250).ColorYUV(levels="TV->PC")
x = x.ConvertToRGB32()
 
transparency0_255 = 128 # ResetMask's new parameter. Also helps testing :)
x2 = ColorBars().ConvertToRGB32().ResetMask(transparency0_255)
 
x_64 = x.ConvertToRGB32().ConvertBits(16)
x2_64 = ColorBars().ConvertToRGB32().ConvertBits(16).ResetMask(transparency0_255 / 255.0 * 65535.0 )
 
#For pixel-wise transparency information the alpha channel of an RGB32 overlay_clip is used as a mask. 
 
op = "darken" # subtract lighten darken mul fast
level=257         # 0..257
level64=65537     # 0..65537
threshold=128                   # 0..255   Changes the transition point of op = "darken", "lighten." 
threshold64=threshold*65535/255 # 0..65535 Changes the transition point of op = "darken", "lighten." 
use_chroma = true 
rgb32=Layer(x,x2,op=op,level=level,x=0,y=0,threshold=threshold,use_chroma=use_chroma )
rgb64=Layer(x_64,x2_64,op=op,level=level64,x=0,y=0,threshold=threshold64,use_chroma=use_chroma ).ConvertBits(8)
StackVertical(rgb32, rgb64)

[Levels]
Levels: 10-16 bit support for YUV(A), PlanarRGB(A), 16 bits for RGB48/64
No float support yet

Level values are not scaled, they are accepted as-is for 8+ bit depths

Test scripts for Levels
# Gamma, ranges (YUV):
x=ConvertToYUV420()
dither=true
coring=true
gamma=2.2
output_low = 55
output_high = 198
clip8 = x.Levels(0, gamma, 255, output_low, output_high , dither=dither, coring=coring)
clip10 = x.ConvertBits(10).Levels(0,gamma,1023,output_low *4,(output_high +1)*4 - 1, dither=dither, coring=coring)
clip16 = x.ConvertBits(16).Levels(0,gamma,65535,output_low *256,(output_high+1) *256 -1,dither=dither, coring=coring)
stackvertical(clip8.Histogram("levels"), clip10.ConvertBits(8).Histogram("levels"), Clip16.ConvertBits(8).Histogram("levels"))

# packed RGB 32/64
xx = ConvertToRGB32()
dither=false
coring=false
gamma=1.4
clip8 = xx.Levels(0, gamma, 255, 0, 255, dither=dither, coring=coring)
clip16 = xx.ConvertBits(16).Levels(0,gamma,65535,0,65535,dither=dither, coring=coring)
stackvertical(clip8.ConvertToYUV444().Histogram("levels"), Clip16.ConvertBits(8).ConvertToYUV444().Histogram("levels"))

[ColorYUV] 
Now it works for 10-16 bit clips

• Slightly modified "demo" mode when using ColorYUV(showyuv=true) 

#old: draws YV12 with 16-239 U/V image (448x448)
#new: draws YV12 with 16-240 U/V image (450x450)
 
• New options for "demo" mode when using ColorYUV(showyuv=true) 
New parameter: bool showyuv_fullrange.
Description: Draws YV12 with 0-255 U/V image (512x512)
Usage: ColorYUV(showyuv=true, showyuv_fullrange=true)
 
New parameter: bits=10,12,14,16 
Result clip is the given bit depth for YUV420Pxx format.
As image size is limited (for 10 bits the range 64-963 requires big image size), color resolution is 10 bits maximum.
#This sample draws YUV420P10 with 64-963 U/V image
ColorYUV(showyuv=true, bits=10).Info()
 
Luma steps are 16-235-16../0-255-0.. up to 0-65535-0... when bits=16
 
• Additional infos for ColorYUV

- Fixed an uninitialized internal variable regarding pc<->tv conversion, 
  resulting in clips sometimes were expanding to pc range when it wasn't asked.
- No parameter scaling needed for high bit depth clips.
  For 8+ bit clips parameter ranges are the same as for the 8 bit clips.
  They will be scaled properly for 10-16 bitdepths.
  e.g. off_u=-20 will be converted to -204 for 10 bits, -20256 for 16 bits
- ColorYUV uses 8-10-12-14-16 bit lut.
- ColorYUV is not available for 32 bit (float) clips at the moment

[Other things you may have not known]
Source filters are automatically detected, specifying MT_SERIALIZED is not necessary for them.

[Known issues/things]
GRunT in MT modes (Avs+ specific)
[done: v2502] Overlay blend with fully transparent mask is incorrect, overlaying pixel=0 becomes 1, overlaying pixel=255 becomes 254.
[done: v2676-] Float-type clips: chroma should be zero based: +/-0.5 instead of 0..1
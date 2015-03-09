
TDeint
======


Abstract
--------

| **author:** tritical
| **version:** v1.0 Final
| **download:** `<http://bengal.missouri.edu/~kes25c/>`_
| **category:** Deinterlacing & Pulldown Removal
| **requirements:**

-   YV12 & YUY2 Colorspace

**license:** GPL

--------


Description
-----------

TDeint is a bi-directionally, motion adaptive, sharp deinterlacer. It can
adaptively choose between using per-field and per-pixel motion adaptivity,
and can use cubic interpolation, kernel interpolation (with temporal
direction switching), or one of two forms of modified ELA interpolation which
help to reduce "jaggy" edges in moving areas where interpolation must be
used. TDeint also supports user overrides through an input file, and can act
as a smart bobber or same frame rate deinterlacer, as well as an IVTC post-
processor.


Syntax
------

``TDeint`` (clip, int "mode", int "order", int "field", int "mthreshL", int
"mthreshC", int "map", string "ovr", int "ovrDefault", int "type", bool
"debug", int "mtnmode", bool "sharp", bool "hints", PClip "clip2", bool
"full", int "cthresh", bool "chroma", int "MI", bool "tryWeave", int "link",
bool "denoise", int "AP", int "blockx", int "blocky", int "APType", PClip
"edeint", PClip "emask", float "blim", int "metric", int "expand", int
"slow", int "opt")


PARAMETERS
----------

**mode:**

Sets the mode of operation. Modes -2 and -1 require progressive input.

- -2 - double height using modified ELA
- -1 - double height using modified ELA-2
- 0 - same rate output
- 1 - double rate output (bobbing)
- 2 - smartbobbed field-matching (same rate output, blend frames from
  bobbed stream)

default - 0 (int)

**order:**

Sets the field order of the video.

- -1 - use parity from Avisynth
- 0 - bottom field first (bff)
- 1 - top field first (tff)

default - -1 (int)

**field:**

When in mode 0 and 2, this sets the field to be interpolated. When in mode 1,
this setting does nothing.

- -1 - will set field equal to order if hints = false or to 0 if hints = true
- 0 - interpolate top field (keep bottom field)
- 1 - interpolate bottom field (keep top field)

default - -1 (int)

**mthreshL/mthreshC:**

The motion thresholds for luma and chroma (mthreshL for luma, mthreshC for
chroma). If the difference between two pixels is less than this value they
are declared static. Smaller values will reduce residual combing, larger
values will decrease flicker and increase the accuracy of field construction
in static areas. The spatially corresponding parts of the luma and chroma
planes are linked (if link != 0), so mthreshC and mthreshL may be somewhat
interconnected. Setting both values to 0 or below will disable motion
adaptation (i.e. every pixel will be declared moving) allowing for a dumb
bob.

- default:

  - mthreshL - 6 (int)
  - mthreshC - 6 (int)

**map:**

Displays an output map instead of the deinterlaced frame. There are three
possible options.

**Note: the maps will not be displayed if the current frame is not being
deinterlaced due to overrides, hints, full=false, or tryWeave=true.**

**AP post-processing is currently not taken into account when using map = 1
or 2.**

- 0 - No map.
- 1 - value (binary) map. This will output a frame in which all the
  pixels have one of the following values (indicating how the frame is to
  be constructed):

  - 0 (use pixel from current frame)
  - 51 (use pixel from previous frame)
  - 102 (use pixel from next frame)
  - 153 (use average of curr/next)
  - 204 (use average of curr/prev)
  - 230 (use [1 2 1] average of prev/curr/next)
  - 255 (interpolate pixel)

- 2 - Merged map. This will output a frame in which all the static
  parts of the frame (values 0, 51, 102, 153, 204, and 230 from map=1) have
  been constructed as they would appear in the deinterlaced frame, and the
  pixels that are to be interpolated are marked in white.

default - 0 (int)

**ovr:**

Sets the name and path to an overrides file. When mode=0, an overrides file
can be used to control the values of mthreshL, mthreshC, field, order, and
type for single frames or for ranges of frames, as well as control which
frames are deinterlaced. When mode=1, an overrides file can be used to
control the values of mthreshL, mthreshC, and type for specific frames or
ranges of frames.

Overrides file specifiers:

- ``+`` = mark frame to be deinterlaced (only useful if ovrDefault = 1)
- ``-`` = mark frame to not be deinterlaced
- ``f`` = field
- ``o`` = order
- ``l`` = mthreshL
- ``c`` = mthreshC
- ``t`` = type

::

    *The c, f, o, l, t specifiers also require a change value to be
    specified when they are used (look at the overrides syntax to see how
    this is done)

Override syntax:

    [] = not required for ``+``, ``-`` specifiers

single frame override:
frame_number specifier [change_value]
examples:

::

    245 f 1
    345 +
    400 -
    450 c -1



override for range of frames:
start_frame_number,end_frame_number specifier [change_value]
examples:

::

    100,200 +
    346,352 f 0
    900,1200 l 5



**The range is inclusive, meaning the end frame and start frame are both
included.**

Pattern based frame range overrides (only for +,- specifiers):
examples:

::

    100,300 +-+++--+++
    400,456 ---+---++



**Will use the given pattern over the specified frame range.**

Things to remember (key points/rules):

1.  Ranges are inclusive
2.  When mode = 1 (bobbing) all overrides except for mthreshL/mthreshC,
    and type overrides are ignored. Also, frame #'s correspond to the input
    clip not the output clip, thus one frame will be two frames in the
    output.
3.  The changed value is always set back to what it was originally set to
    after the override goes out of the specified range. (i.e. if you specify
    an mthresh override for frame 600 to 700 after frame 700 mthresh is set
    back to its original value automatically, you don't need to set it back
    in the overrides file! The original value is what it is set to on load
    (i.e. either the default or what you set it to in your avisynth script).
4.  Frame numbers must be within range for the file.
5.  Frame numbers for specific specifiers must be ascending (if they are
    not, the last entry in the file takes precedence ex. if you specify
    300,400 c 10 then later do 350,450 c 12 frames 350 to 400 will use 12 not
    10).
6.  Frames numbers for the (+, -) specifiers cannot overlap (e.g. don't
    do 300,400 - and then later in the file write 350,500 + or strange things
    will happen. The other specifiers don't have to meet this requirement as
    they all effect different things.
7.  +, - specifiers require no change value.
8.  The spacing is important! Just look at the examples.
9.  Only +, - specifiers can be used in pattern specifications.
10. You can change multiple specifiers over the same frame range as long
    as you follow the rules above (+, - ascending frame numbers for example).
11. You can comment out a line (i.e. it will be ignored) by adding a '#'
    or ';' to the beginning of the line.
12. Entering 0 as the end_frame for a range of frames is taken as meaning
    the last frame of the video.

Example overrides file:

Syntax example => TDeint(order=1,ovr="c:\path\myoverridesfile.txt")

::

    100,300 o 0
    100,300 f 1
    90,250 c 3
    40,500 -
    505 -
    300,700 l -1
    #700,3000 f 1 <- commented out, will be ignored
    800,1000 -++-
    500,1000 c 13


default - ``""`` (string)

**ovrDefault:**

When using an overrides file in mode 0, this specifies the default action for
all frames in the video. Using ovrDefault=1 makes it easy to deinterlace only
a few specific frames in a video. When mode = 1, this setting does nothing.

- 0 - all frames not specified as '-' in the overrides file are deinterlaced
- 1 - all frames not specified as '+' in the overrides file are not
  deinterlaced and simply returned as is

default - 0 (int)

**type:**

Sets the type of interpolation to use. Cubic is the fastest, modified ELA and
ELA2 will give smoother, less "jaggy", edges and are the slowest (ELA2 is
faster), and kernel interpolation will cause significantly less flickering
than cubic or ela when interpolation gets used in almost static areas.
Modified ELA and ELA2 work best with anime/cartoon type material... they are
not that great with real life sources (sometimes they are, test for
yourself).

- 0 - cubic interpolation
- 1 - modified ELA interpolation
- 2 - kernel interpolation (can be normal or sharp, controlled by the
  sharp setting)
- 3 - modified ELA-2 interpolation
- 4 - blend interpolation

default - 2 (int)

**debug:**

Will enable debug output, which for each frame will list the values of order,
field, mthreshL, mthreshC, and type if the frame is being deinterlaced. If
the frame is not being deinterlaced (due to user overrides, hints, or
full=false), it will simply say the frame is not being deinterlaced and list
the specific reason. If the output frame is weaved, then debug output will
report which field the current field was weaved with (PREV or NEXT). The
debug information is output using OutputDebugString(). To view the output you
can use `DebugView`_ from Sysinternals.

default - false (bool)

**mtnmode:**

Controls whether a 4 field motion check or a 5 field motion check is used. 5
field will prevent more artifacts and can deal with duplicate interlaced
frames; however, it is quite a bit slower than the 4 field motion check.
Modes 2 and 3 are like 0 and 1 except that in areas where an average of the
prev and next field would have been used in mode 0 or 1, the pixel value from
the most similar field (computed via field differencing) is used instead
(i.e. no averages are used).

- 0 - 4 field check
- 1 - 5 field check
- 2 - 4 field check (no averages, replace with most similar field)
- 3 - 5 field check (no averages, replace with most similar field)

default - 1 (int)

**sharp:**

Controls whether the sharp or normal kernel is used when using kernel
interpolation (type = 2). The sharp kernel includes more pixels and produces
a sharper result but is slightly slower.

- true - use sharp kernel
- false - use normal kernel

default - true (bool)

**hints:**

Read hints from telecide or tfm indicating which frames are interlaced and
which are not if hints are present in the video stream. To make this work you
need to set post=1 in telecide or PP=1 in tfm and put TDeint immediately
afterwards. TDeint will not effect the hints (as long as your video has a
width of at least 64 pixels) in case any filters later on need to read them.
If hints is set to true, but no hints from telecide or tfm are detected in
the video stream, then all frames will be deinterlaced (TDeint will operate
as if hints=false). If you do not specify a value for hints explicitly, then
TDeint will check to see if hints are present in the stream on load and set
hints to true if they are or false if they aren't (i.e. it is automatically
set).

**NOTE: for IVTC post-processing by reading hints it is recommended to use
TDeint in the following fashion making use of the clip2 parameter.**

::

    orig = last
    fieldmatcher()
    TDeint(clip2 = orig)


- true - read hints if present
- false - don't read hints

default - automatically detected on load (bool)

**clip2:**

If using tdeint as a postprocessor for telecide or tfm via the hints
parameter (or any field matcher), incorrect deinterlacing can occur due to
the fact that telecide changes the order of the fields in the original stream
(it is a field matcher after all). This can cause problems in some cases
since TDeint really needs to have the original stream. To work around this,
you can specify a second clip "clip2" for TDeint to do the actual
deinterlacing from.

In a script this is how it would work:

::

    mpeg2source("c:\mysource.d2v")
    orig = last
    telecide(guide=1, order=1, hints=true, post=1)
    tdeint(order=1, clip2=orig)


So TDeint reads the output clip from telecide as usual. When hints indicate
an interlaced frame, it does the deinterlacing of the frame using clip2. This
method also perserves the hints in the output stream so any other filters
that need them later on will still work.

With the addition of full=false, another way to use TDeint as a post-
processor is to have it use its own combed frame detection as follows (this
also allows it to work with any field matcher, not just telecide or tfm):

::

    mpeg2source("c:\mysource.d2v")
    orig = last
    fieldmatcherofchoice()
    tdeint(order=1, full=false, clip2=orig)


default - NULL (PClip)

**full:**

If full is set to true, then all frames are processed as usual. If
full=false, all frames are first checked to see if they are combed. If a
frame isn't combed, then it is returned as is. If a frame is combed, then it
is processed as usual. The parameters that effect combed frame detection are
cthresh, chroma, blockx, blocky, and MI. full=false allows TDeint to be an
ivtc post-processor without the need for hints.

- true - normal processing
- false - check all input frames for combing first

default - true (bool)

**cthresh:**

Area combing threshold used for combed frame detection. It is like dthresh or
dthreshold in telecide() and fielddeinterlace(). This essentially controls
how "strong" or "visible" combing must be to be detected. Good values are
from 6 to 12. If you know your source has a lot of combed frames set this
towards the low end (6-7). If you know your source has very few combed frames
set this higher (10-12). Going much lower than 5 to 6 or much higher than 12
is not recommended.

default - 6 (int)

**blockx:**

Sets the x-axis size of the window used during combed frame detection. This
has to do with the size of the area in which MI number of pixels are required
to be detected as combed for a frame to be declared combed. See the MI
parameter description for more info. Possible values are any number that is a
power of 2 starting at 4 and going to 2048 (e.g. 4, 8, 16, 32, ... 2048).

default - 16 (int)

**blocky:**

Sets the y-axis size of the window used during combed frame detection. This
has to do with the size of the area in which MI number of pixels are required
to be detected as combed for a frame to be declared combed. See the MI
parameter description for more info. Possible values are any number that is a
power of 2 starting at 4 and going to 2048 (e.g. 4, 8, 16, 32, ... 2048).

default - 16 (int)

**chroma:**

Includes chroma combing in the decision about whether a frame is combed. Only
use this if you have one of those weird sources where the chroma can be
temporally separated from the luma (i.e. the chroma moves but the luma
doesn't in a field). Otherwise, it will just help to screw up the decision
most of the time.

- true - include chroma combing
- false - don't

default - false (bool)

**MI:**

The number of required combed pixels inside any of the blockx by blocky sized
blocks on the frame for the frame to be considered combed. While cthresh
controls how "visible" or "strong" the combing must be, this setting controls
how much combing there must be in any localized area (a blockx by blocky
sized window) on the frame. Min setting = 0, max setting = blockx x blocky
(at which point no frames will ever be detected as combed).

default - 64 (int)

**tryWeave:**

If set to true, when TDeint deinterlaces a frame it will first calculate
which field (PREV or NEXT) is most similar to the current field. It will then
weave this field to create a new frame and check this new frame for combing.
If the new frame is not combed, then it returns it. If it is, then it
deinterlaces using the usual per-pixel motion adaptation. Basically, this
setting allows TDeint to try to use per-field motion adaptation instead of
per-pixel motion adaptation where possible.

default - false (bool)

**link:**

Controls how the three planes (Y, U, and V) are linked during comb map
creation. Possible settings:

- 0 - no linking
- 1 - Full linking (each plane to every other)
- 2 - Y to UV (luma to chroma)
- 3 - UV to Y (chroma to luma)

default - 2 (int)

**denoise:**

Controls whether the comb map is denoised or not. True enables denoising,
false disables.

default - false (bool)

**AP:**

Artifact protection threshold. If AP is set to a value greater than or equal
to 0, then before outputting a deinterlaced frame TDeint will scan all weaved
pixels to see if any create a value greater than AP. Any pixels that do will
be interpolated. Use this to help prevent very obvious motion adaptive
related artifacts. A large value for AP is recommended (25+, or as large as
removes the artifacts that can be seen during full-speed playback), as
smaller values will destroy the benefits of motion adaptivity in static,
detailed areas. The AP metric is the same as the cthresh metric... so the
scale is 0-255. At zero everything but completely flat areas will be detected
as combing. At 255 nothing will be detected. Using AP will slow down
processing. Set AP to a value less than 0 or greater than 254 to disable.

default - -1 (disabled) (int)

**APType:**

When AP post-processing is being used (AP is set >= 0 and < 255), APType
controls whether the motion of surrounding pixels should be taken into
account. There are 3 possible settings:

- 0 = Don't take surrounding motion into account. If a weaved pixel
  creates a value that exceeds the AP threshold then it will be
  interpolated.
- 1 = If a weaved pixel creates a value that exceeds the AP threshold
  and at least half of pixels in a 5x5 window centered on that pixel were
  detected as moving then that pixel will be interpolated.
- 2 = Exactly like 1, except instead of 1/2 only 1/3 of the pixels in
  the surrounding 5x5 window must have been detected as moving.

Modes 1 and 2 provide a way to catch more artifacts (low AP values) without
completely sacrificing static areas.

default - 1 (int)

**edeint:**

Allows the specification of an external clip from which to take interpolated
pixels instead of having TDeint use one of its internal interpolation
methods. If a clip is specified, then TDeint will process everything as usual
except that instead of computing interpolated pixels itself it will take the
needed pixels from the corresponding spatial positions in the same frame of
the edeint clip. To disable the use of an edeint clip simply don't specify a
value for edeint.

default - NULL (PClip)

**emask:**

Allows the specification of an external clip from which to take the motion
mask instead of having TDeint build the mask internally. Using this option
makes the following parameters of TDeint have no effect: mthreshL, mthreshC,
mtnmode, denoise, link. The possible values that can be present in the motion
mask frames are defined as follows:

- 10 - Use pixel from current frame
- 20 - Use pixel from previous frame
- 30 - Use pixel from next frame
- 40 - Use avg of pixels from current and next
- 50 - Use avg of pixels from current and previous
- 60 - Interpolate
- 70 - Use [1 2 1] blend of pixels from prev/curr/next

Behavoir is undefined for other values, but they should end up being treated
internally as though they were 60.

default - NULL (PClip)

**blim:**

Sets the maximum difference value for mode 2. If both differences (src-prev
and src-next) are above this value then src is returned as is. Otherwise, src
is blended with either prev or next depending on which is most similar to
src. This value is on a 0.0 to 100.0 scale based on luma plane difference.
Use debug=true to see the difference values generated and the limit value.
The debug output will look like the following:

::

    [5776] TDeint:  frame 0:  d1 = 0  d2 = 0  lim = 1513728

d1 is the src-prev difference and d2 is the src-next difference. lim is the
maximum value translated from the float value into an unsigned long value.
Set blim to a negative value to disable checking (src will always be blended
with either prev or next).

default - -2.0 (float)

**metric:**

Sets which spatial combing metric is used to detect combed pixels. Possible
options:

Assume 5 neighboring pixels (a,b,c,d,e) positioned vertically.

| ``a``
| ``b``
| ``c``
| ``d``
| ``e``


::

    0:  d1 = c - b;
        d2 = c - d;
        if ((d1 > cthresh && d2 > cthresh) || (d1 < -cthresh && d2 < -cthresh))
        {
           if (abs(a+4*c+e-3*(b+d)) > cthresh*6) it's combed;
        }

    1:  val = (b - c) * (d - c);
        if (val > cthresh*cthresh) it's combed;


Metric 0 is what tdeint always used previous to v1.0 RC7. Metric 1 is the
combing metric used in Donald Graft's FieldDeinterlace()/IsCombed() funtions
in decomb.dll.

default - 0 (int)

**expand:**

Sets the number of pixels to expand the comb mask horizontally on each side
of combed pixels. Basically, if expand is greater than 0 then TDeint will
consider all pixels within 'expand' distance horizontally of a detected
combed pixel to be combed as well.

default - 0 (int)

**slow:**

Sets which field matching function is used. These functions match the
corresponding functions in tfm. Possible values:

- 0 - normal (should have the worst accuracy)
- 1 - slower
- 2 - slowest (should have the best accuracy)

default - 1 (int)

**opt:**

Controls which cpu optimizations are used. Possible settings:

- 0 - use c routines
- 1 - use mmx routines
- 2 - use isse routines
- 3 - use sse2 routines
- 4 - auto detect

default - 4 (int)

--------


Example Scripts
---------------

**Same rate deinterlacing:**

::

    mpeg2source()
    tdeint()

**Bobbing:**

::

    mpeg2source()
    tdeint(mode=1)

**Deinterlacing with EEDI2 for interpolation:**

::

    mpeg2source()
    interp = separatefields().selecteven().eedi2()
    tdeint(edeint=interp)

**Bobbing with EEDI2 for interpolation:**

::

    mpeg2source()
    interp = separatefields().eedi2(field=-2)
    tdeint(mode=1,edeint=interp)

**Smartbobbed field-matching (same rate deinterlacing via blending of bobbed
frames):**

::

    mpeg2source()
    tdeint(mode=2)

**Smartbobbed field-matching with EEDI2 for interpolation:**

::

    mpeg2source()
    interp = separatefields().eedi2(field=-2)
    tdeint(mode=2,edeint=interp)

--------

+----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| Changelog                                                                                                                                                                                    |
+=============+============+===================================================================================================================================================================+
| v1.0 Final  | 10/16/2006 | - added blend deinterlacing option (type = 4)                                                                                                                     |
|             |            | - changed denoise default to false                                                                                                                                |
|             |            | - pixels detected as moving, but with absolute difference < 4 to both vertical neighbors are no longer automatically weaved (should fix problems with slow fades) |
+-------------+------------+-------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| v1.0 RC8    | 10/04/2006 | - added expand parameter                                                                                                                                          |
|             |            | - added slow parameter and slow=1/2 matching modes from tfm                                                                                                       |
|             |            | - fixed a typo causing mode 2 to crash with yuy2 input                                                                                                            |
+-------------+------------+-------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| v1.0 RC7    | 04/10/2006 | - optimized combed frame detection functions (now matches tivtc)                                                                                                  |
|             |            | - added second spatial combing metric and "metric" parameter (same as tfm and is/showcombeditvtc)                                                                 |
|             |            | - optimized denoise routines                                                                                                                                      |
|             |            | - improved the field comparison routine (now equal to slow=0 in tfm)                                                                                              |
|             |            | - mode 2 uses the field comparison routine instead of full frame subtract for determining the best matching frame (more accurate)                                 |
|             |            | - directly assign frames from emask clip (no need to copy)                                                                                                        |
|             |            | - changed blim default to -2.0 (disabled)                                                                                                                         |
|             |            | - call setcachehints for emask/edeint clips when used                                                                                                             |
+-------------+------------+-------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| v1.0 RC6    | 03/22/2006 | - optimized motion map and field comparison routines                                                                                                              |
|             |            | - added opt parameter                                                                                                                                             |
|             |            | - fixed missing cache in mode 2                                                                                                                                   |
+-------------+------------+-------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| v1.0 RC5    | 03/21/2006 | - fixed mode 2 mmx/isse subtract frames functions (contained paddq sse2 instruction)                                                                              |
+-------------+------------+-------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| v1.0 RC4    | 03/19/2006 | - output MIC values in debug info when tryweave=true or full=false                                                                                                |
|             |            | - added value 70 to emask input                                                                                                                                   |
|             |            | - added mmx versions of isse/sse2 compare/blend routines for mode=2                                                                                               |
|             |            | - refactored/rewrote a lot of the code to clean up and simply things, no changes that effect output... should give a slight speed up                              |
+-------------+------------+-------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| v1.0 RC3    | 03/18/2006 | - Added mode 2 and blim parameter                                                                                                                                 |
+-------------+------------+-------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| v1.0 RC2    | 12/18/2005 | - Added emask parameter                                                                                                                                           |
|             |            | - Fixed edeint not working correctly with mode=1                                                                                                                  |
|             |            | - Changed field=-1 operation when hints=false                                                                                                                     |
+-------------+------------+-------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| v1.0 RC1    | 12/03/2005 | - Added edeint parameter                                                                                                                                          |
+-------------+------------+-------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| v1.0 beta 4 | 08/14/2005 | - SetCacheHints call to diameter instead of radius                                                                                                                |
|             |            | - Fixed type=1 YUY2 interpolation routine giving messed up chroma output (bug was introduced in v1.0 beta 3)                                                      |
+-------------+------------+-------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| v1.0 beta 3 | 05/14/2005 | - Added APType parameter, adds 2 new AP post-processing modes that take surrounding motion into account                                                           |
|             |            | - Small changes (hopefully improvements) to type 3 (ELA-2) interpolation                                                                                          |
+-------------+------------+-------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| v1.0 beta 2 | 04/26/2005 | - Added modes -2 and -1... will upsize vertically by a factor of 2 using ELA or ELA2                                                                              |
|             |            | - Call SetCacheHints in filter constructor                                                                                                                        |
|             |            | - Some small optimizations, should give a very small speed up                                                                                                     |
+-------------+------------+-------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| v1.0 beta 1 | 04/23/2005 | - Added AP threshold and post-processing                                                                                                                          |
|             |            | - Added blockx and blocky for variable window size during combed frame detection                                                                                  |
|             |            | - Changed default MI value to 64 (default window size is now 16x16 = 256 pixels)                                                                                  |
|             |            | - changed default cthresh value to 6                                                                                                                              |
|             |            | - Small change to denoising routine                                                                                                                               |
+-------------+------------+-------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| v0.9.7.2    | 04/20/2005 | - Fixed not correctly using the field information from tfm's hints when acting as a post-processor for it. Also fixed not correctly altering the match info of    |
|             |            |   tfm's hints when acting as a post-processor for it (PP=1 in tfm).                                                                                               |
|             |            | - Improvements to type 3 interpolation, renamed to modified ELA-2                                                                                                 |
+-------------+------------+-------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| v0.9.7.1    | 03/10/2005 | - Fixed not correctly reading hints from newer versions of tivtc and if colorimetry hints were present from dgdecode.                                             |
+-------------+------------+-------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| v0.9.7      | 01/20/2005 | - Added link and denoise parameters, link defaults to 2 and denoise to true                                                                                       |
|             |            | - Added ELA interpolation (tomsmocomp version) as type = 3                                                                                                        |
|             |            | - Hints option can now read hints from tfm as well as telecide                                                                                                    |
|             |            | - map = 2 now sets the chroma pixels that are to be interpolated to 255 and not just the luma                                                                     |
|             |            | - Changed default type value to 2 (kernel interpolation)                                                                                                          |
|             |            | - Changed default tryWeave value to false                                                                                                                         |
+-------------+------------+-------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| v0.9.6      | 10/03/2004 | - Added full parameter, allows for ivtc post-processing. full defaults to true.                                                                                   |
|             |            | - Added cthresh, chroma, and MI parameters... these are used when full=false                                                                                      |
|             |            | - Added tryWeave option, allows TDeint to adaptively switch between per-field and per-pixel motion adaptation. tryWeave defaults to true.                         |
|             |            | - Improved field differencing                                                                                                                                     |
|             |            | - changed mtnmode default to 1                                                                                                                                    |
+-------------+------------+-------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| v0.9.5      | 09/26/2004 | - Sped up mtnmodes 2 and 3, was doing it the hard way and not the easy way...                                                                                     |
+-------------+------------+-------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| v0.9.4      | 09/25/2004 | - Added auto hints detection                                                                                                                                      |
|             |            | - Added mtnmodes 2 and 3                                                                                                                                          |
|             |            | - Added ability to deinterlace from the original stream when using hints via clip2 parameter                                                                      |
|             |            | - Fixed field differencing using the wrong fields doh!                                                                                                            |
+-------------+------------+-------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| v0.9.3      | 09/18/2004 | - Added order = -1 option, will detect parity from avisynth                                                                                                       |
|             |            | - Added hints option for reading telecide hints for interlaced/progressive                                                                                        |
|             |            | - 5 field motion check now includes checks over 4 field distances                                                                                                 |
|             |            | - Fixed a bug in YUY2 type = 1 deinterlacing method                                                                                                               |
+-------------+------------+-------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| v0.9.2      | 09/14/2004 | - Added kernel interpolation and sharp parameter                                                                                                                  |
|             |            | - Added 5 field motion check and mtnmode parameter                                                                                                                |
|             |            | - Changed default motion thresholds to 6                                                                                                                          |
+-------------+------------+-------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| v0.9.1      | 09/12/2004 | - Fixed some really stupid bugs, one motion check was incorrect for the first and last frame of a clip,                                                           |
|             |            |   and mode = 1 would only work for the first half of the video                                                                                                    |
+-------------+------------+-------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| v0.9        | 09/12/2004 | - Initial Release                                                                                                                                                 |
+-------------+------------+-------------------------------------------------------------------------------------------------------------------------------------------------------------------+

$Date: 2006/10/19 19:27:32 $

.. _DebugView: http://www.sysinternals.com/Utilities/DebugView.html

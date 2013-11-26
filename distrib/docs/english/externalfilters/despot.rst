
DeSpot
======


Abstract
--------

| **author:** Alexander G. Balakhnin aka Fizick
| **version:** 3.5.0 (based on Conditional Temporal Median Filter)
| **download:** `<http://bag.hotmail.ru/>`_
| **category:** Broadcast Video Plugins
| **requirements:**

-   YV12 Colorspace
-   ISSE support

--------


DeSpot - Conditional Temporal Spot Removing Filter
--------------------------------------------------

This filter is designed to remove temporal noise in the form of dots (spots)
and streaks found in some videos. The filter is also useful for restoration
(cleaning) of old telecined 8mm (and other) films from spots (from dust) and
some stripes (scratches).

The plugin is based on Conditional Temporal Median Filter v.0.93 (C-plugin
for Avisynth 2.5)
by Kevin Atkinson `<http://kevin.atkinson.dhs.org/temporal_median/>`_
(This document is also based on his doc.)


Example images
--------------

One half frame from my old 8mm film (top - before, bottom -after the filter):

.. image:: pictures/despot1.jpg


Avisynth script for this example, which show work of old base filter version
0.93, but adapted to for new version 3.2.

::

    LoadPlugin("despot.dll")
    AviSource("film8mm.avi")
    ConvertToYV12()
    Crop(0,0,720,288) #to show test
    i = last
    # Compare half-frames with and without noise reduction
    DeSpot(p1=35, p2=14, pwidth=70, pheight=70, mthres=25, mwidth=20,
    mheight=15, interlaced=false,
      \  merode=33, ranked=false, p1percent=0, dilate=false,
      fitluma=false, blur=0, motpn=false, seg=0)
    StackVertical(i, last)

The same frame after some parameters using from more new filter version 1.0
(top half - with spot border blur, bottom half - with temporal smoothing in
static areas):

.. image:: pictures/despot2.jpg


Avisynth script for this example :

::

    LoadPlugin("despot.dll")
    AviSource("film8mm.avi")
    ConvertToYV12()
    Crop(0,0,720,288)
    # Compare frames with blur, and with and without temporal noise reduction
    i = last
    # Compare half-frames with and without noise reduction
    b = DeSpot(i, p1=35, p2=14, pwidth=70, pheight=70, mthres=25,
    mwidth=20, mheight=15, interlaced=false,
      \  merode=33, ranked=false, p1percent=0, dilate=false,
      fitluma=false, blur=4, motpn=false, seg=0)
    t = DeSpot(i, p1=35, p2=14, pwidth=70, pheight=70, mthres=25,
    mwidth=20, mheight=15, interlaced=false,
      \  merode=33, ranked=false, p1percent=0, dilate=false,
      fitluma=false, blur=4, motpn=false, seg=0, tsmooth=3)
    StackVertical(b, t)

Some new parameters are introduced in new filter versions, for more correct
spot detection and artifacts-free spot removing.


Needed software
---------------

1.  The program is plugin (filter) for free Avisynth 2.5
    (http://www.avisynth.org). Tested with versions 2.53 and 2.55. Version
    2.54 also may be used.
2.  Old filter versions prior 2.0 (up to last 1.3) were is needed in
    Avisynth C Interface (avithynth_c.dll) by Kevin Atkinson. There were some
    problems with it.
3.  New filter since version 2.0 is native Avisynth plugin and is NOT
    needed in C-interface.


USAGE
-----

DeSpot may be loaded as any Avisynth plugin, or by putting it in the Plugins
directory or by using LoadPlugin. Basic Usage:

::

    AviSource("d:\video.avi")  # the input MUST be YV12
    LoadPlugin("despot.dll")
    ConvertToYV12()
    DeSpot(p1=35, p2=14, mthres=25)

The filter may work in 2 main modes, switched by parameter median (since
version 2.0):

| *median* (true or false, default = false),
|   false - Spots removing mode - filter will attempt to identify noise and
      eliminate it.
|   true - Simple Median mode - filter will apply a simple temporal median filter
      to the non-moving areas of the image.

Function calling syntax:

``DeSpot`` (clip, int mthres, int mwidth, int mheight, int merode, bool
interlaced, bool median, int p1, int p2, int pwidth,  int pheight, bool
ranked, int sign, int maxpts, int p1percent, int dilate, bool fitluma, int
blur, int tsmooth, int show, int mark_v, bool show_chroma, bool motpn, int
seg, bool color, int mscene, int minpts, clip "extmask")


Parameters for DeSpot in spots removing mode are any of the following
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

| **p1** (default 24)
| **p2** (default 12)
| A pixel needs to be at different from its neighbors by at least 'P1' in order
  for it to be considered noise. The surrounding pixels must be different by at
  lease 'P2' in order for the pixel to be considered part of the same spot.

| **pwidth** (default 6)
| **pheight** (default 5)
| A spot can be no larger than pwidth x pheight

| **ranked** (true or false, default=true)
| Enables ranked ordered difference spot detector with 6 points instead of 2.

| **sign** (default 0)
| Set mode for removing of only black or white spots or both:
|   sign = 0 - any spots and outliers (default)
|   sign = 1 - only black (dark) spots and outliers
|   sign = -1 - only white (light) spots and outliers
|   sign = 2 - only black (dark) spots, any outliers
|   sign = -2 - only white (light) spots, any outliers

| **maxpts** (from 0 to 10000000, default=0 - no limit)
| Set upper limit of points (pixels) per every spot.

| **minpts** (from 0 to 10000000, default=0 - no limit)
| Set lower limit of points (pixels) per every spot.

| **p1percent** (from 0 to 100, default=10)
| Set lower limit of relative fracture of high-contrasted (by criterion P1)
  pixels per spot (in percent).

| **dilate** (from 0 to 255, default=1)
| Set range of morphological dilate (growing) of removed spots (in pixel units).

| **fitluma** (true or false, default=true)
| Enables some frame luminosity correction in places of deleted spots

| **blur** (from 0 to 4, default 1)
| Value (length) of local spatial blur near borders of deleted spots

| **tsmooth** (from 0 to 127, default 0)
| Control temporal smoothing in static areas (except spots and motion).
| Set approximate threshold of pixel luma variance in 3 frames,
| The more variance exceed this threshold, the less temporal smoothing.
|    0 - no temporal smoothing.

| **motpn** (true or false, default = true)
| Define motion detecting method.
|   false - detect motion from previous to current and from current to next frame
    (old method used in all versions before 3.0)
|   true - detect motion from previous to next frame (new method since version 3.0)

| **seg** (from 0 to 2, default=2)
| Control spots segments removing method.
|    0 - remove only spots pixels which has no overlap with motion zones (old
         method used in all versions before 3.0, most strong removing);
|    1 - remove only spots line segments which has no any overlap with motion zones;
|    2 - remove only whole spots which has no any overlap with motion zones (most safe).

| **color** (true of false, default = false)
| Control color correction at place of removed spots .
|    true - change color of pixels at places of removed spots to mean value
         previous, current and next frames,
|    false - not change color of pixels at places of detected spots.

| **mscene** (from 0 to 100, default=40)
| Set percent of pixels in motion for scenechange detection and disabling of frame spot removal

| **extmask** (default none)
| Optional external mask clip. It will be binarized with threshold 127 and
  logically added (by "OR" operation) to internal motion mask. It can be used
  for additional protection of good objects (if you have some reliable mask).


Parameters of DeSpot for both spots removing and median modes are
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

| **mthres** (default 16)
| A pixel needs to be different from the another frame by at least 'MTHRES' in
  order to be considered moving. This number should be larger than 'P2' in
  order to prevent noise from being identified as motion.

| **mwidth** (default 7)
| **mheight** (default 5)
| These define the width and height of block for motion map denoising algorithm
  (erode and dilate stages).

| **merode** (default = 33)
| These define a threshold value of percent of motion pixels in the block at
  erode stage.

| **interlaced** (true or false)
| Whether to treat the video as interlaced or not (progressive). The default is
  progressive for DeSpot since version 1.3. (In older versions, the Field based
  video was processed as interlaced by default - it was a bug).

To instead spots removing, show a motion map and noise that would of been eliminated, use parameter:

| **show** (0, 1, 2, default=0)
|   0 - not show,
|   1 - to highlight the noise instead of removing it
|   2 - show a motion map and noise

| If show=1, you may use additional parameter to change marks color and
  brigthness,
| mark_v (0 to 255, default= 255)
| Where  mark_v is the luma value to highlight the noise with.
| The color (pink, green or grey) of noise spot now is depend from parity of mark_v.
| Motion map is also shown.

| If show=2, the luma is changed as follows:
|   255 (White): Noise that will be removed
|   159: Noise that won't be remove because it might be motion
|   95: Motion map for the current image

| **show_chroma** (true or false)
| Use to show clip color data on motion map.


TUNING THE PARMS
----------------

In order for the filter to work right the various parameters MUST be set
correctly. There is no good default values.

The first parameter that needs to be set is interlaced, set it to true if
your video is interlaced, false otherwise.

Than pwidth and pheight need to be set. Set these to be slightly larger than
the specks you want to eliminate. If your video is interlaced than height
represents the height of an individual field. Thus, it will essentially be
doubled.

Than p1, p2, and mthres need to be set. In general, p1 > mthres > p2. If
these are set too low than you may lose detail as small pixel variations
might be mistaken as specks, thus losing detail, and more importantly, real
specks might not be recognized as the size of the filter thinks the spec is
might be larger than pwidth by pheight. show=1 or 2 might be helpful in
setting these parameters.

The mwidth, mheight parameters define the range of motion zones influence on
noise (spots) zones. For decreasing of false spot detecting for fast motion
scenes, you may increase these. After that, to cancel the influence of very
small motion zones, you may increase the merode parameter (relative) .

Than sign may be set, if almost all spots are only white or only black.
Correct tuning reduces number of false spot detections and artifacts.

I recommend to use new parameter ranked=true for stability of spots detection
in noisy video.

Use parameter maxpts as another method (in addition to pwidth and pheight) to
avoid cleaning too large objects - probably not spots.

Use p1percent to not clean weak (small contrast) spots with small relative
fracture of strong points (with p1 threshold).

For better removing of partially damaged pixels near non-sharp spots edges,
you may increase spots sizes by increasing of dilate parameter.

Enable brightness correction in spaces of deleted spots by parameter fitluma.

This correction is local (line segment based) in seg>0 mode and must be used
with properly defined dilated spots to prevent false correction due to non-
sharp spots edges.

To reduce noticeability of some borders in places of deleted spots some more,
tune local spatial smoothing by parameter blur.

For denoising of almost static areas, try to use temporal smoothing, with
tsmooth parameter about 4-8.

If spots have some color, try enable color parameter to correct spots color.

To prevent artifacts at scene change, decrease mscene parameter.

You also may try to use external mask clip (extmask parameter) in addition to
(or instead of) internal motion mask to protect good objects. For example, it
can be motion mask or inverted SAD mask from MVTools plugin.


HOW IT WORKS
------------


The filter works in Denoise mode as follows
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

| 1a. Find pixels that are different from its neighbors by at least p1.
|
|  If ranked parameter is true (new method from version 1.2), the 3 neighbors in
   previous frame (x-1, x, x+1) and same 3 neighbors in next frame are ranked
   (ordered by value), and those min and max are used for luma difference
   calculation for current pixel (x).
|
|  If ranked parameter is false (old method), the only 1 neighbor in previous
   frame (at same position x) and 1 neighbor in next frame are used for min and
   max calculation,
|
|  If sign parameter is not 0, the sign of difference is also taken into
   account.
|
|  These pixels are merged to horizontal line segments.
|
|  Stacked line segments are merged to spots.

| 1b. Enlarge outliers based on difference p2<p1.

| 2a. Determine the size of the specks and reject (will not clean) all
      those larger than pwidth x pheight.
|
|     If numpts parameter is set, the big spots are rejected also.
|
|     If p1percent parameter is set, then weak (by criterion p1) spots, which
      mostly consist of outliers (by criterion p2), are rejected also.

| 2b. If Dilate mode set, than spots are dilated to cover its non-contrast
      edges and close small gaps between its, by applying a morphological
      dilate operation to noise (spots) map.

| 3a. Find moving areas of an image by simply comparing each pixel to the
      another frame and considering all those which are greater than mthres.
|
|     If motpn=false, it is defined from previous to current frame.
|
|     If motpn=true, it is defined from previous to next frame.

| 3b. Mark motion pixels without noise with weight 3 in the motion map.
      If motpn=false, mark pixels determined both noise and motion as weight 1 in
      the motion map.

| 4. Denoise the motion map by constrained erosion and then dilating (as a
     whole it is morphological opening operation).
|
|    During erode phase, the motion map is eroded with range mwidth/2 and
     mheight/2, and zones with small summary weight (less than 3*merode/100) are
     decreased or completely removed from motion map. Such zones correspond to
     small relative (in percent) numbers of motion neighbors within this range (or
     mostly noisy pixels -spots).
|
|    During dilate phase,  remained motion zones are enlarged with same range.
|
|    These are probably the most important steps (especially for motpn=false) .

| 4a. If percent of pixels in motion is more than mscene parameter, the
      plugin detects scenechange, and whole motion map is set as motion.

| 4b. Add optional external mask to motion mask.

| 5. Only remove the specks in which there was no motion (in the current
     frame or next frame if motpn=false).
|
|    In pixel removing mode (seg = 0), test and reject all noise candidate pixels
     which has overlap with motion zones, rest candidates will be removed.
|
|    In segment removing mode (seg = 1), test and reject all noise candidate
     segments which has any overlap with motion zones, rest segments will be
     removed.
|
|    In segment removing mode (seg = 2), test and reject all noise candidate spots
     which has any overlap with motion zones, rest spots will be removed. It is
     the most safe mode, with minimum artifacts false removing

| 6. Optional make luma correction in place of removed pixels and spatial
     smoothing near spot edges.

| 7. Optional make temporal smoothing of static areas.

| 8. Optional correction of color at place of spots.


The filter can also be configured to work as follows (Simple Median mode)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

1) Find moving areas of an image by simply comparing each pixel to the
   previous frame and considering all those which are greater than mthres.

2) Denoise the motion map by erosion and then dilating (i.e.
   morphological opening). This is probably the most important step.

3) Apply a simple temporal median filter on the non-moving areas of the
   image.


OPTIMIZATION NOTES
------------------

DeSpot since version 3.2 is optimized by hand for Integer SSE (Pentium3,
Athlon is needed now).
Speed increasing up to about 30%.


COMPILING
---------

Fizick's version above 1.1 is compiled by free MS VC++ Toolkit 2003 with MS
Platform SDK.

Note: copy lost nmake.exe and cvtres.exe from Bin\win64 dir to Bin dir.

MS VC6, VC7 also may be used.

Use make file "makefile" with command: nmake

Old versions of the C-plugin up to 1.3 may be compiled with GCC-g++ 3.3.1,
MinGW 3.0.0-1, MSYS 1.09. New versions above 2.0 can not be compiled so.


COMBINED USAGE
--------------

Very good results are may be obtained with combined this filter with motion
estimation and compensation: global motion with `DePan`_ plugin (by Fizick),
or local motion with `MVTools`_ plugin (by Manao).

In this case the pixels from previous and next frames are moved to best fit
to current frame, therefore relative motion is decreased, false spots
detection is decreased, and noise reduction is increased.

Example script with DePan 0.9 (you may tune optional DePanEstimate and
DepanInterleave parameters):

::

    LoadPlugin("depan.dll")
    LoadPlugin("despot.dll")
    AviSource("h:\kino.avi")
    i = ConvertToYV12()
    d = DePanEstimate(trust=3)
    DePanInterleave(i, data=d)
    DeSpot(p1=30, p2=15, pwidth=800, pheight=600, mthres=20, motpn=true, dilate=1, seg=1)
    SelectEvery(3, 1)

Example script with MVTools 0.95 (you may tune optional MVAnalyse
parameters):

::

    LoadPlugin("mvtools.dll")
    LoadPlugin("despot.dll")
    AviSource("h:\kino.avi")
    i = ConvertToYV12()
    vf = MVAnalyse(i, isb=false, lambda=2000)
    f = MVCompensate(i, vf)
    vb = MVAnalyse(i, isb=true, lambda=2000)
    b = MVCompensate(i, vb)
    Interleave(f, i, b)
    DeSpot(p1=30, p2=12, mthres=20, dilate=2, fitluma=true, blur=2, seg=2)
    SelectEvery(3,1)

Example script with external mask from MVTools plugin v0.9.13.2 and above
(you may tune optional MVAnalyse parameters):

::

    LoadPlugin("mvtools.dll")
    LoadPlugin("masktools.dll")
    LoadPlugin("degrainmedian.dll")
    LoadPlugin("despot.dll")

    AviSource("h:\kino.avi")
    i = ConvertToYV12()
    prefilt=i.DeGrainMedian() # prefiltered for better motion analysis

    # analyse and compensate motion forward and backward (to current frame)
    ml = 100     # mask scale
    thscd1 = 400 # scene change

    vf = prefilt.MVAnalyse(isb=false, truemotion=true) # forward vectors
    cf = i.MVFlow(vectors=vf, thscd1 = thscd1) # previous compensated forward
    sadf = MVMask(vectors=vf, ml=ml,kind=1,gamma=1, thscd1 = thscd1) # forward SAD mask
    msadf=sadf.Binarize() # binary inverted forward SAD mask

    vb = prefilt.MVAnalyse(isb=true, truemotion=true)  # backward vectors
    cb = i.MVFlow(vectors=vb, thscd1 = thscd1) # next compensated backward
    sadb = MVMask(vectors=vb, ml=ml, gamma=1, kind=1, thscd1 = thscd1) # backward SAD mask
    msadb = sadb.Binarize() # binary inverted backward SAD mask

    msad = Logic(msadf,msadb,"OR") # combined inverted SAD mask
    msad = msad.Expand() # expanded inverted SAD mask
    msadi = Interleave(msad, msad, msad) # interleaved 3-frame inverted SAD mask
    # This mask is high (255) where at least one motion estimation is good,
    # so these areas will be protected

    Interleave(cf,i,cb) # interleave forward compensated, source, and backward compensated

    DeSpot(p1=30,p2=12,pwidth=800,pheight=600,mthres=20,merode=33,\
       sign=0,show=1,seg=0,color=true,motpn=true, extmask=msadi)

    SelectEvery(3,1) # get filtered source

ADDITIONAL INFO
---------------

| Discussion of ConditionalTemporalMedian filter and Despot filter :
| `<http://forum.doom9.org/showthread.php?s=&threadid=59388>`_


LICENSE
-------

This program is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software
Foundation; either version 2 of the License, or (at your option) any later
version.

This program is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
details.

You should have received a copy of the GNU General Public License along with
this program; if not, write to the Free Software Foundation, Inc., 675 Mass
Ave, Cambridge, MA 02139, USA.


+-------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| Changelog                                                                                                                                                         |
+========+===================+================+=====================================================================================================================+
| v3.5.0 | July 14, 2006     |                | - Corrected documentation example (msadb). Thanks to johnmeyer for report.                                          |
+--------+-------------------+----------------+---------------------------------------------------------------------------------------------------------------------+
| v3.5   | November 26, 2005 |                | - Added external motion mask clip option and example.                                                               |
|        |                   |                | - Changed default motpn=true (was really false, contrary to documentation).                                         |
+--------+-------------------+----------------+---------------------------------------------------------------------------------------------------------------------+
| v3.4.0 | June 18, 2005     |                | - Reformatted doc.                                                                                                  |
+--------+-------------------+----------------+---------------------------------------------------------------------------------------------------------------------+
| v3.4   | April 11, 2005    |                | - Added parameter minpts.                                                                                           |
+--------+-------------------+----------------+---------------------------------------------------------------------------------------------------------------------+
| v3.3.3 | March 30, 2005    |                | - Fixed bug with median mode (thanks to slk001 for report).                                                         |
+--------+-------------------+----------------+---------------------------------------------------------------------------------------------------------------------+
| v3.3.2 | March 28, 2005    |                | - More correct clip cache range (now =2 in place of undefined).                                                     |
+--------+-------------------+----------------+---------------------------------------------------------------------------------------------------------------------+
| v3.3.1 | October 8, 2004   |                | - Fixed bugs with scene change detection..                                                                          |
+--------+-------------------+----------------+---------------------------------------------------------------------------------------------------------------------+
| v3.3   | August 4, 2004    |                | - Added mscene parameter for scene change detection.                                                                |
+--------+-------------------+----------------+---------------------------------------------------------------------------------------------------------------------+
| v3.2   | July 4, 2004      | Fizick         | - Corrected enabling and disabling of temporal smooth for some modes.                                               |
|        |                   |                | - Changed temporal smooth method to more fast but simpler, new tsmooth is similar to old tsmooth*2.                 |
|        |                   |                | - Restored median mode processing as was before version 3.0.                                                        |
|        |                   |                | - Partial ISSE optimization for speed increasing (Pentium3 or Athlon is needed now).                                |
|        |                   |                | - Updated doc.                                                                                                      |
+--------+-------------------+----------------+---------------------------------------------------------------------------------------------------------------------+
| v3.1   | June 27, 2004     | Fizick         | - Added color correction at place of removed spots.                                                                 |
+--------+-------------------+----------------+---------------------------------------------------------------------------------------------------------------------+
| v3.0   | June 22, 2004)    | Fizick         | - Version 3.0 is major release update (probably alpha with bugs):                                                   |
|        |                   |                | - Added another motion detection method "motpn" (previous to next frame).                                           |
|        |                   |                | - Added segments removing methods "seg"=1,2.                                                                        |
|        |                   |                | - Changed luma correction to local in segments (spots) removing mode.                                               |
|        |                   |                | - Removed "mratio" parameter (reset as internal constant =3 as in all versions prior 2.1).                          |
|        |                   |                | - Changed some defaults.                                                                                            |
|        |                   |                | - Code reorganization.                                                                                              |
|        |                   |                | - Updated doc.                                                                                                      |
+--------+-------------------+----------------+---------------------------------------------------------------------------------------------------------------------+
| v2.1   | June 14, 2004     | Fizick         | - Added parameter "mratio" parameter (it was internal =3 in all previous versions).                                 |
|        |                   |                | - Changed default of "merode" to dependent from "mratio".                                                           |
|        |                   |                | - Updated doc.                                                                                                      |
|        |                   |                | - At last I begin to understand how the filter works at motion denoising stage :-).                                 |
+--------+-------------------+----------------+---------------------------------------------------------------------------------------------------------------------+
| v2.0   | June 10, 2004     | Fizick         | - Version 2.0 is major release update (probably alpha with bugs):                                                   |
|        |                   |                | - Main interface code is rewrited, and now filter is native Avisynth plugin (not C-plugin).                         |
|        |                   |                | - Added parameter "median" instead of DeSpotMedian function,                                                        |
|        |                   |                | - Added parameter "show" instead of DeSpotMark, DeSpotMap, DeSpotMedianMap functions.                               |
|        |                   |                | - Replaced parameter "mp" to parameter "merode" (relative)                                                          |
|        |                   |                | - Changed "p1percent" to integer                                                                                    |
|        |                   |                | - Updated doc.                                                                                                      |
+--------+-------------------+----------------+---------------------------------------------------------------------------------------------------------------------+
| v1.3   | June 7, 2004      | Fizick         | - Removed parameter "weak" ( "p1percent" is more useful).                                                           |
|        |                   |                | - Added parameter "Dilate" to enlarge spots.                                                                        |
|        |                   |                | - Fixed bug with processing field based video as interlaced by default - now default for any source is progressive. |
|        |                   |                | - Changed parameters order to more functional.                                                                      |
|        |                   |                | - Changed default values of some parameters to more optimal.                                                        |
|        |                   |                | - Updated doc.                                                                                                      |
+--------+-------------------+----------------+---------------------------------------------------------------------------------------------------------------------+
| v1.2   | June 01, 2004     | Fizick         | - Added parameters Ranked, weak, maxpts, p1percent,                                                                 |
|        |                   |                | - changed mark mode to color spot with weak motion map.                                                             |
+--------+-------------------+----------------+---------------------------------------------------------------------------------------------------------------------+
| v1.1   | May 31, 2004      |                | - Source now is compatible with MS VC6, VC7.                                                                        |
+--------+-------------------+----------------+---------------------------------------------------------------------------------------------------------------------+
| v1.0   | Dec 30, 2003      | Fizick         | - not public                                                                                                        |
|        |                   |                | - Added "tsmooth" parameter for temporal smoothing in static areas.                                                 |
|        |                   |                | - Changed filter name to DeSpot, filter file name to despot.dll,                                                    |
|        |                   |                | - and function names:                                                                                               |
|        |                   |                |   - ConditionalDenoise to DeSpot,                                                                                   |
|        |                   |                |   - ConditionalDenoiseMark to DeSpotMark,                                                                           |
|        |                   |                |   - ConditionalDenoiseMap to DeSpotMap,                                                                             |
|        |                   |                |   - ConditionalMedian to DeSpotMedian,                                                                              |
|        |                   |                |   - ConditionalMedianMap to DeSpotMedianMap.                                                                        |
|        |                   |                | - Corrected info.                                                                                                   |
+--------+-------------------+----------------+---------------------------------------------------------------------------------------------------------------------+
| v0.934 | Dec 20, 2003      | Fizick         | - Added "fitluma" and "blur" parameters to reduce noticeability of deleted spots places.                            |
|        |                   |                | - Remove "per" parameter used previously for that.                                                                  |
|        |                   |                | - New default value of "mp" parameter.                                                                              |
+--------+-------------------+----------------+---------------------------------------------------------------------------------------------------------------------+
| v0.93c | Nov 30, 2003      |                | - More short filter name ctmedian.dll                                                                               |
|        |                   |                | - Added version info to DLL                                                                                         |
+--------+-------------------+----------------+---------------------------------------------------------------------------------------------------------------------+
| v0.93b | Nov 13, 2003      | Fizick         | - non public                                                                                                        |
|        |                   |                | - Added "per" parameter for more smoothed specks perimeter.                                                         |
+--------+-------------------+----------------+---------------------------------------------------------------------------------------------------------------------+
| v0.93a | Nov 7, 2003       | Fizick         | - non public                                                                                                        |
|        |                   |                | - Added "sign" parameter for removing only black or white specks.                                                   |
|        |                   |                | - non optimized general version only.                                                                               |
+--------+-------------------+----------------+---------------------------------------------------------------------------------------------------------------------+
| v0.93  | Sep 27, 2003      | Kevin Atkinson | - Fix another nasty bug.                                                                                            |
|        |                   |                | - Included non optimized version.                                                                                   |
|        |                   |                | - Expanded the manual a bit                                                                                         |
+--------+-------------------+----------------+---------------------------------------------------------------------------------------------------------------------+
| v0.92  | Sep 10, 2003      | Kevin Atkinson | - Fixed nasty bug.                                                                                                  |
+--------+-------------------+----------------+---------------------------------------------------------------------------------------------------------------------+

$Date: 2006/12/15 19:29:25 $

.. _DePan: depan.rst
.. _MVTools: mvtools.rst

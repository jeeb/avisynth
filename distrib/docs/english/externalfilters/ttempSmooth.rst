
TTempSmooth
===========


Abstract
--------

| **author:** tritical
| **version:** 0.9.4
| **download:** `<http://bengal.missouri.edu/~kes25c/>`_
| **category:** Temporal Smoothers
| **requirements:**

- YV12 & YUY2 Colorspace

**license:** GPL

--------


Description
-----------

TTempSmooth is a motion adaptive (it only works on stationary parts of the
picture), temporal smoothing filter. TTempSmoothF is a faster (50-75%)
version of TTempSmooth that doesn't take the mdiff/mdiffC parameters (it is
equivalent to running TTempSmooth with mdiff/mdiffC set equal to or greater
then LThresh-1/CThresh-1).


Syntax
------

``TTempSmooth`` (clip, int "maxr", int "lthresh", int "cthresh", int
"lmdiff", int "cmdiff", int "strength", float "scthresh", bool "fp", int
"vis_blur", bool "debug", bool "interlaced", PClip "pfclip")

``TTempSmoothF`` (clip, int "maxr", int "lthresh", int "cthresh", float
"scthresh", bool "fp", int "vis_blur", bool "debug", bool "interlaced", PClip
"pfclip")


PARAMETERS
----------

*maxd* - (maximum temporal diameter)

This sets the maximum temporal radius. By the way it works TTempSmooth
automatically varies the radius used... this sets the maximum boundary.
Possible values are 1 through 7. At 1 TTempSmooth will be (at max) including
pixels from 1 frame away in the average (3 frames total will be considered
counting the current frame). At 7 it would be including pixels from up to 7
frames away (15 frames total will be considered). With the way it checks
motion there isn't much danger in setting this high, it's basically a quality
vs. speed option. Lower settings are faster while larger values tend to
create a more stable image.

- default

  - 3 (int)

*LThresh/CThresh* - (luma and chroma thesholds)

Your standard luma and chroma thresholds for differences of pixels between
frames. lthresh is for luma and cthresh is for chroma. TTempSmooth checks 2
frame distance as well as single frame, so these can usually be set slightly
higher than with most other temporal smoothers and still avoid artifacts.
Valid settings are from 1 to 256. Also important is the fact that as long as
"mdiff" is less than the threshold value then pixels with larger differences
from the original will have less weight in the average. Thus, even with
rather large thresholds pixels just under the threshold wont have much
weight, helping to reduce artifacts.

- default

  - LThresh = 4 (int)
  - CThresh = 5 (int)

*lmdiff/cmdifC* - (maintain diff) - (only used in TTempSmooth not TTempSmoothF)

Any pixels with differences less than or equal to "mdiff" will be blurred at
maximum. Usually, the larger the difference to the center pixel the smaller
the weight in the average. mdiff makes TTempSmooth treat pixels that have a
difference of less than or equal to "mdiff" as though they have a difference
of 0. In other words, it shifts the zero difference point outwards. Set
"mdiff" to a value equal to or greater than lthresh-1/cthresh-1 to completely
disable inverse pixel difference weighting. Valid settings are from 0 to 255.
lmdiff is for luma and cmdiff is for chroma.

- default

  - lmdiff = 2 (int)
  - cmdiff = 3 (int)

*strength* - (spatial weighting factor)

TTempSmooth uses inverse distance weighting when deciding how much weight to
give to each pixel value. The strength option lets you shift the drop off
point away from the center to give a stronger smoothing effect and add weight
to the outer pixels. It does for the spatial weights what mdiff does for the
difference weights. The possible options are:

- 1 = ``0.13 0.14 0.16 0.20 0.25 0.33 0.50 1.00 0.50 0.33 0.25 0.20 0.16 0.14 0.13``
- 2 = ``0.14 0.16 0.20 0.25 0.33 0.50 1.00 1.00 1.00 0.50 0.33 0.25 0.20 0.16 0.14``
- 3 = ``0.16 0.20 0.25 0.33 0.50 1.00 1.00 1.00 1.00 1.00 0.50 0.33 0.25 0.20 0.16``
- 4 = ``0.20 0.25 0.33 0.50 1.00 1.00 1.00 1.00 1.00 1.00 1.00 0.50 0.33 0.25 0.20``
- 5 = ``0.25 0.33 0.50 1.00 1.00 1.00 1.00 1.00 1.00 1.00 1.00 1.00 0.50 0.33 0.25``
- 6 = ``0.33 0.50 1.00 1.00 1.00 1.00 1.00 1.00 1.00 1.00 1.00 1.00 1.00 0.50 0.33``
- 7 = ``0.50 1.00 1.00 1.00 1.00 1.00 1.00 1.00 1.00 1.00 1.00 1.00 1.00 1.00 0.50``
- 8 = ``1.00 1.00 1.00 1.00 1.00 1.00 1.00 1.00 1.00 1.00 1.00 1.00 1.00 1.00 1.00``

The values shown are for maxr = 7, when using smaller radius values the
weights outside of the range are simply dropped. Thus, setting strength to a
value of maxr+1 or higher will give you equal spatial weighting of all pixels
in the kernel. Versions prior to v0.9.3 always used weighting equal to
strength = 2.

- default

  - 2 (int)

*scthresh* - (scenechange threshold)

The standard scenechange threshold as a percentage of maximum possible change
of the luma plane. A good range of values is between 8 and 15. Use the debug
output option to see what frames are detected as a scenechange and the
difference value calculated for each frame. Set scthresh to <= 0.0 to disable
scenechange detection.

- default

  - 12.0 (float)

*fp* - (regular or fixed point average)

Setting fp=true will add any weight not given to the outer pixels back onto
the center pixel when computing the final value. Setting fp=false will just
do a normal weighted average. fp=true is much better for reducing artifacts
in motion areas and usually produces overall better results.

- default

  - true (bool)

*vis_blur* - (visualize blur)

This option outputs a map that indicates which pixels are being filtered and
by how much. For each pixel it computes how much weight was given to the
outer pixels (i.e. not the center pixel) out of the maximum possible weight
that could have been given to the outer pixels. It then scales this result
into the 0 to 255 range so that in areas where no weight was given to the
outer pixels (no smoothing is taking place) it will show black, and in areas
where all possible weight was given to the outer pixels (maximum smoothing is
taking place) it will show white. In other words, the darker the value the
weaker the smoothing, the brighter the value the stronger the smoothing.
Possible values are:

- 0 - no map
- 1 - map of Y plane smoothing
- 2 - map of U plane smoothing
- 3 - map of V plane smoothing

- default

  - 0 (int)

*debug* - (debug output)

If enabled, TTempSmooth will output the scenechange statistics and indicate
which frames were detected as scenechanges. This is intended to help with
tweaking scthresh. To view the output you can use `"DebugView" from
sysinternals.`_

- default

  - false (bool)

*interlaced* - (interlaced yv12 input)

Set this to true if you are using ttempsmooth on a YV12 interlaced clip...
otherwise set it to false. This setting only effects YV12 input and has no
effect if the input is YUY2 because interlaced YUY2 needs no special
handling.

- default

  - false (bool)

*pfclip* - (pre-filtered clip to use for pixel differences)

This allows you to specify a separate clip for ttempsmooth to use when
calculating pixel differences. This applies to checking the motion
thresholds, calculating inverse difference weights, and detecting
scenechanges. Basically, the pfclip will be used to determine the weights in
the average but the weights will be applied to the original input clip's
pixel values. This option is similar to "ppClip" in TBilateral... some
possible suggestions for pre-filtering are a gaussian blur, adaptive median,
etc...

- default

  - NULL (PClip)

+----------------------------------------------------------------------------------------------------------+
| Changelog                                                                                                |
+========+============+====================================================================================+
| v0.9.4 | 11/17/2005 | - Added interlaced parameter (fix incorrect interlaced yv12 chroma handling)       |
|        |            | - Added pfclip parameter                                                           |
|        |            | - Added MMX scenechange routines (for those that don't have an isse capable cpu)   |
|        |            | - Changed default scthresh value to 12.0                                           |
+--------+------------+------------------------------------------------------------------------------------+
| v0.9.3 | 07/20/2005 | - Added scenechange detection                                                      |
|        |            | - Added visualize blur option                                                      |
|        |            | - Added back in fp and the ability to do a regular average (was removed in v0.9.1) |
|        |            | - Added debug output                                                               |
|        |            | - Added strength option to adjust spatial weights                                  |
|        |            | - Rewrote all the code (vastly reduced the total source size)                      |
|        |            | - 10% speed increase for TTempSmooth()                                             |
|        |            | - Changed to maxr instead of maxd, increased max possible radius value from 4      |
|        |            |   to 7 and decreased the min possible radius from 2 to 1                           |
|        |            | - Changed some of the parameter names (mdiff=lmdiff,mdiffc=cmdiff)                 |
|        |            | - Changed default lmdiff/cmdiff values to 2/3 (were 3/4)                           |
+--------+------------+------------------------------------------------------------------------------------+
| v0.9.2 | 07/29/2004 | - Modified buffering code to remove unneeded BitBlts()... small speed up (5-10%).  |
|        |            | - Request frames in linear order when completely refilling buffer.                 |
|        |            | - Added TTempSmoothF(), a faster version of TTempSmooth (50-75%), which is         |
|        |            |   equivalent to using TTempSmooth() with mdiff/mdiffC equal to or greater then     |
|        |            |   LThresh/CThresh. Thus, it only takes maxd, LThresh, and CThresh as parameters.   |
|        |            | - Changed defaults to be less aggressive.                                          |
|        |            | - There were no quality changes, the same settings will produce the same           |
|        |            |   output as in v0.9.1.                                                             |
+--------+------------+------------------------------------------------------------------------------------+
| v0.9.1 | 06/15/2004 | - Fixed a major bug that caused incorrect calculation of the chroma                |
|        |            |   differences, rendering the filter completely useless.                            |
|        |            | - Got rid of amount/amountC and fixedPoint parameters. They were unneeded in       |
|        |            |   the case of amount and fixedPoint = true always worked better then false...      |
|        |            | - Small speed up                                                                   |
|        |            | - Changed some of the defaults.                                                    |
+--------+------------+------------------------------------------------------------------------------------+
| v0.9   | 06/11/2004 | - Initial Release                                                                  |
+--------+------------+------------------------------------------------------------------------------------+

$Date: 2012/07/03 07:44:18 $

.. _"DebugView" from sysinternals.:
    http://www.sysinternals.com/Utilities/DebugView.html

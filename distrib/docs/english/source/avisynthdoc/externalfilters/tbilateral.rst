
TBilateral
==========


Abstract
--------

| **author:** tritical
| **version:** 0.9.10
| **download:** `<http://bengal.missouri.edu/~kes25c/>`_
| **category:** Spatial Smoothers
| **requirements:**

-   YV12 & YUY2 Colorspace

**license:** GPL

--------


Description
-----------

TBilateral is a spatial smoothing filter that uses the bilateral filtering
algorithm. It does a nice job of smoothing while retaining picture structure.
It currently supports YV12 and YUY2
colorspaces and has a gui with preview to help with tweaking.


Syntax
------

``TBilateral`` (clip, int "diameterL", int "diameterC", float "sDevL", float
"sDevC", float "iDevL", float "iDevC", float "csL", float "csC", bool "d2",
bool "chroma", bool "gui", PClip "ppClip", int "kernS", int "kernI", int
"resType")


PARAMETERS
----------

*diameterL/diameterC* - (spatial diameters)

This sets the size of the diameter of the filtering window. Larger values
mean more pixels will be included in the average, but are also slower. Must
be an odd number greater then 1.
diameterL is for luma and diameterC is for chroma if it is being processed.
This must be less then the width of the video and less then the height of the
video.

- default

  - diameterL = 5 (int)
  - diameterC = 5 (int)

*sDevL/sDevC* - (spatial (domain) deviations)

These set the spatial deviations. The larger sDev is, the less effect
distance will have in the weighting of pixels in the average. That is, as you
increase sDev distant pixels will have more weight. sDevL is for luma and
sDevC is for chroma. These must be greater then 0. To get a better idea of
what these settings do try setting iDevL/iDevC to high values, and then
gradually increase sDevL/sDevC from 0 on up while keeping iDevL/iDevC
constant. Increasing these settings will increase the strength of the
smoothing.

- default

  - sDevL = 1.4 (float)
  - sDevC = 1.4 (float)

*iDevL/iDevC* - (pixel intensity (range) deviations)

These set the pixel intensity deviations (or color deviations in the case of
the chroma planes). The larger iDev is, the less effect pixel difference will
have in the weighting of pixels in the average. That is, as you increase iDev
pixels that are very different from the current pixel will have more weight.
iDevL is for luma and iDevC is for chroma. These must be greater then 0. Try
increasing these settings while keeping sDev/sDevC constant to get a better
idea of what these do. Increasing these settings will increase the strength
of the smoothing.

- default

  - iDevL = 7.0 (float)
  - iDevC = 7.0 (float)

*csL/csC* - (center pixel weight multipliers)

These values are multiplied to the center pixel's spatial weight value. A
value of 1 does nothing, < 1 means the center pixel will have less weight
then normal, > 1 means the center
pixel will have more weight then nomral, 0 gives the center pixel no weight.
These must be at least 0. Setting csL/csC to 0 will give you SUSAN denoising.

- default

  - csL = 1.0 (float)
  - csC = 1.0 (float)

*d2* - (second derivative or first derivative)

This setting makes TBilateral use the second derivative instead of the first
when doing the intensity calculations. Using d2 should give better results on
smooth gradients or anything that fits the piecewise linear model. Setting d2
to false will give better results on images that have uniformly colored areas
with sharp edges (anything that fits the piecewise constant
model). The actual difference between the two is usually not big for most
sources. The effect is rather subtle.

- default

  - false (bool)

*chroma* - (enable chroma processing)

If set to true the chroma planes are processed. If set to false the chroma
planes from the source are simply copied to the final output frame.

- default

  - true (bool)

*gui* - (gui interface)

| If set to true, then this instance of TBilateral will start with a gui. Only
  one instance of TBilateral per script can have a gui. The gui allows you to
  change the values of
| ``diameterL/diameterC/sDevL/sDevC/iDevL/iDevC/csL/csC/d2/chroma/kernS/kernI/res``
  Type (basically every setting). The gui also has a real time updating preview
  which allows you to see the effect of changes as you make them.

The gui does have some restrictions though. The diameterL/diameterC values
are currently limited to a maximum of 21 when using the gui interface. Also,
the Dev sliders have maximum limits associated with them to allow the use of
the slider bars to tweak them. However, these maximums should be plenty high
to allow even very strong filtering.

After you have found the settings you want you can close the gui by hitting
the "OK" button. At the bottom of the advanced tab there is string which you
can copy and paste in your avs script to duplicate the current settings.
(NOTE: the ppClip parameter is not set in this string)

- default

  - false (bool)

*ppClip* - (pre-process clip)

Specifies a pre-filtered clip for TBilateral to take pixel values from when
doing the luminance difference calculations. The general recommendation for
pre-processing is
a gaussian blur with standard deviation equal to the sDev settings being
used. Using a prefiltered clip should help in removing impulse noise (i.e.
outliers with very
large pixel differences) which standard bilateral filtering will not touch.
It does tend to produce artifacts sometimes, especially around very fine
details. Another recommendation
for pre-processing is a center-weighted median or adaptive median.

- default

  - NULL (PClip)

*kernS/kernI* - (domain (spatial) and range (intensity) kernels)

These specify what kernel is used for the domain (kernS) weights and range
(kernI) weights. The possible choices are:

- 0 - Andrews' wave
- 1 - El Fallah Ford
- 2 - Gaussian
- 3 - Huber's mini-max
- 4 - Lorentzian
- 5 - Tukey bi-weight
- 6 - Linear descent
- 7 - Cosine
- 8 - Flat
- 9 - Inverse

See the following paper for a description of all the kernels and their
properties:

`<http://dsp7.ee.uct.ac.za/~jfrancis/publicationsDir/PRASA2003.pdf>`_

Gaussian (option 2) is the kernel used by the default (first proposed)
bilateral filter.

- default

  - 2 (kernS) (int)
  - 2 (kernI)

*resType* -

This specifies how the weights and pixel values are combined to obtain the
final result. Possible options:

- 0 - Mean (weighted average)
- 1 - Median (weighted median)
- 2 - CW-Median (weighted median + extra center pixel weight)

Option 0 (weighted average) is the type used by the default bilateral filter.

- default

  - 0 (int)

+---------------------------------------------------------------------------------------------------------------------------------+
| Changelog                                                                                                                       |
+=========+============+==========================================================================================================+
| v0.9.10 | 06/23/2005 | - some optimizations, roughly 15-20% speed increase                                                      |
+---------+------------+----------------------------------------------------------------------------------------------------------+
| v0.9.9  | 06/21/2005 | - Added Inverse kernel (what was called "Linear Descent" in v0.9.8 was, in fact, "Inverse")              |
|         |            | - Fixed a bug in the gui routine that set the sDevL/sDevC slider maximums                                |
+---------+------------+----------------------------------------------------------------------------------------------------------+
| v0.9.8  | 06/20/2005 | - Added kernS, kernI, and resType                                                                        |
|         |            | - Redid the gui interface                                                                                |
|         |            | - Lots of cosmetic fixes for the gui (for some options it wouldn't                                       |
|         |            |   immediately update on change)                                                                          |
+---------+------------+----------------------------------------------------------------------------------------------------------+
|         | 05/24/2005 | - Added ppClip parameter and processing                                                                  |
|         |            | - Changed some default values                                                                            |
|         |            | - Fixed a few thread sync/exit problems with the gui code                                                |
+---------+------------+----------------------------------------------------------------------------------------------------------+
| v0.9.6  | 05/02/2005 | - Added gui with preview                                                                                 |
|         |            | - Removed all yuy2 restrictions (d2)                                                                     |
|         |            | - Fixed iDev settings and sDev settings being switched when calculating the weight tables (doh!)         |
+---------+------------+----------------------------------------------------------------------------------------------------------+
| v0.9.5  | 06/16/2004 | - Fixed iDevC having an i instead of f inside the function param list and only excepting integer values. |
+---------+------------+----------------------------------------------------------------------------------------------------------+
| v0.9.4  | 06/16/2004 | - Fixed the rouding errors with d2 = true, which could have led to +-2 change in the final pixel values  |
|         |            | - The change needed for the rounding fix also sped up d2 = true processing 10-15%                        |
+---------+------------+----------------------------------------------------------------------------------------------------------+
| v0.9.3  | 06/15/2004 | - Added d2 and centerScale/centerScaleC parameters.                                                      |
|         |            | - Unrolled yloops, no real speed up, but it did make the code huge...                                    |
+---------+------------+----------------------------------------------------------------------------------------------------------+
| v0.9.2  | 06/12/2004 | - Unrolled xloops in YV12 and the luma only version of YUY2. Slight speed up.                            |
+---------+------------+----------------------------------------------------------------------------------------------------------+
| v0.9.1  | 06/11/2004 | - Fixed a problem with boundary pixels near the egdes of the picture getting incorrect spatial weights.  |
+---------+------------+----------------------------------------------------------------------------------------------------------+
| v0.9.0  | 06/11/2004 | - Initial Release                                                                                        |
+---------+------------+----------------------------------------------------------------------------------------------------------+

$Date: 2005/07/10 16:11:01 $


MipSmooth
=========


Abstract
--------

| **author:** Klaus Post (Sh0dan)
| **version:** 1.11
| **download:** `<http://cultact-server.novi.dk/kpo/avisynth/mipsmooth.html>`_
| **category:** Spatio-Temporal Smoothers
| **requirements:**

-   YV12 & YUY2 & RGB Colorspace
-   ISSE support
-   width divisible by 4 for YV12 and by 8 for YUY2

--------


Description
-----------

This is the MipSmoother - a reinvention of SmoothHiQ and Convolution3D.
MipSmooth was made to enable smoothing of larger pixel areas than 3x3(x3), to
remove blocks and smoothing out low-frequency noise. It is my hope that when
this filter has been tweaked it will be better than C3D at smoothing out flat
surfaces and gradients. This should be good for cartoons/anime, but it
actually also helps film footage quite nicely.


Syntax
------

Basic syntax: ``MipSmooth`` - all parameters are optional.

Extended syntax:

``MipSmooth`` (clip, optional arguments)

There are a number of arguments that control how much smoothing will be
applied:

*preset* = "MovieHQ" / "MovieLQ" / "AnimeHQ" / "AnimeLQ" / "VHS" / "Rainbow" /
"Custom"

Loads a built-in preset as new defaults. See the preset section for more
information.

::

    spatial = 5,
    temporal = 4,
    spatial_chroma =6,
    temporal_chroma =54,
    scenechange = 2.0

All numbers are valid in the ranges 0 - 255 and this defines the threshold
for each mode of blending.
The thresholds can be compared to the ones of C3D, except higher thresholds
smooth much less than C3D.
The default settings are quite conservative, and only slightly smooths out
surfaces.

"Temporal influence" is replaced by overall scenechange detection.
Scenechanges will disable temporal processing in either direction. Use "show
= true" to display the values on each frame.

*method*  = "strong" / "soft" /  "supersoft" / "superstrong"  -  default:
"supersoft"

This setting controls how much blurring is applied. The strong settings makes
the filter have a larger impact, whereas soft is more gentle. In general
strong should give best results on anime, and soft should be best for filmed
material.

SuperSoft / SuperStrong uses a slightly different approach, and in general
leads to fewer artifacts. These two are an general a bit softer than non-
super modes.

| *downsizer* = "reduce", "point", "bilinear", "bicubic", "lanczos"  -  default:
  "reduce"
| *upsizer* = "point", "bilinear", "bicubic", "lanczos"  -  default: "bilinear"

These two parameters control how the lower resolution maps are created. There
might not be a very big visual impact, when using different resizers.
Experiments are welcome!

| *scalefactor* = 0.5
| This option sets the scale of each mipmap. This is how much each frame is
  reduced in each iteration. The original resolution is multiplied by this
  value.  Usable values are from 0.2 to 4.0, however only values between 0.4
  and 1.5 are recommended. When this setting is 1.0 no spatial blurring is
  done, but this isn't recommended - use spatial = 0 instead. Experiment with
  values above 1.0 - they might be useful.
  This affects the radius of the blur. The lower the scalefactor is, the bigger
  the radius of the blur - and the more crosstalk you'll get.

::

    MipSmooth(downsizer="lanczos", upsizer="bilinear", scalefactor=1.5, method = "strong")

The above script produces very nice gentle smoothing for instance, but is
very slow.

| *weigh* = true / false
| This option selects whether mips should be weighed based on how blurred they
  are.  The most blurred mips are weighed the lowest.  This option is equal to
  "Weighed Average" in SmoothHiQ, or "soft" matrix in Convolution3D.

This option has only effect if SuperMip-mode is used.  It reduces the amount
of blur when turned on, so it can be used to achieve higher thresholds
without artifacts.

| *show* = true / false
| Shows information about scenechange differences, and the settings being used.
| This setting will have no effect in RGB24 mode - use RGB32 instead!

| *storecustom* = true / false
| This will store the settings being used in the registry.
| To reinvoke these presets later, use preset = "custom" parameter.
| This will not automatically become the default parameters.


Presets
-------

Presets are a set of builtin settings to help you get started. The names of
the parameters are not case sensitive.

All presets are loaded as default values. That means that it is still
possible to finetune preset settings by setting individual parameters.

For example: MipSmooth(preset = "movieHQ", scenechange = 8) will use the
MovieHQ, but with scenechange set to 8.

**MovieHQ**

This preset is for natual movies in high quality.

::

    Spatial: 4
    Temporal: 5
    Spatial Chroma: 5
    Temporal Chroma: 7
    Scenechange: 2.0
    Method: SuperSoft
    Downsizer: Bilinear
    Upsizer: Bilinear
    Scalefactor: 0.85
    Weigh = true

**MovieHQ2**

This preset is for natual movies in high quality.

::

    Spatial: 2
    Temporal: 2
    Spatial Chroma: 3
    Temporal Chroma: 3
    Scenechange: 2.5
    Method: SuperSoft
    Downsizer: Bilinear
    Upsizer: Bilinear
    Scalefactor: 0.75
    Weigh = true

**MovieLQ**

This preset is for natural movies that need more smoothing, to have more
blocks and noise removed.

::

    Spatial: 8
    Temporal: 7
    Spatial Chroma: 10
    Temporal Chroma: 8
    Scenechange: 3.5
    Method: SuperSoft
    Downsizer: Bilinear
    Upsizer: Bilinear
    Scalefactor: 0.65
    Weigh = true

**MovieLQ2**

This preset is for natural movies that need more smoothing, to have more
blocks and noise removed.

::

    Spatial: 4
    Temporal: 4
    Spatial Chroma: 5
    Temporal Chroma: 5
    Scenechange: 3.5
    Method: SuperStrong
    Downsizer: Bilinear
    Upsizer: Bilinear
    Scalefactor: 0.65
    Weigh = true **VHS** ::Spatial: 9
    Temporal: 6
    Spatial Chroma: 10
    Temporal Chroma: 9
    Scenechange: 5.0
    Method: SuperStrong
    Downsizer: Bilinear
    Upsizer: Bilinear
    Scalefactor: 0.60
    Weigh = true

**AnimeHQ**

This is for slight blockremoval and gradient restoration. It has more spatial
smoothing than ordinary video to help recreate flat and gradient surfaces.

::

    Spatial: 6
    Temporal: 8
    Spatial Chroma: 6
    Temporal Chroma: 8
    Scenechange: 5.0
    Method: SuperSoft
    Downsizer: Bicubic
    Upsizer: Bicubic
    Scalefactor: 0.60
    Weigh = true

**AnimeLQ**

This is for more noisy material with more blocks and noise.

::

    Spatial: 5
    Temporal: 5
    Spatial Chroma: 5
    Temporal Chroma: 7
    Scenechange: 5.5
    Method: SuperStrong
    Downsizer: Bilinear
    Upsizer: Bicubic
    Scalefactor: 0.5
    Weigh = true

**Rainbow**

This might help removing rainbow flicker. See `this thread at Doom9`_ for
info and alternatives. Reduce scalefactor for even more rainbow removal, but
more chroma blurring.

::

    Spatial: 0
    Temporal: 0
    Spatial Chroma: 255
    Temporal Chroma: 255
    Scenechange: 2.0
    Method: SuperStrong
    Downsizer: Bilinear
    Upsizer: Bilinear
    Scalefactor: 0.65
    Weigh = true

**Custom**

| This will read settings that has been stored in the registry, and use these
  as default settings.
| If there is no settings stored in the registry, an error will be thrown.
| At any time it is possible to use "storecustom = true" this will then store
  the current settings in the registry, and these will then be used as the
  custom parameters


Examples & Tricks
-----------------

Try experimenting with quite hard softening with very low thresholds. For
example:
::

    MipSmooth(spatial=2, temporal=2, method="superstrong", scalefactor=0.75, weigh=true)

This is a quite good setting for high quality material, which will stabilize
the image and give much better compression and still retaining a quite high
amount of detail. These modes are implemented as "MovieHQ2" and "MovieLQ2"
presets.

::

    MipSmooth(spatial=255, scenechange=0, method="soft")
    # This will give a very soft image - seen in some commercials and soap operas.

    MipSmooth(spatial=255, scenechange=0, method="strong", scalefactor=0.65)
    # This will smooth the image very much like a soft gaussian blur.

    MipSmooth(spatial=255, scenechange=255, temporal=255, method="superstrong", scalefactor=0.6, weigh=false)
    # This will also give a very soft image with some temporal blurring.

When cropping right before this filter, use "align=true" parameter, available
in AviSynth 2.5.3 and later. This will in most cases give a slight speedup.


Background information
----------------------

"Mip" comes from "MipMap", which is a term used in the realtime 3D-graphics
world, and basicly describes a downsampled version of a texture (bitmap
surfaces of 3D objects), that is used, when the object is far away.  MipMaps
are always half the size of the original image.

What MipSmooth does is actually very simple:

It takes the source frame, and creates three new versions, each half the size
of the previous. This is done by using `ReduceBy2`_, or a selectable
`resizer`_. These frames are then all scaled back up to original size using
BilinearResize.  These frames are then compared to the original, and if the
difference is below the threshold, the information is used to form the final
pixel.

The same is done to the previous and the next frame - and all these three
frames and their "blurred" mipmaps are used to reconstruct each pixel.
Threshold is adjusted to that more blurred images are given a lower threshold
than the sharper versions.

Supermip first creates three frames (supermips) from spatially blurred images
and does a temporal softening on these three framres. This greatly reduces
artifacts on heavy denoising/blurring.


Further possible improvements
-----------------------------

-   Adjustable number of mips.
-   Image examples in documentation.
-   GUI.

$Date: 2004/08/13 21:57:25 $

.. _this thread at Doom9:
    http://forum.doom9.org/showthread.php?s=&threadid=62873
.. _ReduceBy2: ../corefilters/reduceby2.rst
.. _resizer: ../corefilters/resize.rst

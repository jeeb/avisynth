
AutoCrop
========


Abstract
--------

| **author:** Glenn Bussell
| **version:** 1.2
| **download:** `<http://www.avisynth.org/warpenterprises/>`_
| **category:** Broadcast Video Plugins
| **requirements:** YV12 & YUY2 Colorspace
| **license:** GPL

--------


Description
-----------

AutoCrop is an AviSynth filter that automatically crops the black borders
from a clip. It operates in either preview mode where it overlays the
recommended cropping information on
the existing clip, or cropping mode where it really crops the clip. It can
also ensure width and height are multiples of specified numbers so the
cropped clip can be passed to the video compressor of your choice without
problems.


Usage
-----

AutoCrop takes 12 parameters but they are all optional so you can just do
AutoCrop() to get a feel for how the filter works. Parameters can either be
specified in order or by name. I suggest specifying by name as there is a lot
of parameters!

Parameter order follows.

``AutoCrop`` (clip, "mode", "wMultOf", "hMultOf", "leftAdd", "rightAdd",
"topAdd", "bottomAdd", "threshold", "samples", "samplestartframe",
"sampleendframe", "aspect")


Syntax
------

*mode* - Integer - default 1

+------+------------+---------------------------------------------------------------------------+
| 0    | Crop       | Crops the image.                                                          |
+------+------------+---------------------------------------------------------------------------+
| 1    | Preview    | Suggested cropping information is overlayed on the existing               |
|      |            | clip, including a crop command that you can use to replace AutoCrop with. |
+------+------------+---------------------------------------------------------------------------+
| 2    | Log        | Logs the cropping parameters to the file "AutoCrop.log" in the            |
|      |            | current directory.                                                        |
+------+------------+---------------------------------------------------------------------------+
| 3    | Crop & Log | combination of modes 0 and 2.                                             |
+------+------------+---------------------------------------------------------------------------+

*wMultOf* - Integer - default 4

Ensures that the width of the cropped clip will be a multiple of the number
specified. Use this to ensure that clip is a valid for whatever codec you are
going to compress with.

*hMultOf* - Integer - default 2

As above but for height

*leftAdd* - Integer - default 0

Forces autocrop to crop and additional leftAdd pixels after it completes auto
detection. You can use this option in conjunction with wMultOf. Useful for
cropping out bad quality lines on the side of VHS captures.

*rightAdd* - Integer - default 0

See above but for the right.

*topAdd* - Integer - default 0

Forces autocrop to crop and additional topAdd pixels after it completes auto
detection. You can use this option in conjunction with hMultOf. Useful for
cropping out bad quality lines on the top of VHS captures.

*bottomAdd* - Integer - default 0

See above but for the bottom.

*threshold* - Integer (0-255) - default 30

Threshold is the average luminance a line must have before it's considered
non blank. For DVD sources values as low as 20 should work reliably. For VHS
sources raising the value maybe neccessary.

*samples* - Integer - default 5

The number of frames to examine when determining the cropping information.
This directly affects the startup time of the filter. This number
dramatically effects the amount of time taken for the filter to startup.
Increasing the number to 10 will basically double the startup time.

*samplestartframe* - Integer - default 0 (first frame)

Sets the first frame to be looked at when determining the cropping
parameters.

*sampleendframe* - Integer - default -1 (last frame)

Sets the last frame to be looked at when determining the cropping parameters.
If you want to take the cropping information from a single frame set
samplestartframe and sampleendframe to the same number and samples to be 1.

aspect - Float - default 0 - Aspect is ignored

- -1 - Maintain aspect ratio of the source clip
- 0 - Aspect is ignored
- >0 - Aspect ratio will be set to this. For example for a 4:3 aspect ratio you
  would pass
- 4.0/3.0 or 1.33333333333. Note 4/3 won't work the .0 is need to tell
  AviSynth the value is a float.


Recommended use
---------------

I strongly suggest using the preview mode before committing to a particular
crop. It's also worth noting that copy the suggested settings in to the
standard AviSynth crop command is quicker than running AutoCrop with  mode =
0.


+--------------------------------------------------------------------------------------------------------------------------+
| Changes                                                                                                                  |
+==================+=============================+=========================================================================+
| v1.2             | 3rd January 2005 (by len0x) | - fixed bug when preview and actual crop values were different          |
|                  |                             | - fixed bug when AR was not properly enforced (rounding errors)         |
|                  |                             | - threshold 0 ensures that no cropping is done unless necessary for AR  |
|                  |                             | - fixed bug when leftAdd and rightAdd parameters were not properly      |
|                  |                             |   working in YUY2 mode                                                  |
|                  |                             |                                                                         |
|                  |                             |     - wMulfOf and hMultOf cannot be zero now                            |
|                  |                             |     - wMultOf is relaxed to be mod2 for YV12 (previously mod4)          |
|                  |                             |     - added mode=3 which is mode 0 plus mode 2 (cropping ang logging at |
|                  |                             |       the same time)                                                    |
|                  |                             |                                                                         |
+------------------+-----------------------------+-------------------------------------------------------------------------+
| previous changes | (by CropsyX)                | - Added ability to set a range of frames to sample for cropping         |
|                  |                             |   information.                                                          |
|                  |                             | - Reduced default number of frames to sample to 5.                      |
|                  |                             | - Reduced default threshold to 30, this change and the one above should |
|                  |                             |   perform give equal or better performance for DVD sources much quicker.|
|                  |                             | - Minor documentation updates.                                          |
+------------------+-----------------------------+-------------------------------------------------------------------------+

This version of AutoCrop is compiled for AviSynth 2.5. It will not work with
any of the 2.0 releases. If you need to use AviSynth 2.0 please download
version 0.3 of this filter
from `http://www.videofringe.com/autocrop`_.

$Date: 2005/10/01 23:09:51 $

.. _http://www.avisynth.org/warpenterprises/:
    http://www.avisynth.org/warpenterprises/
.. _http://www.videofringe.com/autocrop:
    http://www.videofringe.com/autocrop

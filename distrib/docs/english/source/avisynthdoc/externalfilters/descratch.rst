
DeScratch
=========


Abstract
--------

| **author:** Alexander G. Balakhnin aka Fizick
| **version:** 0.9.0.0
| **download:** `<http://avisynth.org.ru/>`_
| **category:** Broadcast Video Plugins
| **requirements:** YV12 Colorspace

--------


Purpose
-------

This plugin removes vertical scratches from films. Also it can be used for
removing of horizontal noise lines such as drop-outs from analog VHS captures
(after image rotation).

How it works
------------

The plugin firstly detects scratches, then removes them.
It works with every frame, uses spatial information only from current frame.
I adapt it to restoration of my old 8 mm films, may be it will be useful to
somebody else.


Scratch detecting
~~~~~~~~~~~~~~~~~

- Apply some vertical blur to frame copy, for suppression of image thin
  structure, inclined lines and noise.
- Search for local extremes of luma in every row, with luma difference
  criterion.
- Put these extremes in some map (frame).
- Optional close vertical gaps in extrems by vertical expanding of extreme
  points.
- Test extremes map with length and angle criterions, so select real long
  scratches only.


Scratch removing
~~~~~~~~~~~~~~~~

- Scratches may be either partially transparent, smooth (with image details),
  or opaque (with no details or almost destroyed).
- In the first case, plugin can subtract smooth (blurred) part of luma
  scratches variation from original image. Therefore, image details are kept.
- In the second case, plugin replaces scratched pixels by mean luma values from
  some neighbours pixels (in same row).
- We have also intermediate case by setting some percent of detail to keep.
- The plugin have some adaptation of keeped value according to pixels luma
  difference.
- In all cases, some nearest neighbours may also partially changed for more
  smoothed transition.


Needed software
---------------

- The program is plugin (filter) for free Avisynth 2.5.
- Tested with version 2.55beta (some versions have bug with Turnleft()! ).
- Filter versions above 0.2 is no more needed in Avisynth C Interface
  (avithynth_c.dll) by Kevin Atkinson.
- Compiled with MS VC++ Toolkit 2003.


Calling syntax
--------------

``DeScratch`` (int mindif, int asym, int maxgap, int maxwidth, int minlen, int
maxlen, int maxangle, int blurlen, int keep, int border, int modeY, int
modeU, int modeV, int mindifUV, bool mark)

All parameters are named and optional.


Plugin Parameters
-----------------

| *mindif* - minimal difference of pixel value in scratch from neighbours pixels for luma plane
    (from 1 to 255, default 5):
| *asym* - maximal asymmetry of neighbors pixels (from 0 to 255, default 10)
| *maxgap* - maximal vertical gap to be closed (from 0 to 255, default 3)
| *maxwidth* - maximal scratch width (1 or 3, default=3)
| *minlen* - minimal scratch length (default = 100)
| *maxlen* - maximal scratch length (default = 1000)
| *maxangle* - maximal angle to vertical (in degrees, default = 5)
| *blurlen* - radius of vertical blur for frame analysis (default = 15)
| *keep* - percent of scratch detail to keep (default = 100)
| *border* - thickness of border near scratch for partial restoration (default = 2)
| *modeY* - processing mode for luma plane (0 - no, 1 - low(black), 2 - high(white), 3 - both, default=1)
| *modeU* - processing mode for chroma U plane (0 - no, 1 - low(green), 2 - high(red), 3 - both, default=0)
| *modeV* - processing mode for chroma V plane (0 - no, 1 - low(yellow), 2 - high(blue), 3 - both, default=0)
| *mindifUV* - minimal difference of pixel value in scratch from neighbours pixels for chroma planes
    (from 0 to 255, default 0):
    if = 0, then internal mindifUV value is same as mindif.
| *mark* - mark scratch in debug mode (true or false, default = false)
    (set rejected extrems pixels to grey, set scratches pixels to white or black)

*maxgap*, *maxwidth*, *minlen*, *blurlen*, *border* should be given in pixels.

| You MUST tune parameters for your video.
| Use AviSynth commands ``Greyscale(), UtoY(), VtoY()``, and mark parameter for
  debug and tuning.


Examples
--------

Old scratched 8 mm film. Top half frame - before filter, bottom half frame -
after filter

.. image:: pictures/descratch1.jpg



Sample script for Avisynth (used for sample clip above)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

::

    AviSource("input.avi")
    LoadPlugin("descratch.dll")
    ConvertToYV12()
    DeScratch(mindif=2)

Horizontal noisy lines removing
-------------------------------

Such long thin noisy lines sometimes appears in analog capture material (TV
or VHS line drop-out, etc). This problem was discussed at Doom9, see article
`Removal of clicks and scratches`_ (July 10-31, 2004). But in the article
DeSpot plugin was used, what is not most appropriate tool for this, due to
very weak noise level. DeScratch could be more effective, however is not
ideal too (old versions can process luma plane only). Of course, you must
rotate clip for processing with Descratch (and do not forget rotate it in
reverse direction after cleaning :-).

See results for some VHS source (from **Ivo**).
Top half frame - before processing , bottom half frame - after processing:

.. image:: pictures/descratch2.jpg



Script used for VHS example clip
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

::

    LoadPlugin("descratch.dll")
    AviSource("drop-outs_.avi")
    ConvertToYV12()
    Crop(0,0,-0,288) # select top part of frame to save screen space
    input=last
    AssumeTFF()
    SeparateFields() # for analog interlaced source
    TurnLeft()
    DeScratch(mindif=4, maxgap=20, minlen=300, blurlen=50, keep=100,
    border=0, maxangle=0)
    TurnRight()
    Weave() # restore fields
    # Compare source frame and frame after scratch removing
    StackVertical(input, last)

Optimal Descratch parameters used - big minlen, null maxangle):

For color line dropouts removing, you can use modeU,  modeV and  mindifUV
parameters.


More info
---------

| See doom9 Avisynth forum, special thread
| `<http://forum.doom9.org/showthread.php?s=&threadid=67794>`_


License
-------

This program is FREE software, under GNU GPL licence v2.

+-----------------------------------------------------------------------------------------------------------------------+
| Changelog                                                                                                             |
+==========+====================+=======================================================================================+
| v0.1     | 22 December 2003   | initial beta (not public)                                                             |
+----------+--------------------+---------------------------------------------------------------------------------------+
| v0.2     | 31 December 2003   | - first public                                                                        |
|          |                    | - changed to sharp extrems;                                                           |
|          |                    | - added gaps closing                                                                  |
+----------+--------------------+---------------------------------------------------------------------------------------+
| v0.3     | July 14, 2004      | - ported to native Avisynth plugin                                                    |
|          |                    | - Speed increased due to fast blurring by Avisynth resizing functions.                |
+----------+--------------------+---------------------------------------------------------------------------------------+
| v0.4     | July 17, 2004      | - Added chroma processing: modeY, modeU, modeV parameters,                            |
|          |                    | - introduced in version 0.4 after Ivo's request (old versions can process luma only). |
|          |                    | - mindif is now positive only;                                                        |
|          |                    | - marked scratches value is not inverted but contrasted now.                          |
+----------+--------------------+---------------------------------------------------------------------------------------+
| v0.5     | July 31, 2004      | - fixed read-write pointer bug,                                                       |
|          |                    | - add mindifUV parameter for separate control of chroma U,V planes cleaning,          |
|          |                    | - some code reorganization,                                                           |
|          |                    | - documentation corrected.                                                            |
+----------+--------------------+---------------------------------------------------------------------------------------+
| v0.6     | August 23, 2004    | fixed byte overflow bug (clip output to 0-255)                                        |
+----------+--------------------+---------------------------------------------------------------------------------------+
| v0.7     | November 15, 2004  | added asymmetry check (asym parameter)                                                |
+----------+--------------------+---------------------------------------------------------------------------------------+
| v0.8     | March 13, 2005     | added maxlen parameter                                                                |
+----------+--------------------+---------------------------------------------------------------------------------------+
| v0.9     | March 31, 2005     | maxwidth parameter partially unfixed, allowed value =1 or 3 (was 3 only)              |
+----------+--------------------+---------------------------------------------------------------------------------------+
| v0.9.0   | June 15, 2005      | documentation re-formatted                                                            |
+----------+--------------------+---------------------------------------------------------------------------------------+
| v0.9.0.0 | September 09, 2006 | source licensed under GNU GPLv2                                                       |
+----------+--------------------+---------------------------------------------------------------------------------------+


Current version limitations
---------------------------

| Maximal scratch width is fixed to 1 or 3.
| The plugin works only in YV12.

$Date: 2006/12/15 19:29:25 $

.. _Removal of clicks and scratches:
    http://www.doom9.org/capture/descratch.html


ChromaShift
===========


Abstract
--------

| **author:** Simon Walters
| **version:**  2.7
| **download:** `<http://www.geocities.com/siwalters_uk/fnews.html>`_
| **category:** Misc Plugins
| **requirements:** YV12 & YUY2 & RGB32 Colorspace

--------


Description
-----------

This filter will shift the chrominance information (C == U and V together) or
the U or the V separately, by an even number of pixels, in either horizontal
direction. It can also apply an overall vertical shift of the total
chrominance information, up or down. This applies to clips in YUY2 or YV12
colourspaces.

Wilbert Dijkhof has kindly modified it so that it can also be used to shift
Red, Green or Blue colours independently if the input to the filter is in
RGB32 colourspace.

This filter is primarily intended to correct improper colour registration.

`Download Version 2.7 for Avisynth 2.5 (inc source)`_

`Download OldVersion 1.2 for Avisynth 2.0.x`_ (just operates in YUY2
colourspace)

Example Avisynth Syntax

# Shift both U and V info left by 2 pixels and down by one line
# Note U and/or V cannot be shifted by 1 pixel as each UV pair of values are
tied to 2 pixels in YUY2 colourspace

::

    LoadPlugin("ChromaShift.dll")
    AVISource("Test.avi")
    ConvertToYUY2 # if needed.
    ChromaShift(C=-2,L=1)

# Shift Red info to the left by 2 pixels and Green info to the right by 1
pixel.

::

    LoadPlugin("ChromaShift.dll")
    AVISource("Test.avi")
    ConvertToRGB32 # if needed.
    ChromaShift(R=-2,G=1)

# Shift both U and V info left by 2 pixels and down by 2 lines
# Note U and/or V cannot be shifted by 1 pixel as each UV pair of values are
tied to 4 pixels in YV12 colourspace
# L cannot be shifted by a non-even number of lines in YV12 colourspace for
the same reason

::

    LoadPlugin("ChromaShift.dll")
    AVISource("Test.avi")
    ConvertToYV12 # if needed.
    ChromaShift(C=-2,L=2)

The full parameter list is ``ChromaShift`` (clip, int "C", int "U", int "V",
int "L", int "R", int "G", int "B")

Any non-even parameter value for C, U or V will throw an exception as
chrominance information is only held once for every 2 pixels in a YUY2 bitmap
frame. L can be any value in YUY2 colourspace but must be even in YV12
colourspace.

Any value for C, U, V or L will give an error when using RGB32 colourspace.

Any value for R, G or B will give an error when using YUY2 or YV12
colourspace.

+------------------------------------------------------------------------------+
| Version History                                                              |
+=======+====================+=================================================+
| v2.7  | 4th November       | YV12 handling added.                            |
+-------+--------------------+-------------------------------------------------+
| v2.6a | 4th November       | RGB handling added in by Wilbert Dijkhof.       |
+-------+--------------------+-------------------------------------------------+
|       | 2nd November       | included missing source code.                   |
+-------+--------------------+-------------------------------------------------+
| v2.5  | 30th January, 2003 | re-compiled to work with Avisynth 2.5.          |
+-------+--------------------+-------------------------------------------------+
| v1.2  | 14th Sep, 2002     | added abilty to shift left as well as right and |
|       |                    | removed the restriction on single use of the    |
|       |                    | C, U or V parameters. Also, added the L (Line)  |
|       |                    | parameter to shift vertically. Code speeded up  |
|       |                    | by not using a buffered frame.                  |
+-------+--------------------+-------------------------------------------------+
| v1.1  | 13th Sep, 2002     | added parameters and control over C,U and V.    |
+-------+--------------------+-------------------------------------------------+
|       | 10th Sep, 2002     | First release for Avisynth                      |
+-------+--------------------+-------------------------------------------------+

Source Code
-----------

All source code here is provided under the GPL license. This means you are
free to use and distribute the software and program code for no fee.  This
also means you are free and encouraged to improve and expand upon the source
code, but only as long as you make your modified version also under the GPL,
and thus free software with access to the source code for anyone. The source
code compiles under Visual C++ 6.0 with SP4. If there is a problem with the
source code distribution, please do let me know.

Please email me or post to rec.video.desktop if you have any
queries/comments/ideas/bug reports

Copyright Simon Walters siwalters(at)hotmail.com

$Date: 2004/08/13 21:57:25 $

.. _Download Version 2.7 for Avisynth 2.5 (inc source):
    http://www.geocities.com/siwalters_uk/chromashift27.zip
.. _Download OldVersion 1.2 for Avisynth 2.0.x:
    http://www.geocities.com/siwalters_uk/chromashift12.zip

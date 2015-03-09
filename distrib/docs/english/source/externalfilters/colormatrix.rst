
ColorMatrix
===========


Abstract
--------

| **author:** Wilbert Dijkhof and others
| **version:** 1.9
| **download:** `<http://www.avisynth.org/warpenterprises/>`_
| **category:** Misc Plugins
| **requirements:**

-   YV12 & YUY2 Colorspace
-   none / MMX

**license:** GPL

--------


Usage
-----

``ColorMatrix`` (clip, string "mode", bool  "interlaced", bool  "mmx", bool
"hints", string  "d2v", bool  "debug")


.. _Description of the filter:

Description of the filter
-------------------------

``ColorMatrix`` corrects the colors of MPEG-2 streams of dvds. More
correctly, many MPEG-2 streams use slightly different coefficients (called
Rec.709) for storing the color information than AviSynth's color conversion
routines or the XviD/DivX decoders (called Rec.601) do, with the result that
DivX/XviD clips or MPEG-2 clips encoded by TMPGEnc/QuEnc are displayed with
slighty off colors (which looks like a small difference in brightness). This
can be checked by opening the MPEG-2 stream directly in VDubMod.

This filter recalculates the YUV values (using the default mode =
"Rec.709->Rec.601") assuming the coefficients which are used by
AviSynth/VDub/DivX/XviD, with the consequence that your final encoding
(MPEG-2 or MPEG-4) is displayed correctly. However, you can also use hints =
true instead or specifying the d2v-file d2v = filename which does the
correction automatically if needed. See :ref:`Options` for more information.

In case you captured something or you have a XviD/DivX (both are encoded
Rec.601 coefficients), and you want to encode it to mpeg-2 using CCE (which
assumes Rec.709 coefficients), you should use the following script
(progressive material)

::

    ColorMatrix(clip, mode="Rec.601->Rec.709")

The following converts a YV12/YUY2 stream to RGB using Rec.709 coefficients
(which might be useful if you want to convert DVD to MPEG-2 using TMPGEnc)

::

    ConvertToRGB(clip, matrix="Rec.709")

It should give the same results as
::

    ColorMatrix(clip, mode="Rec.601->Rec.709")
    ConvertToRGB()

As a final note. This filter will **clamp** (= round) your input video to
CCIR-601 compliant values (these ranges are 16-235 for the luma component and
16-240 for the chroma component).


.. _Options:

Options
-------

**mode (default "Rec.709->Rec.601")**

mode can be "Rec.601->Rec.709" or "Rec.709->Rec.601", see
:ref:`Description of this filter <Description of the filter>` for examples of when to use this options.
Note this option will be overrided when using hints = true or d2v = filename.

**interlaced (default false)**

For interlaced material use
::

    Mpeg2source("F:\TestStreams\avs\AguileraGrammies.d2v", info=3) # for dgdecode v1.20 or newer
    ColorMatrix(hints=true, interlaced=true)

or if you don't want to use hints
::

    Mpeg2source("F:\TestStreams\avs\AguileraGrammies.d2v")
    ColorMatrix(interlaced=true)

**mmx (default true)**

Due to rounding differences, the mmx (only present for YV12) and c output is
not exactly the same. The maximum difference on the Y plane is +-2 and for UV
its +-1. An  mmx parameter is included to disable the usage of mmx-
optimizations if you want to though.
::

    ColorMatrix(clip, mode="Rec.601->Rec.709", mmx=false)

**hints (default false)**

DGDecode v1.20 and newer versions output colorimetry hints in the video. The
colorimetry info (see :ref:`Colorimetry`) can be viewed using

::

    Mpeg2source("F:\TestStreams\avs\AguileraGrammies.d2v", info=1)

The hints are used when setting info=3 in Mpeg2source, setting hints = true
in ColorMatrix **and** when using ColorMatrix directly after loading the
video
::

    Mpeg2source("F:\TestStreams\avs\AguileraGrammies.d2v", info=3)
    ColorMatrix(hints=true)

When hints are not passed through (for examle because you are using a wrong
dvd2avi/dgdecode version) it will output an error.

Technically (although I've never seen such streams) the colorimetry info can
be different throughout your video, the hints option will handle this
correctly.

**d2v**

When specifying the d2v file it will take the colorimetry info directly from
the d2v itself
::

    Mpeg2source("F:\TestStreams\avs\AguileraGrammies.d2v")
    ColorMatrix(d2v="AguileraGrammies.d2v")

This is useful when the colorimetry info doesn't change throughout your video
(as is almost always the case), because it is much faster than using hints.
If it does, it will output an error. If the d2v- file is located in a
different folder as the AviSynth script you have to give the full path of the
d2v-file.

For people who are interested, it is this (and consequent) line(s) in the d2v
file

800 **5** 0 8210 0 0 32 32 92 b2 b2 a2 b2 b2 a2 b2 b2 a2

I've made the colorimetry info bold. See :ref:`Colorimetry` for an explanation of
the info.

**debug**

You can use debug = true to check that it is finding the hints. Output debug
information via OutputDebugString() (use DebugView utility to view this
information).


.. _Colorimetry:

Colorimetry
-----------

This is a list of all possibilities according to the mpeg-2 specs and
DGDecode, and behind it how GSpot abbreviates it

+---+----------------------------------------------------+------+
| 1 | ITU-R BT.709                                       | I709 |
+---+----------------------------------------------------+------+
| 4 | FCC (almost the same as ITU-R BT.601)              | FCC  |
+---+----------------------------------------------------+------+
| 5 || ITU-R BT.470-2 (exactly the same as ITU-R BT.601) | I470 |
|   || (recommendation BT.601 is an update BT.470-2)     |      |
+---+----------------------------------------------------+------+
| 6 | SMPTE 170M (exactly the same as ITU-R BT.601)      | S170 |
+---+----------------------------------------------------+------+
| 7 | SMPTE 240M (almost the same as ITU-R BT.709)       | S240 |
+---+----------------------------------------------------+------+

For ColorMatrix we assume I709 = S240 and I470=FCC=S170, because the error
will be very small.


Background information
----------------------

There are several ways to convert a YUV stream to RGB. The most well known
one, uses Rec.601 coefficients. It is for example used in the color
conversion routines of AviSynth, VirtualDub and XviD/DivX. When playing back
a XviD/DivX the stream is converted to RGB using Rec.601 coefficients. The
main issue is that sometimes other coefficients are used for the YUV to RGB
conversion (the other two are Rec.709 coefficients and FCC coefficients). A
problem arises if a stream is encoded using one set of coefficients (Rec.709
coefficients for many dvd streams for example), and somewhere in the
reencoding-processing-playback chain a different set of coefficients is
assumed (Rec.601 coefficients for the XviD/DivX decoder or FCC coefficients
for TMPGEnc/QuEnc or Rec.709 coefficients for CCE). You will get a slightly
color distortion, which looks like a change in brightness (it's not really a
change in brightness, the colors are just slightly off).

How do you know what set of coefficients are using when encoding a MPEG-2
stream? Sometimes the coefficients are stored in the header of the MPEG-2
file (the "matrix coefficients" field in the "sequence display extension").
Newer versions of GSpot will be able to read this part of the header, but
also DGDecode (with Mpeg2source(info=1)) can be used to view them. If this
extension field is not present in the header of the MPEG-2 file, the specs
say we are supposed to use the default Rec.709 coefficients (0.2126, 0.7152,
0.0722).

References
----------

| `users reporting the problem`_ - getting different brightness when comparing
  the avs script with opening the mpeg2 directly VDubMod.
| `background info`_ - doom9 thread about the problem.
  the "matrix coefficients" field specifies a set of coefficients given in
  Table 6-9 of `ISO/IEC 13818-2`_, section 6.3.6 (Rec.709 are not entirely
  correct).
| `ITU-R_BT.709`_ - you can get three free recommendations per valid email
  address.


+------------------------------------------------------------------------------------------------------------------+
| Version                                                                                                          |
+======+==================================+========================================================================+
| v1.9 | 23th February 2005 (by tritical) | - Fixedg the overflow in the rec.601->rec.709 mmx conversion.          |
+------+----------------------------------+------------------------------------------------------------------------+
| v1.8 | 13th February 2005 (by tritical) | - Can use hints from dgdecode.dll (dgdecode v1.2 and higher) when      |
|      |                                  |   hints=true.                                                          |
|      |                                  | - Can use info in d2v when d2v=filename is given.                      |
|      |                                  | - Interlaced support (interlaced=true).                                |
|      |                                  | - Debug information.                                                   |
+------+----------------------------------+------------------------------------------------------------------------+
| v1.7 | 30th January 2005 (by tritical)  | - mmx and other optimizations.                                         |
+------+----------------------------------+------------------------------------------------------------------------+
| v1.6 | 29th January 2005 (by Wilbert)   | - Small corrections (corrected Rec.601->Rec.709 in YV12 mode).         |
+------+----------------------------------+------------------------------------------------------------------------+
| v1.5 | 30th October 2004 (by Wilbert)   | - Corrected mpeg2-coefficients (from ITU-R_BT.709, ISO/IEC 13818-2     |
|      |                                  |   slightly wrong).                                                     |
|      |                                  | - mpeg2-coefficients renamed to Rec.709.                               |
|      |                                  | - mpeg1-coefficients renamed to Rec.601.                               |
|      |                                  | - Removed rgb=true, since the internal ConvertToRGB(clip,              |
|      |                                  |   matrix="Rec.709") is faster.                                         |
+------+----------------------------------+------------------------------------------------------------------------+
| v1.4 | 26th October 2004 (by Wilbert)   | - Added "mpeg1->mpeg2" mode.                                           |
|      |                                  | - Added rgb=true, which converts to RGB24 using mpeg2 coefficients.    |
+------+----------------------------------+------------------------------------------------------------------------+
| v1.3 | 4th of October 2004 (by Manao)   | - Replaced float by integer computations. Almost two times faster.     |
+------+----------------------------------+------------------------------------------------------------------------+
| v1.2 | 12th September 2004 (by Sh0dan)  | - Use float instead of double. They have enough precision and are much |
|      |                                  |   faster. It can however be changed back by changing the typedef in    |
|      |                                  |   ColorMatrix.h                                                        |
|      |                                  | - Simpler algorithms.                                                  |
|      |                                  | - Use internal limiter for output also, instead of very slow if-then.  |
|      |                                  | - Better rounding (adding 0.5 for more exact float to int conversion). |
+------+----------------------------------+------------------------------------------------------------------------+
| v1.1 | 12th September 2004              | - Uses limiter first to get CCIR-601 compliant digital video.          |
|      |                                  | - Also returns CCIR-601 compliant digital video.                       |
+------+----------------------------------+------------------------------------------------------------------------+
| v1.0 | 11th September 2004              | - Initial release.                                                     |
+------+----------------------------------+------------------------------------------------------------------------+

$Date: 2006/12/15 19:29:25 $

.. _users reporting the problem:
    http://forum.doom9.org/showthread.php?s=&postid=514595#post514595
.. _background info:
    http://forum.doom9.org/showthread.php?s=&threadid=81191
.. _ISO/IEC 13818-2: http://le-hacker.org/hacks/mpeg-drafts/is138182.pdf
.. _ITU-R_BT.709:
    http://www.itu.int/rec/recommendation.asp?type=folders&lang=e&parent=R-REC-BT.709

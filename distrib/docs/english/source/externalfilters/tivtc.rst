
TIVTC
=====


Abstract
--------

| **author:** tritical
| **version:** 0.9.9.3
| **download:** `<http://bengal.missouri.edu/~kes25c/>`_
| **category:** Deinterlacing & Pulldown Removal
| **requirements:**

-   YV12 & YUY2 Colorspace

**license:** GPL

--------


Description
-----------

TIVTC is actually a combination of 3 different filters and 1 conditional
function.

The three filters included are ``TFM``, ``TDecimate``, and ``MergeHints``. TFM is a field
matching filter that will recreate the original progressive frames in a
telecined source, and TDecimate
is a decimation filter that removes duplicates. These filters can be used
together to achieve an ivtc or separately to accomplish other tasks.
TDecimate also provides special options
for handling hybrid material which include vfr via matroska (using a
timecodes file). The conditional function is IsCombedTIVTC which can be used
via AviSynth's conditionalfilter
to test if frames are combed or not. It simply uses TFM's combed frame
detection. The third normal filter called MergeHints is mainly a utility
function that allows denoising and other
types of filters to be used inbetween TFM and TDecimate and for TFM's hints
to still be passed to TDecimate.

For more info about using each of the filters consult the individual readme
files.

current filter versions:

`TFM`_: TFM is a field matching filter that recovers the original progressive
frames from a telecined stream. It does not decimate the resulting duplicate
frames though, so to achieve an ivtc you must follow TFM with a decimation
filter such as TDecimate(), which is also included in the tivtc.dll.

`TDecimate`_: TDecimate is a decimaton filter intended to remove duplicates
from a video stream. It supports a couple types of operation which include
M-in-N decimation and an arbitrary framerate decimation scheme that can
support ratios not achievable with M-in-N. It also includes special handling
for hybrid material such as blend decimation (for a single frame rate
solution) or vfr via mkv using a timecodes file.

`MergeHints`_: MergeHints is a simple filter that transfers hints present in
one clip into another. It will work with any filter that uses the method of
hiding hints in the least significant bit of the first 64 pixels in the Y
plane of the image (decomb, dgdecode, tfm, etc...).

`FieldDiff`_: FieldDiff is a simple filter that calculates a field difference
metric using the 5 point metric that TFM uses and then outputs it via the
debug or display options. It operates on full frames (it differences the two
fields in each frame). FieldDiff has a version that can be used in
conditional filtering called "CFieldDiff" it returns the value to the script.

`IsCombedTIVTC`_: TIVTC is a utility function that can be used with
AviSynth's conditionalfilter. It uses TFM's inbuilt combed frame detection to
test whether or not a frame is combed and returns true if it is and false if
it isn't.

contact: forum.doom9.org, nick = tritical, or email: kes25c at mizzou.edu


Examples
--------

This file lists some example scripts for dealing with common cases for which
you would use TIVTC. It assumes, however, that you have at least skimmed over
the README's and know the different parameters. It also assumes you know what
"hybrid" and "vfr" mean, what an mkv timecodes file is and how to use it if
you are looking into hybrid processing. All examples assume we are using an
mpeg2source()... and use TFM's d2v parameter to obtain the field order and
scan for illegal transitions in the d2v file. If you are not using a d2v
source then adjust the examples as necessary to meet your requirments.


NTSC cases
~~~~~~~~~~

1) NTSC Film, normal source (not anime or cartoon). One pass.

::

    mpeg2source("c:\oursource.d2v")
    tfm(d2v="c:\oursource.d2v")
    tdecimate()

2) NTSC Film, anime or cartoon source. One pass.

::

    mpeg2source("c:\oursource.d2v")
    tfm(d2v="c:\oursource.d2v")
    tdecimate(mode=1)

3) NTSC Hybrid, using blend decimation on video sections (not anime or cartoon). One pass.

::

    mpeg2source("c:\oursource.d2v")
    tfm(d2v="c:\oursource.d2v")
    tdecimate(hybrid=1)

4) NTSC Hybrid, using blend decimation on video sectons, anime or cartoon source. One pass.

::

    mpeg2source("c:\oursource.d2v")
    tfm(d2v="c:\oursource.d2v")
    tdecimate(mode=1, hybrid=1)

5) NTSC Hybrid, using vfr via mkv (not anime or cartoon). One pass.

::

    mpeg2source("c:\oursource.d2v")
    tfm(d2v="c:\oursource.d2v")
    tdecimate(mode=3, hybrid=2, vfrDec=0, mkvOut="mkv-timecodesfile.txt")

6) NTSC Hybrid, using vfr via mkv, anime or cartoon source. One pass.

::

    mpeg2source("c:\oursource.d2v")
    tfm(d2v="c:\oursource.d2v")
    tdecimate(mode=3, hybrid=2, vfrDec=1, mkvOut="mkv-timecodesfile.txt")

7) NTSC, two pass (enables use of conCycleTP parameter) mkv vfr for hybrid source.

First pass:

::

    mpeg2source("c:\oursource.d2v")
    tfm(d2v="c:\oursource.d2v", output="matches.txt")
    tdecimate(mode=4, output="metrics.txt")

Second pass (not anime or cartoon):

::

    mpeg2source("c:\oursource.d2v")
    tfm(d2v="c:\oursource.d2v", input="matches.txt")
    tdecimate(mode=5, hybrid=2, vfrDec=0, input="metrics.txt",
    tfmIn="matches.txt", mkvOut="mkv-timecodesfile.txt")

Second pass (anime or cartoon):

::

    mpeg2source("c:\oursource.d2v")
    tfm(d2v="c:\oursource.d2v", input="matches.txt")
    tdecimate(mode=5, hybrid=2, vfrDec=1, input="metrics.txt",
    tfmIn="matches.txt", mkvOut="mkv-timecodesfile.txt")

PAL Cases
~~~~~~~~~

1) PAL, no decimation (just field matching)

::

    mpeg2source("c:\oursource.d2v")
    tfm(d2v="c:\oursource.d2v")

2) PAL, decimate 1 in 25 (most similar)

::

    mpeg2source("c:\oursource.d2v")
    tfm(d2v="c:\oursource.d2v")
    tdecimate(cycle=25)

$Date: 2005/07/10 16:11:01 $

.. _TFM: tivtc_tfm.rst
.. _TDecimate: tivtc_tdecimate.rst
.. _MergeHints: tivtc_mergehints.rst
.. _FieldDiff: tivtc_fielddiff.rst
.. _IsCombedTIVTC: tivtc_iscombedtivtc.rst

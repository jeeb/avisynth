
MergeHints
==========


Abstract
--------

| **author:** tritical
| **version:** 1.0
| **download:** `<http://bengal.missouri.edu/~kes25c/>`_
| **category:** Deinterlacing & Pulldown Removal
| **requirements:**

-   YV12 & YUY2 Colorspace

**license:** GPL

--------


Description
-----------

MergeHints is a simple filter that transfers hints present in one clip into
another. It will work with any filter that uses the method of hiding hints in
the least significant bit of the first 64 pixels in the Y plane of the image
(decomb, dgdecode, tfm, etc...).

NOTE: this only allows the use of filters that do not alter the # of frames
or the frame/field order inbetween the spots the hints are being tranferred
from. That means denoisers, color correction, and the like will work, but not
selectevery(), convertfps(), etc...


Syntax
~~~~~~

``MergeHints`` (clip hintClip, bool "debug")


Examples
~~~~~~~~

This is a script where TFM's hints would normally be destroyed and never
reach TDecimate:

::

    mpeg2source(d2v="source.d2v")
    tfm(d2v="source.d2v")
    temporalsoften(3, 3, 5, 15, 2) #destroys the hints
    blur(0.25) #destroys the hints
    tdecimate(mode=1)

To fix this, and preserve the hints (they can greatly aid decimation,
especially if using hybrid detection), use MergeHints as follows:

::

    mpeg2source(d2v="source.d2v")
    tfm(d2v="source.d2v")
    savedHints = last
    temporalsoften(3, 3, 5, 15, 2)
    blur(0.25)
    MergeHints(hintClip=savedHints)
    tdecimate(mode=1)

Parameters
----------

*hintClip* -

The clip that contains the hints to be transferred.

Default: NULL (PClip)

*debug* -

Will spit out some info using OutputDebugString(). You can use the utility
"DebugView" from sysinternals to view this info. The info is just the
specifier (first 32 lsbs) and the hint (second 32 lsbs) that MergeHints finds
in hintClip.

Default: false (bool)

$Date: 2005/07/10 16:11:01 $

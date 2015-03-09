
FieldDiff
=========


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

FieldDiff is a simple filter that calculates a field difference metric using
the 5 point metric that TFM uses and then outputs it via the debug or display
options. It operates on full frames (it differences the two fields in each
frame). FieldDiff has a version that can be used in conditional filtering
called "CFieldDiff" it returns the value to the script.


Syntax
~~~~~~

| ``FieldDiff`` (clip, int "nt", bool "chroma", bool "display", bool "debug")
| ``CFieldDiff`` (clip, int "nt", bool "chroma", bool "debug")


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

*nt* -

Sets the noise threshold for the field differencing. Recommended values are
between 0 (no noise rejection) and 5.

Default: 3 (int)

*chroma* -

Disables or enables chroma processing.

Default: true (bool)

*display* -

Draws the difference value on the top left of each frame.

Default: false (bool)

*debug* -

Outputs the difference value via OutputDebugString. Use "DebugView" to
view the output.

Default: false (bool)

$Date: 2005/07/10 16:11:01 $

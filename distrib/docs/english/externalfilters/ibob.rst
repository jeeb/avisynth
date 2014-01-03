
IBob (Interpolating Bob)
========================


Abstract
--------

| **author:** Interpolating Bob by Kevin Atkinson
| **version:** v0.10 (C plugin, to be loaded with LoadCPlugin)
| **dowload:** `<http://www.avisynth.org/warpenterprises/>`_
| **category:** Deinterlacing & Pulldown Removal
| **requirements:** YV12 &  YUY2 Colorspace

--------

This simple filter works identically to the Avisynth built-in Bob filter
except that it uses linear interpolation instead of bicubic resizing.
It usage is simple:

::

    IBob()

The advantages of IBob over the builtin Bob is:

1) The lines of the dominate field are untouched. In particular

::

    AssumeTFF()
    orig = last
    IBob()
    AssumeTFF().SeparateFields().SelectEvery(4, 0, 3).Weave()
    Compare(last, orig)

Will report 0 difference between the original and the bob than unbobbed video
(when the video format is yuy2, with yv12 the chroma will be slightly
different). With the builtin bob this is not the case.

2) A bit faster than the built-in bob.

In can be made even faster by 1) dynamically compiling key parts to reduce
register pressure and take advantage of the "base + index + displacement"
addressing mode, and by 2) using vector extensions.

$Date: 2004/08/13 21:57:25 $

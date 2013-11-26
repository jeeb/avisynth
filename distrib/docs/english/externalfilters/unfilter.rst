
UnFilter
========


Abstract
--------

| **author:** Tom Barry
| **version:** 0.0.1.5
| **download:** `<http://mywebpages.comcast.net/trbarry/downloads.htm>`_
| **category:** Sharpen/Soften Plugins
| **requirements:** YV12 & YUY2 Colorspace

--------


Description
-----------

UnFilter is a simple and reasonably fast Avisynth Soften/Sharpen filter.

It implements 5-tap user adjustable horizontal and vertical filters designed
to (slightly) reverse previous efforts at softening or edge enhancment that
are common (but ugly) in DVD mastering. Since DVD's were intended originally
for interlaced displays this has caused content providers to vertically
filter them even a bit more to hide interlacing artifacts. I don't know why
they sometimes over do the edge enhancement.

When softening it will attempt to approximate the inverse of a simple 3-tap
edge enhancement filter. When sharpening it will attempt to approximate the
inverse of a simple 3-tap softening filter. For the math and logic involved
see the comments in the UnFilterALL.inc member included in zip file.

The effects are fairly mild but be aware that excessive sharpening makes
things harder to compress and may bring about the dreaded "edge enhancement
artifacts" the people complain about in DVD's from some studios. And while
excessive softness may hide noise it loses detail and generally just looks
ugly. So it is probably best to just try to reverse whatever has already been
done to your source.


Examples
~~~~~~~~

In your Avisynth file use commands similar to:

::

    LoadPlugin("F:\UnFilter\UnFilter.dll")
    AviSource("D:\wherever\myfile.avi")
    UnFilter(HSharp, VSharp)

Of course replace the file and directory names with your own and supply the
integer values for the amount of horizontal and vertical sharpness. Valid
values for each are from -100 (max softness) through zero (neutral) through
+100 (max sharpness).


Note
~~~~

UnFilter should run on all MMX machines or higher. It has also has some added
code for 3DNOW instructions for when it is running on a K6-II or higher and
some SSEMMX for P3 & Athlon.

$Date: 2004/08/17 20:31:19 $

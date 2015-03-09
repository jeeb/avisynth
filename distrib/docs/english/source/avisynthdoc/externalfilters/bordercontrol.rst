
BorderControl
=============


Abstract
--------

| **author:** Simon Walters
| **version:** 1.4
| **download:** `<http://www.geocities.com/siwalters_uk/fnews.html>`_
| **category:** Broadcast Video Plugins
| **requirements:** YV12 & YUY2 Colorspace

--------


Description
-----------

**Note:** The left and right parameters get doubled by the filter as I haven't
sorted out how to deal properly with having 1 chroma sample for very 2
luminace ones yet :-(

It performs the same functions as the VirtualDub version in that you can set
a black border, set a border region to be faded out and you can "smear" the
border to save having to crop and resize the whole frame for the sake of a
few pixels. Each border (top,bottom.left and right) can be manipulated
independantly.


Syntax
~~~~~~

::

    #Example
    LoadPlugin("BorderControl")
    AVISource("Test.avi")
    BorderControl(YBB=16, YTS=32)

This would put a solid black border at the bottom and smear some lines at the
top of the picture. The plugins accepts up to 16 parameters, 4 for each
border
There are 4 prefixes: YB, YT, XL and  XR for bottom, top, left and right
borders.

There are 4 suffixes:  B for Border - this sets the amount of solid border,
F for Fade,  S for Smear and  SF for SmearFactor.

Increasing SmearFactor (having first set the Smear value correctly) can help
disguise the smear region at the expense of increasing the area being
processed. I suggest using even values for YTSF and YBSF when dealing with
interlaced material.

So the parameters are

``BorderControl`` (clip, "YBB", "YBF", "YBS", "YBSF", "YTB", "YTF", "YTS",
"YTSF", "XLB", "XLF", "XLS", "XLSF", "XRB", "XRF", "XRS", "XRSF")

$Date: 2004/08/13 21:57:25 $

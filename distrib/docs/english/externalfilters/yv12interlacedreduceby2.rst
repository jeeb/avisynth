
YV12InterlacedReduceBy2
=======================


Abstract
--------

| **author:** Tom Barry
| **version:** 0.1.0.0
| **download:** `<http://mywebpages.comcast.net/trbarry/downloads.htm>`_
| **category:** Resizers
| **requirements:**

-   YV12 Colorspace
-   SSEMMX (=ISSE) support

--------


Description
-----------

``YV12InterlacedReduceBy2`` is a fast Reduce By 2 filter. It works by taking
only the even (starting with 0) pixels from the top field. So it will reduce
both the width and height by a factor of 2. It blends the chroma slightly to
avoid YV12 chroma delay.

**USAGE - To use it just:**

1) Place the InterlacedReduceBy2.dll in a directory somewhere.
2) In your Avisynth file use commands similar to

::

    LoadPlugin("F:\YV12InterlacedReduceBy2\YV12InterlacedReduceBy2.dll")
    AviSource("D:\wherever\myfile.avi")
    YV12InterlacedReduceBy2()

Of course replace the file and directory names with your own. There are no
parameters. This is usefull as a very fast downsize that needs no
deinterlacing.

**USAGE 2**

There is a second similar function enclosed in the dll called
YV12InterlacedSelectTopFields(). This will select top fields only and adjust
for chroma delay but leave the width unchanged.

::

    LoadPlugin("F:\YV12InterlacedReduceBy2\YV12InterlacedReduceBy2.dll")
    Avisource("D:\wherever\myfile.avi")
    YV12InterlacedSelectTopFields()

$Date: 2004/08/17 20:31:19 $

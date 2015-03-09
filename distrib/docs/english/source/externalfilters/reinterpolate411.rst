
Reinterpolate411
================


Abstract
--------

**author:** Tom Barry
**version:**
**download:** `<http://mywebpages.comcast.net/trbarry/downloads.htm>`_
**category:** Broadcast Video Plugins
**requirements:**

-   YUY2 Colorspace
-   NTSC DV 4:1:1

--------


Description
-----------

This filter is only tested with the MainConcept decoder so far. But it does
seem that even chroma pixels are just being duplicated in that codec. This
filter will help that by discarding the odd chroma pixels and recreating them
as the average of the 2 adjacent even pixels.

It doesn't matter whether the material is interlaced. There are no parms,
only Avisynth 2.5 YUY2 supported. Probably use it directly after AviSource
as:

::

    AviSource(...)
    ReInterpolate411()

$Date: 2004/08/17 20:31:19 $

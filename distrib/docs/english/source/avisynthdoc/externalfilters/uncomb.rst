
UnComb
======


Abstract
--------

| **author:** Tom Barry
| **version:** 0.1.0.0
| **download:** `<http://mywebpages.comcast.net/trbarry/downloads.htm>`_
| **category:** Deinterlacing & Pulldown Removal
| **requirements:**

-   YV12 Colorspace
-   SSEMMX (ISSE) support

--------


Description
-----------

UnComb is a simple IVTC filter for matching up even and odd fields of
properly telecined NTSC or PAL film source video. Think of it as a poor mans
little brother to Telecide(). It is fast but that's because it makes no
attempt at finding frames that must be deinterlaced.

USAGE - To use it just
~~~~~~~~~~~~~~~~~~~~~~

In your Avisynth file use commands similar to

::

    LoadPlugin("F:\UnComb\UnComb.dll")
    AviSource("D:\wherever\myfile.avi")
    UnComb()

Of course replace the file and directory names with your own. **There are no
parameters**.


KNOWN ISSUES AND LIMITATIONS
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

1) Requires Avisynth 2.5 and YV12 input.

2) Sorry, currently requires a P-III, Athlon, or higher. Needs SSEMMX
   support.

3) It will not handle improperly telecined material or bad video edit. It
   does not even check for combed frames. It just makes the best field match
   from what is available. It will not blend, interpolate, deinterlace, or
   even care. It is for reasonably good source material.

4) It does not and likely will not have fancy options like Telecide().
   Use Telecide() when needed.

5) It does not decimate. Follow UnComb with Decimate(5) when you need to
   decimate frames from 30 to 24 fps.

6) It is wicked fast. ;-)

$Date: 2004/08/17 20:31:19 $

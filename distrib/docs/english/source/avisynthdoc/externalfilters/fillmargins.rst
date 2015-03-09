
FillMargins
===========


Abstract
--------

| **author:** Tom Barry
| **version:** 1.0.2.0
| **download:** `<http://mywebpages.comcast.net/trbarry/downloads.htm>`_
| **category:** Broadcast Video Plugins
| **requirements:** YV12 Colorspace

--------


Description
-----------

Sometimes a video clip has black borders or garbage at the four edges. This
looks ugly and does not compress well but possibly you don't want to crop
because you have to keep the diminsions as a multiple of 16 or some other
number. FillMargins is a simple Avisynth filter that fills the four margins
of a video clip with the outer pixels of the unfilled portion. It takes
integer 4 parms specifying the size of the left, top, right, and bottom
margins. These may be any value and do not have to be any particular
multiple.


Usage - To use it just
----------------------

1) Place the FillMargins.dll in a directory somewhere.

2) In your AviSynth file use commands similar to

::

    LoadPlugin("F:\FillMargins\FillMargins.dll")
    AviSource("D:\wherever\myfile.avi")
    FillMargins(5,7,2,0)  # (left, top, right, bottom)

Of course replace the file and directory names with your own. Apply
appropriate numbers.

$Date: 2004/08/13 21:57:25 $

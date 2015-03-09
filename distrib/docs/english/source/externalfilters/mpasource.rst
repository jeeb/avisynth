
MPAsource
=========


Abstract
--------

| **author:** Ernst Peche
| **version:** 0.2
| **category:** MPA Decoder (source) Plugins
| **download:** `<http://www.avisynth.org/warpenterprises/>`_
| **requirements:**

--------


Description
-----------

Opens mp1/2/3 audio files directly and normalizes output to 100% if
requested.

Take care when testing: by opening the AVS-script the whole file gets scanned
in order to return the exact sample number when *normalize=false* (default)
this is done with partial decoding (~20sec for one hour audio on my PC). When
*normalize=true* it is much slower (~100sec).

Version 0.2 creates a small file .D2A after the first scan, so there is no
multiple scanning necessary.

When backward jumps are requested, the seeking is always done in fast mode
from frame 0 to the requested frame.

Along with the audio, a small 8x8 video is generated, so you always have to
put MPASource as SECOND argument in AudioDub.


Example
~~~~~~~

::

    LoadPlugin("C:\myprojects\mpasource25\release\mpasource.dll")
    V = BlankClip(height=100, width=100, length=20000, fps=25)
    A = MPASource("d:\_mp3\pop\liquido - narcotic.mp3", normalize = false)
    AudioDub(V, A)

$Date: 2004/08/13 21:57:25 $

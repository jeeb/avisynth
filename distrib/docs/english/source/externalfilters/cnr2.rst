
Cnr2 (Chroma Noise Reducer)
===========================


Abstract
--------

| **author:** Chroma Noise Reducer by MarcFD and others
| **version:** 2.61
| **dowload:** `<http://www.avisynth.org/warpenterprises/>`_
| **category:** Temporal Smoothers
| **requirements:**

-   YV12 & YUY2 Colorspace

--------


What it does
------------

This filter is a Chroma Stabilizer for analogic sources.
It's a temporal filter and it's very effective against two chroma artifacts:
stationary rainbows and huge analogic chroma activity.


Usage
-----

Add the following lines in your avisynth script:
::

    LoadPlugin("Cnr2.dll")
    # example
    Cnr2() # for TV/VHS caps
    Cnr2("xxx", 4, 5, 255) # my suggestion to remove rainbows.

Syntax :

``Cnr2`` (clip, mode="oxx", scdthr=8, ln=35, lm=192, un=47), um=255, vn=47,
vm=255, log=false)

*mode*:

mode of each components, "YUV"
two modes are possible : wide ('o') and narrow ('x')
narrow mode is more sensible to variations, and less effective

*scdthr*:

scenechange detection threshold.
lower it to make scd more sensible.
if a scenechange is detected, cnr2 will
reset the filtering for the new scene

*ln,lm,un,um,vn,vm* : 0 to 255

the n values are the movement sensibility :
higher values will denoise more, but could produce ghosting artifacts
the m values are the maximum effectiveness of the denoiser :
lower them to reduce the denoising effect.

*log* :

to log scd internals. use DebugView to see it.
::

    Cnr2()
    Cnr2("oxx", 8, 35, 192, 47, 255, 47, 255, false)
    Cnr2(mode="oxx", scdthr=8, ln=35, lm=192, un=47, um=255, vn=47, vm=255, false)

are three aliases : they would produce the same filtering.



History
-------

This Filter is inspired by the Virtual Dub filter Chroma Noise Reduction 1.1
(by Gilles Mouchard)

cnr 1.1 was a good chroma filter but was using YUV conversion to do is job.
I used it for VHS encodes, because it was the better Chroma Noise filter i
tested

But when i started to use AviSynth and Huffyuv, RGB convertion was an
overhead

So i take a look on the code and wrote my own filter from scratch, but based
of the same algo.

the output of cnr2 is 99,9% to 100% the same as cnr (due to YUV colorspace)
and it's 3x-5x faster (Cnr2 can do real-time denoising in 640x480x25fps with
a 1,4 Ghz CPU)

+--------------------------------------------------------------------------------------------------------+
| Changes                                                                                                |
+==============================+=========================================================================+
| *version 2.1 (31.7.2002)*    | little review. corrections.                                             |
+------------------------------+-------------------------------------------------------------------------+
| *version 2.2 (01.8.2002)*    | First frame bug fixed.                                                  |
+------------------------------+-------------------------------------------------------------------------+
| *version 2.3 (17.11.2002)*   | YV12 Code and SCD (scene change detection)                              |
+------------------------------+-------------------------------------------------------------------------+
| *version 2.4 (17.11.2002)*   | small changes/bug fixes                                                 |
+------------------------------+-------------------------------------------------------------------------+
| *version 2.51 (13.11.2003)*  | Klaus Post: bug fixes.                                                  |
+------------------------------+-------------------------------------------------------------------------+
| *version 2.6 (29.06.2004)*   | tritical: few bug fixes, scene change detection overhaul, code cleanup. |
+------------------------------+-------------------------------------------------------------------------+
| *version 2.6.1 (30.06.2004)* | tritical: made yuy2 processing fast again.                              |
+------------------------------+-------------------------------------------------------------------------+


Distribution
------------

This is a free sofware distribued under the terms of the GNU-GPL
The only restriction is to e-mail me if you want to do something with
the source because i don't want you to lose your time to decode my
cryptic C,C++,Delphi (or any other language) writing.
Thanks :)

Contact
-------

Don't even doubt of `e-mailing me`_ for any suggestion :-), bug report :-(,
feature request :D, or whatever else :-P.
I hope you'll find this prog useful ! (i do...)

$Date: 2004/08/13 21:57:25 $

.. _e-mailing me: mailto:marc.fd@libertysurf.fr

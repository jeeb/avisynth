
TemporalCleaner
===============


Abstract
::::::::

| **author:** TemporalCleaner (by Jim Casaburi; ported to AviSynth by Vlad59)
| **version:** Beta 2
| **dowload:** `<http://www.avisynth.org/warpenterprises/>`_
| **category:** Temporal Smoothers
| **requirements:**

-   YUY2 Colorspace
-   ISSE support

--------

.. sectnum::
    :depth: 3
    :suffix: .

Description
-----------

TemporalCleaner is an avisynth port of the original port of the VirtualDub
filter made by `Jim Casaburi`_.

I just added some ASM to Jim's algo.


What it does
------------

if the difference between previous pixel and current pixel is below a blur
threshold then replace the current pixel by the average between previous and
current pixel else keep current pixel.

An other trick is to add some feedback to this filter : instead of using the
unfiltered previous frame it uses the already filtered one.

Simple isn't it ?


Parameters
----------

``TemporalCleaner`` (clip, int "ythresh", int "cthresh")

ythresh (default value = 5) : blur luma threshold (don't go above 8 otherwise
you'll have some ghosting).

cthresh (default value = 10) : blur chroma threshold (you can safely go a
little above 8 as if luma is above threshold -> chroma is automatically kept
(no blur)).


Current limitations or known problems
-------------------------------------

- Work only with YUV2, CHECKED.
- require a Integer SSE capable CPU (no PII and K6-II), CHECKED.
- Require a mod-4 width (NOT CHECKED)


Credits
-------

Thanks to
- Jim Casaburi for the original idea (I'm just his translator ;))
- Zarxrax and Ligh for the beta testing

Vlad59 (babas.lucas at laposte.net)

+-----------------------------------------------------------------------------------------------------+
| Changelog                                                                                           |
+=======+============+================================================================================+
| beta2 | 2003/01/13 | - Some minor speedup                                                           |
|       |            | - Reorganize all the code to mix Avisynth 2.5 and 2.0X in the same .cpp and .h |
+-------+------------+--------------------------------------------------------------------------------+
| beta1 | 2002/11/09 | - Initial release                                                              |
+-------+------------+--------------------------------------------------------------------------------+

$Date: 2004/08/17 20:31:19 $

.. _Jim Casaburi:
    http://home.earthlink.net/~casaburi/download/#temporalcleaner

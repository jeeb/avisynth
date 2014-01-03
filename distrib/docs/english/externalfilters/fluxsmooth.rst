
FluxSmooth
==========


Abstract
--------

| **author:** Ross Thomas (Sansgrip)
| **version:** 1.1a
| **download:** `<http://www.avisynth.org/warpenterprises/>`_
| **category:** Spatio-Temporal Smoothers
| **requirements:**

-   YV12 & YUY2 Colorspace
-   ISSE support

--------


Description
-----------

One of the fundamental properties of noise is that it's random. One of the
fundamental properties of motion is that it's not. This is the premise behind
FluxSmooth, which examines each pixel and compares it to the corresponding
pixel in the previous and last frame. Smoothing occurs if both the previous
frame's value and the next frame's value are greater, or if both are less,
than the value in the current frame.

I like to call this a "fluctuating" pixel, then I like to wipe that pixel
from existence by averaging it with its neighbours. For FluxSmoothST, this is
(by default) done in a spatio-temporal manner, in that for each fluctuating
pixel its 8 immediate spatial neighbours as well as its 2 temporal neighbours
(the abovementioned corresponding pixel from the previous and next frames)
are considered for inclusion in the average. If the value of each pixel is
within the specified threshold, it is included. If not, it isn't. FluxSmoothT
performs only temporal averaging.

This filter seems to remove almost all noise from low-noise sources (such as
DVD) and a lot of noise from high-noise sources (such as cable TV captures),
while maintaining a good amount of detail.

If your CPU supports integer SSE operations (Intel Pentium III and better,
AMD Athlon and better) an optimized version of the algorithm will be used.

Using FluxSmoothT instead of FluxSmoothST for temporal-only smoothing is
much, much faster (50% faster on my system).


Usage
-----

``FluxSmoothT`` (clip, int "temporal_threshold")

``FluxSmoothST`` (clip, int "temporal_threshold", int "spatial_threshold")

+----------------------+------------------------------------+---------+
| Parameter            | Meaning                            | Default |
+======================+====================================+=========+
| *temporal_threshold* | Temporal neighbour pixels within   | 7       |
|                      | this threshold from the current    |         |
|                      | pixel are included in the average. |         |
|                      |                                    |         |
|                      | If set to -1, no temporal          |         |
|                      | smoothing occurs. (Cannot be set   |         |
|                      | to -1 in ``FluxSmoothT``.)         |         |
+----------------------+------------------------------------+---------+
| *spatial_threshold*  | Spatial neighbour pixels within    | 7       |
|                      | this threshold from the current    |         |
|                      | pixel are included in the average. |         |
|                      |                                    |         |
|                      | If set to -1, no spatial smoothing |         |
|                      | occurs.                            |         |
+----------------------+------------------------------------+---------+

Known Issues
------------

-   The very edges of the frame are unprocessed.
-   The very first and very last frame of a clip is unprocessed.
-   Appears to cause an access violation when used with n-pass encoding
    in CCE. I've been unable to duplicate this. Might've been related to the
    memory leak fixed by fabrice.


TODO
----

- Optimize SSE code with regard to pairing, stalls, and so on.


Author
------

Ross Thomas <`ross at grinfinity.com`_>


+----------------------------------------------------------------------------------+
| History                                                                          |
+======+===========================================================================+
| 1.1a | Yet another "oops" release. Current pixel is once again                   |
|      | considered in the averaging code -- I found the lack of it too            |
|      | aggressive, especially during fast motion. Also fixed stupid "3am bug"    |
|      | involving a couple of variables I'd declared static that shouldn't've     |
|      | been. Thanks to krieger2005 for spotting that one, and ARDA for           |
|      | diagnosing it.                                                            |
+------+---------------------------------------------------------------------------+
| 1.1  | Changed the averaging code so that the current pixel is excluded,         |
|      | which produces better noise reduction. Also split the code into two       |
|      | different filters, FluxSmoothT and FluxSmoothST. The former does          |
|      | temporal-only smoothing (equivalent to setting "spatial_threshold=-1" in  |
|      | FluxSmoothST) and is about 50% faster. Removed Avisynth 2.0x version to   |
|      | tidy up the code base. Does anyone actually use it any more? My thanks to |
|      | fabrice and sh0dan for the 1.01 release during my extended absence :).    |
+------+---------------------------------------------------------------------------+
| 1.01 | Added by sh0dan:                                                          |
|      | - Removed leak in AviSynth 2.5 YV12 mode (code by fabrice)                |
|      | - Aligned tables and variables.                                           |
|      | - Use AviSynth BitBlt for copying chroma.                                 |
|      | - Don't use streaming store. (movntq)                                     |
|      | - All in all an approximate 15% speedup compared to previous version.     |
|      | - All changes are marked with "sh0:".                                     |
+------+---------------------------------------------------------------------------+
| 1.0  | First "stable" release. I think it's been tested enough, but wait         |
|      | for a bunch of bugs to emerge and make me a liar... Fixed a bug that, in  |
|      | conjunction with a bug in the built-in resizers, caused an access         |
|      | violation under certain circumstances. Thanks to sh0dan for spotting that |
|      | one :). Added "SetCacheHints" and upgraded to "AvisynthPluginInit2" in    |
|      | 2.5 version.                                                              |
+------+---------------------------------------------------------------------------+
| 0.4  | Implemented iSSE-optimized version, which runs roughly double the         |
|      | speed of the C++ version. Some small optimizations to C++ version. Now    |
|      | smooths chroma as well as luma.                                           |
+------+---------------------------------------------------------------------------+
| 0.3  | Fixed bad bug that caused incorrect smoothing: no more in-place           |
|      | filtering. Changed defaults back to what they were, now that the          |
|      | algorithm works correctly. Spent some time benchmarking and tweaking      |
|      | various pieces of code, so should now be significantly faster.            |
+------+---------------------------------------------------------------------------+
| 0.2  | Fixed non-fatal bug that caused a request for one frame beyond            |
|      | the end of the clip. Changed to in-place filtering so could squeeze a few |
|      | optimizations here and there. Changed too-high defaults. First Avisynth   |
|      | 2.5/YV12 release.                                                         |
+------+---------------------------------------------------------------------------+
| 0.1  | First release. Alpha code.                                                |
+------+---------------------------------------------------------------------------+

$Date: 2004/08/13 21:57:25 $

.. _ross at grinfinity.com: mailto:ross@grinfinity.com

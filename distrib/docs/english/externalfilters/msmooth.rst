
MSmooth
=======


Abstract
--------

| **author:** Donald Graft
| **version:** 2.02
| **download:** `<http://neuron2.net/mine.html>`_
| **category:** Spatial Smoothers
| **requirements:**

-   YV12 & RGB32 Colorspace

--------


Introduction
------------

This plugin for Avisynth implements an unusal concept in spatial smoothing.
Although designed specifically for anime, it may be useful elsewhere. The
filter is effective at removing mosquito noise as well as effectively
smoothing flat areas in anime. Not only is the noise reduction valuable, but
less bits are needed to encode the resulting clip.

This filter supports RGB32 or YV12 color spaces for input. This filter is not
yet optimized for speed. Optimizations will be included in a future version.

The justification for the filter is simple. The biggest complaint about Smart
Smoother (for example) is that setting the threshold high enough to give the
desired smoothing destroys a significant amount of detail. We need to
decouple the control of detail detection from the amount of smoothing
applied. In Smart Smoother and other thresholded smoothers, one threshold
controls both of these functions.

Also, smoothing should not be done across image edges.

MSmooth implements both of these refinements. To use it, first set the
'threshold' parameter so that desired detail is preserved. Then set the
smoothing strength. You can set very high smoothing strengths without
destroying the preserved detail (because the detail map is used to mask the
smoothing).


MSmooth Function Syntax
-----------------------

MSmooth uses named parameters. That means you do not have to worry about the
ordering of parameters and can simply refer to them by name and put them in
any order in the list of parameters. If you omit a parameter it takes its
default value. For example, if you want to run MSmooth with a strength of 7
and debug enabled, you can simply say:

::

    MSmooth(strength=7, debug=true)

Any combination and order of named parameters is allowed. Remember, however,
that you should always include empty parentheses if you are not specifying
any parameters.

Following is the syntax for MSmooth (replace parameter_list with your comma-
separated list of named parameters).

**MSmooth(parameter_list)**

*threshold* (0-255, default 15): This parameter determines what is detected as
detail and thus preserved. To see what detail areas will be preserved, use
the 'mask' parameter.

*strength* (0-25, default 3): This is the number of iterations of a 3x3
averaging blur to be performed on the areas to be smoothed. It is applied
only to the non-detail areas as determined by the 'threshold' parameter, and
smoothing does not cross image detail. The filter becomes much slower as the
strength is raised.

*chroma* (true/false, default false): When set to true, enables chroma
smoothing for YV12 (it's always enabled for RGB). Running without chroma
smoothing is faster.

*highq* (true/false, default true): When set to true, a higher quality but
slightly slower detail detection algorithm is used.

*mask* (true/false, default false): When set to true, the areas to be preserved
are shown. Use this to set the level of detail to be preserved.

*show* (true/false, default false): This parameter overlays debug output on the
output frame. Currently, only the filter version is output.

*debug* (true/false, default false): This parameter enables debug output to the
DebugView utility. Currently, only the filter version is output.

+------------------------------------------------------------------------------------+
| Changes                                                                            |
+=========+==========================================================================+
| v2.02   | - Fixed the nasty bug which caused a green cast with high strengths in   |
|         |   YV12 color space. As a bonus, YV12 is now faster as well. Finally, the |
|         |   source code is now available.                                          |
+---------+--------------------------------------------------------------------------+


Copyright
---------

| Copyright (C) 2003 Donald A. Graft, All Rights Reserved.
| Feedback/inquiries to neuron2 at attbi.com.

For updates and other filters/tools, visit my web site:
`<http://neuron2.net/>`_

$Date: 2005/10/01 23:09:51 $

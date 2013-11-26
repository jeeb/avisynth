
LeakKernelDeint
===============


Abstract
--------

| **author:** Leak
| **version:** 1.5.4 (based on KernelDeint 1.4.0 by Donald Graft)
| **download:** `<http://www.leoagricola.net:8888/~kp/AviSynth/LeakKernelDeint/>`_
| **category:** Deinterlacing & Pulldown Removal
| **requirements:** YV12 & YUY2 & RGB Colorspace
| **license:** GPL

--------


Introduction
------------

This filter deinterlaces using a kernel approach. It gives greatly improved
vertical resolution in deinterlaced areas compared to simple field
discarding.

If you set the threshold to 0, you can get totally artifact free results (due
to lack of thresholding) but with much less loss of vertical resolution than
simple field discarding. For optimal results, however, set a motion threshold
that allows static areas of the picture to be passed through. In this mode,
the kernel-based deinterlacing of the moving areas preserves their vertical
resolution compared to simple interpolation.

Additionally, this version contains a function named LeakKernelBob() (adapted
from Scharfis_brain's script of the same name) that does full framerate
deinterlacing, i.e. it turn 50 fields per second into 50 frames per second.

The filter accepts RGB32, YUY2, or YV12 input.

To use this filter as a post-processor for Telecide(), use the following
script:

::

    Telecide(..., post=1, hints=true)
    LeakKernelDeint(...)

--------


Function Syntax
---------------

LeakKernelDeint() and LeakKernelBob() take the following named parameters:

| **order** (0-1, default none!)
| This parameter defines the field order of the
  clip. It is very important to set this correctly. Use order=0 for bottom
  field first (bff). Use order=1 for top field first (tff). You must specify
  order; LeakKernelDeint throws an exception if you omit this parameter.

It is essential to set the field order properly for correct rendering.
Because setting it correctly is so important, you are strongly encouraged not
to make assumptions about the field order of a clip, but rather to verify the
field order using the following procedure.

To determine the field order, make an Avisynth script that serves the raw
clip without any processing. If it were an AVI, then just AviSource() would
be used. For our examples, we'll use AviSource(). Add a script line to
separate the fields using top field first, as follows:

::

    AviSource("your_clip.avi")
    AssumeTFF().SeparateFields()

Now serve the script into VirtualDub and find an area with motion. Single
step forward through the motion. Note whether the motion progresses always
forward as it should, or whether it jumps back and forth as it proceeds. For
example, if the field order is wrong, an object moving steadily from left to
right would move right, then jump back left a little, then move right again,
etc. If the field order is correct, it moves steadily to the right.

If the motion is correct with AssumeTFF().SeparateFields(), then your field
order is top field first and you must set order=1. If the motion is
incorrect, then your field order is bottom field first and you must set
order=0. If you are want to double check things, you can use
AssumeBFF.SeparateFields() to check correct operation for bottom field first.

| **threshold** (0-255, default 10)
| This parameter defines the "motion" threshold.
  Moving areas are kernel-deinterlaced while non-moving areas are passed
  through. Use the map parameter to tweak the threshold parameter so that just
  the combed areas of the frame are deinterlaced.

| **sharp** (true/false, default false)
| This parameter, when set to true, selects a
  kernel that provides better vertical resolution and performs some sharpening
  of the video. For less sharpening but also less vertical resolution, set this
  parameter to false.

| **twoway** (true/false, default false)
| This parameter, when set to true, selects
  a kernel that includes both the previous and the following fields for
  deinterlacing. When set to false, the kernel includes only the previous
  field. The latter one-way kernel is faster, crisper, and gives less blending
  (this last advantage makes the filter perform better on anime). The twoway
  parameter is included in case users want to achieve the behavior of previous
  versions.

| **map** (true/false, default false)
| This parameter, when set to true, shows the
  areas that are "moving" as determined by the threshold parameter and which
  will be kernel-deinterlaced. Use this parameter to assist in tweaking the
  threshold parameter.

| **linked** (true/false, default false; YV12/YUY2 only)
| This parameter, when set
  to true, links the motion masks of the chroma and luma planes so
  deinterlacing will take place in the same spots on both planes. Use it if you
  still get residual artifacts in only partially deinterlaced areas, but most
  of the chroma artifacts of version 1.4.0 were eliminated by using a combined
  motion map for both chroma planes, thus the default of false. You probably
  also don't want to set this to true if your video has severe rainbowing, as
  it will deinterlace the luma plane in those spots thus degrading video
  quality more than any (other) chroma artifacts could.

| **debug** (true/false, default false)
| This parameter, when set to true, enables
  debug ouput via the DebugView utility. Currently, it shows the version number
  of the filter and, if hints are present from Telecide(), whether frame are
  hinted as progressive or interlaced. If hints are not present, the debug
  output shows all frames as interlaced. Also, you'll get a hint if your frames
  have different pitches which will disable the MMX optimizations, even though
  I'm not sure if this can even happen in AviSynth.

| **forceCPU** (0-5, default 0)
| This parameter forces the filter to only use specific optimizations:

* 0 ... autodetect
* 1 ... C++ code only
* 2 ... Use MMX code where available
* 3 ... Use MMX & Integer SSE code where available
* 4 ... Use MMX & SSE code where available
* 5 ... Use MMX & SSE & SSE2 code where available

NB: since the code currently only uses MMX, using a value higher than 2 won't
result in a speedup.

--------


+----------------------------------------------------------------------------------------------+
| Changelog                                                                                    |
+========+=====================================================================================+
| v1.5.0 | - Initial release                                                                   |
+--------+-------------------------------------------------------------------------------------+
| v1.5.1 | - Fixed: don't read beyond the end of frames                                        |
|        | - Changed: Small speedup by reading coefficients from memory                        |
+--------+-------------------------------------------------------------------------------------+
| v1.5.2 | - Fixed: motion mask was b0rked due to field order being swapped when building it   |
|        | - Fixed: threshold was off by one                                                   |
|        | - Fixed: map=true Y-values clamped to 235                                           |
+--------+-------------------------------------------------------------------------------------+
| v1.5.3 | - Changed: renamed the plugin to LeakKernelDeint on neuron2's request               |
|        | - Fixed: implicit "AssumeFrameBased" when using LeakKernelBob                       |
+--------+-------------------------------------------------------------------------------------+
| v1.5.4 | - Fixed: LeakKernelBob's cache range was too low, which slowed it down considerably |
+--------+-------------------------------------------------------------------------------------+

Copyright © 2004-2005 Kurt B. Prünner

My web site:
`<http://gast3.ssw.uni-linz.ac.at/~kp/AviSynth/>`_

Donald Graft's web site:
`<http://neuron2.net/>`_

$Date: 2005/10/01 23:09:51 $


DGBob
=====


Abstract
--------

| **author:** Donald A. Graft
| **version:** 1.6.0
| **download:** `<http://neuron2.net/mine.html>`_
| **category:** Deinterlacing & Pulldown Removal
| **requirements:** YV12 & YUY2 & RGB Colorspace

--------


Introduction
------------

This filter splits each field of an interlaced source into its own frame and
then adaptively creates the missing lines either by interpolating the current
field or by using the previous field's data. The filter attempts with some
success to mitigate the flutter that bobbing produces.

Please note that this filter is designed and optimized for producing a bobbed
clip that has twice the frame rate as the input clip. Howvever, it can also
be used as a standard deinterlacer that outputs a frame rate identical to the
input frame rate. To do this, set the 'mode' parameter to 0 (see below).

The filter can also output 50% slow-motion. It achieves this by first smart-
bobbing and then setting the AVI frame rate back to the original rate.

DGBob() supports RGB, YV12, and YUY2 input color spaces.



--------


Function Syntax
---------------

DGBob() takes three named parameters:

| **order** (0-1, default none!)
| This parameter defines the field order of
  the clip. It is very important to set this correctly. Use order=0 for bottom
  field first (bff). Use order=1 for top field first (tff). You must specify
  order; DGBob throws an exception if you omit this parameter.

If you select the wrong field order, smooth motion will jump backward and
forward and you may see some combing. If this occurs, you need to reverse the
field order.

| **mode** (0-2, default 1)
| Set mode=0 to output a clip with a number of
  frames equal to that of the input clip and a frame rate equal to the input
  frame rate. This allows you to use the filter for simple deinterlacing.

Set mode=1 to output a clip with a number of frames equal to twice that of
the input clip (frames from fields) and a frame rate twice the input frame
rate. This allows you to use the filter for smart bobbing.

Set mode=2 to output a clip with a number of frames equal to twice that of
the input clip (frames from fields) and a frame rate equal to the input frame
rate. This allows you to use the filter for 50% slow motion.

| **thresh** (integer, default 12)
| This parameter controls a tradeoff between
  amount of flickering and amount of residual combing and artifacting. Lowering
  the threshold removes more combing and reduces artifacts, but it increases
  flickering. Raising the threshold reduces flickering, but it increases
  residual combing and artifacts. Set it as low as possible without making the
  flickering intolerable.

| **ap** (true/false, default false)
| When true, an extra 'artifact
  protection' check is made that can prevent some kinds of artifacts that occur
  rarely in some video sequences. Enabling artifact protection requires more
  processing time and may increase flickering. Therefore, it should be avoided
  unless absolutely required.



--------

Copyright © 2003, Donald A. Graft, All Rights Reserved.

For updates and other filters/tools, visit my web site:
`<http://neuron2.net/>`_

$Date: 2004/08/13 21:57:25 $

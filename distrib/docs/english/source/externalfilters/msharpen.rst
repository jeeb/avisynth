
MSharpen
========


Abstract
--------

| **author:** Donald A. Graft
| **version:** 1.10 beta 2
| **download:** `<http://neuron2.net/mine.html>`_
| **category:** Sharpen/Soften Plugins
| **requirements:** YV12 & YUY2 & RGB Colorspace

--------


Description
-----------

This plugin for Avisynth implements an unusual concept in spatial sharpening.
Although designed specifically for anime, it also works quite well on normal
video. The filter is very effective at sharpening important edges without
amplifying noise.

This version of MSharpen requires Avisynth version 2.5 or beyond. The filter
works with RGB32, YUY2, or YV12 input. In YUY2 space it sharpens only the
luminance channel. In RGB and YV12 spaces it sharpens all three color
channels. If you want to sharpen luminance only for RGB or YV12 source
material, use ConvertToYUY2(), process, and then convert back if desired. In
all color spaces, color is included in the detail detection.

The justification for the filter is simple. The biggest complaint about
Unsharp Mask (for example) is that setting the strength high enough to
sharpen important edges also amplifyies noise and small detail. MSharpen
solves this problem effectively by detecting important edge areas and then
applying sharpening only to those areas. You first set the 'threshold'
parameter so that desired edges are selected. Then you set the sharpening
strength. You can set very high sharpening strengths without amplifying noise
or fine detail (because the edge map is used to mask the sharpening).

MSharpen Function Syntax
~~~~~~~~~~~~~~~~~~~~~~~~

MSharpen uses named parameters. That means you do not have to worry about the
ordering of parameters and can simply refer to them by name and put them in
any order in the list of parameters. If you omit a parameter it takes its
default value. For example, if you want to run MSharpen with a strength of
100 and debug enabled, you can simply say:
::

    MSharpen(strength=100, debug=true)

Any combination and order of named parameters is allowed. Remember, however,
that you should always include empty parentheses if you are not specifying
any parameters.

You can also use normal positional syntax without the names if you prefer.
The correct parameter order is:

``MSharpen`` (clip, int "threshold", int "strength", bool "highq", bool
"mask", bool "debug")

So you could just use:
::

    MSharpen(15, 100, true, false, false)

Changing Default Parameter Values
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

If you do not like the defaults as documented below, you can set your own
standard defaults. To override the defaults, first create an Avisynth plugins
directory and register it. You register it by putting the following lines in
a text file called 'plugin.reg', changing the path as appropriate, and then
right clicking on the file's icon and selecting Merge.
::

    REGEDIT4

    [HKEY_LOCAL_MACHINE\SOFTWARE\Avisynth]
    "plugindir2_5"="d:\\avisynthplugins"

Next, create defaults files as required in the plugins directory. For
example, to set the default highq=false for MSharpen(), make a file called
MSharpen.def and put this line in it:

``highq = false``

You can list as many parameter assignments as you like, one per line. Those
not specified assume the values given below. Of course, you can always
override the defaults in your scripts when you invoke the functions. NOTE:
The lines in the defaults file must not contain any spaces or tabs.

Following is the syntax for MSharpen (replace *parameter_list* with your
comma-separated list of named parameters):

**MSharpen(parameter_list)**

*threshold* (0-255, default 10): This parameter determines what is detected as
edge detail and thus sharpened. To see what edge detail areas will be
sharpened, use the 'mask' parameter.

*strength* (0-255, default 100): This is the strength of the sharpening to be
applied to the edge detail areas. It is applied only to the edge detail areas
as determined by the 'threshold' parameter. Strength 255 is the strongest
sharpening.

*mask* (true/false, default false): When set to true, the areas to be sharpened
are shown in white against a black background. Use this to set the level of
detail to be sharpened. This function also makes a basic edge detection
filter.

*highq* (true/false, default true): This parameter lets you tradeoff speed for
quality of detail detection. Set it to true for the best detail detection.
Set it to false for maximum speed.

*debug* (true/false, default false): This parameter enables debug output to the
DebugView utility. Currently, only the filter version is output.

Copyright
~~~~~~~~~

| Copyright (C) 2003, Donald A. Graft, All Rights Reserved.
| Feedback/inquiries to neuron2@attbi.com.

For updates and other filters/tools, visit my web site:
`<http://neuron2.net/>`_

$Date: 2005/07/27 17:51:02 $

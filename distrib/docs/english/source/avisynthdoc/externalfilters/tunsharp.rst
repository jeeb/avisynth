
TUnsharp
========


Abstract
--------

| **author:** tritical
| **version:** 0.9.3
| **download:** `<http://bengal.missouri.edu/~kes25c/>`_
| **category:** Sharpen/Soften Plugins
| **requirements:**

-   YV12 & YUY2 Colorspace

**license:** GPL

--------


Description
-----------

TUnsharp is a basic sharpening filter that uses a couple different variations
of unsharpmasking and allows for controlled sharpening based on edge
magnitude and min/max neighborhood value clipping. The real reason for its
existence is that it sports a gui with real time preview.


Syntax
------

``TUnsharp`` (clip, int "strength", int "thresholdL", int "thresholdU", int
"type", int "map", int "lim", int "radius", bool "gui")


PARAMETERS
----------

*strength* - The strength of the sharpening. Range is from 0 to 512.

default = 100 (int)

*thresholdL* - The lower edge magnitude threshold. If the edge magnitude for a
pixel is equal to or greater than thresholdL then it will be sharpened. Range
is 0 to 255.

default = 2 (int)

*thresholdU* - The upper edge magnitude threshold. If the edge magnitude for a
pixel is equal to or less than thresholdU then it will be sharpened. Range is
from 0 to 255 and it must be at equal to or greater then thresholdL or the
filter becomes a slow nop.

default = 40 (int)

*type* - Sets the type of sharpening or unsharpmasking operator. Current
options:

- 0 - linear (5 point)
- 1 - linear (9 point)
- 2 - teager
- 3 - cubic
- 4 - rational
- 5 - subtract
- 6 - Xsharpen

default = 0 (int)

*map* - Sets what type of output map if any. Current options:

- 0 - no map
- 1 - pixels to be sharpened shown
- 2 - as white on current frame binary map, pixels to be
  sharpened set to 255, others 0 on luma... all chroma set to 128.

default = 0 (int)

*lim* - If type less than 6, lim sets the maximum the final sharpened pixel
value can deviate from the min/max of the original 9 pixel neighborhood of
the original image. Range is from 0 to 256.

If type = 6 (XSharpen), then lim sets the maximum difference between the
current pixel and the min or max of the neighborhood (whichever is closer to
the value of the current pixel) there can be and still have the pixel be
mapped. If the difference is larger then lim, the pixel is passed through
untouched and not mapped to the min or max of the neighborhood.

default = 2 (int)

*radius* - Sets the radius of smoothing for producing the blurred clip. The
blurred clip is used for edge magnitude detection and for getting the values
of the unsharpmasking operators. Larger values will be slower but more
effective against noise. Possible settings are 1, 2, 3.

default = 2

*gui* - Sets whether or not this instance of TUnsharp should have a gui. Only
one instance of TUnsharp at a time can have a gui, but there can be multiple
instances per script (as long as the others don't have a gui, it will throw
an error if you try to create a second instance with one).

The options included on the gui are:

*strength*, *thresholdL*, *thresholdU*, *type* and *map*

Normal operation is you change an option on the gui, then you must advance at
least one frame in vdub to see the effect of the change. However, if you
click the gui's PREVIEW button, a new window will pop open with the current
frame and allow for seeing changes real time as you alter the filter
parameters. To change the current frame of the real time preview you must
change the frame in the vdub window.

The DEFAULTS button will reset all values to the values the filter was
constructed with (i.e. either the defaults or the values you set in your
avisynth script).

The OK button will close the dialog window and the preview window if it is
open.

default = false (bool)

+------------------------------------------------------------------------------------------------------------+
| Changelog                                                                                                  |
+========+============+======================================================================================+
| v0.9.3 | 05/24/2005 | - Fixed a few remaining thread sync/exit problems, hopefully for the last time       |
+--------+------------+--------------------------------------------------------------------------------------+
| v0.9.2 | 05/01/2005 | - Added XSharpen sharpening method                                                   |
+--------+------------+--------------------------------------------------------------------------------------+
| v0.9.1 | 05/01/2005 | - Fixed a lot of bugs related to the gui code (thread sync and thread exit problems) |
|        |            | - Release version now works fine                                                     |
+--------+------------+--------------------------------------------------------------------------------------+
| v0.9   | 10/24/2004 | - Initial release                                                                    |
+--------+------------+--------------------------------------------------------------------------------------+

$Date: 2005/07/10 16:11:01 $

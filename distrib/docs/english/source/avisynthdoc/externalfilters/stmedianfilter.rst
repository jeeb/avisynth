
STMedianFilter
==============


Abstract
--------

| **author:** Tom Barry
| **version:** 0.1.0.3
| **download:** `<http://www.avisynth.org/warpenterprises/>`_
| **category:** Spatio-Temporal Smoothers
| **requirements:**

-   YV12 & YUY2 Colorspace
-   SSEMMX (=ISSE) support

**license:** GPL

--------


Description
-----------

STMedianFilter is a (slightly motion compensated) spatial/temporal median
filter.

It fairly very fine grained, using only adjacent pixels in space and time, so
it looks at the adjacent 26 locations to filter each location. It now filters
both luma and chroma but chroma filtering is somewhat more limited, as
described later.

**USAGE - To use it just:**

1) Place the STMedianFilter.dll in a directory somewhere. You can get it
   from `<www.trbarry.com/STMedianFilter.zip>`_

2) In your Avisynth file use commands similar to

::

    LoadPlugin("F:\STMedianFilter\STMedianFilter.dll")
    Avisource("D:\wherever\myfile.avi")
    STMedianFilter(S_FilterThreshHold, S_EdgeThreshHold,
    T_FilterThreshHold, T_MotionThreshHold)

Of course replace the file and directory names with your own and supply the
integer values for parameters. Valid values for each are from 0 to 255.

S and T above stand for spatial and temporal, respectively.

A good starting point for parm values might be

::

    STMedianFilter(8,15,4,7)

Larger values cause more filtering but more artifacts. Parm descriptions:

*S_FilterThreshHold* - don't spatial filter pixel if it would change it more
than this Larger values may cause loss of fine line detail but it's not
extremely sensitive.

*S_EdgeThreshHold* - don't spatial filter pixel if on an edge where vals change
more than this. It seems fairly forgiving, so it can be raised a lot if
desired.

*T_FilterThreshHold* - don't Temporal filter pixel if it would change it more
than this Large values will cause ghosting. Don't set over 5-10 to avoid
ghosting.

*T_MotionThreshHold* - don't Temporal filter pixel if uncompensated motion vals
change more than this. Don't set over about 10 to avoid ghosting.


Method of operation
-------------------

But if I understand it right a simple median filter is just a clipping step
where a value is set to not extend past the high and low of its neighbors.

For instance, if you had 3 pixels in a row that had the values <5,8,7> you
could clip the center one to not be outside the low of 5 or the high of 7, so
you would set it to 7.

Now imagine you had a small 3x3 video screen, like one surface of a Rubiks
cube. Imagine the previous frame was the bottom layer of the cube, the
current frame was the middle layer, and the next frame was the top.

Then the current center pixel would be right in the center of the Rubics cube
and there would be 13 ways you could draw a line through it and a pair of two
nearest neighbors.

What I did was to compare each of those pairs of neighbors to see which pair
was most agreeing on value. I used that pair to clip the value of the center
pixel.

I also clipped the pixel value only if both:

1) It would not change the pixel value by more then the FilterThreshold
   parm value, and

2) The amount of 'uncompensated motion' (agreement on value by neighbors)
   was less than the MotionThreshHold parm value.

+--------------------------------------------------------------------------------------------------------+
| Changelog                                                                                              |
+==========+=============+===========+===================================================================+
| v0.1.0.3 | 2004/08/19  | Fizick    | - Fixed bug with spatial filter radius for YV12                   |
|          | (released   |           |                                                                   |
|          | 2005/01/30) |           |                                                                   |
+----------+-------------+-----------+-------------------------------------------------------------------+
| v0.1.0.2 | 2004/08/10  | Fizick    | - not public                                                      |
|          |             |           | - Fix one main bug (for both YUY2 and YV12 modes):                |
|          |             |           |   in all previous versions light pixels were not filtered,        |
|          |             |           |   both S_EdgeThreshHold and T_MotionThreshHold did not work also. |
+----------+-------------+-----------+-------------------------------------------------------------------+
| v0.1.0.1 | 2003/06/22  | Tom Barry | - Fix bug affecting YV12 only                                     |
+----------+-------------+-----------+-------------------------------------------------------------------+
| v0.1.0.0 | 2003/01/22  | Tom Barry | - Remove chroma filtering again.                                  |
|          |             |           |   Add YV12 support.                                               |
|          |             |           |   Use Pluginit2.                                                  |
|          |             |           |   Also remove Horizontal parms, add them to spatial.              |
|          |             |           |   Hopefully fix purple tint.                                      |
+----------+-------------+-----------+-------------------------------------------------------------------+
| v0.0.3.0 | 2002/08/03  | Tom Barry | - Add some chroma filtering                                       |
+----------+-------------+-----------+-------------------------------------------------------------------+
| v0.0.2.0 | 2002/08/03  | Tom Barry | - Bugs, Split out spatial, Temporal, Horizontal user parms        |
+----------+-------------+-----------+-------------------------------------------------------------------+
| v0.0.1.0 | 2002/08/02  | Tom Barry | - Initial test release                                            |
+----------+-------------+-----------+-------------------------------------------------------------------+

$Date: 2005/07/10 16:11:01 $

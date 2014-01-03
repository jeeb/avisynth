
DctFilter
=========


Abstract
--------

| **author:** Tom Barry
| **version:** 0.0.1.4
| **download:** `<http://mywebpages.comcast.net/trbarry/downloads.htm>`_
| **category:** Misc Plugins
| **requirements:**

-   YV12 & YUY2 Colorspace and screen multiples of 16x16
-   SSEMMX support

--------


Description
-----------

DctFilter is an experimental filter that, for each 8x8 block, will do a
Discrete Cosine Transform (DCT), scale down the selected frequency values,
and then reverse the process with an Inverse Discrete Cosine Transform
(IDCT).

Usage
~~~~~

In your Avisynth file use commands similar to

::

    Avisource("D:\wherever\myfile.avi")
    DctFilter(1,1,1,1,1,1,.5,0)

Parameters
~~~~~~~~~~~

There are 8 positional floating point parameters, all of which must be
specified as in the range (0.0 <= x <= 1.0). These correspond to scaling
factors for the 8 rows and colums of the 8x8 DCT blocks. The leftmost parm
corresponds to the top row, left column. This would be the DC component of
the transform and should always be left as 1.0.

In the above example the highest frequency components in each row and column
would be zeroed while the 2nd highest would be cut in half.

The row & column parms are multiplied together to get the scale factor for
each of the 64 values in a block. So if the top left value was V[0,0] then in
the example above the we would scale row 6, col 6 (V[6,6]) by .5*.5 = .25.

Note that while they look like floating point parms above they really now
only have 3 bit accuracy so the only actual values used are 0, 1/8, 1/4, 3/8
... 1.0. But you can specify any value and it will be rounded to the nearest
one.


DctFilterD - New with v 0.0.1.4
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

You can instead use DctFilterD(DiagCt). This works similar to above but will
zero out DiagCt number of the lower right diagonals of the DCT, leaving other
values unchanged. In an 8x8 DCT result matrix there are 15 possible
diagonals. (visualize a chess board) The DiagCt parm must be an integer from
1-14 saying how many of these diagonals must be zeroed, starting from the
lower right hand corner.


Comments
~~~~~~~~

This new function DctFilterD(DiagCt) works on diagonals. It is simpler and
either zeroes diagonals or leaves them alone.

In a 8x8 DCT result there are 15 possible diagonals (visualize a chess
board). So if you specify:

::

    DctFilterD(4)

then the 4 diagonals in the lower right corner of the DCT result will be set
to 0. The DiagCt parm may be any integer value from 1-14.

I haven't tested this much but, like my results with custom quant tables, it
appears if you set DiagCt very large you will start to get edge noise,
something like ringing.

$Date: 2004/08/13 21:57:25 $

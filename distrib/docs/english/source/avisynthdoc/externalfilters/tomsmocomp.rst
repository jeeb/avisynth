
TomsMoComp
==========


Abstract
--------

| **author:** Tom Barry
| **version:** 0.0.1.7
| **download:** `<http://mywebpages.comcast.net/trbarry/downloads.htm>`_
| **category:** Deinterlacing & Pulldown Removal
| **requirements:**

-   YV12 & YUY2 Colorspace
-   SSEMMX support

--------


Description
-----------

TomsMoComp.dll is a filter that uses motion compensation and adaptive
processing to deinterlace video source. It uses a variable amount of CPU time
based upon the user specified SearchEffort parameter. The SearchEffort may
currently be set anywhere from 0 (a smarter Bob) to about 30 (too CPU
intensive for everybody). Only certain values are actually implemented
(currently 0,1,3,5,9,11,13,15,19,21,max) but the nearest value will be used.
Values above 15 have not been well tested and should probably be avoided for
now.

TomsMoComp should run on all MMX machines or higher. It has also has some
added code for 3DNOW instructions for when it is running on a K6-II or higher
and some SSEMMX
for P3 & Athlon.


Sample avs script
~~~~~~~~~~~~~~~~~

::

    LoadPlugin("d:\AVISynth\TomsMoComp\Release\TomsMoComp.dll")
    clip = AviSource("c:\vcr\bikes.avi")
    return clip.TomsMoComp(1, 15, 1)

Of course replace the file and directory names with your own and supply the
desired integer values for parameters.

The above avs file specifies for TopFirst, SearchEffort=15, and a Vertical
Filter option to be turned on. I've so far tested it only with
Avisynth/VirtualDub.


TomsMoComp Parm list
~~~~~~~~~~~~~~~~~~~~

``TomsMoComp`` (clip, int TopFirst, int SearchEffort, int VerticalFilter)

All the values are integer, 0=no, 1=yes:

*TopFirst* - assume the top field, lines 0,2,4,... should be displayed first.
The default is the supposedly more common BottomFirst (not for me). You may
have to bring it up in Virtualdub and look at a few frames to see which looks
best. (0=BottomFirst, 1=TopFirst)

*New* - setting TopFirst=-1 will automatically pick up whatever Avisynth
reports. THIS DOES NOT SEEM TO WORK CORRECTLY WITH THE 2.5 ALPHA AND
MPEG2DEC3 v 0.9 !!!

*SearchEffort* - determines how much effort (CPU time) will be used to find
moved pixels. Currently numbers from -1 to 30 with 0 being practically just a
smarter bob and 30 being fairly CPU intensive.

For Avisynth only, a value of -1 is supported. In this case the TomsMoComp
filter will not deinterlace but instead assume you already have progressive
frames but want to double the vertical size. I found by accident that this
could give slightly better apparent detail than regular scaling algorithms
and is useful for low bit rate captures that are hard to IVTC/deinterlace or
where you have just kept the even fields for some other reason. I'm
considering making a DirectShow version of this to be run at display time, or
possibly adding it to ffDshow.

A VALUE OF -1 IS NOT SUPPORTED BY THE 2.5 ALPHA!!!

*VerticalFilter* - If turned on will very slightly blend each pair of
horizontal lines together. This loses only a small amount of vertical
resolution but is probably a good idea as it can somewhat hide remaining
deinterlace artifacts and will probably also make you clip compress a bit
better. (0 = no filter, 1 = filter)


Known issues and limitations
----------------------------

1) Assumes YUV (YUY2) Frame Based input. Use an Avisynth function to
   convert first if needed. YV12 is supported by the 2.5 Avisynth Alpha
   version only.

2) Currently still requires the pixel width to be a multiple of 4. This
   probably shouldn't be required but I pilfered the code from some of my
   other filters. Sorry, I'll fix it later.

3) So far it has only been tested on SSEMMX machines. (now others)

4) TomsMoComp is for pure video source material. Use IVTC, DeComb, or
   DScaler's Auto Pulldown processing for mixed or film source material.

$Date: 2004/08/17 20:31:19 $

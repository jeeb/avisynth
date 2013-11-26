
GreedyHMA
=========


Abstract
--------

| **author:** Tom Barry
| **version:** 0.4.1.0
| **download:** `<http://mywebpages.comcast.net/trbarry/downloads.htm>`_
| **category:** Deinterlacing & Pulldown Removal
| **requirements:**

-   YUY2 Colorspace
-   SSEMMX support

--------


Description
-----------

NOTE - This version is for Avisynth 2.5 (YUY2 only). Use the dll in the
"Older" folder for Avisynth 2.0x.

GreedyHMA - Greedy (High Motion for Avisynth)

GreedyHMA.dll is an Avisynth filter that executes DScaler's Greedy/HM
algorithm code to perform pulldown matching, filtering, and video
deinterlace.

Just unzip the contents into your Avisynth directory, or somewhere. As the
script shows, I made a subdirectory under Avisynth just to keep it separate.

Following is Bikes.avs, one of the scripts I was testing:

::

    LoadPlugin("d:\AVISynth\GreedyHMA\Debug\GreedyHMA.dll")
    clip = AviSource("c:\vcr\bikes.avi")
    return clip.GreedyHMA(1,0,4,0,0,0,0,0)

It specifies the file spec (change yours) and asks for TopFirst and
AutoPulldown to be turned on, with decimation (frame dropping) to 24 fps.
I've so far tested it only with Avisynth/VirtualDub.


Warning
~~~~~~~

Previous to V 0.4.0.0 I was telling everyone that TopFirst almost always work
better but that just turned out to be because of a bug in the BottomFirst
GreedyHMA code. So you should verify any previous tests here. Some folks were
getting unnecessary weave or deinterlace artifacts because of this.

On some sources I get slightly better results with TopFirst on (say HDTV
captures) but others are better with it off. I notice it's possible to run 2
copies of Vdub and do a frame by frame compare. There doesn't seem to be to
much difference but a still compare of two frames (look at diagonals) will
usually show one to be better. You can just look at the VirtualDub inputs for
this. It's not necessary to actually create any output files yet.

For Versio 0.4.0.0 I've added two new filter scripts to the zip to tell for
sure whether you should use TopFirst. Modify the BottomFirst.avs script to
point to your file and load it into VirtualDub. Then single step a few
frames. Ignore the single line up and down jitter but watch to see if every
other frame seems to go backwards. If so you probably need TopFirst. Try the
TopFirst.avs script to verify that. Neither of these two scripts depends upon
GreedyHMA.

After finding the correct TopFirst setting (T=0 or 1) then the apply the
following guidelines, substituting the TopFirst setting for 'T'.

If you think you have mostly film source, or DVD2AVI says so, then use:

::

    GreedyHMA(T,0,5,0,0,0,0,0) # which is Force Film+Decimation to 24 fps.

If you have all video, use:

::

    GreedyHMA(T,0,0,0,0,0,0,0) # for 30 FPS output (25 FPS PAL), or
    GreedyHMA(T,0,3,0,0,0,0,0) # for 24 FPS output

and when you don't know, don't care, or it's all mixed up, just make it auto:

::

    GreedyHMA(T,0,4,0,0,0,0,0) # the all purpose most automatic setting (NTSC),

    or

    GreedyHMA(T,0,1,0,0,0,0,0) # the all purpose most automatic setting (PAL)

Other switches can be added as below:


GreedyHMA Parameterlist
~~~~~~~~~~~~~~~~~~~~~~~

``GreedyHMA`` (clip, TopFirst, SwapFields, AutoPullDown, "MedianFilter",
"VerticalFilter", "EdgeEnhance", "GoodPullDownLvl", "BadPullDownLvl")

All the values are integer, 0=no, 1=yes:

*TopFirst* - assume the top field, lines 0,2,4,... should be displayed first.
The default is the supposedly more common BottomFirst but on ATSC captures
from my WinTV-HD card TopFirst still works better for me.

*SwapFields* - for busted capture drivers that put lines 1,3,5.. over lines
0,2,4...

*AutoPullDown* - Sets what types of Pulldown processing is desired. Valid parm
values are:

- 0 - No pulldown, just do pure deinterlace (Force Video). Doesn't drop any
  frames so FPS is left at 30 FPS, or wherever. Doesn't decimate
  (pentimate?). Use if you have (and want) 30 fps video source or maybe 25 FPS
  PAL video.

- 1 - Auto Pulldown. Automatically decide which frames should be IVTC'd or
  deinterlaced, depending upon the settings of the Good and Bad Pulldown
  Lvls (see below). Don't decimate. This is best if you have mixed film and
  video and wan't 30 fps output. Also better for PAL sources.

- 2 - Pulldown only (Force Film). Assume film source, always do IVTC and
  never deinterlace. Don't decimate. Use for PAL film source.

Values 3-5 match those above but with frame dropping (Decimation,
pentimation?) Five frames at a time are looked at and the one most likely to
be a duplicate is dropped, thus lowering the FPS from, say, 30 to 24.

- 3 - Deinterlace (Force Video) but with frame dropping. Use for NTSC video
  if you still want to decimate. With video source there really are no
  proper fields to drop, but this will drop the ones looking most like
  dupes.

- 4 - Auto Pulldown with frame dropping. The best (most automatic) setting.
  Works in most cases, at least if I get all the bugs out.

- 5 - Pulldown only (Force Film) with frame dropping. This will give the
  best results if you have 100% properly mastered NTSC film source with no
  video sections and not too many edits. It can adjust for most scene
  changes and changes in pulldown cadence, but not mixed up fields.

I haven't tried to see what's best with all this yet. Pulldown stuff's still
not perfect. But unless you're certain, you can often leave this on 4 (Auto).
It should still handle high motion video sports ok. If it falls into
deinterlace mode too often during film processing you can try raising the Bad
Pulldown Level a bit.

**NOTE! THE MEDIAN FILTER IS DISABLED FOR THIS RELEASE.**

*MedianFilter* (temporal) - Helps get rid of noise if needed but causes a 2
field video delay. (Otherwise there's a 1 field delay) Specifying 1 turns it
on using the current default of 3 (may change) but you can also specify 2-255
to override this. But values over about 3-5 can cause motion artifacts in
fast motion video.

See note above. I broke the Median Filter adding frame dropping since it runs
at a delay of 2 instead of everything else. It seemed less important so I
just disable it for now. The parm is currently ignored.

The Median Filter is an unneeded complication while GreedyHMA is new because
it changes the internal timing logic, confusing testing of some parts of the
Avisynth specific code. I'll re-enable it after everything else is solid, but
for now it's still disabled.

**NOTE**

*VerticalFilter* - Not only gets rid of noise but also some deinterlacing
artifacts since it's done last, after deinterlace. Useful if you are going to
down scale. Use only values 0,1 for parm. This also helps compression.

*EdgeEnhance* (horizontal) - Add a little sharpness to full res video. This one
can also use parm values of 2-100. The default is currently 50 if 1 is
specified. I may also support negative values in the future, making it a
horizontal filter to help set up for downscaling.

*Good Pulldown Lvl* - This is a number from 1-255 that must be exceeded before
GreedyHMA will do pulldown for a frame. This parm is honored only when Auto
Pulldown is also turned on (AP=1 or 4). The default is subject to change but
is currently set at 90. But you will get the default value just by specifying
0, which is recommended for most cases. It is really just a few frame average
of the savings in comb/weave factors by properly matching frames.

*Bad Pulldown Lvl* - This is a number from 1-255 that must NOT be exceeded
before GreedyHMA will do pulldown for a frame. This parm is honored only when
Auto Pulldown is also turned on. The default is subject to change but is
currently set at 85. But you will get the default value just by specifying 0,
which is recommended for most cases. This is really just a measure of how bad
a frame's comb/weave factors would be increased if we did pure field
matching.

New with V 0.4.0.0 when Force Fiim is specified (AP=2,5) the Bad Pulldown
level defaults to an impossibly huge amount. But it may now be overridden to
still deinterace on extreme frames of interlacing artifacts. I haven't tested
to find the optimum value but it's probably less than 150 or so. Specifying 0
gives the defualt of always Force Film.

For now, all 8 parms must be present, there is no error checking, and no more
doc except about Greedy/HM at `<www.dscaler.org>`_


Known issues and limitation
~~~~~~~~~~~~~~~~~~~~~~~~~~~

#. THIS IS FOR SSE MACHINES ONLY!

   This is written like the advanced options of DScaler Greedy/HM, partly in
   assembler for SSE machines only. I've so far got zero error checking in it,
   so it will just CRASH if you don't have at least a faster Celeron, P-III,
   Athlon, or higher. I'm not sure if this will be corrected in the future, but
   I might if time permits.

#. Currently no error checking or messages. I haven't figured out how to
   issue messages in Avisynth yet, and there is a bunch of stuff I should
   check that I'm not.

#. Assumes YUV (YUY2) Frame Based input. Use an AVIsynth function to
   convert first if needed.

#. Currently limited to a frame resolution of 1928x1088 pixels.

#. Fixed

#. There is a strange bug that I don't understand at all. If you add a
   new or changed GreedyHMA.dll to your machine, the first time you run
   VirtualDub/Avisynth/GreedyHMA then VirtualDub may stop responding. If you
   then use the Task Manager to cancel VirtualDub it will run after that. A
   work-around seems to be to first open any other AVI file in VirtualDub
   the first time you bring it up after installing GreedyHMA, before opening
   any Avisynth scripts. Later notes: it doesn't depend upon GreedyHMA at
   all, it happens without it on my machine, but I still don't understand
   it.

#. Currently works properly only if fed a screen width in pixels that is
   a multiple of 16. Otherwise it may produce garbage at the right of the
   screen that you may have to crop.

#. As stated above, the Median Filter option is temporarily ignored.


Please send comments to Tom Barry (trbarry at trbarry.com)

$Date: 2004/08/13 21:57:25 $

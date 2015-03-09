
PeachSmoother
=============


Abstract
--------

| **author:** Lindsey Dubb
| **version:** 1.0c
| **download:** `<http://www.avisynth.org/warpenterprises/>`_
| **category:** Spatio-Temporal Smoothers
| **requirements:** YUY2 Colorspace

--------


Introduction
------------

This is yet another filter which cleans up the picture, averaging out visual
noise between and within frames.

The Peach is designed to cope with the oddities of broadcast TV. It
automatically fine tunes itself for good or poor quality signal and
reception, and can even deal reasonably with some kinds of interference. It's
spiffy!

The filter examines the image, infers the amount of noise in the picture, and
uses that estimate to determine how to cancel the noise. To differentiate
between noise and motion, the filter makes use of correlations within and
between fields as well as local color differences. The degree of certainty
about motion, noise, and detail is used to decide what mix of the previous,
current, and nearby colors to show.

To use the Peach, your computer will have to be able to run SSE instructions.
In English, that means you will need to have a Pentium 3, Athlon, later
Celeron, or any more recent processor.

This filter works in the YUY2 colorspace. So if your clip is encoded as RGB
colors, you'll first need to process it with ConvertToYUY2().

The Peach Smoother AVISynth filter was written by Lindsey Dubb, copyright
2002. If you have any questions, comments, problems, or suggestions, you are
welcome to `send me an email`_ or post to Doom9's `AVISynth Forum`_.

--------


The Settings
------------

| **NoiseReduction**
| An integer from 1 to 200; 35 by default

Sometimes the filter doesn't know whether change in the picture comes from
noise or motion. When that happens, there's a tradeoff - Guess motion, and
the picture will be noisier if you're wrong. Guess noise, and a mistake will
blur the movement.

The NoiseReduction setting lets you tell the filter how to make this
tradeoff. Set it low, and the computer will play it safe, avoiding any parts
of the image which might be in motion. Set it high, and noise will be greatly
reduced - at the cost of blurring and loss of detail in slowly moving areas.
This tradeoff is affected by the amount of noise - The more noise there is in
the image, the more blurring will be necessary in order to reduce it.

There's no perfect way to choose this - Just try different values until it
looks as nice as possible. Comedies and dramas are probably the best material
for picking settings, since facial close-ups make any blurring really easy to
see. (That's because we're especially good at seeing distortion in faces.)

Remember that this filter automatically adjusts for the amount of noise in
the picture. So once you have a setting you like, you can use it for both
clean and noisy video.

However, there are two kinds of video which deserve some special
consideration: cartoons and sports. With cartoons, you can use a much higher
NoiseReduction, since animation tends to have less fine detail. And with
sports (or any other show with constant motion), there is very little to gain
from a noise filter, so you can just skip it.

Before you fine tune this setting, make sure that the filter has been able to
estimate the noise in your clip. To do so, make use of the Dot, described
below.

| **Stability**
| An integer from -100 to 100; 20 by default

This setting tells the filter how much to mix colors when it is uncertain
about whether there is motion. To preserve a small amount of noise throughout
the picture, decrease this setting. To get a really solid looking picture,
use a higher value. Don't set it too high, though, or you'll get
posterization.

Regardless of your Stability setting, the filter will always preserve a
certain amount of variation in order to maintain the color depth of the
original image.

| **DoSpatial**
| A boolean; TRUE by default

This setting decides whether you want the filter to use spatial smoothing.
It's a very subtle smoother, and is used mostly in low contrast, moving parts
of the picture. The filter is somewhat faster when spatial filtering is
turned off.

| **Spatial**
| An integer from 0 to 400; 100 by default

This determines how much spatial smoothing to use. It's measured in percent,
relative to the amount of temporal smoothing. But the Peach prefers to use
temporal smoothing whenever possible, so the spatial smoothing only kicks in
when temporal smoothing fails.

| **Dot**
| A boolean; FALSE by default

It's... a small green dot!

With this option, a tiny green dot will appear when the filter's estimate of
the noise is confirmed by the current picture. It will show up near the upper
left of the screen - specifically 16 down and 16 across from the upper left
corner. The dot is an indication that the filter has settled on a noise
value. In general, it will turn off when all of the picture is in rapid
motion - When there's too much motion, the filter tends not to believe the
current estimate. In that case, the filter makes do by weighting previous
good values.

If the picture is constantly in motion, it'll take a while before the filter
can figure out the noise level. Interference also tends to make noise
estimation take longer - The estimation time depends on the kind and amount
of the interference.

| **Readout**
| A boolean; FALSE by default

To help decide how much to smooth, this filter measures the noise in the
video. When you enable Readout, these measurements will be shown in the
output.

You will see two numbers. The first one is a measurement of the mean noise
level. The second is a measure of the lowest amount of noise seen anywhere in
the picture. That provides some idea about how much noise varies in the
video. These estimates will adjust as the video progresses.

The numbers are measured as the expected absolute change in a single pixel
(counting both chroma and luma) between two adjacent frames.

This provides an objective measurement of noise. It is also the way to figure
out values for the NoiseLevel and Baseline options, described below.

| **NoiseLevel** and **Baseline**
| Two settings - each is a floating point number from 0.0 to 100.0; Unused by
  default

Rather than estimate the noise level, you can tell the Peach exactly how much
noise there is in the picture.

To do so, choose values for both NoiseLevel and Baseline. To get these
values, use the Readout option, described above. Note that  Baseline should
never be more than NoiseLevel.

Why would you want to specify these numbers? Because the Peach can take a
while to figure out the amount of noise. And on rare occasions it can be
badly mislead. (This is usually caused by scenes with smoke and dust clouds,
which the Peach can mistakenly identify as noise.) By specifying the noise
values, the estimate will never waver. Skipping noise estimation also makes
things run a little faster.

On the other hand, some video really does have changing amounts of noise. In
that case, you'll be much better off with the Peach's usual noise detection.

| **ShowMotion**
| A boolean; FALSE by default

This is another obscure diagnostic. When this is turned on, the filter will
interleave its estimate of whether motion is occuring. White areas are
definitely motion, black areas are definitely stationary. Gray areas are gray
areas.

| **Debug**
| A boolean; FALSE by default

This option is only meant for the stout of heart. For an explanation of its
output, see the comments toward the bottom of `FLT_AdaptiveNoise.c`_.


About the Peach
---------------


How much noise reduction should I use?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Let your eyes be the judge. I like to keep the settings pretty low - Blurred
faces look worse than a little static.

Surprisingly, small amounts of color variation can sometimes improve an
image. By switching back and forth between colors, the picture is able to
give the impression of a color somewhere in between them. Also, noise can
break up artifactual patterns in the picture, making it easier for you to
ignore the errors.

With that in mind, the Peach tries to preserve a small amount of color
variation. As a result, it will never give you a completely stable picture.
This is probably bad for compression, but it does improve picture quality.


I'm seeing lots of blurring in early parts of my constant motion (or very dark) video. What should I do to improve the results?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

If you're seeing lots of blurring, try using the Dot option. If the green dot
doesn't show up, then the problem is that the Peach wasn't able to figure out
how much noise there was.

There are a couple ways to solve this. The best is to turn on the Readout
option, and watch a later part of the video where the picture is still. Make
a note of the NoiseLevel and Baseline from that stationary part. Then specify
them in your command, and the whole sequence should look fine.

If your video doesn't have any stationary parts, then you should just skip
this filter. A temporal smoother isn't going to do much good for pure high
motion material.

Another way to solve this problem is to put some still video (from the same
clip) at the beginning of the sequence. That will allow the Peach to estimate
the noise before the fast stuff shows up.


The picture looks a little soft. Is there anything I can do about that?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Try reducing the Stability setting.


Noise in General
----------------


What's the best way to get rid of noise?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Make sure you have a good signal. Noise can come from cable jumbles, poor
connections, poor power and grounding, poorly designed video input cards,
electrical gadgets (anything from a dimmer switch to various computer
components) or from a bad video source. These issues are all beyond the scope
of this help file. I'd suggest a look at the `AV Science Forum`_'s Home
Theater Computers FAQ and board, where these topics are discussed at length.


Should I use a noise filter?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

It depends what you're watching. Sports and nature shows in general - really
anything with lots of moving low contrast texture - are not handled well by
temporal smoothers. That's because those textures look a lot like noise.

The Peach does well with difficult material, disabling itself where it
detects motion. It causes surprisingly few problems with field sports so long
as the background noise isn't too bad. But it isn't perfect - Road races are
especially prone to blurring. When that happens, it's best to skip any
temporal smoothers.

Otherwise, it's generally worth running a noise filter.


How many noise filters should I use?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Be very careful about using more than one. Running multiple temporal noise
filters can cause the later filters to have excess confidence in their
noise/motion estimate. That usually leads to posterization, speckling, and
banding.

Running a spatial filter after Peach Smoother can still work reasonably well,
since its spatial smoothing is pretty subtle - Just be sure not to run the
spatial filter first.


Where in my script should I put the noise filter?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

In general, it is best to filter noise as early as possible. This is
especially true for the Peach, since its noise estimation can be thrown off
by some filters. For example, if you subtitle your video, the Peach may
detect the lack of noise in the subtitles.

With interlaced material, it is a very good idea to run the filter before
deinterlacing. That's because good noise reduction can greatly improve the
accuracy of the deinterlacer.

For inverse telecine the situation is harder to judge. But as a rule of
thumb, it is still better to run the noise reduction first.

The exception to this rule is comb filtering. Any comb filters should be run
before noise reduction, since the noise filter will otherwise interpret color
crosstalk as motion.


Can any other tricks help reduce noise?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

If you're using a Bt8x8 card, you can turn off Odd and Even Luma Peaking.
Turning on the card's Horizontal Filter will reduce noise but will also lose
some detail.


+-------------------------------------------------------------------------------------------+
| Filter Changes                                                                            |
+=======+===================================================================================+
| v0.9a | Initial release                                                                   |
+-------+-----------------------------------------------------------------------------------+
| v0.9b || Allowed parameters to be set;                                                    |
|       || Different defaults;                                                              |
|       || Corrected to deal with Y = 16 black; Additional orange smoke                     |
+-------+-----------------------------------------------------------------------------------+
| v0.9c | Added parameter checking                                                          |
+-------+-----------------------------------------------------------------------------------+
| v0.9d | Corrected to allow spatial smoothing of more than 100%                            |
+-------+-----------------------------------------------------------------------------------+
| v0.9e | Filter now turns off when jumping between frames                                  |
+-------+-----------------------------------------------------------------------------------+
| v0.9f || Filter now handles interlaced material correctly;                                |
|       || The smoothing option has been rescaled to something closer to a real percentage; |
|       || Different defaults;                                                              |
|       || Yet more orange smoke                                                            |
+-------+-----------------------------------------------------------------------------------+
| v0.9g || Enabled spatial (but not temporal) smoothing when jumping between frames;        |
|       || Turning off spatial smoothing now improves speed;                                |
|       || enabled prefetching for a speedup;                                               |
|       || improved parameter handling                                                      |
+-------+-----------------------------------------------------------------------------------+
| v0.9h || Provided a (legible) onscreen readout of the amount of noise.                    |
|       || Allowed noise values to be supplied by the user.                                 |
|       || Allowed spatial smoothing in the first frame.                                    |
+-------+-----------------------------------------------------------------------------------+
| v1.0a || Improved determination of amount of spatial smoothing to use.                    |
|       || Improved horizontal correlation code.                                            |
|       || Improved accuracy of noise estimate readout.                                     |
|       || Made temporal color combination closer to the theoretical goal.                  |
|       |  (Side effect: you might want to drop your NoiseReduction by about ten points.)   |
|       || Increased role of spatial correlation in motion estimates.                       |
|       || Changed the name of the Estimates option to Readout.                             |
+-------+-----------------------------------------------------------------------------------+
| v1.0b || Provided a display of the internal motion estimate with the ShowMotion option.   |
|       || Allowed negative Stability settings.                                             |
+-------+-----------------------------------------------------------------------------------+
| v1.0c | Moved the readout a little further from the edge of the screen.                   |
+-------+-----------------------------------------------------------------------------------+

$Date: 2004/08/17 20:31:18 $

.. _send me an email: mailto:lindsey@alumni.caltech.edu
.. _AVISynth Forum: http://forum.doom9.org/forumdisplay.php?s=&forumid=33
.. _FLT_AdaptiveNoise.c : http://cvs.sourceforge.net/cgi-bin/viewcvs.cgi/deinterlace/DScaler/Plugins/FLT_AdaptiveNoise/
.. _AV Science Forum : http://www.avsforum.com/

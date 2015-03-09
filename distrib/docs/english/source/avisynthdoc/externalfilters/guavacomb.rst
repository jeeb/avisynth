
GuavaComb
=========


Abstract
--------

| **author:** Lindsey Dubb
| **version:** 0.9a
| **download:** `<http://www.avisynth.org/warpenterprises/>`_
| **category:** Broadcast Video Plugins
| **requirements:** YUY2 Colorspace

--------


Description
-----------

This filter removes dot crawl, rainbows, and some kinds of shimmering. The
effect is most noticeable on computer generated images like subtitles,
weather maps, and station logos using a composite or poorly separated S-Video
signal. Normal video is more subtly improved.

Note that this filter is not in any way related to "comb" in the sense used
by many video folks! It's not supposed to reduce weave artifacts (lagging
horizontal lines) at all. For that, you need to take a look for a filter
meant for deinterlacing.

This comb filter only works temporally, not spatially. As a result, it only
helps with dot crawl, rainbows, and shimmering in stationary portions of a
video.

This filter works in the YUY2 colorspace. So if your clip is encoded as RGB
colors, you'll first need to process it with ConvertToYUY2(). It is meant to
be run on material which is captured at the full NTSC or PAL resolution - You
can run it on other video, but it probably won't do you much good.

To use the Guava, your computer will have to be able to run MMX instructions.
That means any computer more recent than an original Pentium I will do.


Who Should Use It?
------------------

The Guava is meant for anyone processing a composite signal. That's what you
have if you're using an internal TV tuner or an RCA connector.

A comb filter can also be worthwhile if your S-Video source isn't separated
very well. DVDs should have great separation if they are internal or
connected with S-Video. LaserDiscs and S-VHS VCRs vary - If they have a "3D
comb filter," then dot crawl should already be taken care of. If you have any
other kind of comb filter, then the Guava Comb may or may not help - you'll
have to judge for yourself. If your S-VHS VCR or LaserDisc does not have a
comb filter, then the Temporal Comb filter should be worthwhile.

The other question to keep in mind is the quality of your input card. If your
input card has its own 2D comb filter, then the Guava will be much less
useful. But Guava Comb can help in some places where a 2D filter cannot, so
it may still be worth using.

SECAM handles color in a very different way than PAL or NTSC. Except for
video from Saudi Arabia (which uses SECAM with PAL-style color encoding),
this filter should not be used on SECAM clips.


The Settings
------------

Unfortunately, the best settings for this filter depend on the details of
your video input card. In particular, some obscure settings like luma peaking
(on the Bt8x8 card) can greatly affect the settings you should use.

| **Mode**
| Either "NTSC" or "PAL"; This setting has no default.

You can probably guess how to choose this one - "NTSC" for Japanese and North
American video, "PAL" for most other material. If you pick it wrong, the
filter will be worse than useless.

At the moment there is also a third mode called "Progressive". This is meant
for testing to see whether this filter can be used to correct for compression
artifacts. I have no idea whether it will really do any good.

| **Recall**
| An integer from 0 to 99; 85 by default

This is the really important setting. It decides just how far back into the
past the Guava will look in order to confirm that there's really some
shimmering going on. Set it high (above 75%) to avoid artifacts in scenes
with repetitive motion. Set it higher (above 85%) to avoid problems with
ticker tapes. But don't set it too high (above 95%), or it'll need a long
time for the filter to kick in.

When you change this setting, it will take a moment before the screen can
adjust. Really high settings (above 90%) need especially long to take.

| **MaxVariation**
| An integer from 0 to 100; 25 by default

This option lets you choose just how much the color can change before the
filter decides the difference must be due to motion. This is an important
setting for avoiding artifacts, but is unfortunately somewhat tricky to
choose.

The lower you set it, the less often you will see transient spots when
switching scenes. But if you set it too low, you won't get rid of as much
crosstalk.

The best setting depends on the details of your input card. For example, if
you are using a Bt8x8 card and have Luma Peaking enabled, you will need to
increase this setting by 10 to 15 points.

There isn't an intuitive way to choose MaxVariation, so here's a guide to
help you set it:

-   First, find the worst stationary dot crawl or rainbows you have. In
    the US, CNN Headlines is a dependable source, as are the "T" in the
    Country Music Station's logo and the radar maps on the Weather Channel.
    If you've got a video reference, connect it via composite and record a
    still with neighboring green and magenta color bars. (But if your input
    card has a 2D comb filter, then color bars won't work, since the card's
    comb filter will correct it.)
-   Set Recall and Activation to very low values - 0 should do. Set
    MaxVariation really high (to about 80). The artifacts should disappear.
-   Reduce your MaxVariation down until the dot crawl or shimmering
    returns. Then increase it just enough to make the artifacts disappear,
    again. You've now got yourself the optimal value for it.
-   Remember to switch back to more reasonable Recall and Activation
    settings..



**Activation**

This setting has a much subtler effect. If you set it high, the filter will
only correct when there has been uninterrupted shimmering. Set it low, and
the filter will tolerate more interruptions - but will also be a little more
susceptible to artifacts.

If you run a noise filter before the Guava, you can afford to set this low -
30% works well for me. Without a noise filter, you'll need to set it pretty
high (about 70%) to avoid transient spots where dot crawl has just
disappeared. If you don't care about shimmering and just want to take care of
dot crawl, you can safely set this near 100%.

**Looking for Trouble**

There are a few main kinds of problems to look for. The most important occurs
at scene transitions, where regions which had been showing dot crawl can
potentially be averaged with the first field after the transition. The second
most important artifact to watch for is stray incorrectly blended pixels
where there is fast motion. These can both be greatly reduced by decreasing
your MaxVariation setting.

The final (but generally least significant) place to check is with repetitive
motion - If it happens to occur at just the wrong frequency, the Guava may
misinterpret it as crosstalk. In particular, strobe effects may cause fits.
Also, some ticker tapes can be misread as crosstalk. You can generally avoid
this kind of problem by increasing the Recall setting.

--------


Where should I put it in my script?
-----------------------------------

In general, it is best to run comb filtering immediately after any smoothing
filters. The exception to this is the Peach Smoother, which works best right
after the comb filter. That's because the Peach will otherwise interpret
color crosstalk as noise, preventing smoothing around it.

Comb filtering should definitely be run before any inverse telecine steps.
Color crosstalk can badly mislead inverse telecine, so it is worth cancelling
it out before trying to figure out the pulldown pattern.

--------


What the Heck is a Comb Filter?
-------------------------------

In technical terms, it averages out chrominance/luminance crosstalk caused by
imperfect separation from a composite signal. It's called a "comb" for a
really obscure reason - because the frequency response looks a (very little)
bit like a comb. Apparently, signal processing engineers are obsessed with
grooming.

Here's an example of the problem, taken from a US Cable TV broadcast with an
internal tuner. There's heavy dot crawl in the year number, and more subtle
problems in and around the other text. On your screen, those dots would seem
to be moving slowly upward.

.. image:: pictures/gcomb_off_election.png

And here's the effect of the Guava Comb Filter on the same picture. Though
the dot crawl is corrected, the banal headline remains.

.. image:: pictures/gcomb_on_election.png


Though crosstalk is rarely as obvious as in that last graphics, there is
often subtle crosstalk with edges in cartoons and normal video. For example,
first picture shows "Jack" without filtering.

.. image:: pictures/gcomb_off_jack.jpg

...and here he is after comb filtering. Note the smoother boundary between
his head/neck and the background. (Ignore the stuff under his lip - it comes
from the image compression.)

.. image:: pictures/gcomb_on_jack.jpg

Here you see another effect of color crosstalk - rainbows.

.. image:: pictures/gcomb_off_seattle_channel.png

And this is a picture from the same clip, corrected with the Guava Comb.

.. image:: pictures/gcomb_on_seattle_channel.png

$Date: 2004/08/13 21:57:25 $

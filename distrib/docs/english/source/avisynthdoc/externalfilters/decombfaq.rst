
Decomb Plugin (Version 5.2.1) for Avisynth
==========================================

Frequently Asked Questions (FAQ)
::::::::::::::::::::::::::::::::

**by Donald A. Graft**

.. toctree::
    :maxdepth: 3


My video sequences come out jerky, even when I don't decimate the clip.
-----------------------------------------------------------------------

Use post=4 (and post=5) rather than post=2 (and post=3). Also, do not set an
unnecessarily low *vthresh* parameter.


The *dthreshold* parameter is not working.
------------------------------------------

The *dthreshold* parameter was renamed to *dthresh* in Telecide for Decomb
5.0.0. It is still called *dthreshold* in FieldDeinterlace.


Why was Decomb 5.0.0 made field-order dependent?
------------------------------------------------

It's faster and less error-prone to do 2 comparisons rather than 3. But
Decomb 5.0.0 will still try the third match if a frame is still combed after
the 2-way match. So no extra combed frames are emitted compared to Decomb 4.
The tutorial provides a really easy way to determine a clip's field order.


Why am I getting some bad field matches?
----------------------------------------

First, verify that your field order is set correctly using the procedure
given in the tutorial!

The field matching process is not perfect. However, it can be improved in
some situations. For example, if you have a noisy clip (as you might get from
an analog capture), you can often improve the matching by using the *nt*
parameter to add more noise tolerance than the default value of 10. The best
value will depend on how noisy your clip is. But be cautious when raising the
noise tolerance, because it can degrade matching for clean clips.

You can also improve matching in some cases by enabling pattern guidance. In
some uncommon cases you can improve matching by disabling pattern guidance!
Try it both ways and see which gives you better results.


Why am I getting combed frames after some scene changes?
--------------------------------------------------------

These scene changes require a third (backward) match. Decomb will test the
third match only if *post* is set to any value other than 0. Note that you
can enable the third match testing without enabling postprocessing: you can
set post=1. You can also enable three-way matching on all frames (not just
ones that are combed after two-way matching) by using the *triple* parameter.
Setting triple=true, however, can produce jerkiness.


Even after a frame is deinterlaced, I still can see combing on it.
------------------------------------------------------------------

First, use post=3 or post=5 to verify that the frame is actually being
deinterlaced. If it is not, then you need to reduce your *vthresh* parameter
until it is. If it is being deinterlaced, then you can decrease the *dthresh*
parameter to attack more areas of the frame, but see the caveat below.

Make sure that what you think is combing is really combing. For example,
sometimes the clip has content that is intentional but looks like combing. If
you use SeparateFields() (by itself without Decomb) and still see "combing"
on the fields, then it is not really combing but intentional content.

| If you have a color conversion before Decomb, be sure to use
| ConvertToXXX(interlaced=true).

Do not resize vertically before Decomb.

*Caveat About Residual Combing*: People will often freeze on an frame and
zoom in closely and see some faint residual combing and agonize over it. But
that is a seriously misguided way to assess residual combing. Human
perception cannot register contrast when the difference becomes less than a
threshold. But the threshold gets lower as the feature size gets larger. That
means that when you zoom in, you get an incorrect idea of what can actually
be seen at normal size. Also, when things are in motion we see them with
reduced resolution; therefore, freezing the frame gives an incorrect idea of
what can be perceived. To assess residual combing, play the clip at normal
speed and size. If it looks good, then be happy!

Finally, reducing *dthresh* to low values (such as 0-5) throws away most of
the advantages of the area-based adaptive deinterlacing that Decomb performs.
You will lose resolution on static areas of the frames, where you want it the
most.


I get "marching ants" on some hard horizontal edges when using FieldDeinterlace.
--------------------------------------------------------------------------------

This artifact is a consequence of the field-differencing algorithm employed
by FieldDeinterlace. All deinterlacers have characteristic faults like this.
There is no perfect solution to deinterlacing. This is usually seen at the
edges of letterboxing. You can crop off the letterboxes first and restore
them afterwards (if you need them) as a workaround. Alternatively, you can
use a frame-differencing deinterlacer, such as TomsMoComp, if you can accept
the characteristic artifacts of frame differencing.

I am exploring ways to reduce this artifact and hope for better performance
in the future.

I'm confused about the difference between *threshold* and *dthreshold* in FieldDeinterlace.
-------------------------------------------------------------------------------------------

The process goes like this (if full = false): First the frame is tested to
see if it is combed. This is done by detecting combed areas and seeing if
there is enough combing to reasonably declare the frame as combed. The
*threshold* parameter determines the sensitivity of the combing detection in
this step. Second, if the frame is declared as combed, the combed areas of
the frame are again detected but this time using the *dthreshold* parameter.
The areas of the frame detected as combed using *dthreshold* are then
deinterlaced.

If full=true, the first step is omitted and all frames are declared as
combed.

FieldDeinterlace's *map* parameter allows you to directly visualize the
effect of these parameters.


I get "ghosting" on my deinterlaced frames.
-------------------------------------------

Refer to the
:ref:`question on blending versus interpolation <Technically, what is the difference between blending and interpolation>`.
To get rid of ghosting, set blend=false.


.. _Technically, what is the difference between blending and interpolation:

Technically, what is the difference between blending and interpolation?
-----------------------------------------------------------------------

Suppose you have 3 pixels on successive lines and the same x offset:
::

    a
    b
    c

If you decide that pixel b is combed, and you replace it with
(0.5*a+0.5*c) that is (linear) interpolation. If you replace it with
(0.25*a+0.5*b+0.25*c), that is blending. Note that when interpolating,
the even lines are passed through and the odd lines are calculated as
described. For blending, every line is calculated. Blending thus
produces a blend of the two fields while interpolation uses data from
one field only. That is why people say that blending causes "ghosting".
You can eliminate the ghosting by setting blend=false. Also note that
while interpolation can be calculated in place, blending cannot.
Interpolation is a little faster.

Different parts of my movie seem to require different *vthresh* values. What can I do?
--------------------------------------------------------------------------------------

The combing detector used by Decomb is quite sensitive but that can sometimes
cause you trouble when you have content that has sections that are not combed
but have detail that looks like combing. If you set *vthresh* high to avoid
deinterlacing these areas, you may let combs slip through elsewhere. While
it's better to err on the side of caution and deinterlace some good frames,
some extreme cases make you wish you could set different *vthresh* values for
different parts of the clip. Fortunately, Decomb allows this through its
manual override capability. Refer to the reference manual for further
details.


How does *gthresh* affect the field matching when pattern guidance is enabled?
------------------------------------------------------------------------------

If the pattern guidance thinks that the calculated (blind) field match is not
what it predicts based on pattern analysis, it tries to override the blind
field match. The override is allowed if the pattern mismatch is less than
*gthresh*. Think of *gthresh* as a limiter: it limits how large the mismatch
can be before a predicted match is overruled by the actual calculated match.
If your *gthresh* is too high, it may cause a new pattern in the clip to be
erroneously overridden by the previous pattern. There is a happy medium that
allows new patterns to be locked onto quickly while still allowing the
pattern guidance to correct mismatches due to noise, etc. The happy medium is
usually in the range 5-10%, but it will be clip-specific.

--------

Copyright (C) 2003, Donald A. Graft, All Rights Reserved.

For updates and other filters/tools, visit my web site:
`<http://shelob.mordor.net/dgraft/>`_

$Date: 2004/08/13 21:57:25 $


GrapeSmoother
=============


Abstract
--------

| **author:** GrapeSmoother by Lindsey Dubb
| **version:**
| **dowload:** `<http://www.avisynth.org/warpenterprises/>`_
| **category:** Temporal Smoothers
| **requirements:** YUY2 Colorspace

--------


Description
-----------

This filter cleans up the picture, averaging out visual noise between frames.

When colors change just a little, the filter decides that it's probably
noise, and only slightly changes the color from the previous frame. As the
change in color increases, the filter becomes more and more convinced that
the change is due to motion rather than noise - and the new color gets more
and more weight.

To use the Grape, your computer will have to be able to run MMX instructions.
That means any computer more recent than an original Pentium I will do. It
runs very quickly.

This filter works in the YUY2 colorspace. If your clip is encoded as RGB
colors, you'll first need to process it with ConvertToYUY2().

The Grape Smoother AVISynth filter was written by Lindsey Dubb, copyright
2002. If you have any questions, comments, problems, or suggestions, please
`send me an email`_ or post to Doom9's `AVISynth Forum`_.

Syntax
~~~~~~

``GrapeSmoother`` (clip, int "smoothing")

| **Smoothing**
| An integer from 1 to 100; 30 by default

This is the only option. The higher you set it, the more you'll cut down on
noise - but the more blurring you will see. In general, reasonable values
range from 10 (to soften the occasional speckle without artifacts) to 25 (to
reduce noise with minimal artifacts) to 45 (for better reduction, but visible
blurring) up to 80 (to average out really heavy noise at the cost of bad
blurring). At extremely high Smoothing values, you'll start to see
artifactual vertical lines.

To choose a setting for it, try it on just about any comedy or drama -
anything with close-ups of moving faces. Increase Smoothing until the
suppression of noise doesn't seem worth the blur.

$Date: 2004/08/13 21:57:25 $

.. _send me an email: mailto:lindsey@alumni.caltech.edu
.. _AVISynth Forum: http://forum.doom9.org/forumdisplay.php?s=&forumid=33

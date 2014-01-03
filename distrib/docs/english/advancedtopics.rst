
Advanced Topics
===============


.. contents:: Table of contents
    :depth: 3


Interlaced and field-based video
--------------------------------

Currently (v2.5x and older versions), AviSynth has no interlaced flag which
can be used for interlaced video. There is a field-based flag, but contrary
to what you might expect, *this flag is not related to interlaced video*. In
fact, all video (progressive or interlaced) is frame-based, unless you use
AviSynth filters to change that. There are two filters who turn frame-based
video into field-based video: `SeparateFields`_ and `AssumeFieldBased`_.

More information about this can be found `here[1]`_

Color format conversions, the Chroma Upsampling Error and the 4:2:0 Interlaced Chroma Problem
---------------------------------------------------------------------------------------------

The *Chroma Upsampling Error* is the result of your video is upsampled
incorrectly (interlaced YV12 upsampled as progressive or vice versa).
Visually, it means that you will often see gaps on the top and bottom of
colored objects and "ghost" lines floating above or below the objects. The
*4:2:0 Interlaced Chroma Problem* is the problem that 4:2:0 Interlaced itself
is flawed. The cause is that frames which show both moving parts and static
parts are upsampled using interlaced upsampling. This result in chroma
problems which are visible on bright-colored diagonal edges (in the static
parts of the frame). More about these issues can be found `here[2]`_.

Colorspace Conversions
----------------------

About the different RGB <-> YUV `color conversions`_.

Wrong levels and colors upon playback
-------------------------------------

When playing back video content, several issues might go wrong. The levels
could be wrong, resulting in washed out colors (black is displayed as dark
gray and white is displayed as light gray). This is described in more detail
`here[3]`_. The other issue is a slight distortion in color (which often looks
like a small change in brightness) and this is described `here[4]`_.

AviSynth, variable framerate (vfr) video and Hybrid video
---------------------------------------------------------

There are two kinds of video when considering framerate. Constant framerate
(cfr) video and variable framerate (vfr) video. For cfr video the frames have
a constant duration, and for vfr video the frames have a non-constant
duration. Many editing programs (including VirtualDub and AviSynth) assume
that the video has cfr. One of the reasons is that avi doesn't support vfr.
This won't change in the near future for `various reasons`_. Although the avi
container doesn't support vfr, there are several contains (mkv, mp4 and wmv
for example) which do support vfr.

It's important to realize that in general video is intrinsically cfr (at
least in the capping video or ripping dvds arena). There is one exception
where converting to vfr is very useful, which is hybrid video. Hybrid video
consists of parts which are interlaced/progressive NTSC (29.97 fps) and FILM
(which is telecined to 29.97 fps). When playing hybrid video the NTSC part
(also called video part) is played back at 29.97 fps and the telecined part
at 23.976 fps.  Examples of hybrid video include some of the anime and Star
Trek stuff.

More info about creating vfr video and opening it in AviSynth can be found
`here[5]`_.

Importing your media in AviSynth
--------------------------------

A lot of media formats (video, audio and images) can be imported into
AviSynth by using one of AviSynth's internal filters, specific plugins or
DirectShowSource in combination with the appropriate DirectShow filters. It
is not always trivial to import your media into AviSynth, because there are
often many ways to do so, and for each way you need to have some specific
codecs installed. `This document`_ describes which formats can be imported in
AviSynth and how they should be imported. Also a short summary is included
about how to make graphs (graphs of approriate DirectShow filters which can
be used to play you media file) in Graphedit and how to open the graphs in
AviSynth.

Resizing
--------

Resampling is the process of converting a signal from one sampling rate to
another, while changing the information carried by the signal as little as
possible. When applied to an image, this process is sometimes called image
scaling. More about image scaling, various resampling kernels and the
implementation in AviSynth can be found `on avisynth.nl`_ (...).

$Date: 2010/02/28 14:31:47 $

.. _SeparateFields: corefilters/separatefields.rst
.. _here[1]: advancedtopics/interlaced_fieldbased.rst
.. _AssumeFieldBased: corefilters/parity.rst
.. _here[2]: advancedtopics/sampling.rst
.. _color conversions: advancedtopics/color_conversions.rst
.. _here[3]: advancedtopics/luminance_levels.rst
.. _here[4]: advancedtopics/colorimetry.rst
.. _various reasons:
    http://forum.doom9.org/showthread.php?s=&threadid=69132
.. _here[5]: advancedtopics/hybrid_video.rst
.. _This document: advancedtopics/importing_media.rst
.. _on avisynth.nl: http://avisynth.nl/index.php/Resampling

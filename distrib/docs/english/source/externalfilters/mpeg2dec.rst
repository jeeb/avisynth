
MPEG2Dec
========


Abstract
========

| **authors:** Dividee and others
| **version:** 1.10
| **category:** MPEG Decoder (source) Plugins
| **download:** `<http://www.avisynth.org/warpenterprises/>`_ (source can be found on `Dividee's homepage`_)
| **requirements:**

--------


Description
-----------

This filter is able to output in the RGB and YUY2 colorformats.

It is a MPEG-2 decoder and it is able to decode any MPEG-2 streams readable
by dvd2avi 1.76

Filters
-------

A list of all filters included in this special mpeg2dec.dll:


TemporalSmoother
~~~~~~~~~~~~~~~~

``TemporalSmoother`` (clip, int "strength", int "radius")

This is a port of the TemporalSmoother filter from `VirtualDub`_. It works in
RGB32 & YUY2 pixel formats.
From VirtualDub help file:

This filter is an adaptive noise reducer, working along the time axis; it
is most effective when the image is not moving much. Increase the filter
strength to increase noise reduction, and decrease it to reduce speckling and
ghosting artifacts. It is recommended that you combine this filter with a
spatial (area-based) noise reducer for greatest effect. All frames no more
than radius away are examined. Minimum radius is 1, maximum is 7. Larger
values are of course slower. Default is 3, which is the value used in
VirtualDub. The default value for strength is 2.

Note that unlike its VirtualDub counterpart, this filter has no lag. It
ensures that the frames needed for its work are in its buffer. Therefore,
random access in the clip is slow, especially with a large radius.

Example:
::

    # The filter effects in RGB and YUV are not 100% equivalent, especially with a high strength.
    # Let's visualize the difference

    cYUY2 = ConvertToRGB(TemporalSmoother(ConvertToYUY2,10))
    cRGB = TemporalSmoother(10)
    Subtract(cYUY2, cRGB)

SelectRangeEvery
~~~~~~~~~~~~~~~~

``SelectRangeEvery`` (clip, int every, int length, int "offset")

Select length frames every every frames, starting after offset.

SelectRangeEvery(every, length, offset) is the same as
SelectRangeEvery(Trim(offset, 0), every, length)

BlendFields
~~~~~~~~~~~

``BlendFields`` (clip)

Deinterlace a clip by blending the fields together. This is the blend mode of
the internal vdub deinterlace filter ported to avisynth. It works in RGB and
YUV modes. If avisynth reports a field-based clip, `Weave`_ is performed, so
the output is always frame-based. If avisynth guessed wrong, use `AssumeFrameBased`_ beforehand.

MotionMask
~~~~~~~~~~

``MotionMask`` (clip, int mode, int treshold, bool "denoise", bool "fast")

Creates a two color clip that identifies moving parts of the images. The
algorithm comes from `Donald Graft`_'s Smart Deinterlacer filter for
VirtualDub, version 2.5. mode is an integer that can take 3 values: 1 for
frame-only differencing, 2 for field-only differencing and 3 for frame-and-
field differencing. treshol (integer) is the sensivity of the filter. A lower
treshold makes it more sensitive to motion and, as a side-effect, to noise.
denoise (boolean, default=true) activates additional filtering to reduce
false motion detection due to noise and allows you to use lower tresholds.
fast (boolean, default=false) makes the filter runs faster by working two
pixels at a time but is less precise.

For a more thorough explanation of the parameters, please read the help file
included with the `Smart Deinterlacer for VirtualDub`_.

Currently ``MotionMask`` only handles YUY2 data. Also note that the color
mode of the VirtualDub filter is not supported (yet).


MaskedDeinterlace
~~~~~~~~~~~~~~~~~

``MaskedDeinterlace`` (clip, clip mask, bool blend)

``MaskedDeinterlace`` is designed to work together with ``MotionMask``. It
selectively deinterlace the clip based on the mask values. mask is a clip
returned by ``MotionMask``. blend (boolean, default=false) indicates whether
deinterlacing is performed by discarding half of the lines (one field) and
interpolating them (when blend is false), or by blending together adjacent
lines (when blend is true).

mask and clip should have the same width, height and pixel format


SmartDeinterlace
~~~~~~~~~~~~~~~~

``SmartDeinterlace`` (clip, clip mode, int treshold, int "denoise", bool
"blend", bool "fast")

This is just a shortcut for:

MaskedDeinterlace (clip, MotionMask(clip, mode, treshold, denoise, fast),
blend)

It shows that these filters are really only a port of Donald Graft's Smart
Deinterlacer.


MaskedMix
~~~~~~~~~

``MaskedMix`` (clip1, clip2, clip mask)

Returns a clip where each pixel is selected either from clip1 or clip2 based
on the mask value for the pixel. The three clips should have the same width,
height and pixel format. It allows you to use other deinterlacing methods
than the two provided with ``MaskedDeinterlace``.

As an example, the following script can be used to reproduce the "Show motion
areas only" option of Smart Deinterlacer for VirtualDub:

::

    mask = MotionMask(3, 10, true)
    deint = BlendFields
    tmp = Blackness.Levels(0,1,255,127,127)
    MaskedMix(tmp, deint, mask)

$Date: 2004/08/13 21:57:25 $

.. _Dividee's homepage: http://users.win.be/dividee/avisynth.html
.. _VirtualDub: http://www.virtualdub.org/index
.. _Weave: ../corefilters/weave.rst
.. _AssumeFrameBased: ../corefilters/parity.rst
.. _Donald Graft: http://sauron.mordor.net/dgraft/
.. _Smart Deinterlacer for VirtualDub:
    http://sauron.mordor.net/dgraft/smart.html

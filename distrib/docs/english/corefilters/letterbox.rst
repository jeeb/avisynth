
Letterbox
=========

``Letterbox`` (clip, int top, int bottom, [int left], [int right], int "color")

``Letterbox`` simply blackens out the top *top* and the bottom bottom, and
optionally left and right side of each frame. This has a couple of uses: one,
it can eliminate stray video noise from the existing black bands in an image
that's already letterboxed; two, it can eliminate the garbage lines that
often appear at the bottom of the frame in captures from VHS tape.

The functionality of ``Letterbox`` can be duplicated with a combination of
`Crop`_ and `AddBorders`_, but ``Letterbox`` is faster and easier.

Generally, it's better to crop this stuff off using ``Crop`` or
`CropBottom`_ than to hide it with ``Letterbox``. However, in some cases,
particularly if you're compressing to MPEG, it's better to use ``Letterbox``
because it lets you keep a standard frame size like 352x288 or 320x240. Some
MPEG players get confused when the source video has a strange frame size.

The color parameter is optional, default=0 <black>, and is specified as an
RGB value regardless of whether the clip format is RGB or YUV based. See
`here`_ for more information on specifying colors.

Another use could also be to clear out overscan areas in VCD or SVCD
encodings.

$Date: 2008/06/06 11:37:04 $

.. _Crop: crop.rst
.. _CropBottom: crop.rst
.. _AddBorders: addborders.rst
.. _here: ../syntax_colors.rst

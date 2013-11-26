
VideoScope
==========


Abstract
--------

| **author:** VideoScope by Randy French
| **version:** v1.2
| **dowload:** `<http://www.avisynth.org/warpenterprises/>`_
| **category:** Misc Plugins
| **requirements:** YUY2 Colorspace

--------


Syntax
------

``VideoScope`` (clip, string DrawMode, bool TickMarks, string HistoTypeSide,
string HistoTypeBottom, string FrameType)

*DrawMode*: can be "side", "bottom", or "both"

*TickMarks*: true or false

*HistoTypeSide* / *HistoTypeBottom* : "Y", "U", "V", "UV", or "YUV"

*FrameType*: "blank", "colormap", "Y", "U", "V", "UV"
(only seen when drawmode is "both")

``VideoScope`` only works with YUY2 video. To use it with RGB add the command
'ConvertToYUY2' in front of it.


What is does
------------

You can think of ``VideoScope`` as a 'super' histograph. Instead of drawing a
histograph for the entire frame, it draws one for each line (side), or each
column (bottom).

As an example lets say you used the command:

::

    VideoScope("bottom", true, "Y", "Y", "Y")

You will see the graph below the picture. The vertical position on the graph
shows the Y-value (brightness) of the pixels in the picture. The brightness
of the pixels in the graph show how many pixels had that Y-value. If only one
pixel has that value then it will be dim (but still visible). If many pixels
have that value it will be bright white. The graph is 256 pixels tall since
there are 256 possible Y-values.

| You can use the tickmarks to get a pretty exact reading of the Y-values.
| The blue ticks are 0-63 and 128-191.
| The brown ticks are 64-127 and 192-255.
| The widely spaced vertical ticks are 16 apart.
| The closer spaced ticks are 8 apart.
| the triple ticks are 4 apart.

::

    VideoScope("both", false, "Y", "Y", "Y")

In the bottom right corner is the full-frame histo. It is similar to the
histogram in VirtualDub. However it uses linear scaling whereas VirtualDub
seems to use a logarithmic scale.

::

    VideoScope("both", true, "U", "V, "UV")

| The side (purple) graph shows the U-value.
| The bottom (green) graph shows the V-value.
| The interesting full-frame display is the UV map. The background color is the
  same as 'colormap' It shows the full UV range.
| Y is held constant at 128.
| V is vertical (0-255 from bottom to top).
| U is horizontal (0-255 from left to right).
| The crosshair represents the neutral (grey) 128,128 value.
| The UV values of all the pixels are mapped to the graph. If just one pixel
  has a particular UV value, it will show as a black dot. If many pixels have
  that value it will become brighter.

+-----------------------------------------------------------------------------------------------------------+
| History                                                                                                   |
+=========+=================================================================================================+
| v1.2    | compiled for AviSynth v2.5 and removed the ShowDelta and StackVert functions (Wilbert Dijkhof). |
+---------+-------------------------------------------------------------------------------------------------+
| v1.1    | original version (by Randy French)                                                              |
+---------+-------------------------------------------------------------------------------------------------+


$Date: 2004/08/17 20:31:19 $

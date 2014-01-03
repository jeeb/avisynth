
IsCombedTIVTC
=============


Abstract
--------

| **author:** tritical
| **version:** 04/20/2005
| **download:** `<http://bengal.missouri.edu/~kes25c/>`_
| **category:** Deinterlacing & Pulldown Removal
| **requirements:**

-   YV12 & YUY2 Colorspace

**license:** GPL

--------


Description
-----------

IsCombedTIVTC is a utility function that can be used with AviSynth's
conditionalfilter. It uses TFM's inbuilt combed frame detection to test
whether or not a frame is combed and returns true if it is and false if it
isn't.


Syntax
~~~~~~

``IsCombedTIVTC`` (clip, int "cthresh", int "MI", bool "chroma", int
"blockx", int "blocky")


Example
~~~~~~~

::

    conditionalfilter(last,source1,source2,"IsCombedTIVTC","=","true")

Parameters
----------

The parameters are exactly the same as the corresponding parameters in TFM so
I have just copied their descriptions... note that the default values do
differ slightly.

*cthresh* -

This is the area combing threshold used for combed frame detection. It is
like dthresh or dthreshold in telecide() and fielddeinterlace(). This
essentially controls how "strong" or "visible" combing must be to be
detected. Larger values mean combing must be more visible and smaller values
mean combing can be less visible or strong and still be detected. Valid
settings are from -1 (every pixel will be detected as combed) to 255 (no
pixel will be detected as combed). This is basically a pixel difference
value. A good range is between 8 to 12.

Default: 10 (int)

*MI* -

The # of combed pixels inside any of the blocky by blockx size blocks on the
frame for the frame to be detected as combed. While cthresh controls how
"visible" the combing must be, this setting controls "how much" combing there
must be in any localized area (a window defined by the blockx and blocky
settings) on the frame. Min setting = 0, max setting = blocky x blockx (at
which point no frames will ever be detected as combed).

Default: 85 (int)

*chroma* -

Sets whether or not chroma is considered in the combed frame decision. Only
disable this if your source has chroma problems (rainbowing, etc...) that are
causing problems for the combed frame detection with chroma enabled.

| true = chroma is included
| false = chroma is not included

Default: true (bool)

*blockx* -

Sets the x-axis size of the window used during combed frame detection. This
has to do with the size of the area in which MI number of pixels are required
to be detected as combed for a frame to be declared combed. See the MI
parameter description for more info. Possible values are any number that is a
power of 2 starting at 4 and going to 2048 (i.e. 4, 8, 16, 32, ... 2048).

Default: 16 (int)

*blocky* -

Sets the y-axis size of the window used during combed frame detection. This
has to do with the size of the area in which MI number of pixels are required
to be detected as combed for a frame to be declared combed. See the MI
parameter description for more info. Possible values are any number that is a
power of 2 starting at 4 and going to 2048 (i.e. 4, 8, 16, 32, ... 2048).

Default: 16 (int)

$Date: 2005/07/10 16:11:01 $

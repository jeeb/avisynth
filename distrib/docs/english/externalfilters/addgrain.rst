
AddGrain/AddGrainC
==================


Abstract
--------

| **author:** Tom Barry, Foxyshadis
| **version:** 1.4
| **download:** `<http://foxyshadis.slightlydark.com/random/AddGrainC.zip>`_
| **category:** Misc Plugins
| **requirements:**  YV12, YUY2, RGB Colorspace, SSE CPU

--------


Description
-----------

AddGrain generates film like grain or other effects (like rain) by adding
random noise to a video clip. This noise may optionally be horizontally or
vertically correlated to cause streaking.


Usage
~~~~~

1) Place the AddGrain.dll in a directory somewhere. If that is the
    C:\Program Files\AviSynth 2.5\plugins folder folder (recommended) then
    you can omit the LoadPlugin command in your script. In your Avisynth file
    use commands similar to

::

    LoadPlugin("F:\AddGrain\AddGrainC.dll")
    AviSource("D:\wherever\myfile.avi")
    AddGrain(20, 0, 0)

Of course replace the file and directory names with your own.


Parameters
~~~~~~~~~~

``AddGrain`` (float "var", float "hcorr", float "vcorr", float "uvar", int
"seed")

``AddGrainC`` (float "var", float "uvar", float "hcorr", float "vcorr", int
"seed")

where:

*var* (1.0), *uvar* (0) = the standard deviation (strength) of the luma and chroma
noise generated, 0 is disabled.
uvar does nothing in RGB mode.

*hcorr* (0), *vcorr* (0) = the horizontal and vertical correlation, which cause
steaking effect. (0.0 to 1.0)

*seed (-1)* Specifies a repeatable grain sequence. Set to at least 0 to use.

*constant* (false) Specifies a constant grain pattern on every frame.

The correlation factors are actually just implemented as exponential
smoothing which give a weird side affect that I did not attempt to adjust.
But this means that as you increase either corr factor you will have to also
increase the stddev (grain amount) in order to get the same visible amount of
grain, since it is being smooth out a bit.

Increase both corr factors can somewhat give clumps, or larger grain size.

And there is an interesting effect with, say, AddGrain(800,0,.9) or any huge
amount of strongly vertical grain. It can make any scene look like it is
raining.


+-------------------------------------------------------------------------+
| Version History                                                         |
+=====+============+======================================================+
| 1.0 | 2003/06/18 | *Tom Barry* Initial Release                          |
+-----+------------+------------------------------------------------------+
| 1.1 | 2006/06/01 | *Foxyshadis* Chroma grain + constant seed            |
+-----+------------+------------------------------------------------------+
| 1.2 | 2006/06/06 | *Foxyshadis* Supports YUY2, RGB. Fix cache mess.     |
+-----+------------+------------------------------------------------------+
| 1.3 | 2006/06/10 | *Foxyshadis* Crashfix, noisegen optimization         |
+-----+------------+------------------------------------------------------+
| 1.4 | 2006/08/11 | *Foxyshadis* Constant replaces seed, seed repeatable |
+-----+------------+------------------------------------------------------+

$Date: 2006/09/17 20:03:08 $

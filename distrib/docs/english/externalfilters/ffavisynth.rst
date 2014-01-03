
ffavisynth
==========


Abstract
--------

| **author:** Milan Cutka
| **version:**
| **download:** `<http://sourceforge.net/projects/ffdshow-tryout/>`_ (plugin is included in ffdshow package)
| **category:** Misc Plugins
| **requirements:**

-   YV12 & YUY2 & RGB Colorspace

**license:** GPL

--------


Description
-----------

**ffavisynth** - A plugin which lets you directly use ffdshow image processing
filters from AviSynth scripts.

Just install ffdshow (with Avisynth support option checked), and the plugin
ffavisynth.dll will be installed in your avisynth plugin folder.

In ffdshow-20051124.exe and more recent versions, ffavisynth filter uses the
AviSynth C interface, and thus should be loaded using LoadCPlugin (or
Load_StdCall_Plugin). Since 2007-10-30 ffavisynth.dll is loaded automatically
by AviSynth (with ffavisynth.avsi).


Syntax
~~~~~~

| video: ``ffdshow`` (clip, string "preset", string "options")
| audio: ``ffdshowAudio`` (clip, string "preset", string "options")


PARAMETERS
~~~~~~~~~~

| *preset* - existing ffdshow preset to be used
| *options* - array of "name=value" pairs separated by commas (without spaces for
  revisions older 2007-10-30)

| Both parameters are optional. If preset is not specified, a new preset called
  "ffavisynth" is created temporarily. Options override preset settings. List
  of allowed options names and values should be documented once, but for now
  look at registry key
| ``HKEY_CURRENT_USER\Software\GNU\ffdshow\default`` to get
  the list.

**Examples:**

Use the current ffdshow settings:

::

    AviSource("E:\testi.avi")
    ffdshow("default")

Or create a new preset by hitting the new button (under 'Image settings'),
rename it by clicking twice on "default 1" or what the new one is named and
change its settings to your liking, then use

::

    AviSource("E:\testi.avi")
    ffdshow("<name of your new preset>")

Try to append this:

::

    AviSource("E:\testi.avi")
    ffdshow(options="isLevels=1,levelsMode=0,levelsGamma=1500")

to your script. If it would work, the effect should be very visible.

Another example using a created preset:

::

    AviSource("E:\testi.avi")
    # denoise is a created preset:
    ffdshow(preset="denoise")

and you can combine both

::

    AviSource("E:\testi.avi")
    ffdshow(preset="denoise",
    options="isLevels=1,levelsMode=0,levelsGamma=1500")

which will load preset, modify given values and process video.


Limitation
~~~~~~~~~~

Input and output colorspaces are equal. Even if ffdshow image processing
filter chain would produce image with different colorspace, it will be
converted to match that on input.

$Date: 2007/11/21 18:02:03 $

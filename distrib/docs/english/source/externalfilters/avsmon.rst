
AvsMon
======


Abstract
--------

| **author:** Johann Langhofer
| **version:** 25.01.2003
| **download:** `<http://members.nextra.at/johann.langhofer/avisynth/avsmon/latest/>`_
| **category:** Misc Plugins
| **requirements:** YV12 & YUY2 & RGB Colorspace

--------


Description
-----------

This plugin enables you to preview the video during the conversion and to
determine the exact audio delay. The histogram can be enabled if you feed it
with a YUY2 clip,  and wil be attached to the righthand side of the clip. The
preview window can be enabled if you check "Visible" in the Preview Windows
section.

**Example:**

::

    LoadPlugin("C:\Program Files\AviSynth25\avsmon25a.dll")

    AviSource("D:\Test\Quicktime\dido.avi")
    converttoyuy2()
    monitorfilter()

.. image:: pictures/avsmon.jpg


$Date: 2004/08/13 21:57:25 $

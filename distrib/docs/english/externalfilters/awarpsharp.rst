
aWarpSharp
==========


Abstract
--------

| **author:** Marc FD
| **version:** beta 1
| **download:** `<http://www.avisynth.org/warpenterprises/>`_
| **category:** Sharpen/Soften Plugins
| **requirements:**

-   YV12 Colorspace
-   ISSE support

--------


Description
-----------


Syntax
~~~~~~

``aWarpSharp`` (float "depth", int "blurlevel", float "thresh", int "cm")

| *depth* & *blurlevel*:
| The settings you may be familiar with in VDub's WarpSharp filter. They are
  the only settings you need to tweak to achieve any effect. Blurlevel consumes
  cpu power, but gives a big boost to the warpsharpening.
| default : 16.0 & 2

| *thresh*:
| A float value. 1.00 mean 100% (max). It's the bump mapping saturation
  setting. The default value is recommended for maximum quality. if you tweak
  this setting, keep in mind it would enhance inegality of warping between
  edges.
| default : 0.5 (50%)

| *cm*: (chroma mode)
| cm = 0 will disable chroma filtering.
| cm = 1 enables chroma warping with luma bump map (recommended).
| cm = 2 enables chroma independant warping & bump map.
| default : 2

Advanced settings are undocumented. they aren't needed anyway. (i don't use
them, and i think it's better if nobody uses them ^^)


About quality & speed
~~~~~~~~~~~~~~~~~~~~~

``aWarpSharp`` implements high-quality original warpsharping. "Original"
means the algo used differs on several points to other warpsharping filters.
The code is fully iSSE optimised with high accuracy. see yourself ^^. You
should need about 550 Mhz cpu load to warpsharp 640x480 images at 25 fps.
This is a worst case, I achieved about 40 fps PAL full resolution MPEG-2
decoding deinterlacing & warpsharping using
MPEG2Dec("dvd.d2v").aDeInt().aWarpSharp() with a 1.4 Ghz cpu.

``aWarpSharp`` is very optmised for my cpu (Athlon XP), but all modern cpus
would run this filter at the maximal speed allowed by your hardware ^_^.

I hope you'll enjoy my last filter. it's one of the better piece of code I
ever wrote and it was very fun to code.

For all anime fans and all avisynth users who like stuff i coded in the last
months.

$Date: 2004/08/13 21:57:25 $


FFT3DGPU
========


Abstract
--------

| **author:** Tonny Petersen aka tsp
| **version:** 0.6.4
| **download:** `<http://www.avisynth.org/tsp/>`_
| **category:** Misc Plugins
| **requirements:** YV12 Colorspace, Directx 9 graphics card
| **license:** GPL


Introduction
------------

FFT3dGPU is a GPU version of Fizick's `FFT3DFilter`_. The algorithm (Fast
Fourier Transform, denoising) is the same for the most part. Currently the
following is not implemented: support for interlaced video or YUY2 colorspace
or noise pattern.

In this version the next frame is processed while waiting for the GPU to end
it's work. Meaning the filters before fft3dGPU are working concurrently with
it.


Install
-------

To use this filter you need directx 9.0c or better and a graphics card
supporting directx 9 in hardware. That is at least an ATI Radeon 95xx or
Nvidia Geforce fx 5xxx. Geforce 6xxx or better is recommended. If you have
downloaded the installer just run it at you're done, else copy fft3dgpu.hlsl
and copy FFT3dGPU.dll into the same directory, also copy d3dx9_30.dll to the
``c:\windows\system32`` directory.

Older versions also had fft3dgpu9b.dll (not available at the moment) for
Directx 9.0b support (DON'T copy both dll into the autoload directory.)
Directx 9.0c might be faster for people using Nvidia Geforce 6xxx because it
adds support for pixelshader 3.0. If you don't have the latest version of
directx installed (april 2006 or later) you can get it `here`_ or extract
the file d3dx9_30.dll to the ``c:\windows\system32`` directory. The installer
will copy d3dx9_30.dll to the right location meaning that it shouldn't be
neccesary to run the directx installer if you have Directx 9c installed.


Syntax
------

``FFT3DGPU`` (clip, float "sigma", float "beta", int "bw", int "bh", int "bt",
float "sharpen", int "plane", int "mode", int "bordersize", int "precision",
bool "NVPerf", float "degrid", float "scutoff", float "svr", float "smin",
float "smax", float "kratio", int "ow", int "oh", int "wintype"  )


Function parameters
-------------------

*clip*: the clip to filter. The clip must be YV12.

*sigma* and *beta* has the same meaning as in fft3dfilter. Default=1.

*bw,bh*: blockwide and block height. It should be a power of 2 ie valid values
is 4,8,16,32,64,128,256,512 (note that bw should be greater than 4 for best
result). Default=32

*bt*: mode. default 1

- bt=-1 sharpen only
- bt=0 kalman filtering
- bt=1 is 2d filtering
- bt=2 uses the current and previous frame
- bt=3 uses the previous current and next frame
- bt=4 uses the two previous frames, the current and next frame.

*sharpen*: positive values sharpens the image, negative values blurs the image.
0 disables sharpening. Default 0.

*plane*: 0 filters luma, 1,2 and 3 filters Chroma (both U and V). 4 filters
both luma and chroma. Default 0.

*mode*:

- mode=0 only overlaps 1:1. This is faster but produces artifacts with high sigma values.
- mode=1 block overlaps 2:1. This is slower but produces fewer artifacts.
- mode=2 again 1:1 overlap but with a additional border. This reduces border
  artifacts seen with mode=0. The speed is between mode 0 and 1.
  Kalman(bt=0) works well with mode=0. Default 1

*bordersize*: only used with mode 2. Defines the size of the border. Default is
1.

*precision*:

- 0: to use 16 bit floats(half precision),
- 1: to use 32 bit float(single precision) for the fft and 16 bit float for
  the wienner/kalman and sharpening.
- 2: allways use 32 bit floats.
  Using 16 bit float increases the performance but reduces precision. With a
  Geforce 7800GT precision=0 is ~1.5 times faster than than mode 2. Default=0.

| *NVPerf*: Enables support for NVPerfHUD
| (`<http://developer.nvidia.com/object/nvperfhud_home.html>`_). Default false.

*degrid*: Enables degriding. Only works well with mode=1. Doesn't degrid the
Kalman filter (but it does degrid the sharpening (if enabled) after kalman
filter). default 1.0 for mode=1, 0.0 for mode=0 or 2

*scutoff, svr, smin, smax*: Same meaning as fft3dfilter. Controls the
sharpening. default scutoff=0.3, svr=1.0, smin=4.0, smax=20.0

*kratio*: same as fft3dfilter. Control the threshold for reseting the Kalman
filter. Default 2.0

*ow,oh*: this only works with mode=1. This specifies how big the overlap
between the blocks are. Overlap size must be less than or equal to half the
blocksize. Ow must be even. Default: ow=bw/2 ,oh=bh/2

*wintype*: Change the analysis and syntesis window function. Same as
fft3dfilter

FAQ
---


Q: What does it mean when I get a popup box Unexpected error encountered with Error Code: D3DERR_OUTOFVIDEOMEMORY.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

A: It means that fft3dgpu needs more memory than there are availebol on the
graphics card. So either you will have to upgrade or try lowering the
resolution,bt,bh,bw,ow,oh or use usefloat16=true or mode 0 or 2


Q: What setting gives the same result as fft3dfilter?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

A:fft3dGPU(mode=1,precision=2) is similair to fft3dfilter() but please note the different default values for bw,ow,bh,ow

Q: Is there any differences between fft3dfilter and fft3dgpu?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

A: Some of the features from fft3dfilter is still missing.

Q: Why is fft3dGPU so slow compaired to fft3dfilter?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

A: either you have a slow graphics card like a Geforce FX 5200 or you are not using it while doing cpu heavy encoding (like XviD/DivX)

Q: How do I use NVPerfHUD?
~~~~~~~~~~~~~~~~~~~~~~~~~~

A: set NVperf=true and used this commandline or make a shortcut to run it:
``"PATH TO NVPerfHUD\NVPerfHUD.exe"`` ``"PATH TO VIRTUALDUBMOD\virtualdubmod.exe"``
``"PATH TO AVS\test.avs"`` and enabled "force NON PURE device"

Q: I get this errormessage: "Only pixelshader 2.0 or greater supported"
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

A: It is because you need a graphics card that has hardware support for Directx 9.

The following cards will not work
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

::

    Nvidia:
    TNT
    TNT2
    Geforce 256
    GeForce2 Ultra, Ti, Pro,MX,Go and GTS
    Geforce3 Ti 200, Ti 500
    GeForce4 Ti, MX, Go

    Ati:
    Radeon 7xxx
    Radeon 8xxx
    Radeon 90xx
    Radeon 92xx

    Matrox:
    G2xx
    G4xx
    G5xx
    maybe Parhelia

The following should work
~~~~~~~~~~~~~~~~~~~~~~~~~

::

    Nvidia:
    Geforce FX 5xxx
    Geforce 6xxx
    Geforce 7xxx

    Ati:
    Radeon 9500
    Radeon 9550
    Radeon 9600
    Radeon 9700
    Radeon 9800
    Radeon Xxxx

    where x means any digit.

Support
-------

`This thread`_ on the doom9 forum or my email address (tsp (at) person.dk).


TODO
----

| Interlaced, YUY2, different sigma values and (maybe) noise pattern support.
| Fix all the stupid bugs. Add the directx 9.0b version back.


+----------------------------------------------------------------------------------------------------+
| Changelog                                                                                          |
+=========+==========================================================================================+
| v0.1    | first release. Buggy and used Brook                                                      |
+---------+------------------------------------------------------------------------------------------+
| v0.2    | sigma should now work like fft3dfilter                                                   |
+---------+------------------------------------------------------------------------------------------+
| v0.3    || Rewrote the code to use Directx 9.0 directly                                            |
|         || support for 16 bit float increasing performance and stability.                          |
+---------+------------------------------------------------------------------------------------------+
| v0.31   | Fixed bug causing aliased edges.                                                         |
+---------+------------------------------------------------------------------------------------------+
| v0.4    | Added sharpen, mode 1,2, reduceCPU and multithreading                                    |
+---------+------------------------------------------------------------------------------------------+
| v0.41   | Fixed bug when calculating PSD.                                                          |
+---------+------------------------------------------------------------------------------------------+
| v0.42   | Fixed memory leak when reloading                                                         |
+---------+------------------------------------------------------------------------------------------+
| v0.43   || Fixed bug that caused coruptions on the Geforce FX cards and some more memory leaks.    |
|         || Added more comments to the sourcecode and small performance improvement in the shaders. |
|         || Also added support for directx 9.0b                                                     |
+---------+------------------------------------------------------------------------------------------+
| v0.44   || fft3dgpu can now reset a lost device and continue work.                                 |
|         || The direcx 9.0b version should work now.                                                |
+---------+------------------------------------------------------------------------------------------+
| v0.45   | fixed bug when filtering the chromaplane and mode=0 or 2 crashed the filter.             |
+---------+------------------------------------------------------------------------------------------+
| v0.46   || fixed lockups on hyperthread enabled machines(hopefull).                                |
|         || Also fixed infinite loop when closing WMP 6.4.                                          |
+---------+------------------------------------------------------------------------------------------+
| v0.46.1 || fixed issue with nvperf=true causing fft3dgpu to lock up.                               |
|         || Added a FAQ section to this file.                                                       |
+---------+------------------------------------------------------------------------------------------+
| v0.47   || fixed bug with corrupted frames after reseting a lost device.                           |
|         || Renamed the readme.txt to fft3dgpu.txt.                                                 |
|         || Uses a newer version of DirectX 9.0c so please **read the install instructions**!!!     |
+---------+------------------------------------------------------------------------------------------+
| v0.5    || Added Kalman, sharpening, bt=4, degrid from fft3dfilter.                                |
|         || Renamed ps.hlsl to fft3dgpu.hlsl.                                                       |
|         || Rewrote some of the code.                                                               |
|         || Added new bugs.                                                                         |
+---------+------------------------------------------------------------------------------------------+
| v0.5a   | fixed bug with bt=2. Only file changed is fft3dgpu.hlsl                                  |
+---------+------------------------------------------------------------------------------------------+
| v0.51   || Fixed bug with parameters after NVPerf was shifted.                                     |
|         || iedegrid=scutoff,scutoff=svr.                                                           |
|         || Improved download speed from GPU.                                                       |
|         || Geforce fx 5xxx now works with Kalman filter.                                           |
+---------+------------------------------------------------------------------------------------------+
| v0.6    || Added wintypes, plane=4 and variable overlap size (ow,oh).                              |
|         || Change useFloat16 to precision.                                                         |
|         || Changed default value for mode to 1                                                     |
+---------+------------------------------------------------------------------------------------------+
| v0.6.1  || variable overlap now works on the geforce fx 5xxx.                                      |
|         || Default value for mode is 1 now.                                                        |
+---------+------------------------------------------------------------------------------------------+
| v0.6.2  || bugfix: Degrid works better and vertical banding is gone when using mode 1.             |
|         || Right edge artifacts gone when using non mod 8 width and plane>0.                       |
+---------+------------------------------------------------------------------------------------------+
| v0.6.3  || New fft code.                                                                           |
|         || Should improve performance when using larger blocksize and precision=2 (by upto 70%).   |
|         || Fixed bug with HC 0.17 crashing.                                                        |
|         || New html doc(thanks Fizicks for creating this).                                         |
+---------+------------------------------------------------------------------------------------------+
| v0.6.4  | new fft code should now work with ati cards.                                             |
+---------+------------------------------------------------------------------------------------------+

Sourcecode released under GPL see copying.txt

$Date: 2006/06/11 17:25:07 $

.. _FFT3DFilter: fft3dfilter.rst
.. _here: http://www.microsoft.com/downloads/details.aspx?FamilyID=fb73d860-5af1-45e5-bac0-9bc7a5254203&DisplayLang=en
.. _This thread: http://forum.doom9.org/showthread.php?t=89941

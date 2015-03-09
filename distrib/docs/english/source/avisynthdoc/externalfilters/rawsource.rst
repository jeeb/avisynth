
RawSource
=========


Abstract
--------

| **author:** Ernst Pech
| **version:** 20050921
| **dowload:** `<http://www.avisynth.org/warpenterprises/>`_
| **category:** Misc Plugins
| **requirements:**
| **license:** GPL

--------


What it does
------------

This filter loads raw video data.


Usage
-----

``RawSource`` (string filename, int width, int height, string pixel_type, int
"offset")

*filename*: the raw file e.g. a YUV-file.

*width* = 720, *height* = 576: you must specify the image dimensions. width is
max 2880.

*pixel_type* = "YUY2": the type of the raw data. An appropriate mapping to
AviSynth's internal data is done. Supported types: RGB, RGBA, BGR, BGRA,
YUYV, UYUV, YV12, I420.

*offset* = 0: constant header offset.

The framerate is fixed to 25fps, you can change it with `AssumeFPS`_, if you
need (e.g. for NTSC-material).

If a YUV4MPEG2-header is found, width/height/framerate/pixeltype is set
according to the header data. Only fixed-length FRAME headers are supported.
Note that YUV4MPEG2 is raw video with a header (avsyuv outputs it for
example).

**Example:**

::

    LoadPlugin("c:\myprojects\rawsource\release\rawsource.dll")

    RawSource("d:\blue_sky.yuv", 1920, 1080, "YV12")
    bilinearresize(352, 288)

    # raw UYVY file with and offset of 1024:
    RawSource("c:\video\test2.2vuy", 720, 304, "UYVY", offset=4*256)

    # uncompressed mov with an offset of 48:
    RawSource("x:\uncompressed_yuv_no_audio_720x486.mov", 720, 486,"UYVY", offset=48)

Here's a link which contains `avi2yuv`_, which can be used to make raw yuv
files. You need to feed it with uncompressed RGB files, otherwise it doesn't
work.

$Date: 2005/10/01 23:09:51 $

.. _AssumeFPS: ../corefilters/fps.rst
.. _avi2yuv: http://www.ee.surrey.ac.uk/Personal/S.Worrall/dloads/

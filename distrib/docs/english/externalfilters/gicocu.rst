
GiCoCu
======


Abstract
--------

| **author:** E-Male
| **version:** 062005
| **download:** `<http://www.avisynth.org/warpenterprises/>`_
| **category:** Misc Plugins
| **requirements:**

-   RGB Colorspace

**license:** open source

--------


Description
-----------

Reproduces photoshop's handling amp-files and gimp's handling of color curve
files.


Syntax
~~~~~~

``GiCoCu`` (clip, string filename, bool "alpha", bool "photoshop")


Parameters
~~~~~~~~~~

| - optional alpha channel support:
|   if *alpha* = true and colorspace is rgb32 the alpha-channel of the video is
  processed as well
|   default: false

| - cur/amp files support (format of photoshop and bugsbunny's vdub plug-in)
|   if *photoshop* = true the plug-in expects a photoshop amp-file instead of a
  gimp color curve file
|   default: false


Example
~~~~~~~

We created the following GIMP curve file in GIMP.

::

    # GIMP Curves File
    2 27 -1 -1 -1 -1 54 51 -1 56 -1 -1 -1 108 106 115 -1 180 -1 185 157
    189 -1 140 -1 198 -1 208 -1 199 -1 -1 255 255
    0 0 -1 -1 -1 0 -1 0 61 0 84 76 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1
    -1 -1 -1 -1 -1 -1 -1 255 255
    0 0 -1 -1 -1 -1 -1 50 66 0 87 79 -1 180 -1 203 133 134 -1 133 -1 139
    -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 255 255
    -1 0 -1 0 -1 0 -1 0 64 0 -1 -1 -1 70 -1 126 126 128 -1 -1 -1 -1 -1 -1
    -1 -1 -1 -1 -1 -1 -1 -1 255 255
    0 0 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1
    -1 -1 -1 -1 -1 -1 -1 -1 255 255

Save it to a text-file: Layers tab -> Colors -> Curves -> Save -> gimp.cur.
Create the following script:

::

    ImageSource("D:\Captures\obelisk.jpg")
    GiCoCu("D:\Captures\gimp.cur")

.. image:: pictures/obelisk.jpg
.. image:: pictures/obelisk2.jpg

source applying the GIMP curve

$Date: 2005/08/21 20:49:12 $

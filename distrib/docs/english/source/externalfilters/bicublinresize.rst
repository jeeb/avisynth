
BicublinResize
==============


Abstract
--------

| **author:** MarcFD
| **version:** beta 1
| **download:** `<http://ziquash.chez.tiscali.fr/>`_
| **category:** Resizers
| **requirements:**

-   YV12 Colorspace
-   ISSE support

--------


Description
-----------

This is a set of resamplers/resizers for avisynth 2.5 all the resamplers here
are unfiltered ones, this means they're damn fast, but far from perfect,
because they use a small number of sampling points. anyway, for normal tasks,
they may perfectly suits you ^^

BTW, it's important to test it and to let YOUR eyes decide. The reason is
that even if it's not real full filtered interpolation, it looks very decent
anyway, really.


Syntax
~~~~~~

| ``FastBilinearResize`` (h, v)
| ``FastBicubicResize`` (h, v, "b", "c")
| ``BicublinResize`` (h, v, "b", "c')

| ``FastBilinear`` is like trbarry's simpleresize.
| ``FastBicubic`` is a unfiltered Bicubic resampler.
| ``Bicublin`` use bicubic on Y plane and bilinear on UV planes.

| *h*, *v* = horizontal and vertical resolutions.
| *b*, *c* = bicubic coefficients. by default : b = 1/3, c = 1/3

$Date: 2004/08/13 21:57:25 $

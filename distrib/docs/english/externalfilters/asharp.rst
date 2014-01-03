
Asharp
======


Abstract
--------

| **author:** MarcFD
| **version:** 0.95
| **download:** `<http://ziquash.chez.tiscali.fr/>`_
| **category:** Sharpen/Soften Plugins
| **requirements:** YV12 Colorspace

--------


Description
-----------

Basically, this filter is a very common unsharp mask, simply because unsharp
mask is the most pleasant sharpenning technique for my eyes ^^
but i've added 3 simple ideas i had about sharpening:

- adaptive thresholding to avoid noise enhancement
- block adaptive sharpenning to avoid DCT block edges enhancement
- block based adaptive thresholding to avoid DCT block edges enhancement

it's optimised for quality, not speed. but... i love speed ^^
so i've added very optimised MMX/ISSE code.

it's working in YV12 colorspace, and on luma only
(chroma sharpenning seems to only introduce artifacts)

Usage of asharp
~~~~~~~~~~~~~~~

To do simple unsharp masking with a strength of 2x :

::

    Asharp(2,0)

To try some adaptive sharpenning :

::

    Asharp(2, 4)

Suggestion for divx anime decoding :

::

    Asharp(2. 5, 4.5, 0.25, hqbf=true)

Syntax
~~~~~~

``asharp`` (float "T", float "D", float "B", bool "hqbf")

**T** : unsharp masking threshold. 0 will do nothing. (value clamped to
[nothing=0..32])

T = 1 is like 32 with ffdsow/Vdub unsharp mask filter, it'll enhance contrast
1x. Default is T = 2.

**D** : adaptive sharpenning strength. (value clamped to [disabled=0..16])

set to 0 to disable.
If D > 0, adaptive thresholding is enabled.
The threshold is adapted for each pixel (bigger for edges).
If adaptive sharpenning is enabled,  T acts like a maximum.
Default is D = 4.

**B**  : block adaptive sharpenning. (value clamped to [disabled=0..4])

Set to a negative value to disable.
If B >= 0, block adaptive sharpenning is enabled.
It acts very simply, by lowering the threshold around DCT-blocks edges.
If you use it, avoid any non mod8 cropping between the decoder and asharp.
(it works only wit adaptive sharpenning, when D > 0)
Default is B = -1 (disabled)

**hqbf** : high quality block filtering. (aka block based adaptive
thresholding.)

It was painfull to implement in SIMD, but i love it.
Try with and without on a blocky video, and you'll understand why ^^
It works only with adaptive thresholding.
Default is false (disabled)


$Date: 2004/08/13 21:57:25 $

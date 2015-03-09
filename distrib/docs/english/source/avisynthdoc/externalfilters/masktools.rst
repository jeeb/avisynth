
MaskTools
=========


Abstract
::::::::

| **author:** kurosu and Manao
| **version:** 1.5.8
| **download:** `<http://manao4.free.fr/>`_
| **category:** Misc Plugins
| **requirements:** YV12 Colorspace
| **license:** GPL

--------


.. toctree::
    :maxdepth: 3


About MaskTools
---------------


Simple version
~~~~~~~~~~~~~~

After a processing, you may need to keep only a part of the output. Say, you
have a clip named smooth that is the result of a smoothing (blur() for
instance) on a clip named source.
Most of the noise from source have disappeared in smooth, but so have
details. You could therefore want to only keep filtered pixels and discard
those where there are big difference of color or brightness. That's what does
MSmooth by D. Graft for instance. Now consider that you write on an image
pixels from smooth that you want to keep as white pixels, and the other ones
from source as black pixels. You get what is called a mask. MaskTools deals
with the creation, the enhancement and the manipulating of such mask for each
component of the YV12 colorspace.


Description
~~~~~~~~~~~

This Avisynth 2.5 YV12-only plugin offers several functions manipulating
clips as masks:

-   `Binarize`_ will binarize the input picture depending on a threshold
    and a command.
-   `CombMask`_ outputs a mask which gives aeres that presents combing.
-   `DEdgeMask / DEdgeMask2`_ will build a mask of the edges of a clip,
    applying thresholdings (proper values will enable or disable them).
-   `EdgeMask`_ will build a mask of the edges of a clip, applying
    thresholdings (proper values will enable or disable them). Similar as
    DEdgeMask with prefined kernels.
-   `Expand <#inpand-expand-deflate-inflate>`_ will 'expand' the high values in a plane, by putting in the
    output the maximum value in the 3x3 neighbourhood around the input pixel.
    The opposite function is called `Inpand <#inpand-expand-deflate-inflate>`_.
-   `FitY2UV / FitY2U / FitY2V <#fity2u-fity2v-fity2uv-fitu2y-fitv2y-fitu2v-fitv2u>`_ resizes Y plane and replace UV/U/V plane(s)
    by the result of the resize (you can specify your resizer filter, even
    one that isn't built-in AviSynth); the opposite functions are FitU2Y and
    FitV2Y.
-   `Inflate <#inpand-expand-deflate-inflate>`_ will 'inflate' the high values in a plane, by putting in
    the output plane either the average of the 8 neighbours if it's higher
    than the original value, otherwise the original value. The opposite
    function is called `Deflate <#inpand-expand-deflate-inflate>`_ (dedicated to Phil Katz).
-   `Invert`_ will invert the pixel (i.e. out = 255 - in); this can be
    also used to apply a 'solarize' effect to the picture.
-   `Logic`_ will perform most typical logical operations (in fact, the
    ones provided by MMX mnemonics, though C functions are still available,
    mainly because of the picture dimensions limits).
-   `RGBLUT / YV12LUT / YV12LUTxy <#rgblut-yv12lut-yv12lutxy-yuy2lut>`_ are look-up tables, allowing to apply
    fastly a function to each pixels of the picture.
-   `MaskedMerge`_ will take 3 clips and apply a weighed merge between
    first and second clips depending on the mask represented by clip3.
-   :ref:`MotionMask` will create a mask of the motion on the picture.
-   OverlayMask will compare 2 clips based on luminance and chrominance
    thresholds, and output whether pixels are close or not (close to what
    ColorKeyMask does).
-   `YV12Convolution`_ will allow you to convole the picture by the
    matrix of your choice.
-   YV12Layer is the equivalent to OverLay.
-   `YV12Subtract`_ is the same as Subtract, also works in YV12, but
    *should* be a bit faster (because MMX optimised).

In addition, all functions take 3 parameters: Y, U and V (except the FitPlane
functions, where obviously the name tells what is processed). Depending on
their value, different operations are applied to each plane:

-   value = 3 will do the actual process of the filter,
-   value = 2 will copy the 2nd video plane (if appliable) to the output
    corresponding plane
-   value = 1 will not process it (i.e., most often, left it with 1st
    clip plane or garbage - check by yourself)
-   value = [-255...0] will fill the output plane with -value (i.e. to
    have grey levels, use U=128,V=128)

A last point is the ability of some functions to process only a part of the
frame:

-   this behaviour is set by the parameters (offX, offY) (position of the
    start point) and (w,h) (width and height of the processed area); filters
    should modify those parameters so that the processed area is inside the 2
    pictures
-   in case of a filter (except YV12Layer) using 2 clips, the 2 clips
    must have the same dimensions
-   in all cases, the picture must be at least MOD8 (MOD16 sometimes) in
    order to enable the filter to use MMX functions (ie work at full speed)

This was intended for modularity and atomic operations (or as useful as
possible), not really speed. It became both bloated and slow. I let you
decide whether this statement is totally true, or a bit less... The examples
in III) are most probably much faster applied with the original filters.



Function descriptions
---------------------


Binarize
~~~~~~~~

``Binarize`` (clip, int "threshold", bool "upper")

The ``Binarize`` filter allows a basic thresholding of a picture. If
upper=true, a pixel whose value is strictly superior to threshold will be set
to zero, else to 255. On the contrary, if upper=false, a pixel whose value is
strictly superior to  threshold will be set to 255, else to zero.

Defaults are threshold = 20 and upper = true.


CombMask
~~~~~~~~

``CombMask`` (clip, int "thY1", int "thY2")

This filter produces a mask showing areas that are combed. The thresholds
work as for the other filters : after calculating the combing value, if this
one is under thY1, the pixel is set to 0, over thY2, it is set to 255, and
inbetween, it is set to the combing value divided by 256.

The combing value is (upper_pixel - pixel)*(lower_pixel - pixel). Thus, it is
not normalized to the range 0..255, because if it was done, value would be
close to 1 or 2, no more. That means you can use threshold higher than 255,
even if they should not be useful.

Defaults are thY1 = 10 and thY2 = 10 ( thus making a binary mask ).


DEdgeMask / DEdgeMask2
~~~~~~~~~~~~~~~~~~~~~~

| ``DEdgeMask`` (clip, int "thY1", int "thY2", int "thC1", int "thC2", string
  "matrix", float "divisor", bool "setdivisor", bool "vmode")
| ``DEdgeMask2`` (clip source, clip low_thres, clip high_thres, string
  "matrix", float "divisor", bool "setdivisor", bool "vmode")

This filter creates an edge mask of the picture. The edge-finding algorithm
uses a convolution kernel, and the result of the convolution is then
thresholded with  thY1 and  thY2 ( luma ) and  thC1 and  thC2 ( chroma ). The
thresholding happens like that ( r is the result of the convolution ) :

-   r <= th1 gives 0.
-   th1 < r <= th2 gives r.
-   th2 < r gives 255.

In order to create a binary mask, you just have to set th1=th2.

The choice of the convolution kernel is done with matrix. The matrix must be
a 3 by 3 matrix, whose coefficients are integers, separated by a single
space. Hence, the strings "-1 -1 -1 -1 8 -1 -1 -1 -1" and "0 -1 0 -1 0 1 0 1
0" will respectively give the kernels "laplace" and "sobel" of the filter
`EdgeMask`_.

As coefficients must be integers, *divisor* is used to refine the result of
the convolution. This result will simply be divided by divisor. If divisor
isn't defined, it is defaulted to the sum of the positive coefficient of the
matrix, thus allowing a classic normalization. It can be either a float or an
integer, the later being the faster.

*setdivisor* is present only for backward compatibility. Do not use it.

Finally *vmode* allows to output a mask centered to 128 instead of zero.

Defaults are : thY1 = 0, thY2 = 20, thC1 = 0, thC2 = 20, matrix = "-1 -1 -1
-1 8 -1 -1 -1 -1" and vmode=false.

DEdgemask2 basically works like DEdgeMask, except that instead of 2 low /
high thresholds, it takes 2 other clips. Each clips contains local thresholds
for each pixels. Let's say you want adaptive thresholds taking local contrast
into account. Well, local min & max can be obtained through inpand() and
expand(). Difference can be made with YV12LUTxy or YV12Subtract. And voilà,
you've got a threshold clip containing local contrasts.


EdgeMask
~~~~~~~~

``EdgeMask`` (clip, int "thY1", int "thY2", int "thC1", int "thC2", string
"type")

This filter creates an edge mask of the picture. The edge-finding algorithm
uses a convolution kernel, and the result of the convolution is then
thresholded with  thY1 and  thY2 ( luma ) and  thC1 and  thC2 ( chroma ). The
thresholding happens like that ( r is the result of the convolution ) :

-   r <= th1 gives 0.
-   th1 < r <= th2 gives r.
-   th2 < r gives 255.

In order to create a binary mask, you just have to set th1=th2.

The choice of the convolution kernel is done by  type :

-   type = "roberts" :

::

     2 -1
    -1  0

-   type = "sobel" :

::

     0 -1 0
    -1  0 1
     0  1 0

-   type = "laplace" :

::

    -1/8 -1/8 -1/8
    -1/8  1   -1/8
    -1/8 -1/8 -1/8

-   type = "special" :

::

    -1/4 0 -1/4
     0   1  0
    -1/4 0 -1/4

-   type = "roberts" :

::

     2 -1
    -1  0

Finally, there are also two other possible values for  type ( "cartoon" and
"line" ), which have behaviors which are not documented here.

Defaults are : thY1 = 0, thY2 = 20, thC1 = 0, thC2 = 20 and type = "sobel".


FitY2U / FitY2V / FitY2UV / FitU2Y / FitV2Y / FitU2V / FitV2U
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

``FitPlane`` (clip, string resizer)

``FitPlane`` has the following incarnations:

- luma to chroma: ``FitY2U``, ``FitY2V``, ``FitY2UV``
- chroma to luma: ``FitU2Y``, ``FitV2Y``
- chroma to chroma: ``FitU2V``, ``FitV2U``

You can by this mean propagate a mask created on a particular plane to
another plane.


Inpand / Expand / Deflate / Inflate
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

| ``Inpand`` (clip)
| ``Expand`` (clip)
| ``Deflate`` (clip)
| ``Inflate`` (clip)

This filters allow to enlarge / reduce a mask. ``Expand`` will replace the
value of a pixel by the highest surrounding value. ``Inpand`` will on the
contrary replace it by the lowest surrounding value. ``Inflate`` will compute
the mean of the surrounding pixels, and will replace the value of the pixel
by it only if this mean is superior to the original value of the pixel.
``Deflate`` will do the same only if the mean is inferior to the original
value.

The picture returned by ``Expand`` / ``Inflate`` will always be higher than
the original picture. On the contrary, the one returned by ``Inpand`` /
``Deflate`` will always be lower.

The enlarging / reducing produced by ``Deflate`` / ``Inflate`` is softer than
the one of ``Expand`` / ``Inpand``.


HysteresyMask
~~~~~~~~~~~~~

``HysteresyMask`` (mask_clip1, mask_clip2)

This filter creates a mask from two masks. Theorically, the first mask should
be inside the second one, but it can work if it isn't true ( though results
will be less interesting ). The principle of the filter is to enlarge the
parts that belongs to both masks, inside the second mask.

This algorithm is interesting because it allows for example to obtain an edge
mask with all the interesting edges, but without the noise. You build two
edge masks, one with a lot of edges and noise, the other one with a few edges
and almost no noise. Then, you use this filter, and you should obtain the
edges, without the noise, because the noise wasn't there in the second mask.


Invert
~~~~~~

``Invert`` (clip, int offX, int offX, int w, int h)

This filter seplaces the pixel's value by 255-pixel's value.

Binarize(upper=false) could be seen (but isn't processed as) as

``Invert().Binarize(upper=true)``


Logic
~~~~~

``Logic`` (mask_clip1, mask_clip2, string "mode")

This filter produces a new mask which is the result of a binary operation
between two masks. The operation is chosen with the parameter mode.

-   mode="and" : works only with binary masks ( only pixels at 0 or 255
    ). The output mask is the intersection of the two masks. It means that if
    both corresponding pixels are 255, the resulting pixel will be 255, else
    0.
-   mode="or" : works only with binary masks. The output mask is the
    union of the two masks. It means that if one of the corresponding pixels
    are 255, the resulting pixel will be 255, else 0.
-   mode="xor" : works only with binary masks. The output mask is the
    difference between the two masks. It means that if one ( exclusively ) of
    the corresponding pixels are 255, the resulting pixel will be 255, else
    0.
-   mode="andn" : works only with binary masks. The output mask is the
    subtraction of the second mask from the first one. It means that if the
    pixel of the first mask is 255 and the second is 0, it will return 255,
    else 0.
-   mode="min" : returns for each pixel the minimum value between the two
    pixels of the input masks. It amounts to mode="and", but for non binary
    masks.
-   mode="max" : returns for each pixel the maximum value between the two
    pixels of the input masks. It amounts to mode="or", but for non binary
    masks.

If a logical operator is used with a non binary mask, the results are
unpredictable.

Default : mode = "and".


RGBLUT / YV12LUT / YV12LUTxy / YUY2LUT
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

| ``YV12LUT`` (clip, string "yexpr", string "uexpr", string "vexpr")
| ``YUY2LUT`` (clip, string "yexpr", string "uexpr", string "vexpr")
| ``RGBLUT`` (clip, string "Rexpr", string "Gexpr", string "Bexpr", string
  "AMPFile")
| ``YV12LUTxy`` (clipx, clipy, string "yexpr", string "uexpr", string "vexpr")

These filters apply a function to each pixel of the picture. In order to
allow a fast computation, every possible value of the function are
precomputed and stored in a Look-Up Table ( hence the name ). That makes the
filters fairly fast. ``RGBLUT`` works exactly the same way as ``YV12LUT``,
except that it has an additional argument AMPFile. It allows you to load a
photoshop color profile.

In order to be able to apply almost every possible function, this one is
given by a string which represents an expression in reverse polish notation.
The principle of this notation is to write firstly the operands / parameters
of an operator / function, and then the operator / function itself. Hence, ``"3
+ 7"`` becomes ``"3 7 +"``, and ``"sin(3)"`` becomes ``"3 sin"``. Going further in the
explanations, ``"3 * 7 + 5"`` becomes ``"3 7 * 5 +"``, and ``"(3 + 7) * 5"`` : ``"3 7 + 5
*"``. Now, you understand the main asset of this notation : no need of
parenthesis.

Computations are lead on real numbers. Positive numbers also represent a true
statement, whereas negative numbers represent a false statement. In the
string, the symbol "x" is tha value of the pixel before the use of the
function. For ``YV12LUTxy`` you also have the symbol "y", which represents
the value of the collocated pixel in the second clip. The symbols must be
separated by a single space.

Some operators and functions are implemented :

-   ``+``, ``-``, ``/``, ``*``, ``^``, ``%`` are the operators plus, minus, divide, multiply,
    power and modulo.
-   ``&``, ``|``, ``°``, ``!&`` are the logical operators and, or, xor, and not. If the
    result is true, they return 1.0, else -1.0.
-   ``<``, ``<=``, ``>``, ``>=``, ``=``, ``!=`` are the relationnal operators less than, less or
    equal to, more than, more or equal to, equal to, not equal to. If the
    result is true, they return 1.0, else -1.0.
-   ``cos``, ``sin``, ``tan``, ``acos``, ``asin``, ``atan``, ``exp``, ``log``, ``abs`` are the functions
    cosine, sine, tangent, arccosine, arcsine, arctangent, exponential,
    napierian logarithm, absolute value.
-   ``?`` allows to do a condition test. It's a ternary operator, the first
    operand being the condition, the second the value if the condition is
    true, the third if false.

Some examples :

* Binarization of the picture with a threshold at 128 :

::

    "x 128 < 0 255 ?"

    It is translated as :

    "(x < 128) ? 0 : 255"

* Levels(il, gamma, ih, ol, oh) ( have a look at the filter `Levels`_ ) :

::

    "x il - ih il - / 1 gamma / ^ oh ol - *"

    It is translated as

    "(((x - il) / (ih - il)) ^ (1 / gamma)) * (oh - ol)"

Defaults are : Yexpr = Uexpr = Vexpr = "x" ( hence, the filter does nothing
).


MaskedMerge
~~~~~~~~~~~

``MaskedMerge`` (base_clip, overlay_clip, mask_clip)

This filter applies the clip overlay_clip on the clip base_clip, considering
the clip mask_clip. More precisely, with *bc*, *oc* and *mc* the values of
three pixels taken respectively on base_clip, overlay_clip and mask_clip, the
result will be :

::

    v = ((256 - mc) * bc + mc * oc + 128) / 256

128 is here in order to reduce the error due to the rounding of the integer division.

So, if the mask is 255, the pixel will be the pixel from the overlay_clip, if
the mask is 0, the pixel will be from the base_clip, and in between, it will
be blended between both clips.


.. _MotionMask:

MotionMask
~~~~~~~~~~

``MotionMask`` (clip, int "thY1", int "thY2', int "thC1", int "thC2", int
"thSD")

This filter creates a mask of the motion of the picture. As with the other
filters which create masks, once the motion is computed, it is thresholded by
two thresholds. This filter will also check for scene changes, and won't
output a mask if one is detected.

Scene change detection is made by computing the sum of absolute differences
of the picture and the previous one. This sum is averaged, and then compared
to thSD. If it is more than thSD, a scene change is detected.

Motion is computed the same way as `NoMoSmooth`_, meaning that for each
pixel, we'll compute the absolute sum of differences between the pixel and
its surrounding, and the pixel and its surrounding in the previous picture.
The resulting value is then divided by 9, in order to normalize the result
between 0 and 255.

This algorithm only gives an approximation of the motion. It will work well
on the edges of an object, but not on its inside.

Defaults are : thY1= 20, thY2 = 20, thC1 = 10, thC2 = 10 and thSD = 10.


YV12Convolution
~~~~~~~~~~~~~~~

``YV12Convolution`` (clip, string "horizontal", string "vertical", int
"total", bool "automatic", bool "saturate")

This filters computes the convolution product between the picture and the
kernel defined by the multiplication of horizontal by vertical. These two
strings represent vectors. They must have an odd number of integer or real
numbers, separated by single spaces. total is a normalization factor, by
which the result of the product is divided. If  automatic is set to 'true',
total is the sum of the coefficients of the matrix. It means that, that way,
overall brightness of the picture isn't touched. Saturate allows to choose
the behavior of the filter when the result is a negative number.

-   saturate = true : negative values are zeroed.
-   saturate = false : negative values are inverted.

If total is not defined, it is set to the sum of the coefficients of the
convolution kernel, thus allowing a good normalization for bluring /
sharpening kernels.

If one of the coefficients of horizontal or vertical is a real number, all
the computations will be made with floats, so the filter will be slower.

Defaults are : horizontal = "1 1 1", vertical = "1 1 1" and automatic =
false, saturate = true.


YV12Subtract
~~~~~~~~~~~~

``YV12Subtract`` (clip1, clip2, int tol, bool "widerange")

This filter computes the difference between the two clips. There are several
ways of computing this difference, depending on the values of widerange and
of tol.

-   widerange = true : we compute the difference ( n ) between the two
    clip, and we return ``r = 128 + 128 * pow(n / 255,1 / tol)``. If tol < 0, 1 /
    tol becomes 0.5.
-   widerange = false :

    -   tol < 0 : we compute the absolute difference ( n ) between the
        two clip, and we return ``r = n / 2 + 128``.
    -   tol >= 0 : we compute the absolute difference ( n ) between the
        two clip, and we return 0 if n is lower than tol, n - tol else.

Defaults are : tol = -1 and widerange = false.


Some practical uses (not tested extensively)
--------------------------------------------

Those won't produce the exact same results as the original filters they try
to mimic, in addition to be far more slower. Despite the numerous additional
functions, no newer idea.

Notes:

- I'm too lazy to update the syntax, especially regarding how mode=2 works,
  and how EdgeMask was updated (now longer needs of a Binarize for instance)
- Some filters I describe as 'to create' already exist (imagereader, levels
  for clamping, ...).


MSharpen
~~~~~~~~

::

    # Build EdgeMask of clip1, Binarize it and store the result into clip3
    # Apply any sharpening filter to clip1 and store it into clip2
    ...
    return MaskMerge(clip1, clip2, clip3)

The sharpened edges of clip2 higher than the threshold given to Binarize will
be sharpened and used to replace their original value in clip1. You could
also write a filter with a particular Look-up table (best would look like a
bell), replace Binarize by it, and have a weighed sharpening depending on the
edge value: that's the HiQ part in SmartSmoothHiQ

::

    clip2 = clip1.<EdgeEnhancer>(<parameters>)
    #U and V planes don't need filtering, Y needs it
    #EdgeMask(<...>, "roberts", Y=3, U=-128, V=-128) for greyscale map
    clip3 = clip1.EdgeMask(15, 60, "roberts", Y=3, U=1, V=1)
    return MaskedMerge(clip1, clip2, clip3)

MSoften
~~~~~~~

Replace EdgeEnhancer by a spatial softener (cascaded blurs?
spatialsoftenMMX?) and use upper=true to select near-flat pixels.


Rainbow reduction (as described in `this thread`_ )
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Warning, this isn't a miracle solution either

::

    clip2 = clip1 soften at maximum (using deen("m2d") or edeen for instance)
    #Get luma edgemap and increase edges by inflating
    # -> wider areas to be processed
    clip3 = clip1.EdgeMask(6, "roberts", Y=3, U=1, V=1).Inflate(Y=3, U=1, V=1)
    #Now, use the luma edgemask as a chroma mask
    clip3 = YtoUV(clip3, clip3).ReduceBy2().Binarize(15, upper=false, Y=1, U=3, V=3)
    #We have to process pixels' chroma near edges, but keep intact Y plane
    return MaskedMerge(clip1, clip2, clip3, Y=1, U=3, V=3)

Supersampled fxtoon
~~~~~~~~~~~~~~~~~~~

Not tested

- Use tweak to darken picture or make a plugin that scales down Y values -> clip2
- Build edge mask, Supersample this mask, Binarize it with a high
  threshold (clamping sounds better), Inflate it -> clip3
- Apply the darker pixels of clip2 depending on the values of clip3

Warpsharp for dark luma
~~~~~~~~~~~~~~~~~~~~~~~

Not tested

- Apply warpsharp -> clip2 (replacement pixels)
- Create a clamping filter or a low-luma bypass filter -> clip3 (mask)

pseudo-deinterlacer (chroma will still be problematic)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Not tested

::

    clip2 = clip1.SeparateFields().SelectEven().<Method>Resize(<parameters>)
    clip3 = clip1.<CombingDetector>(<parameters>)
    return MaskedMerge(clip1, clip2, clip3, Y=3, U=3, V=3)

(chroma even more problematic)


Non-rectangular overlays
~~~~~~~~~~~~~~~~~~~~~~~~

In fact, this is handled more nicely by layer and mask...

::

    #Simple hack because ImageReader needs an integer fps...
    #Most sources are natively in YUY2/YV12
    clip = AviSsource("test.avi").ConvertToYV12().assumefps(fps)
    #Load the picture to be overlayed
    image = ImageReader("mask.bmp", 0, clip.framecount()-1, 24, use_DevIl=false)
    #Simple way: assume black is transparent
    #Any other colour would be quite more complicated*
    masktemp = imageYV12.Binarize(17, upper=false, Y=3)
    #We set the luma mask to fit the chroma planes
    mask = Mask.FitY2UV()
    #Now that we have the mask that tells us what we want to keep...
    #Replace by image the parts of clip masked by mask!
    MaskedMerge(clip, image, mask, Y=3, U=3, V=3)
    #*solution: mask = OverlayMask(image, image.BlankClip("$xxxxxx"), 1, 1)

Replace backgrounds
~~~~~~~~~~~~~~~~~~~

This example clearly would look better in RGB. To avoid typical problems due
to noise or compression, you would better use blurred versions of the clip
and picture.

::

    source = AviSource("overlay.avi").AssumeFPS(24)
    #blur the source
    clip = source.Blur(1.58).Blur(1.58).Blur(1.58)
    #load the background to replace, captured from the blurred sequence
    bgnd = ImageReader("bgnd.ebmp", 0, clip.framecount()-1, 24,
    use_DevIl=false)
    #load new background
    new = ImageReader("new.ebmp", 0, clip.framecount()-1, 24,
    use_DevIl=false)
    #integrated filter to output the mask = (clip~overlay?)
    mask = OverlayMask(clip, overlay.ConvertToYV12(), 10, 10)
    MaskedMerge(source, new.ConvertToYV12(), mask, Y=3, U=3, V=3)

K-mfToon
~~~~~~~~

I need to include more info (original urls/posts) but for now I think
mfToon's original author, mf (mf@onthanet.net) will not react too violently
to it, while it's still not addressed.
The output of the function inside K-mfToon.avs should be identical to the
output of the original mftoon.avs (also included), with twice the speed.

The requirements are:

- For mfToon:
- load the plugins called "MaskTools", "warsharp", "awarsharp"


TODO
----

Nothing, it all depends in feeback



Disclaimer
----------

This plugin is released under the GPL license. You must agree to the terms of
'Copying.txt' before using the plugin or its source code.

You are also advised to use it in a philanthropic state-of-mind, i.e. not
"I'll keep this secret for myself".

Last but not least, a very little part of all possible uses of each filter
was tested (maybe 5% - still a couple of hours spent to debug ;-). Therefore,
feedback is *very* welcome (the opposite - lack of feedback - is also
true...)


Revisions
---------

+----------------------------------------------------------------------------------------------------------------+
| Changelog                                                                                                      |
+===========+====================================================================================================+
| v1.5.8    | - 8th August 2005                                                                                  |
|           | - Added DEdgeMask2.                                                                                |
+-----------+----------------------------------------------------------------------------------------------------+
| v1.5.7    | - Added YUY2LUT.                                                                                   |
+-----------+----------------------------------------------------------------------------------------------------+
| v1.5.6    | - Corrected chroma handling of YV12LUTxy.                                                          |
+-----------+----------------------------------------------------------------------------------------------------+
| v1.5.5    | - 6 November 2004                                                                                  |
|           | - Version with binarize working;                                                                   |
|           | - Pentium4 with HT enabled.                                                                        |
+-----------+----------------------------------------------------------------------------------------------------+
| v1.5.4    | - 14 October 2004                                                                                  |
|           | - A lot of filters which were working in place aren't anymore ( because                            |
|           |   of an AviSynth strangeness which was causing slowdowns ). Before, a                              |
|           |   filter such as YV12LUT, with U = V = 1, would have had it's chroma left                          |
|           |   untouched. Now, if you want to preserve the chroma, you have to specify U                        |
|           |   = V = 2, as it is said in the documentation.                                                     |
+-----------+----------------------------------------------------------------------------------------------------+
| v1.5.2 -  | - ?                                                                                                |
| v1.5.3    |                                                                                                    |
+-----------+----------------------------------------------------------------------------------------------------+
| v1.5.1    | - Complete rewritting of the documentation ( a huge thanks to Wilbert).                            |
|           | - DEdgeMask now supports a float divisor. However, if the divisor is                               |
|           |   integer, it *should* be faster.                                                                  |
|           | - DEdgeMask now has back its parameter "setdivisor", but it's only for                             |
|           |   backward compatibility with some scripts.                                                        |
|           | - YV12LUTxy : it's a new filter, implementing the idea presented by                                |
|           |   Didee on the previous post. The "y" symbol has the value of the                                  |
|           |   collocated pixel in the second clip.                                                             |
+-----------+----------------------------------------------------------------------------------------------------+
| v1.4.16   | - Bugfixes : Logic "min" & "max" modes weren't properly working, it's corrected.                   |
|           | - Bugfixes : Logic & Subtract weren't using MMX & iSSE optimizations,                              |
|           |   due to a very silly bug. It's Corrected.                                                         |
+-----------+----------------------------------------------------------------------------------------------------+
| v1.4.15.3 | - RGBLUT added : works the same as YV12LUT ( except R, G and B replace Y, U and V ).               |
|           | - In addition, you can specify an AMP file ( arbitrary color mapping file format from photoshop ). |
+-----------+----------------------------------------------------------------------------------------------------+
| v1.4.15.2 | - Bug finally solved on YV12LUT. Silly programming bug, as always...                               |
|           | - In YV12LUT, logical and relationnal operators added ( ``<``, ``<=``, ``>``, ``>=``,              |
|           |   ``==``, ``!=``, ``&``, ``!&``, ``|``, ``°`` ( xor ) )                                            |
|           | - In YV12LUT, a ternary operator added : ? ( works as in C )                                       |
+-----------+----------------------------------------------------------------------------------------------------+
| v1.4.15.1 | - In YV12LUT, another bug which was still preventing it from working                               |
|           |   fine. Hopefully, it should really work now.                                                      |
+-----------+----------------------------------------------------------------------------------------------------+
| v1.4.15   | - New Filter : HysteresyMask. It will allow you to build a new edge                                |
|           |   mask from two edge masks, one only having a few edges ( but we're sure                           |
|           |   they indeed are edges ), the other having two much edges ( due to a too                          |
|           |   low thresholding for example ). Look in the documentation to have further                        |
|           |   explanations.                                                                                    |
+-----------+----------------------------------------------------------------------------------------------------+
| v1.4.14.2 | - Several bugfixes concerning the behavior of negative values for Y, U                             |
|           |   and V ( edgemask, dedgemask, motionmask, combmask, logic )                                       |
|           | - Several bugfixes concerning the use of offX / offY / w and h (                                   |
|           |   filters than can use it are : maskedmerge, binarize, expand,                                     |
|           |   YV12subtract, yv12lut )                                                                          |
|           | - In YV12LUT, a bug prevented to use it with some filter. It should work now.                      |
|           | - In YV12Convolution, float coefficients can be used now. If none is                               |
|           |   used, all the processing will take place with integer, so it will be                             |
|           |   faster than if you use a float. Moreover, if                                                     |
|           |   there is the possibility of overflow ( giving a result over 255 or under 0 )                     |
|           |   during computation, a slower but safe function will be used to saturate                          |
|           |   computation to 0 and 255.                                                                        |
+-----------+----------------------------------------------------------------------------------------------------+
| v1.4.14.1 | - Bugfix in YV12LUT to allow the use of negative numbers                                           |
+-----------+----------------------------------------------------------------------------------------------------+
| v1.4.14   | - Bugfix : In YV12Layer, a useless test could prevent the filter to                                |
|           |   work. The test has been removed                                                                  |
|           | - Bugfix : In DEdgeMask, threshold weren't taken into account. They are now                        |
|           | - Bugfix : Logic filter is now fully functionnal, in C and MMX                                     |
|           | - Added : documentation to Logic filter                                                            |
|           | - Added : two modes for Logic : "Min" and "Max" ( C, MMX, iSSE )                                   |
|           | - Added : In DEdgeMask, the possibility to set the normalization factor                            |
|           | - Corrected : documentation.                                                                       |
+-----------+----------------------------------------------------------------------------------------------------+
| v1.4.13   | - Bugfix : One more, in the MotionMask ( the last row was not correctly computed )                 |
|           | - Optimizations : MaskedMerge gives now the same output in MMX and C,                              |
|           |   so MMX optimizations for it are back by default.                                                 |
|           | - Added : In EdgeMask, you now can use the laplace kernel. See the                                 |
|           |   documentation on that filter                                                                     |
|           | - Added : 'New' filter, DEdgeMask, which allows you to choose your                                 |
|           |   kernel ( at a cost : speed )                                                                     |
+-----------+----------------------------------------------------------------------------------------------------+
| v1.4.12   | - Behavior modifications : MotionMask and EdgeMask now also computes                               |
|           |   pixels on the borders mainly by extending the mask to these pixels.                              |
|           | - Bugfix : Inflate / Inpand / Expand / Deflate, when using negative                                |
|           |   parameters for y,u and v, some weird problems could occur.                                       |
|           | - Added functionnality : In YV12LUT, the function abs is now defined.                              |
+-----------+----------------------------------------------------------------------------------------------------+
| v1.4.11   | - Bugfix : EdgeMask, MMX optimizations give different results. They are                            |
|           |   disabled by default. To activate them, use usemmx = true. They'll be used                        |
|           |   only with mod 16 resolution                                                                      |
|           | - Bugfix : EdgeMask : first and last lines weren't always computed.                                |
|           | - Bugfix : MaskedMerge : MMX optimizations darken slightly the picture.                            |
|           |   They are disabled by default. To activate them, use usemmx = true.                               |
|           |   They'll be used only with mod 16 resolution.                                                     |
+-----------+----------------------------------------------------------------------------------------------------+
| v1.4.10   | - Bugfix : first and last lines were not correctly computed with                                   |
|           |   inflate / deflate                                                                                |
|           | - Invert is no longer a filter of the Masktools, it has been moved                                 |
|           |   inside AviSynth.                                                                                 |
+-----------+----------------------------------------------------------------------------------------------------+
| v1.4.9    | - New filter : CombMask. As usual, read further for more documentation                             |
+-----------+----------------------------------------------------------------------------------------------------+
| v1.4.8    | - YV12Convolution now supports negative coefficients in the matrix. It                             |
|           |   allows to use the filter has an edge detecter.                                                   |
|           | - YV12Convolution has now a new parameter : bool saturate, which, if                               |
|           |   set to true, or if there is a possibility of getting out of the range                            |
|           |   [0..255] during calculation, clips each pixel into that range ( which                            |
|           |   means it's slightly slower )                                                                     |
|           | - A new filter : LUT. Read further for more information on how to use it.                          |
+-----------+----------------------------------------------------------------------------------------------------+
| v1.4.7    | - Rename MotionDetection to MotionMask. I know it's kind of silly, but                             |
|           |   it's a lot more logical that way.                                                                |
|           | - Add the check of the width for the use MMX in MotionMask                                         |
|           | - Slightly modify MMX optimizations in Binarize.                                                   |
|           | - Add a new filter : YV12Convolution. It allows you to convole the                                 |
|           |   picture by a matrix of (almost) any size. Look further in the readme to                          |
|           |   learn how to use it                                                                              |
+-----------+----------------------------------------------------------------------------------------------------+
| v1.4.6    | - Made the scenechange detection in MotionDetection iSSE optimized (                               |
|           |   meaning you need an Athlon XP / Pentium IV ). It works with an Athlon XP,                        |
|           |   it is not tested with                                                                            |
|           |   an Pentium IV, it is possible to disable it by using usemmx = false in the                       |
|           |   paremeters of the filter.                                                                        |
|           | - Optimized the calculation of the motion, without using MMX ( just by                             |
|           |   avoiding to do 3 times the same calculations... ). So the filter should                          |
|           |   be more or less three times faster.                                                              |
+-----------+----------------------------------------------------------------------------------------------------+
| v1.4.5    | - Added MotionDetection filter, no MMX / assembler optimizations for it                            |
|           |   yet. Look further in the Readme to learn how to use it. It takes the idea                        |
|           |   of Sansgrip's filter (NoMoSmooth) and outputs the motion mask directly in                        |
|           |   the correct colorspace for the MaskTools.                                                        |
+-----------+----------------------------------------------------------------------------------------------------+
| v1.4.4    | - Reactivated MMX optimizations for MaskedMerge                                                    |
|           | - Came back to Kurosu's optimizations for Invert                                                   |
+-----------+----------------------------------------------------------------------------------------------------+
| v1.4.3    | - Made some MMX optimizations ( binarize, invert )                                                 |
|           | - Corrected some MMX optimizations ( which means mostly 'disabled some                             |
|           |   MMX optimizations' ). It should now work with P4.                                                |
+-----------+----------------------------------------------------------------------------------------------------+
| v1.4.2    | - Fixed bugs concerning the inpand / expand / inflate / deflate functions                          |
+-----------+----------------------------------------------------------------------------------------------------+
| v1.4.1    | - Fixed the dreadly bug "multiple instances of a filter with different                             |
|           |   functions needed"                                                                                |
+-----------+----------------------------------------------------------------------------------------------------+
| v1.4.0    | - Added an experimental LUT filter. Not tested, debug later.                                       |
+-----------+----------------------------------------------------------------------------------------------------+
| v1.3.0    | - (private version)                                                                                |
|           | - Made usable the FitPlane function (still an overload of work when                                |
|           |   only one plane has to be resized) which was previously undocumented;                             |
|           |   therefore, added FastFitPlane functions (corresponding FitPlane ones                             |
|           |   should be useless now, except for the resizers settings)                                         |
|           | - Allowed the specification of a processing area for many filters;                                 |
|           |   however, this should not produce any noticable speed increase.                                   |
|           | - Cleaned YV12Layer (in particular the unusable "Darken"/"Lighten" modes)                          |
|           | - Added OverlayMask, a function that compares 2 clips, and outputs a                               |
|           |   mask of the parts that are identical (slow and far from perfect).                                |
+-----------+----------------------------------------------------------------------------------------------------+
| v1.2.0    | - (private version)                                                                                |
|           | - YV12Layer: no more useless RGB32 conversion! Approximately the same                              |
|           |   as Arithmetic (except a third clip is not used), so that one is gone...                          |
|           | - YV12Substract: hey, why only a C version? Masks are really an                                    |
|           |   underused feature of AviSynth ``|-[``                                                            |
+-----------+----------------------------------------------------------------------------------------------------+
| v1.1.0    | - (private version)                                                                                |
|           | - Older inflate/deflate are renamed expand/inpand while newer functions replace them               |
|           | - Logic and Arithmetic functions added (shouldn't produce the expected                             |
|           |   results because of no debugging)                                                                 |
|           | - Edgemask now takes 4 thresholds (2 for luma and 2 for chroma). They                              |
|           |   are used for: setting to 0 or leaving as is a value depending on first                           |
|           |   threshold, setting to 255 or leaving as is a value depending on the                              |
|           |   second one.                                                                                      |
+-----------+----------------------------------------------------------------------------------------------------+
| v1.0.2    | - (last version - public project dropped):                                                         |
|           | - Fix the shift for edgemask using sobel and roberts (misplaced MMX instruction)                   |
|           | - MaskMerge now works (mask cleared before being used... check with                                |
|           |   MaskMerge(clip3,clip3) for instance)                                                             |
+-----------+----------------------------------------------------------------------------------------------------+
| v1.0.1    | - Initial release                                                                                  |
+-----------+----------------------------------------------------------------------------------------------------+

Developer's walkthrough
-----------------------

Skip to V) if you're not interested in developing the tools available.

The project is a VC++ 6 basic project. Each filter has its own folder which
stores the header used by the interface, the source for the function members,
the source for processing functions and its header. Let's look at EdgeMask:

- EdgeMask.h is included by the interface to know what the filter 'looks
  like' (but interface.cpp still holds the definition of the calling
  conventions and exported functions)
- EM_func.h describes the different processing functions (they should all
  have the same prototype/parameters):

  - Line_MMX and Line_C
  - Roberts_MMX and Roberts_C
  - Sobel_MMX and Sobel_C

- EM_func.cpp, as all <filter's initials>_func.cpp, stores the implementation
  of the processing functions, and sometimes their MMX equivalents.
- EdgeMask.cpp implements the class; the constructor select the appropriate
  processing function (MMX? C? Roberts? Line? Sobel?) and uses it to fill the
  generic protected function pointer used in GetFrame

Interface.cpp stores the export function and all of the calling functions
(AVSValue ... Create_<filter>).

ChannelMode.cpp defines the Channel operating modes. There could be added the
equivalent of a debugprintf.

This quick walkthrough won't probably help most developers, as the examples
of V) for users, but that's the best I've come with so far. It will improve
of course over time depending on the success of the idea, which main
drawback, speed, will probably make it scarcely used, if ever. <g>

$Date: 2005/10/05 18:12:43 $

.. _Levels: http://jourdan.madism.org/corefilters/levels.htm
.. _NoMoSmooth: http://jourdan.madism.org/%7Emanao/nomosmooth.htm
.. _this thread: http://forum.doom9.org/showthread.php?s=&threadid=48167


VqmCalc
=======


Abstract
--------

| **author:** Lefungus
| **version:** 0.21
| **download:** `<http://perso.wanadoo.fr/reservoir/dl/VqmCalc-0.21.rar>`_
| **source**: `<http://perso.wanadoo.fr/reservoir/dl/VqmCalcSrc-0.21.rar>`_
| **category:** Plugins to compare video quality using specific video quality metrics
| **requirements:** YV12 Colorspace

--------


Description
-----------

The compare function of AviSynth, calculates psnr from two clips. The
following method (called VQM) should be a better video quality metrics than
psnr. It is based on the following report:

`<http://ise.stanford.edu/class/ee392j/projects/xiao_report.pdf>`_

It is not exactly the same as in the white-paper as some vital informations
are lacking like the matrix we need to use for spatial masking.
In this filter, i use MPEG Matrix with hope it reflects a bit Human Visual
System.
No temporal masking is implemented in this method.

A few experiments were done with a little clip encoded with xvid at constant
quant2/quant4/quant6 and quant2 with VHQ1.

Keep in mind these results may not reflect video quality

| Mean VQM:
| Quant2: 37.16
| Quant4: 52.58
| Quant6: 63.12
| Quant2+VHQ1: 37.74

More points (may) means less quality. It seems to scale well with Quants but
it doesn't like VHQ.

It will only work with YV12. And it seems to not works with B-Frames.
The clip must be mod8. If not it may crash.
Another bugs i've encountered are:

-   May crash when the log already exist.
-   May crash during the clip, try again until it works.

I hope you're not afraid to test it.

**an example:**

::

    a = Avisource("J:\Video\Source.avs")
    b = Avisource("J:\Video\Quant6.avi")
    return VqmCalc(a,b,"results.log")

PS: I use fdct_mmx from xvid sources, so i guess i need to release it GPL,
and include GPL headers. I'm not sure if i can, because it's inspired from a
white paper. I'll be glad to receive any help on this subject.

$Date: 2004/08/17 20:31:19 $


SSIM
====


Abstract
--------

| **author:** Lefungus
| **version:** 0.23
| **download:** `<http://www.avisynth.org/warpenterprises/>`_
| **source:** `<http://perso.wanadoo.fr/reservoir/dl/SSIMSrc-0.23.rar>`_
| **category:** Plugins to compare video quality using specific video quality metrics
| **requirements:** YV12 Colorspace

--------


Description
-----------

This filter has been created following the ideas of `Zhou Wang`_.
It has been coded with the great help of Mfa, who worked on the core
functions.

For a given reference video and a given compressed video, it is meant to
compute a quality metric, based on perceived visual distortion. Unlike the
well-known PSNR measure, it's not purely mathematical, and should correlate
much better with human vision.

Some examples can be found `here`_.

A higher MSE (and so lower PSNR) should mean that the compressed clip is a
worse image but MSE and PSNR are flawed in this respect as numerous tests
have shown. However with SSIM, according to tests carried out on the VQEG
dataset, a higher Q (SSIM value) has a much better relation to the visual
quality of the compressed clip. Despite this, bear in mind the SSIM metric
still isn't perfect.

This filter is designed to compute an SSIM value by two methods, the original
one, and a "enhanced" one that weight these results by lumimasking. On the
todo list is to include the motion weighting.

This filter has five parameters:

``SSIM`` (clip1, clip2, "results.csv", "averageSSIM.txt", lumimask=true)

*clip1* and *clip2* are the reference clip and the compressed clip.
*"results.csv"* is the file where obtained SSIM values will be written (this
can be easily read in excel or notepad for those unfamiliar with the comma
separated variables format)

lumimasking switch between the two methods.

When the video is closed, the filter will write a file named
*"averageSSIM.txt"* that will contain the global SSIM value.

An SSIM value is between 0 and 1, 1 meaning perfect quality.

To analyse locally the results, you could use the csv files, and manipulate
data in any excel-clone. Examples:

-   `codec A vs codec B`_
-   `codec A with lumi option`_

In the csv file, when lumimasking is activated, both SSIM values and its
weigth is written.

**Note:**

If you use B frames under xvid, trim the first dummy frame of the xvid clip,
and the last frame of the original clip.

$Date: 2004/08/17 20:31:19 $

.. _Zhou Wang: http://www.cns.nyu.edu/~zwang/
.. _here: http://http://www.cns.nyu.edu/~zwang/files/research/quality_index/demo_lena.html
.. _codec A vs codec B: http://perso.wanadoo.fr/reservoir/dl/ssim.png
.. _codec A with lumi option:
    http://perso.wanadoo.fr/reservoir/dl/ssimlumi.png

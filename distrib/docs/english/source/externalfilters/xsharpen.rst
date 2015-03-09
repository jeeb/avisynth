
WarpSharp
=========


Abstract
--------

| **author:** anonymous (japanese)
| **version:** in warpsharp_2003_1103.cab
| **download:** `<http://www.geocities.co.jp/SiliconValley-PaloAlto/2382/>`_ (`english intro`_)
| **category:** Sharpen/Soften Plugins
| **requirements:** YUY2 & YV12 Colorspace

--------

You will also need to copy `msvcp71.dll and msvcr71.dll`_ to your system dir.


Description
-----------

The syntax is given by

``XSharpen`` (clip, strength = 128, threshold = 8)

This filter performs a subtle but useful sharpening effect. It operates by
running a small window over the frame and each center pixel is either passed
through untouched (depending upon a threshold setting), or mapped to either
the brightest or dimmest pixel in the window, depending upon which is nearest
to the center pixel. If the center pixel is mapped, it is also alpha-blended
with the original pixel value using a configurable  strength setting. The
result is a sharpening effect that not only avoids amplifying noise, but also
tends to reduce it. A welcome side effect is that files processed with this
filter tend to compress to smaller files.

Description of parameters

| **strength**
| When this value is 255, mapped pixels are not blended with the original pixel
  values, so a full-strength effect is obtained. As the value is reduced, each
  mapped pixel is blended with more of the original pixel. At a value of 0, the
  original pixels are passed through and there is no sharpening effect.

| **threshold**
| This value determines how close a pixel must be to the brightest or dimmest
  pixel to be mapped. If a pixel is more than threshold away from the brightest
  or dimmest pixel, it is not mapped. Thus, as the threshold is reduced, pixels
  in the mid range start to be spared.

Description is taken from the `equivalent VDub filter of Donald A. Graft`_.

$Date: 2004/08/17 20:31:19 $

.. _english intro:
    http://niiyan.s8.xrea.com/avisynth/en/warpsharp_introduction_en.html
.. _msvcp71.dll and msvcr71.dll: ../faq.htm#JapanesePlugin
.. _equivalent VDub filter of Donald A. Graft:
    http://neuron2.net/xsharp.html


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

You will also need to copy :ref:`msvcp71.dll and msvcr71.dll <JapanesePlugin>` to your system dir.


Description
-----------

The syntax is given by

``WarpSharp`` (clip, int depth = 128, int blur = 3, int bump = 128, float
cubic = -0.6)

Description of parameters

| **depth**
| Depth of warp.

| **blur**
| Number of times of blur.The processing will become slow if this value
  increases.If this value is not defined, the value is set to the minimum, 1.

| **bump**
| Threshold of unevenness detection. It will become more sharpen effect if it
  increases.

| **cubic**
| Coefficient of warp value (cubic interpolation). There is no need to change
  this setting.

`<http://niiyan.s8.xrea.com/avisynth/en/warpsharp_plugin_en.html>`_

$Date: 2005/03/24 22:07:09 $

.. _english intro:
    http://niiyan.s8.xrea.com/avisynth/en/warpsharp_introduction_en.html

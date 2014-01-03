
UnDot
=====


Abstract
--------

| **author:** Tom Barry
| **version:** 0.0.1.1
| **download:** `<http://mywebpages.comcast.net/trbarry/downloads.htm>`_
| **category:** Misc Plugins
| **requirements:**

-   YV12 & YUY2 Colorspace
-   SSEMMX support

--------


Description
-----------

UnDot is a simple median filter for removing dots, that is stray orphan
pixels and mosquito noise. Basically, it just clips each pixel value to stay
within min and max of its eight surrounding neigbors.

Usage
-----

In your Avisynth file use commands similar to

::

    LoadPlugin("F:\UnDot\UnDot.dll")
    Avisource("D:\wherever\myfile.avi")
    UnDot()

Of course replace the file and directory names with your own. **There are no
parameters**.


Known issues
------------

In YV12 format it will filter both luma and chroma. In YUY2 format it will
only filter luma.

+--------------------------------------------------------------------------+
| Changelog                                                                |
+==========+============+==================================================+
| v0.0.1.1 | 2003/01/18 | Use AvisynthPluginInit2                          |
+----------+------------+--------------------------------------------------+
| v0.0.1.0 | 2002/11/03 | Initial test release for Avisynth 2.5 alpha only |
+----------+------------+--------------------------------------------------+

$Date: 2004/08/17 20:31:19 $

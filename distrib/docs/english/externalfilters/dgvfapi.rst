
DGVfapi 1.4.7
=============

Frameserving with the New Decoding Functionality

--------

VFAPI frameserving is supported. It uses the `DGDecode.dll`_, so both
DGVfapi.vfp and DGDecode.dll must be placed in the same directory as
DGIndex.exe. Do not rename these files. This version supports multiple
instantiations, e.g., create and run multiple "fake" AVI files. It also can
read Avisynth scripts (AVS files) as well as D2V files from DGIndex.

The procedure is as follows:

1. Place the files as described above.

2. Run DGIndex and verify that when you pull down the Help menu the VFAPI
   Plugin item is checked. You don't have to check it; it is checked
   automatically if the files are placed in the right place. Generate your
   D2V project file as usual.

3. Ensure that the `VFAPI reader codec`_ is installed.

4. Run the VFAPI Converter, select the D2V file, and hit OK.

5. Hit Convert to create the "fake" AVI file.

DVD2AVI's clipping feature is not supported.

VFAPI upsamples to RGB. For D2V files, by default this is done in interlaced
mode. To force progressive upsampling, put "_P" just before the extension in
the filename. To force the upsampling to follow the progressive_frame flag,
put "_A" just before the extension in the filename. Note that using the "_A"
also means that a YUY2 frame will be returned instead of a YV12 frame for
4:2:0 input.

For example:

::

    movie_P.d2v


Although you can open both D2V files and AVS files with DGVfapi, the _P and
_A naming trick works only for D2V files. When AVS files are opened,
progressive RGB upsampling is used by default. If the script doesn't return
RGB24, DGVFapi just attaches a ConvertTorRGB24() with default parameters to
the end of the script. Note that it is progessive upsampling and not
interlaced for D2V files. So if you want an RGB conversion different from the
default one, then you should explicitly specify the conversion at the end of
the AVS script.

The multiple instantiation and AVS file support was done by "tritical".

Donald Graft (c) 2004, 2005

$Date: 2007/11/21 18:03:03 $

.. _DGDecode.dll: dgdecode.htm
.. _VFAPI reader codec:
    http://www.doom9.org/Soft21/SupportUtils/VFAPIConv-1.05-EN.zip

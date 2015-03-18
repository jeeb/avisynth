
SDKNecessaries
==============

What is necessary to create an AviSynth plugin?
-----------------------------------------------

You must have some compatible development tool
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

-   Microsoft Visual C/C++ 6, 7, 7.1, or 8 (2005)
-   Microsoft Visual C++ Toolkit 2003 (free, try search
    vctoolkitsetup.exe) with some IDE (e.g. free `CodeBlocks`_)
-   Microsoft Visual C++ 2005 Express edition
-   Microsoft Visual C++ 2010 Express edition (free download `[1]`_)
-   Intel ICL Compiler v7 (?) or above

Notes: Visual C/C++ 6, 7, 7.1 Standard Edition (and NET 1.1 SDK) lack in
optimizing compiler (only expensive Professional or Enterprise Edition
included it). As a partial workaround you can add free Visual C++ Toolkit
2003 optimizing compiler to your Standard Edition IDE (by setting directories
properly) or use Visual C++ 2005 (any edition).


You also need in Microsoft Platform SDK (if it is not included withcompiler).
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Free download it from `Microsoft site`_. The last SDK that will work with VC
6.0 was the February 2003 Edition.

For some very special plugins (GPU) you may need in DirectX SDK.


Finally, you must include small header file 'avisynth.h'.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

You can get it with this FilterSDK, download with AviSynth source code, or
take from some plugin source package. There are several versions of this
header file from various AviSynth versions.

Header file avisynth.h from v1.0.x to v2.0.x have
``AVISYNTH_INTERFACE_VERSION = 1.`` Plugins compliled with them will be not
(natively) compatible with AviSynth 2.5.x.

Header file avisynth.h from v2.5.0 to v2.5.5 have
``AVISYNTH_INTERFACE_VERSION = 2.`` Plugins compliled with them will
(generally) work in AviSynth v2.5.0 to v2.5.7 (and above). But avisynth.h
files from versions v2.5.0 - v2.5.5 (and betas) are not identical. We
recommend to use avisynth.h from versions 2.5.5 or later. Previous versions
of avisynth.h are obsolete and have some bugs.

Header file avisynth.h from v2.5.6 to v2.5.8 are almost identical and have
``AVISYNTH_INTERFACE_VERSION = 3.`` Plugins compliled with them will work in
v2.5.6 and up. They will also work in v2.5.5 and below, if you do not use new
interface features and do not call ``env->CheckVersion`` function.

New beeing developed AviSynth version 2.6.x will use new header avisynth.h,
currently with ``AVISYNTH_INTERFACE_VERSION = 5.`` Plugins compiled with
AviSynth v2.5.x header will work in AviSynth 2.6.x too, but plugins compliled
with new AviSynth v2.6.x header will probably not work in AviSynth v2.5.x.

Generally good start is to take some similar plugin source code as a draft
for improving or own development. Attention: there are many old plugins
source code packages with older avisynth.h included. (for example,
SimpleSample zip file contains avisynth.h file from from AviSynth v2.5.4).
Simply replace it by new one.


Compiling options
~~~~~~~~~~~~~~~~~

Plugin CPP source code must be compiled as Win32 DLL (multi-threaded or
multi-threaded DLL) without MFC.

Notes. If you use Visual C++ Toolkit 2003 itself (without VC++ 7), you can
not build plugin as multi-treaded DLL: the toolkit missed some libraries, in
particular msvcrt.lib. You can get additional libs with MS .NET 1.1 SDK (free
download) or simply use multi-treaded option (IMHO it is better - no need in
MSVCRT71.DLL).

Of course, use Release build with optimization. Typical compiler switches are
/MT /O2 and /dll /nologo for linker

See :doc:`step by step instructions. <CompilingAvisynthPlugins>`


Other compilers
~~~~~~~~~~~~~~~

You can NOT use other compiler like GNU C++ to create regular AviSynth
plugins.

You may also try to use some tools (GNU C++, Visual Basic, Delphi) to create
so called AviSynth C plugins with AviSynth C API (but it is not mainsteam).

Original AviSynth C API by Kevin Atkinson is at
http://kevin.atkinson.dhs.org/avisynth_c/

Updated version of AviSynth C API header and library files are distributed
with AviSynth since v2.5.7.

There is also `Pascal conversion of avisynth_c.h`_ by Myrsloik

Some info about `Using in Visual Basic`_

`PureBasic port of the Avisynth C Interface`_ by Inc

There is also `AvsFilterNet`_ wrapper for Avisynth in .NET (any .NET
language) by SAPikachu, see `discussion`_


Back to :doc:`FilterSDK <FilterSDK>`

$Date: 2010/08/15 13:51:15 $

.. _CodeBlocks: http://www.codeblocks.org
.. _[1]: http://www.microsoft.com/express/Downloads/
.. _Microsoft site: http://www.microsoft.com/downloads/details.aspx?familyid=EBA0128F-A770-45F1-86F3-7AB010B398A3&displaylang=en
.. _Pascal conversion of avisynth_c.h:
    http://forum.doom9.org/showthread.php?t=98327
.. _Using in Visual Basic: http://forum.doom9.org/showthread.php?t=125370
.. _PureBasic port of the Avisynth C Interface:
    http://forum.doom9.org/showthread.php?t=126530
.. _AvsFilterNet: http://www.codeplex.com/AvsFilterNet
.. _discussion: http://forum.doom9.org/showthread.php?t=144663

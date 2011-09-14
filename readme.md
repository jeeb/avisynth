Avisynth 2.x
============

This is the CVS repository of Avisynth 2.x
cloned with git cvsimport to make work easier
with the repository for the ones of us who have
gotten more used to the newer version control systems.

Is upkept with
--------------

    git cvsimport -p x3 -v -d :pserver:anonymous@avisynth2.cvs.sourceforge.net:/cvsroot/avisynth2 avisynth

Main changes
------------

* src/core/main.cpp

    Around line 1405 there is a definition of GUIDs
    GUID KSDATAFORMAT_SUBTYPE_PCM and GUID KSDATAFORMAT_SUBTYPE_IEEE_FLOAT
    which both are in at least Win7 SDK.
    
    To enable building with this SDK, they are now commented out.
    If you experience problems regarding this, remove the comment lines
    before and after it.

* distrib/include/SoftWire/SoftWire.dsp
* distrib/include/SoundTouch/SoundTouch.dsp
* distrib/include/pfc/pfc.dsp
* src/avisynth.dsp

    These files had their EOLs changed to Dos\Windows compatible ones.
    This fixes project conversion with certain Visual Studio versions.
    (Visual Studio should be fine with opening them, but the converting
    part does have problems with mixed up EOLs).
    
    
How to build
------------

Building Avisynth itself should be rather straightforward. You would need
a recent Windows SDK set to your Visual Studio, and a DirectX SDK with the
DirectDraw portion intact (February 2010 SDK being the last one with it) for
the DirectShowSource plugin portion.

I am planning to make proper MSVS2008, 2010 projects for Avisynth, but so far
you can follow these steps to get it to build:

    * Open up the src/avisynth.dsw in the Visual Studio of your choice.
    * Let Visual Studio convert the workspace and the projects.
    * After conversion, see if the avisynth project builds.
    * If it doesn't (as is with VS2010), open up the converted avisynth project
      file with a text editor, such as Notepad++. replace all occurences of
      $(InputName) with %(Filename) .
    * Try re-building. If you have your Windows SDK properly set, it should now
      build and link avisynth.dll, even though the copy to System32 will
      fail for obvious reasons.
    * Try building the DirectShowSource, and add needed library and include
      paths from your Windows SDK's folders as well as the DirectX SDK's
      folders. This should bring you working copies of Avisynth and DSS.
    

AviSynth Interface Version
==========================

The AVISYNTH_INTERFACE_VERSION describes the level of features
available, both in the core avisynth.dll and the third party plugin.

For a plugin author it describes what the core IScriptEnvironment
vtable contains.

- Version 1 is Avisynth 2.0
- Version 2 is Avisynth 2.5, with the vtable having members up to
  IScriptEnvironment::SetWorkingDir(const char * newdir)
- Version 3 is Avisynth 2.5.6, with the IScriptEnvironment vtable
  adding 3 new members ManageCache, PlanarChromaAlignment and
  SubframePlanar.
- Version 4 is reserved and does not apply to any Avisynth version.
  It's only significance is it greater then 3 and less then 5.
- Version 5 is Avisynth 2.6.0a1-a5, with the IScriptEnvironment
  vtable adding 3 more new members DeleteScriptEnvironment,
  ApplyMessage and GetAVSLinkage. Also with version 5 the core
  provides AVS_Linkage support for baked code replacement.
- Version 6 is Avisynth 2.6.0, with the IScriptEnvironment vtable
  adding one more new member GetVarDef. It also uses size_t for
  things that are memory sizes, ready for 64 bit port.

Through the IClip interface it is the authors responsibility to declare
the level of support the plugin provides:
::

    virtual int __stdcall IClip()::GetVersion() { return AVISYNTH_INTERFACE_VERSION; }


- Version 1 is Avisynth 2.0
- Version 2 and 3 are Avisynth 2.5, supporting YV12, YUY2, RGB32 and
  RGB24 colour spaces.
- Version 4 is reserved and does not apply to any Avisynth version.
  It's significance is it greater then 3 and less then 5.
- Version 5 and 6 are Avisynth 2.6, and the IClip interface must
  support this update:

::

    virtual int __stdcall IClip::SetCacheHints(int cachehints,int frame_range); # Plugins that do not implement the interface must always return zero.

____

Back to :doc:`FilterSDK`

$Date: 2014/12/30 00:24:54 $

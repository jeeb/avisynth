
Changes.
========


Changes from 3.7.2 to 3.7.3
---------------------------

Additions, changes
~~~~~~~~~~~~~~~~~~
- MIPS build support
- New: add a sixth array element to ``PlaneMinMaxStats``: average. Defines variable "PlaneStats_average" as well if setting variables is required.
- "Text" ``halo_color`` allows to have both halo and shaded background when halo_color MSB=$FE
- "Text" (#308) much nicer rendering of subsampled formats
- "Text" (#310) Support any width of external bdf fonts (but still of fixed width)
- "Text" (#310) Support more from the BDF standard: per-character boundary boxes and shifts
- "Text": draw rightmost on-screen character even if only partially visible (was: not drawn at all)
- "Text" Halo is not limited to original character matrix boundaries and is rendered on the displayed string as a whole.
- "Text" add "left" chroma placement support; new ``placement`` parameter (left, center, auto), accept frame property ``_ChromaLocation`` hint
- "TimeStretch" (#278) add TimeStretch overload with rational pair arguments and update SoundTouch library to v2.3.1.
- Enhancement (#315): Show exception message as well if a v2.6-style plugin throws AvisynthError in its ``AvisynthPluginInit3`` instead of only "'xy.dll' cannot be used as a plugin for AviSynth."
- Fix (#327) Histogram "color2" markers. Fix right shifted 15 degree dots, fix square for bits>8
- Feature #337: add more resizers

  - filters: SinPowerResize, SincLin2Resize, UserDefined2Resize
  - and their equivalent for the ConvertToXXXX family 'chromaresample': "sinpow",  "sinclin2" and "userdefined2"

- Feature #337: add "param1" and "param2" to ConvertToXXXX where 'chromaresample' exists (b,c,taps and p parameters can be set).
- #306: Add ConvertToYUVA420, ConvertToYUVA422 and ConvertToYUVA444
- Expr: Add remaining stack element count to "Unbalanced stack..." error message
- Add back audio cache from classic Avisynth 2.6. Handle modes and hints on cache audio:
  CACHE_AUDIO, CACHE_AUDIO_NOTHING, CACHE_AUDIO_AUTO_START_OFF, CACHE_AUDIO_AUTO_START_ON,
  CACHE_GETCHILD_AUDIO_MODE and CACHE_GETCHILD_AUDIO_SIZE
- Set automatic MT mode MT_SERIALIZED to ConvertToMono, EnsureVBRMP3Sync, MergeChannels, GetChannel, Normalize, MixAudio, ResampleAudio
- Alter default VfW export (e.g. VirtualDub2) channel layout mask for 3 channels (Surround to 2.1), 4 channels (Quad to 4.0) and 6 channels (6.1(back) to 6.1)
- Add audio channel (speaker layout) support

  - Implemented by using 20 bits from VideoInfo struct image_type flag
  - Follow layout definition from WAVEFORMATEXTENSIBLE
  - Add Script functions for audio layout support:

    - bool IsChannelMaskKnown(clip)
    - int GetChannelMask(clip)
    - SetChannelMask(clip, bool known, int dwChannelMask)
    - SetChannelMask(clip, string ChannelDescriptor)

  - Add friendly names for the 18 speaker positions (e.g. "FL" for front left)
  - Add friendly names for their frequently used combinations (e.g. "stereo" for "FL+FR")
    to be used in SetChannelMask.

  - GetChannel, GetChannels, MergeChannels: set default channel layout if channel count is 1 to 8
  - ConvertToMono, GetLeftChannel, GetRightChannel: sets channel layout to "mono"
  - AudioDub will inherit channel layout setting from the audio clip.
  - KillAudio calls SetChannelMask(false, 0)
  - "Info": displays channel mask info in the audio section when exists

Build environment, Interface
~~~~~~~~~~~~~~~~~~~~~~~~~~~~
- Feature (#317): (v10) The color format of a ``VideoFrame`` can now be retrieved with its ``GetPixelType()`` (CPP) and ``avs_video_frame_get_pixel_type`` (C)
  function. Before, there was no reliable way of knowing it on a frame from ``propGetFrame()``.
  The internally stored ``pixel_type`` in ``VideoFrame`` is properly converted upon a ``Subframe`` (Y8-32), ``SubframePlanar`` (strip alpha).
- Feature (#317): (v10) added ``VideoFrame::AmendPixelType`` and ``avs_video_frame_amend_pixel_type``.
  Introduced in order to keep ``VideoInfo`` and ``VideoFrame`` ``pixel_type`` synchronized for special cases:
  when filter constructor would just change ``VideoInfo::pixel_type``, but the frame would be passed w/o any change, like in ``ConvertFromDoubleWidth`` or ``CombinePlanes``.
- Feature (#314): Added ``AVSValue::GetType()`` (v10)
  Returns an ``AvsValueType`` enum directly, one can use it instead of calling all IsXXX functions to establish the type. (Rust use case)
- Fix in headers (#314): Changed ``NewVideoFrameP()`` and ``avs_new_video_frame_p()`` property source argument to const
- Enhancement in headers (#314): Made ``VideoFrameBuffer`` destructor public like in other classes of the public API to prevent compiler errors downstream when calling non-const member functions
- Address Issue #305: Support for non-decorated ``avisynth_c_plugin_init`` in C-plugins
- Fix: C API undefined behavior when upstream throw runtime error
- CMakeLists.txt: fix clang-cl/intel with ninja generator
- Fix: C interface crash when using avs_new_video_frame_p(_a)
- Fix: C interface avs_prop_get_data behave like C++ counterpart. Interface version is dor this fix is 9.2
- CMakeLists.txt: add support for Intel C++ Compiler 2022
- CMakeLists.txt: add support for Intel C++ Compiler 2023
- Bump Interface version. Interface version is 10.0
  AVISYNTH_INTERFACE_VERSION = 10,
  AVISYNTHPLUS_INTERFACE_BUGFIX_VERSION = 0
- Address #282: make 32-bit MSVC build to generate both decorated and undecorated export function names for C plugins
- (v10) Add audio channel support to C++ interface by using 18+2 VideoInfo.image_type bits

  - Check for existence: bool VideoInfo::IsChannelMaskKnown()
  - Setting: void VideoInfo::SetChannelMask(bool isChannelMaskKnown, unsigned int dwChannelMask)
  - Retrieving: unsigned int VideoInfo::GetChannelMask()
  - enums AvsChannelMask and AvsImageTypeFlags 

- (v10) Add audio channel support to C interface

  - Check for existence: bool avs_is_channel_mask_known(const AVS_VideoInfo * p);
  - Setting: void avs_set_channel_mask(const AVS_VideoInfo * p, bool isChannelMaskKnown, unsigned int dwChannelMask);
  - Retrieving: unsigned int avs_get_channel_mask(const AVS_VideoInfo * p);
  - supporting enums



Bugfixes
~~~~~~~~
- Fix: "Text" filter negative x or y coordinates (e.g. 0 instead of -1)
- Fix: "Text" filter would omit last character when x<0
- Fix: "Text" ``halo_color`` needs only MSB=$FF and not the exact $FF000000 constant for background fade
- Fix: (#304) "ColorYUV" analyze=true was displaying wrong min-max values for YUY2
- Fix: (#293) "Text" to throw proper error message if the specified font name (e.g. Arial) is not found among internal bitmap fonts.
- Fix: (#293) "Subtitle" and "Text" filter to respect the explicitely given coorditanes for y=-1 or x=-1, 
  instead of applying vertical/horizontal center alignment.
- Fix (#283): broken runtime functions Min/Max/MinMaxDifference when threshold is not 0 (returned -1). Regression in 3.7.2
- Fix (#282): ConvertToRGB
  - do check for exact 8 or 16 bit input, because packed RGB formats exist only for 8 and 16 bits
  - keep alpha for RGBA planar - convert RGBAP8/16 to RGB32/64, while RGBP8/16 is still RGB24/48
- Fix crash when outputting VfW (e.g. VirtualDub) for YUV422P16, or P10 in Intel SSE2 clang builds
- Fix Clang build AviSource crash on yuv422p10le UTVideo at specific widths (SSE2 or SSE4.1)
- Fix: (#340): stop memory leak on propSet / MakePropertyWritable
- Fix. (#347): possible crash of LLVM builds (clang-cl, Intel NextGen) on pre-AVX (SSE4-only) CPUs.

Optimizations
~~~~~~~~~~~~~
- Enhanced performance in ConvertBits Floyd dither (dither=1) for 10->8, 16->8 and 16->10

Documentation
~~~~~~~~~~~~~
- Internal plugins, syntax, ...: almost fully revised and made up-to-date. Big thanks to Reel-Deal!
- Update build documentation with 2023 Intel C++ tools. See Compiling Avisynth+ 


Please report bugs at `github AviSynthPlus page`_ - or - `Doom9's AviSynth+
forum`_

$Date: 2023/03/21 10:10:00 $

.. _github AviSynthPlus page:
    https://github.com/AviSynth/AviSynthPlus
.. _Doom9's AviSynth+ forum:
    https://forum.doom9.org/showthread.php?t=181351

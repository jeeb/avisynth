
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
- Feature #317: add more resizers:
  - filters: SinPowerResize, SincLin2Resize, UserDefined2Resize
  - and their equivalent for the ConvertToXXXX family 'chromaresample': "sinpow",  "sinclin2" and "userdefined2"
- Feature #317: add "param1" and "param2" to ConvertToXXXX where 'chromaresample' exists (b,c,taps and p parameters can be set).

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
- Bump Interface version. Interface version is 10.0
  AVISYNTH_INTERFACE_VERSION = 10,
  AVISYNTHPLUS_INTERFACE_BUGFIX_VERSION = 0

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

Optimizations
~~~~~~~~~~~~~

Documentation
~~~~~~~~~~~~~
- Internal plugins, syntax, ...: almost fully revised and made up-to-date. Big thanks to Reel-Deal!

Please report bugs at `github AviSynthPlus page`_ - or - `Doom9's AviSynth+
forum`_

$Date: 2023/02/15 10:10:00 $

.. _github AviSynthPlus page:
    https://github.com/AviSynth/AviSynthPlus
.. _Doom9's AviSynth+ forum:
    https://forum.doom9.org/showthread.php?t=181351

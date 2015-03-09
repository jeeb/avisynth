
DGDecode
========


Abstract
::::::::

| **authors:** MarcFD, Nic, trbarry, Sh0dan, Graft and others
| **version:** 1.4.7
| **category:** MPEG Decoder (source) Plugins
| **download:** `<http://neuron2.net/dgmpgdec/dgmpgdec.html>`_
| **requirements:**
| **license:** GPL

--------

.. toctree::
    :maxdepth: 3


Description
-----------

DGDecode, part of the `DGMPGDec`_ package, is an MPEG-1/2 decoder plug-in
designed for AviSynth v2.5 or higher. It's able to decode any MPEG-1 or
MPEG-2 stream readable by DGIndex. Additional features include: YV12, I420,
and YUY2 colorspace output (and RGB24 via `DGVfapi`_), optimized iDCTs, post-
process deblocking and deringing, luminosity filtering, and more!

DGDecode is based on MPEG2Dec3 v1.10, which itself is based on MPEG2Dec2 from
the SourceForge project "save-oe".

Important Note: This filter was renamed to DGDecode to avoid naming
confusions and to clearly link it with neuron2's version of DVD2AVI called
DGIndex. Neuron2 wants to take great pains to acknowledge the origins of
DGDecode as described by MarcFD in the text below! Yes, neuron2 has continued
the evolution and made some fixes, but he stands on the shoulders of the
giants documented below in the Credits section.

| You can get the latest binaries and source code of DGMPGDec at
| http://neuron2.net/dgmpgdec/dgmpgdec.html

This is free software distributed under the terms of the GNU GPL v2 license.
You must agree to the terms of the license before using the plug-in or its
source code. Please see the License section for details.


Filter Syntax
-------------


MPEG2Source()
~~~~~~~~~~~~~

``MPEG2Source`` (string "d2v", int "idct", int "cpu", bool "iPP", int
"moderate_h", int "moderate_v", str "cpu2", bool "upConv", bool "iCC", bool
"i420", int "info", bool "showQ", bool "fastMC")

Although DGDecode can now decode both MPEG-1 and MPEG-2, this function is
still called MPEG2Source() for compatibility with existing scripts. You can
use it for both MPEG-1 and MPEG-2 streams.

*d2v*: "[PATH\]project.d2v"

| DGIndex Project File.
| Required parameter!
| Note 1: PATH can be ignored if "project.d2v" is in the same directory as your
  AviSynth (``*.avs``) script.

*idct*: 0 to 7 (default: 0)

| iDCT Algorithm.
| For more infomation on iDCTs please see Appendix B.
| Please see Appendix C for supported CPUs.

- 0: Use value specified by DGIndex (iDCTs 6 and 7 unavailable in DGIndex)
- 1: 32-bit MMX
- 2: 32-bit SSEMMX
- 3: 64-bit SSE2MMX
- 4: 32-bit Floating Point
- 5: 64-bit IEEE-1180 Reference
- 6: 32-bit SSEMMX (Skal)
- 7: 32-bit Simple MMX (XviD)

*cpu*: 0 to 6 (default: 0)

| Post-Processing Quickset Options.
| (Y=luma, C=chroma, H=horizontal, V=vertical)

- 0: DISABLE POST-PROCESSING
- 1: DEBLOCK_Y_H
- 2: DEBLOCK_Y_H, DEBLOCK_Y_V
- 3: DEBLOCK_Y_H, DEBLOCK_Y_V, DEBLOCK_C_H
- 4: DEBLOCK_Y_H, DEBLOCK_Y_V, DEBLOCK_C_H, DEBLOCK_C_V
- 5: DEBLOCK_Y_H, DEBLOCK_Y_V, DEBLOCK_C_H, DEBLOCK_C_V, DERING_Y
- 6: DEBLOCK_Y_H, DEBLOCK_Y_V, DEBLOCK_C_H, DEBLOCK_C_V, DERING_Y, DERING_C

*iPP*: true/false (default: auto)

| Field-Based Post-Processing.
| DGDecode automatically uses the PROGRESSIVE_FRAME flag to switch between
  field/frame based post-processing on a per-frame-basis.
| You should only specify the iPP parameter if you want to force DGDecode to
  use a particular post-processing mode.

- true: force field-based (interlaced) post-processing
- false: force frame-based (progressive) post-processing

*moderate_h*, *moderate_v*: 0 to 255 (default: moderate_h=20, moderate_v=40)

| Block Detection Sensitivity.
| (moderate_h=horizontal, moderate_v=vertical)
| Smaller values are stronger, use with care.

*cpu2*: (default: "")

| Post-Processing Custom Options.
| Specify a six character string of x's and o's according to list below. (case-
  insensitive)
| Each "x" enables the corresponding post-processing feature.
| Each "o" disables the corresponding post-processing feature.

- character 1: luma horizontal deblocking
- character 2: luma vertical deblocking
- character 3: chroma horizontal deblocking
- character 4: chroma vertical deblocking
- character 5: luma deringing
- character 6: chroma deringing

For example, to enable chroma-only post-processing use:

::

    MPEG2Source("project.d2v", cpu2="ooxxox")

*upConv*: true/false (default: false)

| Upconvert to YUY2 (4:2:2) from YV12 (4:2:0) based on the PROGRESSIVE_FRAME
  flag.
| Ignored if the input is not YV12 (4:2:0)
| Use AviSynth conversion filters to force a constant upsampling mode.

- true: Upconvert based on the PROGRESSIVE_FRAME flag
- false: Do not upconvert

*iCC*: true/false (default: auto)

| Upsampling Mode.
| DGDecode automatically uses the PROGRESSIVE_FRAME flag to switch between
  field/frame based upsampling on a *per-frame-basis*.
| You should *only* specify the iCC parameter if you want to force DGDecode to
  use a particular upsampling mode.

- [unspecified]: follow the PROGRESSIVE_FRAME flag
- true: force field-based (interlaced) upsampling
- false: force frame-based (progressive) upsampling

*i420*: true/false (default: false)

| Output I420 Colorspace.
| Possibly required by some legacy applications.
| Ignored if the input is not YV12 (4:2:0), or if upConv=true.

- true: Output I420
- false: Output YV12

*info*: 0 to 3 (default: 0)

Debug Information.

- 0: Do not generate debug information
- 1: Overlay debug information on the video
- 2: Output debug information via OutputDebugString()
- 3: Output hints in the video (as defined in utilities.cpp/utilities.h)

*showQ*: true/false (default: false)

Show Macroblock Quantizers.

- true: Show quantizers
- false: Do not show quantizers

*fastMC*: true/false (default: false)

| Vlad's Fast Motion Compensation.
| Very small speedup, but with degraded accuracy.
| For testing purposes, and may be removed in a future version.
| Requires SSE or 3DNow!, please see Appendix C for supported CPUs.


LumaYV12()
~~~~~~~~~~

``LumaYV12`` (clip, int "lumoff", float "lumgain")

| This function is unrelated to DGIndex's Luminance Filter.
| The transformation is: Y = (y * lumgain) + lumoff

LumaYV12() outputs a 0->255 YUV range, and not a CCIR-601 16->235 range. Use
the Avisynth built-in filter ColorYUV() instead if you need to enforce a
16->235 range. The functionality of LumaYV12() can be achieved using
ColorYUV(), which has more features, but LumaYV12() is optimized for speed in
performing basic luma adjustment.

*lumoff*: -255 to 255 (default: 0)

| Luminosity Offset.
| Adjust the luma of all pixels by a fixed amount.

*lumgain*: 0.0 to 2.0 (default: 1.0)

| Luminosity Gain.
| Adjust the luma of all pixels by a proportional amount.


BlindPP()
~~~~~~~~~

``BlindPP`` (clip, int "quant", int "cpu", bool "iPP", int "moderate_h", int
"moderate_v", string "cpu2")

| Deblock and/or Dering any video source.
| Requires YUY2 or YV12 input.
| Please see Appendix A for usage notes.

*quant*: 0 to 31 (default: 2)

| Emulated Quantizer.
| Specifies strength of the deblocking process.

*cpu*: 0 to 6 (default: 6)

Same function as in MPEG2Source(), but with different default value.

*iPP*: true/false (default: false)

| Same function as in MPEG2Source(), but with different default value.
| Automatic-mode is NOT available.

*moderate_h*, *moderate_v*, *cpu2*: (defaults: moderate_h=20, moderate_v=40,
cpu2="")

Same functions as in MPEG2Source(), but with different default values.


DeBlock()
~~~~~~~~~

``Deblock`` (clip, int "quant", int "aOffset", int "bOffset", bool "mmx",
bool "isse")

| Manao's H.264 Deblocking Filter. (v0.9.5)
| Requires YV12 input.
| Both (quant + aOffset) AND (quant + bOffset) must be >= 16 or the filter does
  nothing at all.

*quant*: 0 to 51 (default: 25)

| Emulated Quantizer.
| Specifies strength of the deblocking process.

*aOffset*: (default: 0)

| Modifier to the blocking detector threshold.
| Setting it higher means that more edges will be deblocked.

*bOffset*: (default: 0)

| Modifier for block detecting and for deblocking strength.
| There again, the higher, the stronger.

*mmx*: true/false (default: true)

Automatically disables if not supported by CPU.

- true: Enable MMX optimizations
- false: Disable MMX optimizations

*isse*: true/false (default: true)

| Automatically disables if not supported by CPU.
| Please see Appendix C for supported CPUs.

- true: Enable SSE optimizations
- false: Disable SSE optimizations


Usage Examples
--------------


AviSynth LoadPlugin() Example
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

AviSynth's LoadPlugin() must be called before any DGDecode functions can be used.
To accomplish this, add the following line to the beginning of your AviSynth (``*.avs``) script:
::

    LoadPlugin("[PATH\]DGDecode.dll")

Note: PATH can be ignored if DGDecode.dll is in the default AviSynth plug-in
directory, otherwise PATH must be specified.


MPEG2Source() Examples
~~~~~~~~~~~~~~~~~~~~~~

MPEG2Source() should be used only with MPEG-1 and MPEG-2 video sources.

To do plain YV12 decoding:

::

    MPEG2Source("[PATH\]project.d2v")

Note: PATH can be ignored if "project.d2v" is in the same directory as your
AviSynth (``*.avs``) script.

To do deblocking only:

::

    MPEG2Source("project.d2v", cpu=4)

To do deblocking on an interlaced source with increased vertical sensitivity:

::

    MPEG2Source("project.d2v", cpu=4, iPP=true, moderate_v=20)

To do deringing only:

::

    MPEG2Source("project.d2v", cpu2="ooooxx")

To select the optimized 32-bit SSE2 iDCT and also output I420 colorspace:

::

    MPEG2Source("project.d2v", idct=5, i420=true)

To convert to YUY2 based on the PROGRESSIVE_FRAME flag:

::

    MPEG2Source("project.d2v", upConv=true)

To do display onscreen information about the video:

::

    MPEG2Source("project.d2v", info=1)

LumaYV12() Examples
~~~~~~~~~~~~~~~~~~~

The following LumaYV12() examples are completely subjective, of course.
Adjust them to your liking.

To darken luminosity:

::

    MPEG2Source("project.d2v")
    LumaYV12(lumoff=-10, lumgain=0.9)

To lighten luminosity:

::

    MPEG2Source("project.d2v")
    LumaYV12(lumoff=10, lumgain=1.1)

BlindPP() Examples
~~~~~~~~~~~~~~~~~~

BlindPP() should not be used when the video is opened using the
MPEG2Source() function, because its postprocessing options will work better.
Typically BlindPP() is used when opening the video with AviSynth's
AviSource() or DirectShowSource().

To do default deblocking and deringing:

::

    AVISource("my_video.avi")
    BlindPP()

To do deblocking only:

::

    AVISource("my_video.avi")
    BlindPP(cpu=4)

To do deblocking on an interlaced source with increased horizontal
sensitivity:

::

    AVISource("my_video.avi")
    BlindPP(cpu=4, iPP=true, moderate_h=10)

To do stronger deblocking and deringing:

::

    DirectShowSource("my_video.mpg")
    BlindPP(quant=12)

To do deringing only:

::

    DirectShowSource("my_video.mpg")
    BlindPP(cpu2="ooooxx")

Deblock() Examples
~~~~~~~~~~~~~~~~~~

Deblock() should not be used with MPEG-2 video sources.
Typically this means opening the video with AviSynth's AviSource() or
DirectShowSource().

To do default deblocking:

::

    AVISource("my_video.avi")
    Deblock()

To do strong deblocking with increased sensitivity:

::

    DirectShowSource("my_video.mpg")
    Deblock(quant=32, aOffset=16, bOffset=24)

APPENDIX A: BlindPP() Notes
---------------------------

Blocks result from the 8x8-pixel DCT used by the MPEG encoder. So first, you
must be sure that the blocks that you want to deblock are still aligned at
8-pixel boundaries. That means no cropping and no resizing before you apply
BlindPP(). If your source is encoded interlaced, set iPP=true, if it's
progressive then the default is already correct (iPP=false).

The parameters are: quant, cpu2, moderate_h, and moderate_v.

*quant* specifies the overall strength at which the deblocking process is to
perform.

Set *cpu2* to "xxxxoo" for horizontal and vertical deblocking on luma and
chroma, to "xooxoo" for horizontal luma and vertical chroma deblocking, etc.
You get the idea.

*moderate_h* and *moderate_v* specify the horizontal and vertical sensitivities,
that is, where to perform deblocking, and where not to. They control the
sensitivity for recognizing that a block is present.

quant=2, moderate_h=35-45, moderate_v=45-55 will give you a very gentle
softening on strong, clearly visible blocks only. It will retain very much
detail and sharpness, but will also leave intact weaker blocks, and not
totally kill stronger ones.

quant=16, moderate_h=15-20, moderate_v=20-30 will perform rather strong
deblocking on almost anything that perhaps could be a block, but will also
smooth away a lot of detail and sharpness.

The rest is up to you, your taste, and your source material.

One other example... Since the excellent denoiser PixieDust() may sometimes
cause blocking by itself in moving areas, I sometimes do this:

::

    PixieDust(2)
    BlindPP(quant=8, cpu2="xxxxoo", moderate_h=45, moderate_v=55)

This takes away a good amount of the most visible blocking, if and only if
PixieDust() has produced some. On the remaining 99.8% where PixieDust()
didn't block, this will do almost nothing, as desired.


APPENDIX B: iDCT Algorithm Notes
--------------------------------

The FlasKMPEG readme file contains an excellent technical description of
iDCTs. It states:

> *"The video information inside MPEG files is stored in the frequency domain
rather than in the spatial domain (the images we see). That way, the
information gets compacted and that compaction can be used to compress
(reduce) the amount of information you have to send over the transmission
channel. MPEG uses the DCT (Discrete Cosine Transform) to translate spatial
information into frequency information. To bring back the spatial information
from the MPEG stream you have to apply the iDCT, that is, the Inverse
Discrete Cosine Transform, that undoes the DCT that was used during
encoding."*
>
> *"Although MPEG is almost deterministic (given a MPEG stream the output
should be identical in all decoders), the standard has a degree of freedom
when choosing the iDCT to use. That way, the decoder can be more easily
implemented depending on the hardware below it. What the standard requires
from the decoder is that the iDCT meets IEEE-1180 specs, or in plain words,
that the error from the iDCT doesn't go beyond that the ones pointed out in
the IEEE-1180."*

Which iDCT you should use depends primarily on what CPU you have and to a
lesser degree, on how accurate an iDCT you desire. Most people will not be
able to tell the difference in quality between these algorithms but they can
be easily observed by combining the AviSynth filters `Subtract`_ and
`Levels`_. All of the available options are IEEE-1180 compliant, except for
**SSE/MMX (Skal)**.

Qualitywise: **IEEE-1180 Reference** > **64-bit Floating Point** > **Simple MMX (XviD)** > Remaining iDCTs.

Speedwise: **SSE2/MMX** and **SSE/MMX (Skal)** are usually the fastest. The **IEEE-1180 Reference** is easily the slowest.


APPENDIX C: SIMD Instructions
-----------------------------

SIMD is an acronym for *Single Instruction*, *Multiple Data*. It is a term
that refers to a set of operations for efficiently handling large quantities
of data in parallel. This is especially productive for applications in which
video or audio files are processed. What usually required a repeated
succession of instructions can now be performed in one instruction.

There are seven different sets of SIMD instructions available to Intel and
AMD processors, but not every CPU supports all of these advanced instruction
sets. This is why many of DGDecode's functions provide options for specifying
which set of optimizations to use. The table below lists the SIMD
instructions supported by DGDecode, and the processors required utilize them.

*Table 1: SIMD Instructions supported by DGDecode*

+---------------------+----------------+---------------------------+----------------------+-----------+
|                     |  MMX           | 3DNow!                    | SSE                  | SSE2      |
+=====================+================+===========================+======================+===========+
| Required Intel CPUs | All Intel CPUs | Unsupported by Intel CPUs | Pentium 3, Pentium 4 | Pentium 4 |
+---------------------+----------------+---------------------------+----------------------+-----------+
| Required AMD CPUs   | All AMD CPUs   | All AMD CPUs              | Athlon XP, Athlon 64 | Athlon 64 |
+---------------------+----------------+---------------------------+----------------------+-----------+


+--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| Version History                                                                                                                                                                |
+================================================================================================================================================================================+
| **Based on MPEG2Dec2 (save-oe CVS 28.09.2002), and recast as MPEG2Dec3 (MarcFD).**                                                                                             |
+------------------------+----------+--------------------------------------------------------------------------------------------------------------------------------------------+
| beta versions (1 to 6) |          | - Added Nic's Post Processing with Field-Based PP                                                                                          |
|                        |          | - Overrided iDCT / luma filtering choice                                                                                                   |
|                        |          | - Fixed Luma filtering MMX code (3 bugs at least)                                                                                          |
|                        |          | - YV12->YUY2 Convertion optimised (+10 % speed)                                                                                            |
|                        |          | - a PP bug fixed. a bit slower now.                                                                                                        |
|                        |          | - trbarry's SSE2 optimisation disabled.                                                                                                    |
|                        |          | - Added showQ debugging trigger                                                                                                            |
|                        |          | - Added vlad's new MC (3dnow/ssemmx) / re-writed ssemmx                                                                                    |
|                        |          | - Added working MMX memory transfer for seeking (+3% speed)                                                                                |
|                        |          | - Added Interlaced Upsampling support                                                                                                      |
+------------------------+----------+--------------------------------------------------------------------------------------------------------------------------------------------+
| v0.9                   | 09.11.02 | - heavy code cleaning                                                                                                                      |
|                        |          | - redesigned the whole Avisynth interface                                                                                                  |
|                        |          | - YV12 support                                                                                                                             |
|                        |          | - RGB24 support                                                                                                                            |
|                        |          | - other misc stuff                                                                                                                         |
+------------------------+----------+--------------------------------------------------------------------------------------------------------------------------------------------+
| v0.91                  | 10.11.02 | - cleaned a bit more the source                                                                                                            |
|                        |          | - added MPEG2Dec3.def default settings loading (like don's filters)                                                                        |
|                        |          | - bff mode in SeparateFieldsYV12                                                                                                           |
+------------------------+----------+--------------------------------------------------------------------------------------------------------------------------------------------+
| v0.92                  | 17.11.02 | - code released                                                                                                                            |
|                        |          | - blindPP implemented                                                                                                                      |
+------------------------+----------+--------------------------------------------------------------------------------------------------------------------------------------------+
| v0.93                  | 25.11.02 | - total YV12 code convertion...                                                                                                            |
|                        |          | - ...who fixed YV12 bugs                                                                                                                   |
|                        |          | - less memory is needed                                                                                                                    |
|                        |          | - fast MMX copy (faster seeking)                                                                                                           |
+------------------------+----------+--------------------------------------------------------------------------------------------------------------------------------------------+
| v0.94                  | 08.12.02 | - very little bugfix                                                                                                                       |
+------------------------+----------+--------------------------------------------------------------------------------------------------------------------------------------------+
| v1.00                  | 19.01.03 | - final version                                                                                                                            |
|                        |          | - i squashed all bugs i were aware of                                                                                                      |
+------------------------+----------+--------------------------------------------------------------------------------------------------------------------------------------------+
| v1.01                  | unknown  | - trbarry: Fixed HDTV bug (0x21 PID hardcoded)                                                                                             |
+------------------------+----------+--------------------------------------------------------------------------------------------------------------------------------------------+
| v1.02                  | 12.05.03 | - Nic: aligned malloc done different                                                                                                       |
+------------------------+----------+--------------------------------------------------------------------------------------------------------------------------------------------+
| v1.03                  | 12.05.03 | - Nic: Now supports both DVD2AVI 1.77.3 D2V Files and 1.76 ones                                                                            |
+------------------------+----------+--------------------------------------------------------------------------------------------------------------------------------------------+
| v1.04                  | 12.05.03 | - Nic: Removed another memory leak, slightly quicker                                                                                       |
+------------------------+----------+--------------------------------------------------------------------------------------------------------------------------------------------+
| v1.05a                 | 12.05.03 | - trbarry: test version for optimisations                                                                                                  |
+------------------------+----------+--------------------------------------------------------------------------------------------------------------------------------------------+
| v1.06                  | 24.05.03 | - Nic: Added 2 new iDCT's Skal's (fastest!, idct=6) & SimpleiDCT (very accurate, idct=7)                                                   |
|                        |          | - Nic: Support for external use of MPEG2Dec3.dll without AviSynth added back in (See Source code for example.zip and GetPic example)       |
|                        |          | - trbarry: Added new Add_Block optimisations as well as optimised Block Decoding for SSE2 machines                                         |
|                        |          | - sh0dan: Uses AviSynth's fast BitBlt for mem copys where possible                                                                         |
|                        |          | - Nic: General optimisations :) Faster now on all machines tested.                                                                         |
+------------------------+----------+--------------------------------------------------------------------------------------------------------------------------------------------+
| v1.07                  | 06.06.03 | - Nic & Sh0dan: Bug Fixes, better stability on broken streams                                                                              |
+------------------------+----------+--------------------------------------------------------------------------------------------------------------------------------------------+
| v1.08                  | 08.06.03 | - trbarry: Optimised Simple_iDCT, lots faster now :)                                                                                       |
|                        |          | - Nic: added CPUCheck elsewhere, forgot to fix Lumafilter last time (Thanx ARDA!), robUx4 helped me make simple_idct into a fastcall       |
+------------------------+----------+--------------------------------------------------------------------------------------------------------------------------------------------+
| v1.09                  | 26.07.03 | - Nic: Now skal's Sparse iDCT is used instead for idct=6 (fastest!)                                                                        |
|                        |          | - Nic: Added the Luminance_Filter from DVD2AVI 1.77.3, for when Luminance_Filter is used in the .d2v                                       |
+------------------------+----------+--------------------------------------------------------------------------------------------------------------------------------------------+
| v1.10                  | 28.07.03 | - Nic: Damn! There was a problem with the Luminance filter and 1.77.3 D2V files. Fixed!                                                    |
+------------------------+----------+--------------------------------------------------------------------------------------------------------------------------------------------+
| **Recast as DGDecode (neuron2)**                                                                                                                                               |
+------------------------+----------+--------------------------------------------------------------------------------------------------------------------------------------------+
| v1.0.13                | 08.06.04 | - DG: Fixed DGDecode to not truncate B frames prior to the first P frame.                                                                  |
|                        |          | - DG: Fixed to not unconditionally reduce the frame count by two.                                                                          |
|                        |          | - DG: Rewrote the decoding and random access code to work correctly with the D2V files generated by the fixed DGIndex.                     |
|                        |          | - DG: Fixed DGDecode so that it (hopefully) no longer crashes VirtualDub on exit when the last frame is incomplete.                        |
|                        |          | - DG: Fixed the iDCT selection to not be global in DGDecode.                                                                               |
+------------------------+----------+--------------------------------------------------------------------------------------------------------------------------------------------+
| v1.1.0                 | 21.01.05 | - DG: Added workaround for crashes due to corrupted files.                                                                                 |
|                        |          | - DG: Added fix for files that change quantizer matrices on the fly.                                                                       |
|                        |          | - tritical: Fixed the DC mode decision of post-process deblocking.                                                                         |
|                        |          | - tritical: Added "info" option to mpeg2source() to display info on frames.                                                                |
|                        |          | - tritical: iPP now defaults to "auto". "auto" uses the PROGRESSIVE_FRAME flag to switches between field/frame based post-processing.      |
|                        |          | - tritical: Added 4:2:2 input support.                                                                                                     |
|                        |          | - tritical: Added "upConv" option to mpeg2source() to upsample to 4:2:2 from 4:2:0 based on the PROGRESSIVE_FRAME flag.                    |
|                        |          | - tritical: BlindPP now supports YUY2 colorspace.                                                                                          |
|                        |          | - tritical: Fixed BlindPP case syntax bug. (x vs X)                                                                                        |
|                        |          | - tritical: Fixed a small bug with showQ option.                                                                                           |
|                        |          | - tritical: Fixed small memory leak with FrameList/GOPList not being free'd.                                                               |
|                        |          | - tritical: Fixed a bug in the vertical chroma deblocking post-processing QP pointer being passed was incorrect for 4:2:0.                 |
|                        |          | - tritical: Faster MMX 4:2:2 to packed YUY2 and YUY2 to planar 4:2:2 conversions.                                                          |
|                        |          | - tritical: Fixed: "info" output would not work correctly if temporal_reference was not zero based at the beginning of GOPs.               |
+------------------------+----------+--------------------------------------------------------------------------------------------------------------------------------------------+
| v1.2.0                 | 17.02.05 | - DG: Revised "info" parameter.                                                                                                            |
|                        |          | - DG: Fixed bug in the deringing post-processing.                                                                                          |
|                        |          | - DG: Fixed crashing problem with BlindPP() for YUY2 input.                                                                                |
+------------------------+----------+--------------------------------------------------------------------------------------------------------------------------------------------+
| v1.2.1                 | 22.02.05 | - DG: Fixed crashing bug in BlindPP().                                                                                                     |
|                        |          | - DG: Fixed incorrect post-processing for YUY2.                                                                                            |
|                        |          | - DG: Changed DGDecode to deliver YV12 by default.                                                                                         |
|                        |          | - DG: Added i420 parameter to DGDecode.                                                                                                    |
|                        |          | - Manao: Added Deblock() blind H.264 deblocking filter.                                                                                    |
+------------------------+----------+--------------------------------------------------------------------------------------------------------------------------------------------+
| v1.3.0                 | 01.05.05 | - DG: Added exception handling so that DGDecode doesn't crash on an incomplete picture.                                                    |
|                        |          | - DG: Changed VFAPI upsampling method ("_P" before extension of D2V filename forces progressive upsampling; otherwise interlaced is used). |
+------------------------+----------+--------------------------------------------------------------------------------------------------------------------------------------------+
| v1.4.0                 | 15.06.05 | - DG: Added standard call wrappers for dgdecode.dll access functions for use by VB, etc. E.g.: getRGBFrame_SC().                           |
|                        |          | - DG: Fixed slice error recovery bug that caused some (rare) picture decoding errors.                                                      |
|                        |          | - DG: Program streams with video stream IDs other than 0xE0 are now supported.                                                             |
|                        |          | - DG: Fixed decoding errors due to improper recovery from corruption of transport streams.                                                 |
|                        |          | - DG: Added support for MPEG1 files.                                                                                                       |
|                        |          | - DG: Fixed a bug in transport parsing that could cause a mismatch between linear decoding and random frame access.                        |
|                        |          | - DG: Removed YV12toYUY2() and YV12toRGB24() as they duplicate native Avisynth filters.                                                    |
|                        |          | - ARDA: Replaced buggy LumaFilter() with reliable and optimized LumaYV12().                                                                |
|                        |          | - jackei: Fixed the Reference iDCT (type 4).                                                                                               |
|                        |          | - tritical: Fixed the pitch=width assumption, allowing proper operation under versions of Avisynth that modify the alignment rules.        |
|                        |          | - tritical: DGVfapi now supports multiple instantiation and can also open AVS scripts as well as D2V files.                                |
+------------------------+----------+--------------------------------------------------------------------------------------------------------------------------------------------+

Latest changes list is in "Changes.txt" file.


Credits
-------

| Mathias Born, author of MPEG2Dec
| Donald Graft ("neuron2"), frame loss fix, accurate indexing, PVA support, and more
| Peter Gubanov, author of the MMX/SSEMMX iDCT
| Chia-chen Kuo ("jackei"), author of DVD2AVI
| "MarcFD", YV12 support and more
| "Nic", post-processing and more
| Miha Peternel, author of the Floating Point and Reference iDCT
| Dmitry Rozhdestvensky, author of the SSE2 iDCT
| "sh0dan", code optimizations
| "Skal", for his SSEMMX iDCT
| "trbarry", transport parsing, and code optimizations
| "tritical", upsampling, info overlay, VFAPI enhancements, and lots of bug fixes
| "Manao", for his Deblock() filter
| "ARDA", for the LumaYV12() filter
| "Didée", for Appendix A: Notes on BlindPP() Usage
| "Cyberia", for Appendices B and C, and users manual modernization

$Date: 2007/09/22 21:28:24 $

.. _DGMPGDec: dgmpgdec_quickstart.rst
.. _DGVfapi: dgvfapi.rst
.. _Subtract: ../corefilters/subtract.rst
.. _Levels: ../corefilters/levels.rst

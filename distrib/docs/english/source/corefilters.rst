
Core Filters
============

Introduction
------------

The available (internal) filters are listed here and divided into categories.
A short description is added, including the supported color formats (and
sample types for the audio filters). There are some functions which combine
two or more clips in different ways. How the video content is calculated is
described for each function, but :doc:`here is a summary which explains which
properties that the resulting clip will have <filters_mult_input_clips>`.

.. _Media file filters:

Media file filters
------------------

These filters can be used to read or write media files. Usually they produce
source clips for processing. See debug filters for non-file source filters.

- `AVISource / OpenDMLSource / AVIFileSource`_ Opens an AVI file.
- `DirectShowSource`_ DirectShowSource reads filename using DirectShow
- `ImageReader / ImageSource / ImageSourceAnim`_ These filters produce a video
  clip by reading in still images or an animated image.
- `Imagewriter`_ Writes frames as images to your hard disk.
- `Import`_ Import an AviSynth script into the current script
- `SegmentedAVISource / SegmentedDirectShowSource`_ The SegmentedAVISource
  filter automatically loads up to 100 avi files per argument
- `SoundOut`_ SoundOut is a GUI driven sound output module for AviSynth (it
  exports audio to several compressors).
- `WAVSource`_ Opens a WAV file or the audio of an AVI file.

.. _Color conversion and adjustment filters:

Color conversion and adjustment filters
---------------------------------------

These filters can be used to change the color format or adjust the colors of
a clip.

- `ColorYUV`_ Adjusts colors and luma independently.
- `ConvertBackToYUY2 / ConvertToRGB / ConvertToRGB24 / ConvertToRGB32 /
  ConvertToYUY2 / ConvertToY8 / ConvertToYV411 / ConvertToYV12 / ConvertToYV16
  / ConvertToYV24`_ AviSynth can deal internally with the color formats, RGB24,
  RGB32, YUY2, Y8, YV411, YV12, YV16 and YV24. These filters convert between
  them.
- `FixLuminance`_ Correct shifting vertical luma offset
- `Greyscale`_ Converts a video to greyscale.
- `Invert`_ Inverts selected color channels of a video.
- `Levels`_ The Levels filter scales and clamps the blacklevel and whitelevel
  and adjusts the gamma.
- `Limiter`_ A filter for clipping levels to within CCIR-601 range.
- `MergeARGB / MergeRGB`_ This filter makes it possible to select and combine a
  color channel from each of the input videoclips.
- `Merge / MergeChroma / MergeLuma`_ This filter makes it possible to merge
  luma, chroma or both from a videoclip into another. There is an optional
  weighting, so a percentage between the two clips can be specified.
- `RGBAdjust`_ Adjust each color channel seperately.
- `ShowAlpha / ShowRed / ShowGreen / ShowBlue`_ Shows the selected channel of
  an (A)RGB clip.
- `SwapUV / UToY / UToY8 / VToY / VToY8 / YToUV`_ Swaps/copies chroma channels
  of a clip.
- `Subtract`_ Subtract produces an output clip in which every pixel is set
  according to the difference between the corresponding pixels
- `Tweak`_ Adjust the hue, saturation, brightness, and contrast.

.. _Overlay and Mask filters:

Overlay and Mask filters
------------------------

These filters can be used to layer clips with or without using masks and to
create masks.

- `ColorKeyMask`_ Sets the alpha-channel (similar as Mask does) but generates
  it by comparing the color.
- `Layer`_ Layering two videos.
- `Mask`_ Applies an alpha-mask to a clip.
- `MaskHS`_ Returns a mask (as Y8) of clip using a given hue and saturation
  range.
- `Overlay`_ Overlay puts two clips on top of each other with an optional
  displacement of the overlaying image, and using different overlay methods.
  Furthermore opacity can be adjusted for the overlay clip.
- `ResetMask`_ Applies an "all-opaque" alpha-mask to clip.

.. _Geometric deformation filters:

Geometric deformation filters
-----------------------------

These filters can be used to change image size, process borders or make other
deformations of a clip.

- `AddBorders`_ AddBorders adds black borders around the image.
- `Crop / CropBottom`_ Crop crops excess pixels off of each frame.
- `FlipHorizontal / FlipVertical`_ Flips the video upside-down or left-to-right
- `Letterbox`_ Letterbox simply blackens out the top and the bottom and
  optionally left and right side of each frame.
- `Overlay`_ Overlay puts two clips on top of each other with an optional
  displacement of the overlaying image, and using different overlay methods.
  Furthermore opacity can be adjusted for the overlay clip.
- `ReduceBy2 / HorizontalReduceBy2 / VerticalReduceBy2`_ ReduceBy2 reduces the
  size of each frame by half.
- `BilinearResize / BicubicResize / BlackmanResize / GaussResize /
  LanczosResize / Lanczos4Resize / PointResize / SincResize / Spline16Resize /
  Spline36Resize / Spline64Resize`_ The Resize filters rescale the input video
  frames to an arbitrary new resolution, using different sampling algorithms.
- `SkewRows`_ SkewRows skews the rows of a clip.
- `TurnLeft / TurnRight / Turn180`_ Rotates the clip 90 degrees counterclock
  wise / 90 degrees clock wise / 180 degrees.

.. _Pixel restoration filters:

Pixel restoration filters
-------------------------

These filters can be used for image detail (pixel) restoration (like
denoising, sharpening) of a clip.

- `Blur / Sharpen`_ These are simple 3x3-kernel blurring and sharpening
  filters.
- `GeneralConvolution`_ General 3x3 or 5x5 convolution matrix.
- `SpatialSoften / TemporalSoften`_ The SpatialSoften and TemporalSoften
  filters remove noise from a video clip by selectively blending pixels.
- `FixBrokenChromaUpsampling`_ I noticed that the MS DV codec upsamples the
  chroma channels incorrectly, and I added a FixBrokenChromaUpsampling filter
  to compensate for it.

.. _Timeline editing filters:

Timeline editing filters
------------------------

These filters can be used to arrange frames in time (clip cutting, splicing
and other editing).

- `AlignedSplice / UnalignedSplice`_ AlignedSplice and UnalignedSplice join two
  or more video clips end to end.
- `AssumeFPS / AssumeScaledFPS / ChangeFPS / ConvertFPS`_ Changes framerates in
  different ways.
- `DeleteFrame`_ DeleteFrame deletes a set of single frames, given as a number
  of arguments.
- `Dissolve`_ Dissolve is like AlignedSplice, except that the clips are
  combined with some overlap.
- `DuplicateFrame`_ DuplicateFrame duplicates a set of single frames, given as
  a number of arguments.
- `FadeIn0 / FadeOut0 / FadeIn / FadeOut / FadeIn2 / FadeOut2 / FadeIO0 /
  FadeIO / FadeIO2`_ FadeIn and FadeOut cause the video stream to fade linearly
  to black at the start or end.
- `FreezeFrame`_ The FreezeFrame filter replaces all the frames between first-
  frame and last-frame with a selected frame.
- `Interleave`_ Interleave interleaves frames from several clips on a frame-by-
  frame basis.
- `Loop`_ Loops the segment from start frame to end frame a given number of
  times.
- `Reverse`_ This filter makes a clip play in reverse.
- `SelectEven / SelectOdd`_ SelectEven makes an output video stream using only
  the even-numbered frames
- `SelectEvery`_ SelectEvery is a generalization of filters like SelectEven and
  Pulldown.
- `SelectRangeEvery`_ This filters selects a range of frames with a certain
  period.
- `Trim`_ Trims a video clip so that it includes only the frames first-frame
  through last-frame.

.. _Interlace filters:

Interlace filters
-----------------

These filters can be used for creating and processing field-based material
(which is frame-based material separated into fields). AviSynth is capable of
dealing with both progressive and interlaced material. The main problem is,
that it often doesn't know what it receives from source filters. This is the
reason that the field-based flag exists and can be used when dealing with
interlaced material. More information about field-based video can be found
`here`_.

- `AssumeFrameBased / AssumeFieldBased`_ Forces frame-based or field-based
  material.
- `AssumeTFF / AssumeBFF`_ Forces field order.
- `Bob`_ Bob takes a clip and bob-deinterlaces it
- `ComplementParity`_ Changes top fields to bottom fields and vice-versa.
- `DoubleWeave`_ The DoubleWeave filter operates like Weave, except that it
  produces double the number of frames by combining both the odd and even pairs
  of fields.
- `PeculiarBlend`_ This filter blends each frame with the following frame in a
  peculiar way.
- `Pulldown`_ The Pulldown filter simply selects two out of every five frames
  of the source video.
- `SeparateColumns / SeparateRows`_ Takes a clip and separates the columns or
  rows of each frame into new frames.
- `SeparateFields`_ SeparateFields takes a frame-based clip and splits each
  frame into its component top and bottom fields.
- `SwapFields`_ The SwapFields filter swaps the two fields in an interlaced
  frame
- `Weave`_ Weave takes even pairs of fields from a Fields Separated input video
  clip and combines them together to produce interlaced frames.
- `WeaveColumns / WeaveRows`_ Takes a clip and weaves sets of columns or rows
  together to produce a composite frames.

.. _Audio processing filters:

Audio processing filters
------------------------

These filters can be used to process audio. Audio samples from a clip will be
automatically converted if any filters requires a special type of sample.
This means that if a filter doesn't support the type of sample it is given,
it will automatically convert the samples to something it supports. The
internal formats supported in each filter is listed in the sample type
column. A specific sample type can be forced by using the `ConvertAudio`_
functions.

If the sample type is float, when AviSynth has to output the data, it will be
converted to 16 bit, since float cannot be passed as valid AVI data.

- `Amplify / AmplifydB`_ Amplify multiply audio samples by amount.
- `AssumeSampleRate`_ Adjusts the playback speed of the audio.
- `AudioDub / AudioDubEx`_ AudioDub takes the video stream from the first
  argument and the audio stream from the second argument and combines them.
  AudioDubEx is similar, but it doesn't throw an exception if both clips don't
  have a video or audio stream.
- `AudioTrim`_ Trims an audio clip so that it includes only the start_time
  through end_time.
- `ConvertToMono`_ Merges all audio channels.
- `ConvertAudioTo8bit / ConvertAudioTo16bit / ConvertAudioTo24bit /
  ConvertAudioTo32bit / ConvertAudioToFloat`_ Converts audio samples to 8, 16,
  24, 32 bits or float.
- `DelayAudio`_ DelayAudio delays the audio track by seconds seconds.
- `EnsureVBRMP3Sync`_ Corrects out-of-sync mp3-AVI's, when seeking ot trimming.
- `GetChannel`_ Returns a channel from an audio signal.
- `KillAudio`_ Removes the audio from a clip completely.
- `KillVideo`_ Removes the video from a clip completely.
- `MergeChannels`_ Merges channels of two or more audio clips.
- `MixAudio`_ Mixes audio from two clips.
- `Normalize`_ Amplifies the entire waveform as much as possible, without
  clipping.
- `ResampleAudio`_ Performs a change of the audio sample rate.
- `SSRC`_ Performs a high-quality change of the audio sample rate. It uses SSRC
  by Naoki Shibata, which offers the best resample quality available.
- `SuperEQ`_ High quality 16 band sound equalizer.
- `TimeStretch`_ This filter can change speed of the sound without changing the
  pitch, and change the pitch of a sound without changing the length of a
  sound.

.. _Meta filters:

Meta filters
------------

These special filters can be used to control other filters execution.

- `Animate / ApplyRange`_ Animate (ApplyRange) is a meta-filter which evaluates
  its parameter filter with continuously varying (the same) arguments.
- `TCPDeliver`_ This filter will enable you to send clips over your network.
  You can connect several clients to the same machine.

.. _Conditional filters:

Conditional filters
-------------------

The basic characteristic of conditional filters is that 'their scripts' are
evaluated (executed) at every frame instead of the whole clip. This allows
for complex video processing that would be difficult or impossible to be
performed by a normal AviSynth script.

- `ConditionalFilter / FrameEvaluate / ScriptClip / ConditionalSelect`_
  ConditionalFilter returns source1 if some condition is met, otherwise it
  returns source2. ScriptClip/FrameEvaluate returns the clip which is returned
  by the function evaluated on every frame. ConditionalSelect returns one frame
  from several sources based on an integer evaluator.
- `ConditionalReader`_ ConditionalReader allows you to import information from
  a text file, with different values for each frame - or a range of frames.
- `WriteFile / WriteFileIf / WriteFileStart / WriteFileEnd`_ These filters
  evaluate expressions and output the results to a text-file.

.. _Debug filters:

Debug filters
-------------

- `BlankClip / Blackness`_ The BlankClip filter produces a solid color, silent
  video clip of the specified length (in frames).
- `ColorBars / ColorBarsHD`_ The ColorBars filters produce a video clip
  containing SMPTE color bars scaled to any image size.
- `Compare`_ Compares two clips and prints out information about the
  differences.
- `Echo`_ Forces getframe calls to all input clips. Returns only first clip
  result.
- `Histogram`_ Adds a Histogram.
- `Info`_ Prints out image and sound information.
- `Preroll`_ Preroll the audio or video on non linear access.
- `MessageClip`_ MessageClip produces a clip containing a text message
- `ShowFiveVersions`_ ShowFiveVersions takes five video streams and combines
  them in a staggered arrangement from left to right.
- `ShowFrameNumber / ShowSMPTE / ShowTime`_ ShowFrameNumber draws text on every
  frame indicating what number Avisynth thinks it is.
  ShowSMPTE displays the SMPTE timecode. **hh:mm:ss:ff**
  ShowTime displays the duration with millisecond resolution. **hh:mm:ss.sss**
- `StackHorizontal / StackVertical`_ StackHorizontal takes two or more video
  clips and displays them together in left-to-right order.
- `Subtitle`_ The Subtitle filter adds a single line of anti-aliased text to a
  range of frames.
- `Tone`_ This will generate sound.
- `Version`_ The Version filter generates a video clip with a short version and
  copyright statement

$Date: 2013/01/06 13:38:34 $

.. _AVISource / OpenDMLSource / AVIFileSource:
    corefilters/avisource.rst
.. _WAVSource:
    corefilters/avisource.rst
.. _DirectShowSource: corefilters/directshowsource.rst
.. _ImageReader / ImageSource / ImageSourceAnim:
    corefilters/imagesource.rst
.. _Imagewriter: corefilters/imagewriter.rst
.. _Import: corefilters/import.rst
.. _SegmentedAVISource / SegmentedDirectShowSource:
    corefilters/segmentedsource.rst
.. _SoundOut: corefilters/soundout.rst
.. _ColorYUV: corefilters/coloryuv.rst
.. _ConvertBackToYUY2 / ConvertToRGB / ConvertToRGB24 / ConvertToRGB32
    / ConvertToYUY2 / ConvertToY8 / ConvertToYV411 / ConvertToYV12 /
    ConvertToYV16 / ConvertToYV24: corefilters/convert.rst
.. _FixLuminance: corefilters/fixluminance.rst
.. _Greyscale: corefilters/greyscale.rst
.. _Invert: corefilters/invert.rst
.. _Levels: corefilters/levels.rst
.. _Limiter: corefilters/limiter.rst
.. _MergeARGB / MergeRGB: corefilters/mergergb.rst
.. _Merge / MergeChroma / MergeLuma: corefilters/merge.rst
.. _RGBAdjust: corefilters/adjust.rst
.. _ShowAlpha / ShowRed / ShowGreen / ShowBlue: corefilters/showalpha.rst
.. _SwapUV / UToY / UToY8 / VToY / VToY8 / YToUV: corefilters/swap.rst
.. _Subtract: corefilters/subtract.rst
.. _Tweak: corefilters/tweak.rst
.. _Mask: corefilters/layer.rst
.. _ResetMask: corefilters/layer.rst
.. _Layer: corefilters/layer.rst
.. _ColorKeyMask: corefilters/layer.rst
.. _MaskHS: corefilters/maskhs.rst
.. _Overlay: corefilters/overlay.rst
.. _AddBorders: corefilters/addborders.rst
.. _Crop / CropBottom: corefilters/crop.rst
.. _FlipHorizontal / FlipVertical: corefilters/flip.rst
.. _Letterbox: corefilters/letterbox.rst
.. _ReduceBy2 / HorizontalReduceBy2 / VerticalReduceBy2:
    corefilters/reduceby2.rst
.. _BilinearResize / BicubicResize / BlackmanResize / GaussResize /
    LanczosResize / Lanczos4Resize / PointResize / SincResize /
    Spline16Resize / Spline36Resize / Spline64Resize: corefilters/resize.rst
.. _SkewRows: corefilters/skewrows.rst
.. _TurnLeft / TurnRight / Turn180: corefilters/turn.rst
.. _Blur / Sharpen: corefilters/blur.rst
.. _GeneralConvolution: corefilters/convolution.rst
.. _SpatialSoften / TemporalSoften: corefilters/soften.rst
.. _FixBrokenChromaUpsampling: corefilters/fixbrokenchromaupsampling.rst
.. _AlignedSplice / UnalignedSplice: corefilters/splice.rst
.. _AssumeFPS / AssumeScaledFPS / ChangeFPS / ConvertFPS:
    corefilters/fps.rst
.. _DeleteFrame: corefilters/deleteframe.rst
.. _Dissolve: corefilters/dissolve.rst
.. _DuplicateFrame: corefilters/duplicateframe.rst
.. _FadeIn0 / FadeOut0 / FadeIn / FadeOut / FadeIn2 / FadeOut2 /
    FadeIO0 / FadeIO / FadeIO2: corefilters/fade.rst
.. _FreezeFrame: corefilters/freezeframe.rst
.. _Interleave: corefilters/interleave.rst
.. _Loop: corefilters/loop.rst
.. _Reverse: corefilters/reverse.rst
.. _SelectEven / SelectOdd: corefilters/select.rst
.. _SelectEvery: corefilters/selectevery.rst
.. _SelectRangeEvery: corefilters/selectrangeevery.rst
.. _AudioTrim: corefilters/trim.rst
.. _Trim: corefilters/trim.rst
.. _here: advancedtopics/interlaced_fieldbased.rst
.. _AssumeFrameBased / AssumeFieldBased: corefilters/parity.rst
.. _AssumeTFF / AssumeBFF: corefilters/parity.rst
.. _ComplementParity: corefilters/parity.rst
.. _Bob: corefilters/bob.rst
.. _DoubleWeave: corefilters/doubleweave.rst
.. _PeculiarBlend: corefilters/peculiar.rst
.. _Pulldown: corefilters/pulldown.rst
.. _SeparateColumns / SeparateRows: corefilters/separatefields.rst
.. _SeparateFields: corefilters/separatefields.rst
.. _SwapFields: corefilters/swapfields.rst
.. _Weave: corefilters/weave.rst
.. _WeaveColumns / WeaveRows: corefilters/weave.rst
.. _ConvertAudio: corefilters/convertaudio.rst
.. _Amplify / AmplifydB: corefilters/amplify.rst
.. _AssumeSampleRate: corefilters/assumerate.rst
.. _AudioDub / AudioDubEx: corefilters/audiodub.rst
.. _ConvertToMono: corefilters/converttomono.rst
.. _ConvertAudioTo8bit / ConvertAudioTo16bit / ConvertAudioTo24bit /
    ConvertAudioTo32bit / ConvertAudioToFloat: corefilters/convertaudio.rst
.. _DelayAudio: corefilters/delayaudio.rst
.. _EnsureVBRMP3Sync: corefilters/ensuresync.rst
.. _GetChannel: corefilters/getchannel.rst
.. _KillAudio: corefilters/killaudio.rst
.. _KillVideo: corefilters/killaudio.rst
.. _MergeChannels: corefilters/mergechannels.rst
.. _MixAudio: corefilters/mixaudio.rst
.. _Normalize: corefilters/normalize.rst
.. _ResampleAudio: corefilters/resampleaudio.rst
.. _SSRC: corefilters/ssrc.rst
.. _SuperEQ: corefilters/supereq.rst
.. _TimeStretch: corefilters/timestretch.rst
.. _Animate / ApplyRange: corefilters/animate.rst
.. _TCPDeliver: corefilters/tcpdeliver.rst
.. _ConditionalFilter / FrameEvaluate / ScriptClip / ConditionalSelect:
    corefilters/conditionalfilter.rst
.. _ConditionalReader: corefilters/conditionalreader.rst
.. _WriteFile / WriteFileIf / WriteFileStart / WriteFileEnd:
    corefilters/write.rst
.. _BlankClip / Blackness: corefilters/blankclip.rst
.. _ColorBars / ColorBarsHD: corefilters/colorbars.rst
.. _Compare: corefilters/compare.rst
.. _Echo: corefilters/echo.rst
.. _Histogram: corefilters/histogram.rst
.. _Info: corefilters/info.rst
.. _Preroll: corefilters/preroll.rst
.. _MessageClip: corefilters/message.rst
.. _ShowFiveVersions: corefilters/showfive.rst
.. _ShowFrameNumber / ShowSMPTE / ShowTime: corefilters/showframes.rst
.. _StackHorizontal / StackVertical: corefilters/stack.rst
.. _Subtitle: corefilters/subtitle.rst
.. _Tone: corefilters/tone.rst
.. _Version: corefilters/version.rst


VideoInfo
=========

VideoInfo provides basic information about the clip your filter receives.


Getting information from VideoInfo
----------------------------------


Video-related information
~~~~~~~~~~~~~~~~~~~~~~~~~

::

    bool HasVideo();

This will return true if there is any video in the given clip.

::

    bool IsRGB24();
    bool IsRGB32();
    bool IsRGB();

This will return true if the colorspace is `RGB`_ (in any way). The first two
return true if the clip has the specific RGB colorspace (:doc:`RGB24 <ColorSpaces>` and
:doc:`RGB32 <ColorSpaces>`). The third returns true for any RGB colorspace; future formats
could also apply.

::

    bool IsYUY2();
    bool IsYV12();
    bool IsYUV();

This will return true if the colorspace is :doc:`YUV <ColorSpaces>` (in any way). The first two
return true if the clip has the specific YUV colorspace (:doc:`YUY2 <ColorSpaces>` and
:doc:`YV12 <ColorSpaces>`). The third returns true for any YUV colorspace; future formats could
also apply. Note that I420 is also reported as YV12, because planes are
automatically swapped.

::

    bool IsColorSpace(int c_space);

This function will check if the colorspace (VideoInfo.pixel_type) is the same
as given c_space (or more general it checks for a :doc:`Colorspace property <ColorspaceProperties>` (see
avisynth.h)).

::

    bool IsSameColorspace(const VideoInfo& vi2);

This function will compare two VideoInfos, and check if the colorspace is the
same. Note: It does not check imagesize or similar properties.

::

    bool Is(int property);

This function is reserved for future use. Currently works as IsColorSpace.

::

    bool IsPlanar();

This will return true if the video is planar. For now only YV12 returns true,
but future formats might also do so. See the :doc:`Planar <PlanarImageFormat>` image format.

::

    bool IsFieldBased();

This will return true if the video has been through a :doc:`SeparateFields <../corefilters/separatefields>`, and
the video has not been :doc:`weaved <../corefilters/weave>` yet. Otherwise it will return false.

::

    bool IsParityKnown();

This will return true if the video parity is known.

::

    bool IsBFF();
    bool IsTFF();

This will return true if the video is bottom-field-first or top-field-first
respectively.

::

    void SetFieldBased(bool isfieldbased);

This will set the field-based property to true (respectively false) if
isfieldbased=true (respectively false).

::

    void Set(int property);
    void Clear(int property);

This sets respectively clears an image_type property like: IT_BFF, IT_TFF or
IT_FIELDBASED. See field.h for examples.

::

    int BitsPerPixel();

This will return the number of bits per pixel. This can be:

+------------------+------------+
| pixel_type       | nr of bits |
+==================+============+
| CS_BGR24         | 24         |
+------------------+------------+
| CS_BGR32         | 32         |
+------------------+------------+
| CS_YUY2          | 16         |
+------------------+------------+
| CS_YV12, CS_I420 | 12         |
+------------------+------------+


::

    void SetFPS(unsigned numerator, unsigned denominator);

This will set the framerate.

::

    void MulDivFPS(unsigned multiplier, unsigned divisor);

This will multiply the denominator by *multiplier* and scale the numerator
and modified denominator.

There is some other useful information in VideoInfo structure (width, height,
fps_numerator, fps_denominator, num_frames, pixel_type and image_type). See
'avisynth.h' header file.


Audio-related information
~~~~~~~~~~~~~~~~~~~~~~~~~

::

    bool HasAudio();

This will return true if there is any audio in the given clip.

::

    int AudioChannels();

This will return the number of audio channels.

::

    int SampleType();

This will return the sampletype. This can be:

+--------------+------------+
| pixel_type   | nr of bits |
+==============+============+
| SAMPLE_INT8  | 1<<0       |
+--------------+------------+
| SAMPLE_INT16 | 1<<1       |
+--------------+------------+
| SAMPLE_INT24 | 1<<2       |
+--------------+------------+
| SAMPLE_INT32 | 1<<3       |
+--------------+------------+
| SAMPLE_FLOAT | 1<<4       |
+--------------+------------+


::

    bool IsSampleType(int testtype);

This function will check if the sampletype (VideoInfo.sample_type) is the
same as testtype.

::

    int SamplesPerSecond();

This will return the number of bytes per second.

::

    int BytesPerAudioSample();

This will return the number of bytes per sample:

::

    int BytesPerChannelSample()

This will return the number of bytes per channel-sample. This can be:

+--------------+----------------------+
| sample       | nr of bytes          |
+==============+======================+
| SAMPLE_INT8  | sizeof(signed char)  |
+--------------+----------------------+
| SAMPLE_INT16 | sizeof(signed short) |
+--------------+----------------------+
| SAMPLE_INT24 | 3                    |
+--------------+----------------------+
| SAMPLE_INT32 | sizeof(signed int)   |
+--------------+----------------------+
| SAMPLE_FLOAT | sizeof(SFLOAT)       |
+--------------+----------------------+


::

    __int64 AudioSamplesFromFrames(__int64 frames);

This returns the number of audiosamples from the first *frames* frames.

::

    int FramesFromAudioSamples(__int64 samples);

This returns the number of frames from the first *samples* audiosamples.

::

    __int64 AudioSamplesFromBytes(__int64 bytes);

This returns the number of audiosamples from the first *bytes* bytes.

::

    __int64 BytesFromAudioSamples(__int64 samples);

This returns the number of bytes from the first *samples* audiosamples.

There is some other useful information in VideoInfo structure
(audio_samples_per_second, sample_type, num_audio_samples and nchannels). See
'avisynth.h' header file.

----

Back to :doc:`FilterSDK`

$Date: 2014/10/27 22:04:54 $

.. _RGB: http://avisynth.org/mediawiki/RGB

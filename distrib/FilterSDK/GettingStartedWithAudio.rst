
`Getting started with audio`_
=============================

The best filters to take a look at if you are searching for a way to get
started with an audio filter is the `internal audio filters`_ of AviSynth.
Mainly `audio.cpp`_ is interesting.

Basically you override GetAudio(...) instead of GetFrame, and fill the buffer
with data. A simple filter could look like this:


Filter creation - skip if no audio:
:::::::::::::::::::::::::::::::::::

::

    AVSValue __cdecl HalfVolume::Create(AVSValue args, void*,
    IScriptEnvironment* env) {
      if (!args[0].AsClip()->GetVideoInfo().AudioChannels())
        return args[0];

      return new HalfVolume(args[0].AsClip());
    }

Constructor
-----------

::

    HalfVolume::HalfVolume(PClip _child)
        : GenericVideoFilter(ConvertAudio::Create(_child,
        SAMPLE_INT16 | SAMPLE_FLOAT, SAMPLE_FLOAT)) {
    }


This is a bit tricky. It'll require you to include `ConvertAudio.cpp`_.
What it does is automatic sample type conversion. Basically what it does is
that you tell that your filter supports SAMPLE_INT16 and SAMPLE_FLOAT, and
that it prefers SAMPLE_FLOAT. If the input isn't 16 bit or float, it'll be
converted to float.


GetAudio override
-----------------

::

    void __stdcall HalfVolume::GetAudio(void* buf, __int64 start, __int64
    count, IScriptEnvironment* env) {
      child->GetAudio(buf, start, count, env);
      int channels = vi.AudioChannels();

      if (vi.SampleType() == SAMPLE_INT16) {
        short* samples = (short*)buf;
        for (int i=0; i< count; i++) {
          for(int j=0;j< channels;j++) {
             samples[i*channels+j] /= 2;
          }
        }
      } else if (vi.SampleType() == SAMPLE_FLOAT) {
        SFLOAT* samples = (SFLOAT*)buf;
        for (int i=0; i< count; i++) {
          for(int j=0;j< channels;j++) {
             samples[i*channels+j] /= 2.0f;
          }
        }
      }
    }


Implementation of a half volume filter. Very explicit, so it isn't going to
be the fastest possible, but it should serve the purpose. Furthermore have a
look `discussion here`_ and look also at `audio.cpp`_ for a bunch of more
advanced stuff. A lot of technical details are also to be found in `AviSynth Two-Five Audio`_.

$Date: 2007/07/04 00:12:07 $

.. _internal audio filters:
    http://avisynth2.cvs.sourceforge.net/avisynth2/avisynth/src/audio/
.. _audio.cpp: http://avisynth2.cvs.sourceforge.net/avisynth2/avisynth/src/audio/audio.cpp?view=markup
.. _ConvertAudio.cpp: http://avisynth2.cvs.sourceforge.net/avisynth2/avisynth/src/audio/convertaudio.cpp?view=markup
.. _discussion here: http://forum.doom9.org/showthread.php?s=&threadid=72760&highlight=ConvertAudiohere
.. _AviSynth Two-Five Audio: AviSynthTwoFiveAudio.rst
.. _Getting started with audio:
    http://www.avisynth.org/GettingStartedWithAudio

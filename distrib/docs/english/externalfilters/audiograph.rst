
AudioGraph
==========


Abstract
--------

| **author:** Richard Ling (modified by Sh0dan)
| **version:**
| **download:** `<http://www.avisynth.org/warpenterprises/>`_
| **category:** Misc Plugins
| **requirements:** YUY2 & RGB Colorspace

--------

| AudioGraph filter for AviSynth (2.5)
| Richard Ling - r.ling(at)eudoramail.com

This filter displays the audio waveform for a video, superimposed on the
video. It is mainly intended to help during editing rather than for final
output. It can be useful for finding and isolating specific sequences of
dialogue or sound, and for checking that overdubbed audio (especially speech)
is in sync with video.

The audio is displayed as a green waveform stretching from left to right
across the frame. The filter can graph the audio for the currently visible
frame only; or it can include the audio for several successive frames on
either side of the current frame. Graphing several frames makes it easier to
find a sound of interest. It is also really cool to watch the waveform
scrolling across the video as the video plays :-)


USAGE
-----

``AudioGraph`` (clip, int frames_either_side)

*Parameters:*

**clip**: The source clip. YUY2, RGB24 or RGB32 video, with 8-bit or 16-bit
mono or stereo audio.

**frames_either_side**: The number of frames, either side of the current
frame, which should be graphed.

The effect of the frames_either_side parameter is perhaps better explained by
this table:

| *value effect*:
| 0 only audio for the currently visible frame is graphed.
| 1 audio for the preceding, current, and following frames are graphed.
| 2 audio for the preceding 2 frames, current frame, and following 2 frames
    are graphed.
| ...and so on.

The current frame's audio is displayed in the centre of the video frame in
bright green, while audio for preceding and following frames is displayed in
darker green.


EXAMPLE
-------

The following .avs file creates a video from a WAV file. Just replace the
WAVSource filename with one existing on your system. You can also adjust the
length passed to BlankClip to match the duration of your WAV file.

::

    LoadPlugin("audgraph.dll")
    audio = WAVSource("sample.wav")
    return AudioGraph(AudioDub(BlankClip(1000), audio), 20)

Coverted to AviSynth 2.5 by Klaus Post.
---------------------------------------

- No YV12 support.
- Should support multiple channels.
- YUY2 mode made a bit more eyepleasing, IMO.
- It makes the graph a bit more blocky.
- YUY2 mode is converted to greyscale.

$Date: 2004/08/13 21:57:25 $


TMonitor
========


Abstract
--------

| **author:** tritical
| **version:** 0.9
| **download:** `<http://bengal.missouri.edu/~kes25c/>`_
| **category:** Misc Plugins
| **requirements:**

-   YV12 & YUY2 & RGB Colorspace

**license:** GPL

--------


Description
-----------

TMonitor is a filter very similar to AVSMon. It enables monitoring of an
AviSynth clip via previewing the video, viewing clip information (such as
video width, height, colorspace, number of frames, audio samples, sample
rate, number of audio channels, and more), and adjusting the audio delay. It
also supports multiple instances per script, allowing viewing of differences
between different parts of a processing chain.


Syntax
~~~~~~

``TMonitor`` (clip, string "name", int "everyP", int "everyD", float "delay",
bool "esync", int "range", int "offset", bool "audio")


PARAMETERS
~~~~~~~~~~

*name* -

This is an optional string you can use to identify the current instance of
TMonitor. This name will show up in the names of the windows as well as in
the bottom left of the main dialog window. The name is limited to a maximum
of 20 characters. If not given, then the name is set as the current instance
number.

- default

  - "" (string)

*everyP*,  *everyD* -

These set the update intervals. everyP controls how often the video preview
window is updated. everyD controls how often the extended info dialog is
updated. The intervals are in # of frames and can be set to the following
values - 0 (never), 1, 5, 10, 20, 50, 100, 200, 500, 1000. These values can
be changed/set via the gui as well.

- default

  - everyP - 1 (int)
  - everyD - 0

*delay* -

Audio delay in seconds. This can also be adjusted via the gui. The only
restriction is that this must be within the range defined via the
offset/range settings.

- default

  - 0.0 (float)

*esync* -

This option will ensure sync is accurately maintained when adjusting the
audio delay and seeking in files with vbr mp3 audio. It works in the same way
as EnsureVBRMP3Sync(), and thus can cause long pauses at times due to having
to read from the beginning back up to the current position. Because of this,
it is recommended to first decode the audio to wav and use the wav as input
if adjusting the audio delay for files with vbr audio.

- default

  - false (bool)

*range*,  *offset* -

These control the audio delay slider in the main dialog window. The range
setting sets the maximum adjustment in milliseconds one way or the other
(forward or backward). By default this value is 1100, which means you would
be able to delay the audio up to 1.1 seconds or advance the audio up to 1.1
seconds. If you instead set this to 5000, the slider would allow delaying and
advancing up to 5 seconds. The maximum value for range is 20000 (20 seconds)
and the minimum is 1.

offset sets the offset (in ms) from the 0 ms delay point at which the range
is centered. Meaning if you set offset to 5000 and range to 1000, then the
delay slider on the gui would allowing adjusting the delay between 4 and 6
seconds. By combining range and offset you can accurately adjust the overall
delay more easily. offset can be set to anything both positive (delaying) and
negative (advancing).

- default

  - range - 1100 (int)
  - offset - 0 (int)

*audio* -

This allows forcing the audio slider and related audio controls on the main
dialog to be disabled even if the clip has audio. This is to avoid possibly
altering the delay value when you don't want to. true enables the controls if
audio is present, false disables the controls.

- default

  - true (bool)

$Date: 2005/07/10 16:11:01 $

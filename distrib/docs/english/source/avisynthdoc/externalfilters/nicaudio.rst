
NicAudio
========


Abstract
--------

| **author:** Nic
| **version:** 1.1
| **download:** `<http://www.avisynth.org/warpenterprises/>`_
| **category:** Audio Decoder (source) Plugins
| **requirements:**
| **license:** GPL

--------


Description
-----------

Audio Plugins for MPEG Audio/AC3/DTS/LPCM. NicLPCMSource expects raw LPCM
files or LPCM WAV files. However, at present it only supports 2-channel LPCM
WAV files.


Syntax
~~~~~~

| AC3: NicAC3Source ("filename.ac3", int "downmix")
| DTS: NicDTSSource ("filename.dts", int "downmix")
| MPA: NicMPASource ("filename.mpa", int "downmix")
| LPCM: NicLPCMSource ("filename.lpcm", int "sampleRate", int "sampleBits", int
  "channels")


PARAMETERS
~~~~~~~~~~

Note: downmix specifies the maximum number of channels to output. This is the
only optional parameter.

Example:

::

    LoadPlugin("NicAudio.dll")
    NicAC3Source("c:\File.AC3")

or

::

    NicLPCMSource("c:\File.lpcm", 48000, 16, 2)

Filters compiled and gathered together by Nic. All under the GPL.

All credit should go to the excellent creator of these original filters used
in FilmShrink.sf.net - Attila T. Afra.

$Date: 2005/07/10 16:11:01 $


This page describes SetChannelMask, GetChannelMask and IsChannelMaskKnown

GetChannelMask
==============

Returns the actual channel layout mask. If layout mask is not set, returns 0.

::

    int GetChannelMask (clip)

.. describe:: clip

    Source clip.


IsChannelMaskKnown
==================

Returns true, if the channel layout mask is set, false otherwise.

::

    bool IsChannelMaskKnown (clip)

.. describe:: clip

    Source clip.


SetChannelMask (int version)
============================

Sets the actual channel layout mask.


::

    SetChannelMask (clip , bool IsChannelMaskKnown, int dwChannelMask)

.. describe:: clip

    Source clip of which these properties must be set.

.. describe:: IsChannelMaskKnown

    - true: Channel mask is valid. dwChannelMask parameter will be set.
    - false: makes channel mask info invalid. Also sets the mask to 0 internally.

    Default: false

.. describe:: dwChannelMask

    Channel mask is an integer number, which describes the speaker (audio layout) 
    configuration of the clip. The number of '1' bits in mask must match with the number
    of audio channels.
    The lowest 18 bit is used, as defined in WAVE_FORMAT_EXTENSIBLE dwChannelMask definitions 
    (https://learn.microsoft.com/en-us/windows/win32/api/mmreg/ns-mmreg-waveformatextensible)
    As a special case $80000000 value (32 bit msb bit is 1) implies "speaker all" setting.
    (for developers: these bits are specified in avisynth.h as well)

    Default: 0

::

     enum AvsChannelMask {
           MASK_SPEAKER_FRONT_LEFT = 0x1,
           MASK_SPEAKER_FRONT_RIGHT = 0x2,
           MASK_SPEAKER_FRONT_CENTER = 0x4,
           MASK_SPEAKER_LOW_FREQUENCY = 0x8,
           MASK_SPEAKER_BACK_LEFT = 0x10,
           MASK_SPEAKER_BACK_RIGHT = 0x20,
           MASK_SPEAKER_FRONT_LEFT_OF_CENTER = 0x40,
           MASK_SPEAKER_FRONT_RIGHT_OF_CENTER = 0x80,
           MASK_SPEAKER_BACK_CENTER = 0x100,
           MASK_SPEAKER_SIDE_LEFT = 0x200,
           MASK_SPEAKER_SIDE_RIGHT = 0x400,
           MASK_SPEAKER_TOP_CENTER = 0x800,
           MASK_SPEAKER_TOP_FRONT_LEFT = 0x1000,
           MASK_SPEAKER_TOP_FRONT_CENTER = 0x2000,
           MASK_SPEAKER_TOP_FRONT_RIGHT = 0x4000,
           MASK_SPEAKER_TOP_BACK_LEFT = 0x8000,
           MASK_SPEAKER_TOP_BACK_CENTER = 0x10000,
           MASK_SPEAKER_TOP_BACK_RIGHT = 0x20000,
           MASK_SPEAKER_ALL = 0x80000000
     }
     


SetChannelMask (string version)
===============================

::

    SetChannelMask (clip , string ChannelMask)

.. describe:: clip

    Source clip of which this property must be set.

.. describe:: ChannelMask

    Channel mask is a string, which can contain one or more 

    - predefined, frequently used layout name,
    - speaker position name
    - a number followed by 'c' meaning a default layout for a given channel count
    - or a direct decimal mask value 

    They can be set alone or combined with the plus sign (+).

    String is case sensitive!

    If ChannelMask is "" (empty) then the layout will be marked as 'not set': same 
    as SetChannelMask(clip, false, 0)

    Default: ""


Examples
--------

These are the same:

::

    SetChannelMask("mono")
    SetChannelMask("FC")

These are the same:

::

    SetChannelMask("stereo")
    SetChannelMask("FL+FR")

These are the same:

::

    SetChannelMask("stereo+LFE")
    SetChannelMask("2.1")

Sets the default choice for 2 channels (which is "stereo"):

::

    SetChannelMask("2c")

Sets the default choice for 6 channels (which is "5.1"):

::

    SetChannelMask("6c")

Sets the exact mask or 3=1+2, that is "FL+FR" = "stereo":

::

    SetChannelMask("3")

Set layout info as invalid:

::

    SetChannelMask(false, 0)
    SetChannelMask("")

Set layout info to exact number, then the same with the string variant:

::

    SetChannelMask(true, 1+2+8)
    SetChannelMask("FL+FR+LFE")



Individual Speaker Channels
---------------------------

    | "FL"  front left
    | "FR"  front right
    | "FC"  front center
    | "LFE" low frequency
    | "BL"  back left
    | "BR"  back right
    | "FLC" front left-of-center
    | "FRC" front right-of-center
    | "BC"  back center
    | "SL"  side left
    | "SR"  side right
    | "TC"  top center
    | "TFL" top front left
    | "TFC" top front center
    | "TFR" top front right
    | "TBL" top back left
    | "TBC" top back center
    | "TBR" top back right


Predefined frequently used channel layouts
------------------------------------------

    | "mono"
    | "stereo"
    | "2.1"
    | "3.0"
    | "3.0(back)"
    | "4.0"
    | "quad"
    | "quad(side)"
    | "3.1"
    | "5.0"
    | "5.0(side)"
    | "4.1"
    | "5.1"
    | "5.1(side)"
    | "6.0"
    | "6.0(front)"
    | "hexagonal"
    | "6.1"
    | "6.1(back)"
    | "6.1(front)"
    | "7.0"
    | "7.0(front)"
    | "7.1"
    | "7.1(wide)"
    | "7.1(wide-side)"
    | "7.1(top)"
    | "octagonal"
    | "cube"
    | "speaker_all"


$Date: 2023/03/21 15:41:00 $

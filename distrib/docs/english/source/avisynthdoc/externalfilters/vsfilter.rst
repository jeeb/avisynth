
VSFilter (TextSub & VobSub)
===========================


Abstract
--------

| **author:** Gabest
| **version:** 2.32
| **download:** `guliverkli`_
| **category:** Subtitle (source) Plugins
| **requirements:**

--------


Description
-----------

The format of the subtitles can be ``*.sub``, ``*.srt``, ``*.ssa``, ``*.ass``, etc. (ssa =
Sub Station Alpha).

**syntax:**
::

    TextSub("C:\My Folder\MY subFile.XXX")
    or
    VobSub("C:\My Folder\MY subFile.XXX")

**examples:**
::

    mpeg2source("F:\From_hell\from_hell.d2v")
    VobSub("F:\From_hell\VTS_01_0.sub")

    mpeg2source("F:\From_hell\from_hell.d2v")
    TextSub("F:\From_hell\fh_ned.srt")

**bug:** You will see a small rectangle where the subs are put (if you look
closely). More info can be found `here`_.

**remark:** The package contains two dll's. The Unicode release can be used
with W2k or higher, the other one is for W98 users (W98 doesn't support
unicode natively).


More information (by Ernst Peché)
---------------------------------


SubRip
~~~~~~

Subtitles on a DVD are saved as small pictures, not as text - but the TextSub
filter needs a textfile.
They are all stored inside the VOB file, each language with a separate
stream, you have to select the desired stream first.
Then you convert these pictures to plain text with the OCR (optical character
recognition) of this tool.
You have to teach the program each character once (you can save these maps
for later use) - it works really good, so you won't spend that much time.

There are many subtitle formats, but most do not contain formatting
information.
So it is a good choice to use the "SSA" format, which contains the format in
the file, too.


VSFilter.DLL
~~~~~~~~~~~~

This contains the AviSynth ``TextSub`` function, which basically takes a
textfile as input and paints the subtitles from this textfile on the video
clip. It has the following quite simple syntax:

``TextSub`` ("path\filename.ext" [,  charset [,  fps]])

So typically you only write

::

    TextSub ("your_file.ssa")

Besides SSA (Sub Station Alpha) TextSub can deal with SRT (SubRip), SUB
(MicroDVD), PSB (PowerDivx), SMI (SAMI), ASS (Advanced Substation Alpha).

To override the default style you can use a second file named e.g.
"your_file.ssa.style" (the first file + ".style") which must be in SSA or ASS
syntax an contains only formatting information (makes only sense when NOT
using SSA/ASS).

Most SSA commands are ignored, as they are not really useful here.


SubResync
~~~~~~~~~

If you want to adjust the format of your subtitles (color, position and so
on) you can do it with this tool - of course you can simply edit the SSA file
if you know the syntax.


SSA syntax
~~~~~~~~~~

If you want to set the format manually here is the basic SSA syntax.
You must strictly obey the syntax else the line or the whole file will be
rejected.
Those entries which are definitely used by TextSub are written bold.

A line beginning with a ";" (semicolon) is treated as a comment line.

Each line starts with a line desriptor, which describes the entry type.

First a little example:

::

    [Script Info]
    ; This is a Sub Station Alpha v4 script.
    ;
    ScriptType: v4.00
    Collisions: Normal
    PlayResX: 720
    PlayResY: 576
    Timer: 100.0000

    [V4 Styles]
    Format: Name, Fontname, Fontsize, PrimaryColour, \
            SecondaryColour, TertiaryColour, BackColour, Bold, Italic, BorderStyle, Outline, \
            Shadow, Alignment, MarginL, MarginR, MarginV, AlphaLevel, Encoding
    Style: Style1,Arial,32,&Hffffff,&H000000,&H404040,&H404040,0,0,1,2,0,2,30,30,40,0,0
    Style: Default,Arial,18,&Hffffff,&H00ffff,&H000000,&H000000,-1,0,1,2,3,2,20,20,20,0,1

    [Events]
    Format: Marked, Start, End, Style, Name, MarginL, MarginR, MarginV, Effect, Text
    Dialogue: Marked=0,0:02:12.69,0:02:13.92,Style1,Comment,0000,0000,0000,,San Francisco, 1876
    Dialogue: Marked=0,0:07:23.37,0:07:24.96,Style1,Comment,0000,0000,0000,,Er ist unverschämt.
    Dialogue: Marked=0,0:07:25.13,0:07:28.28,Style1,Comment,0000,0000,0000,,So ist es hier.\NDas Land einfacher Krämerseelen.

There are (typically three) sections in the file:


Script Info
:::::::::::

This section contains headers and general information about the script.
The line that says "[Script Info]" must be the first line in a v4 script.

-   **Script Type**: This is the SSA script format version eg. "V4.00". It is
    used by SSA to give a warning if you are using a version of SSA older
    than the version that created the script. ASS version is "V4.00+".

-   **Collisions**: This determines how subtitles are moved, when
    automatically preventing onscreen collisions.If the entry says "Normal"
    then SSA will attempt to position subtitles in the position specified by
    the "margins". However, subtitles can be shifted vertically to prevent
    onscreen collisions. With "normal" collision prevention, the subtitles
    will "stack up" one above the other - but they will always be positioned
    as close the vertical (bottom) margin as possible - filling in "gaps" in
    other subtitles if one large enough is available. If the entry says
    "Reverse" then subtitles will be shifted upwards to make room for
    subsequent overlapping subtitles. This means the subtitles can nearly
    always be read top-down - but it also means that the first subtitle can
    appear half way up the screen before the subsequent overlapping subtitles
    appear. It can use a lot of screen area.

-   **PlayResY**: This is the height of the screen used.

-   **PlayResX**: This is the width of the screen used.
    This entry is definitly used and scales the resulting text.

-   **Timer**: This is the Timer Speed for the script, as a percentage. This
    entry seems to be NOT used.
    eg. "100.0000" is exactly 100%. It has four digits following the decimal
    point. The timer speed is a time multiplier applied to SSA's clock to stretch
    or compress the duration of a script. A speed greater than 100% will reduce
    the overall duration, and means that subtitles will progressively appear
    sooner and sooner. A speed less than 100% will increase the overall duration
    of the script means subtitles will progressively appear later and later (like
    a positive ramp time). The stretching or compressing only occurs during
    script playback - this value does not change the actual timings for each
    event listed in the script. Check the SSA user guide if you want to know why
    "Timer Speed" is more powerful than "Ramp Time", even though they both
    achieve the same result.



v4 Styles
:::::::::

This section contains all Style definitions required by the script. Each
"Style" used by subtitles in the script should be defined here.

Any of the the settings in the Style, (except shadow/outline type and depth)
can overridden by control codes in the subtitle text.

The fields which appear in each Style definition line are named in a special
line with the line type "Format:". The Format line must appear before any
Styles - because it defines how SSA will interpret the Style definition
lines. The field names listed in the format line must be correctly spelled!
The fields are as follows:

``Name, Fontname, Fontsize, PrimaryColour, SecondaryColour, TertiaryColour,
BackColour, Bold, Italic, BorderStyle, Outline, Shadow, Alignment, MarginL,
MarginR, MarginV, AlphaLevel, Encoding``

The format line allows new fields to be added to the script format in future,
and yet allow old versions of the software to read the fields it recognises -
even if the field order is changed.

-   **Name**: The name of the Style. Case sensitive. Cannot include
    commas.
-   **Fontname**: The fontname as used by Windows. Case-sensitive.
-   **Fontsize**:
-   **PrimaryColour**: A long integer BGR (blue-green-red) value. ie. the
    byte order in the hexadecimal equivelent of this number is BBGGRR
    This is the colour that a subtitle will normally appear in.
-   **SecondaryColour**. A long integer BGR (blue-green-red) value. ie. the
    byte order in the hexadecimal equivelent of this number is BBGGRR
    This colour may be used instead of the Primary colour when a subtitle is
    automatically shifted to prevent an onscreen collsion, to distinguish the
    different subtitles.
-   **TertiaryColour**. A long integer BGR (blue-green-red) value. ie. the
    byte order in the hexadecimal equivelent of this number is BBGGRR
    This colour may be used instead of the Primary or Secondary colour when a
    subtitle is automatically shifted to prevent an onscreen collsion, to
    distinguish the different subtitles.
-   **BackColour**. This is the colour of the subtitle outline or shadow,
    if these are used. A long integer BGR (blue-green-red) value. ie. the
    byte order in the hexadecimal equivelent of this number is BBGGRR.
-   **Bold**. This defines whether text is bold (true) or not (false). -1
    is True, 0 is False. This is independant of the Italic attribute - you
    can have have text which is both bold and italic.
-   **Italic**. This defines whether text is italic (true) or not
    (false). -1 is True, 0 is False. This is independant of the bold
    attribute - you can have have text which is both bold and italic.
-   **BorderStyle**. 1=Outline + drop shadow, 3=Opaque box
-   **Outline**. If BorderStyle is 1, then this specifies the width of
    the outline around the text, in pixels.
    Values may be 0, 1, 2, 3 or 4.
-   **Shadow**. If BorderStyle is 1, then this specifies the depth of the
    drop shadow behind the text, in pixels. Values may be 0, 1, 2, 3 or 4.
    Drop shadow is always used in addition to an outline - SSA will force an
    outline of 1 pixel if no outline width is given.
-   **Alignment**. This sets how text is "justified" within the
    Left/Right onscreen margins, and also the vertical placing. Values may be
    1=Left, 2=Centered, 3=Right. Add 4 to the value for a "Toptitle". Add 8
    to the value for a "Midtitle".
    eg. 5 = left-justified toptitle
-   **MarginL**. This defines the Left Margin in pixels. It is the
    distance from the left-hand edge of the screen.The three onscreen margins
    (MarginL, MarginR, MarginV) define areas in which the subtitle text will
    be displayed.
-   **MarginR**. This defines the Right Margin in pixels. It is the
    distance from the right-hand edge of the screen. The three onscreen
    margins (MarginL, MarginR, MarginV) define areas in which the subtitle
    text will be displayed.
-   **MarginV**. This defines the vertical Left Margin in pixels.

    - For a subtitle, it is the distance from the bottom of the screen.
    - For a toptitle, it is the distance from the top of the screen.
    - For a midtitle, the value is ignored - the text will be vertically centred

-   **AlphaLevel**. This defines the transparency of the text. SSA does
    not use this yet, but TextSub uses it.
-   **Encoding**. This specifies the font character set or encoding and on
    multi-lingual Windows installations it provides access to characters used
    in multiple than one languages. It is usually 0 (zero) for English
    (Western, ANSI) Windows.



Events
::::::

These contain the subtitle text, their timings, and how it should be
displayed. Quite all features are supported by TextSub.
The fields which appear in each Dialogue line are defined by a Format: line,
which must appear before any events in the section. The format line specifies
how SSA will interpret all following Event lines. The field names must be
spelled correctly, and are as follows:

``Marked, Start, End, Style, Name, MarginL, MarginR, MarginV, Effect, Text``

The last field will always be the Text field, so that it can contain commas.
The format line allows new fields to be added to the script format in future,
and yet allow old versions of the software to read the fields it recognises -
even if the field order is changed.

-   **Marked**

    - Marked=0 means the line is not shown as "marked" in SSA.
    - Marked=1 means the line is shown as "marked" in SSA.

-   **Start**

    - Start Time of the Event, in 0:00:00:00 format ie. Hrs:Mins:Secs:hundredths.
    - This is the time elapsed during script playback at which the text will appear
      onscreen. Note that there is a single digit for the hours!

-   **End**

    - End Time of the Event, in 0:00:00:00 format ie. Hrs:Mins:Secs:hundredths.
    - This is the time elapsed during script playback at which the text will
      disappear offscreen. Note that there is a single digit for the hours!

-   **Style**
    Style name. If it is "Default", then your own ``*Default`` style will be subtituted.
    However, the Default style used by the script author IS stored in the script
    even though SSA ignores it - so if you want to use it, the information is
    there - you could even change the Name in the Style definition line, so that
    it will appear in the list of "script" styles.
-   **Name**
    Character name. This is the name of the character who speaks the dialogue. It
    is for information only, to make the script is easier to follow when
    editing/timing.
-   **MarginL**
    4-figure Left Margin override. The values are in pixels. All zeroes means
    the default margins defined by the style are used.
-   **MarginR**
    4-figure Right Margin override. The values are in pixels. All zeroes
    means the default margins defined by the style are used.
-   **MarginV**
    4-figure Bottom Margin override. The values are in pixels. All zeroes
    means the default margins defined by the style are used.
-   **Effect**
    Transition Effect. This is either empty, or contains information for one of
    the three transition effects implemented in SSA v4.x
    The effect names are case sensitive and must appear exactly as shown. The
    effect names do not have quote marks around them.

*Effect examples*:
::

    ``
    "Scroll up;y1;y2;delay[;fadeawayheight]"
    "Scroll down;y1;y2;delay[;fadeawayheight]"


`````` means that the text/picture will scroll up/down the screen. The parameters
after the words "Scroll up" are separated by semicolons.

The y1 and y2 values define a vertical region on the screen in which the text
will scroll. The values are in pixels, and it doesn't matter which value (top
or bottom) comes first. If the values are zeroes then the text will scroll up
the full height of the screen.

The delay value can be a number from 1 to 100, and it slows down the speed of
the scrolling - zero means no delay and the scrolling will be as fast as
possible.

fadeawayheight parameters can be used to make the scrolling text at the sides transparent.
::

    ``
    "Banner;delay"
    "Banner;delay[;lefttoright;fadeawaywidth]"

`````` means that text will be forced into a single line, regardless of length,
and scrolled from right to left accross the screen.
lefttoright 0 or 1. This field is optional. Default value is 0 to make it
backwards compatible.

The delay value can be a number from 1 to 100, and it slows down the speed of
the scrolling - zero means no delay and the scrolling will be as fast as
possible.

fadeawaywidth parameters can be used to make the scrolling text at the sides transparent.


$Date: 2004/08/17 20:31:19 $

.. _guliverkli: http://sourceforge.net/project/showfiles.php?group_id=82303&package_id=84359
.. _here: http://forum.doom9.org/showthread.php?s=&threadid=66285

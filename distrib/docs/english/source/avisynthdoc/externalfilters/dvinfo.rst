
DVInfo
======


Abstract
--------

| **author:** WarpEnterprises
| **version:**
| **download:** `<http://www.avisynth.org/warpenterprises/>`_
| **category:** Misc Plugins
| **requirements:**

-   DV-AVI input

--------


Description
-----------

DVInfo reads the Timecode and Recording Date information out of an DV-AVI
file.


Syntax
~~~~~~

``DVInfo`` (clip, string "filename", string "output", int "x", int "y",
string "font", int "size", int "text_color", int "halo_color", string
"rec_format",
string "tc_format", bool "show_error", float "threshold", int "autoframes")

DVInfo opens an DV-AVI file (Type-1 and Type-2, legacy and openDML > 4GB is
supported) and reads the timecode and the recording timestamp of the frame
passed through. Take care: it DOES NOT load the video or audio content of the
AVI, this has to be done DIRECTLY BEFORE with AviSource!

The timecode and recording date are then printed on the frame as if you were
using Subtitle() and the parameters x, y, font, size, text_color, halo_color
are passed right through to Subtitle.

The "run-time" variables "current_frame", "tc_time", "rec_time" are set so
that you can use them in an expression in "output" (this works similar as
with ScriptClip, look there in the docu for more infos).

You can set the generated time format with the parameters rec_format and
tc_format.

With "threshold">0 the output is switched on if there is a difference of the
recording date between current and last frame which is bigger than
"threshold" seconds. After "autoframes" frames the output is switched off.
That way you get the recording date only at scenechanges.
Take care: the last *processed* frame is used, which is not the previous one
when you seek in the clip (works only for linear playback).
With e.g. threshold=0.5 and autoframes=2 you can print it every second and 2
frames long.

This is best explained with an example:

::

    LoadPlugin("c:\myprojects\dvinfo\release\dvinfo.dll")

    file = "c:\myprojects\type2.avi" # so you don't need to type the filename twice

    Avisource(file)          # open the video content

    DVInfo(file, "rec_time") # reads the recording timestamp
                             # prints it with the default format into the variable "rec_time"
                             # evaluates this expression (which consists only of the single variable)
                             # displays the result using the default Subtitle parameters

You can ommit even "rec_time", since it is the default value of "output".

::

    DVInfo(file)   # will do the same as above



--------

This won't work (more than one file):

::

    LoadPlugin("c:\myprojects\dvinfo\release\dvinfo.dll")

    file1 = "c:\myprojects\type2a.avi"
    file2 = "c:\myprojects\type2b.avi"

    Avisource(file1)+Avisource(file2)  #this is OK so far

    DVInfo(file1, "rec_time")  # but now DVInfo tries to read in file1 the frame numbers of file2!
    DVInfo(file2, "rec_time")  # and vice versa

Instead, write it this way:

::

    LoadPlugin("c:\myprojects\dvinfo\release\dvinfo.dll")

    file1 = "c:\myprojects\type2a.avi"
    file2 = "c:\myprojects\type2b.avi"

    Avisource(file1).DVInfo(file1, "rec_time") +
    Avisource(file2).DVInfo(file2, "rec_time")
    # now each DVInfo reads its own file


This won't work (using Trim):

::

    LoadPlugin("c:\myprojects\dvinfo\release\dvinfo.dll")

    file1 = "c:\myprojects\type2a.avi"

    Avisource(file1)
    Trim(1000,2000)
    DVInfo(file1, "rec_time")  # now DVInfo gets the wrong frame numbers

Instead, write it this way:

::

    LoadPlugin("c:\myprojects\dvinfo\release\dvinfo.dll")

    file1 = "c:\myprojects\type2a.avi"
    file2 = "c:\myprojects\type2b.avi"

    Avisource(file1).DVInfo(file1, "rec_time") +
    Avisource(file2).DVInfo(file2, "rec_time")
    # now each DVInfo reads its own file




--------


More examples
~~~~~~~~~~~~~

You can modify the time format (see AviSynth Docu >> Syntax >> function Time
for all details):

::

    DVInfo(file, "rec_time", rec_format="%H:%M:%S") # print only time without date

or
::

    DVInfo(file, "rec_time", rec_format="%A, %H:%M:%S") # print full weekday name plus time

The tc_time holds the position on the tape in hours, minutes, seconds and
frames.
I put the frames into the day-of-month position, this is somewhat dirty but
you can easily display the frame number by using the "month" format symbol:


::

    DVInfo(file, "tc_time", tc_format="%d") # print only the frame number

You can put a more complex expression in the "output":

::

    DVInfo(file, "tc_time + chr(32) + rec_time + chr(32) + current_frane")
    # displays timecode, recording time and frame number separated by spaces

If you want to add other text:

::

    text = "HALLO WORLD "
    DVInfo(file, "text + rec_time") # displays "HALLO WORLD " and the recording time.

You have to do it this way as it is not possible to put quotes in a string.

As you see you can use every script variable in "output".

Printing the recording date only at scenechanges:.

::

    DVInfo(file, threshold=1)   # will print it 25 frames long if the difference is >1 sec

Error handling
~~~~~~~~~~~~~~

If the framenumber requested is bigger than the framecount (this should not
happen) an error message is put into rec_time and tc_time.
If no timecode or recording timestamp can be found, or if there is some other
read error, an error message is put into rec_time and tc_time.

These error messages are supressed if you use show_error = false.

If the AVI can't be opened, or the result of "output" is not a string, an
AviSynth error is thrown.

--------


Technical note
~~~~~~~~~~~~~~

To read the AVI-data I used the code from kino by Arne Schirmacher.
I modified it so it will work with openDML - files > 2GB, compile on Win32
and stripped of all other than read functions.

Ernst Peché, 2003-12-16

$Date: 2004/08/13 21:57:25 $

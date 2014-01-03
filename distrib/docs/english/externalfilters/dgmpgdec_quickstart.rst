
DGMPGDec Quick Start Guide
==========================

This document is intended to help newcomers to DGMPGDec to quickly understand
the process and become productive. It is intentionally short and to the
point, and is not intended to be a complete users manual or tutorial.

--------


What is DGMPGDec?
-----------------

DGMPGDec is an MPEG decoder suite. It is used to decode MPEG1 or MPEG2
streams from such sources as DVD VOBs, captured transport streams,
``*.mpg/*.m2v/*.pva`` files, etc. Perhaps its most common use is in decoding VOBs
from DVDs.

--------


What Do I Need to Use It?
-------------------------

You need the DGMPGDec package and Avisynth. First get Avisynth 2.5 (or
better) and install it:

`Avisynth 2.5`_

You are going to use DGIndex.exe and DGDecode.dll from the DGMPGDec package,
so extract them from the ZIP file and put them together in a directory.

Also get VirtualDub as we will use that to view the decoded video:

`VirtualDub Web Site`_

We'll assume you have a VOB that you have obtained from a DVD (possibly using
Smart Ripper, or other such tool).

--------


OK. Now What?
-------------

Fire up DGIndex. Using File/Open, open your VOB. You should see the video.
Now select Audio/Output Method/Demux All Tracks. That will cause your audio
to be saved in a file(s) when you save the project.

Now select File/Save Project and enter a name for the index file (D2V file)
that is going to be generated. Suppose your VOB is called 'myvob.vob'; you
might choose the name 'myvob' to enter here, because DGIndex will
automatically append '.d2v'. Good. Hit Save. The indexing process will start
and you'll see the indicator moving along the time line to indicate the
progress. Be patient if your video is large. When the process finishes, exit
DGIndex.

--------


What is This Index File and What Do I Do with It?
-------------------------------------------------

DGIndex created an index file called ``*.d2v``. It is read by DGDecode, which
actually decodes the MPEG and delivers the video. The index file just
contains information that tells DGDecode where each frame is located and some
information about each frame.

But you can't just execute DGDecode directly! It has to be done through
Avisynth. We'll make a script file called myvob.avs using a text editor.
Later in this document I'll show you how to configure DGIndex to make the
script automatically, but for now, you need to know the old-fashioned way. So
put this text into a new text file you make called 'myvob.avs':
::

    LoadPlugin("...\DGDecode.dll")
    MPEG2Source("myvob.d2v")

Replace the path '...' in the first line with the path to the location where
you placed DGDecode.dll.

Finally, use VirtualDub to open the 'myvob.avs' script file just as if it was
an AVI file. That's it! You have your video and can navigate randomly on the
VirtualDub timeline. Does life get much sweeter than this?

--------


Sure, Sure, But What About My Audio
-----------------------------------

DGIndex saved your audio in a file(s). It will have a ".wav", ".ac3", or
".mpa" extension. If you have a ".wav" file, you can load that directly in
VirtualDub. But you can also use Avisynth, which gives you access to powerful
audio filtering.

Suppose we have a ".wav" file. Our Avisynth script will be like this:
::

    LoadPlugin("...\DGDecode.dll")
    video=MPEG2Source("myvob.d2v")
    audio=WAVSource("myvob.wav")
    AudioDub(video,audio) ``

Now when you open this script in VirtualDub, you will have video and audio.

We saw processing for a ".wav" audio file above. You need the corresponding
source filter for the type of audio you have. Use WAVSource() for ".wav",
MPASource() for ".mpa", AC3Source() for ".ac3", etc. WAVSource() is built
into Avisynth. The others can be found here: `Avisynth Filter Collection.`_

Don't forget to use LoadPlugin() to load your audio source filter. And read
the Avisynth documentation to learn about how to adjust the audio/video
synchronization using DelayAudio(), and other useful things.

--------


Yeah, But How Do I Do That Automatic Script File Thing?
-------------------------------------------------------

Ahh, you have to pay extra for that! No, not really.

Let's suppose you have a script that you use all the time. Maybe like this:
::

    loadplugin("...\DGDecode.dll")
    loadplugin("...\Decomb.dll")
    mpeg2source("myvob.d2v",cpu=6)
    fielddeinterlace()

Copy this to a file and call it 'template.avs'. Then edit it to replace the
D2V file name with __vid__ (that's *two* underscores before "vid" and two
after). template.avs should then look like this:
::

    loadplugin("...\DGDecode.dll")
    loadplugin("...\Decomb.dll")
    mpeg2source("__vid__",cpu=6)
    fielddeinterlace()

You see, DGIndex is going to use this as a template and insert the right file
name whenever it sees __vid__. Slick, eh? You can also use __aud__ to
generate the audio filename; refer to the DGIndex users manual for details.

OK, all you have to do now is fire up DGIndex, select your template file with
the Options/AVS Template menu item, and then do a Save Project. If the ``*.avs``
file does not already exist, DGIndex will make one for you based on the
template! Of course, the template has to be created only once, while you'll
get an automatically generated AVS script every time you save a DGIndex
project.

--------


Cool. One Last Question...
--------------------------

Shoot!

--------


Why the Two-Step Tango? Why Can't I Do Everything Right in DGIndex?
-------------------------------------------------------------------

Good question! We want to make our video available to any application that we
might find useful. Surely we can't put every possible function into DGIndex.
So instead we create a way to 'serve' the video into all these other
applications. Avisynth is an AVI file server. It creates a 'fake AVI' and
tricks applications into thinking they have a real AVI when they open the
``*.avs`` file.

If you just want to make an AVI out of your video, it's easy. Open the AVS in
VirtualDub, set your compression, and do Save AVI.

There's another way to serve called 'VFAPI' that is also supported, but you
get the idea: we are just setting up serving of the decoded MPEG2 video with
DGMPGDec. The receiving application then does its thing without even knowing
it has been tricked.

--------

Copyright (C) 2004, 2005 Donald A. Graft, All Rights Reserved

$Date: 2006/09/18 19:02:03 $

.. _Avisynth 2.5: http://sourceforge.net/project/showfiles.php?group_id=5
    7023&package_id=72557
.. _VirtualDub Web Site: http://www.virtualdub.org
.. _Avisynth Filter Collection.: http://www.avisynth.org/warpenterprises

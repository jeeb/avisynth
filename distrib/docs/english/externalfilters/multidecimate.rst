
MultiDecimate
=============


Abstract
--------

| **author:** Donald A. Graft
| **version:** 1.0.7
| **download:** `<http://neuron2.net/mine.html>`_
| **category:** Deinterlacing & Pulldown Removal
| **requirements:** YUY2 Colorspace

--------


Introduction
------------

This filter provides extended decimation capabilities not available from
Decimate(). It can remove all the duplicate frames from a clip, or it can
remove N out of every M frames, where N and M can be selected by the user,
removing the frames most similar to their predecessors. Special modes are
available to protect static scenes from decimation.

This filter uses a 2-pass approach to allow for full timeline random access
navigation, random decimation, and the later addition of a GUI-based manual
tweaking capability.

This filter is valuable when traditional 1-in-5 decimation is not sufficient.
For example, many silent films are transferred to DVD by adding duplicates in
unusual patterns, because the original frame rates are not 24fps. It is not
unusual to see clips requiring strange decimations such as 20 in 43.
Sometimes these strange decimations can be attained, or nearly attained,
through repeated application of Decimate() using different cycles, but that
is a cumbersome approach that cannot always attain the exact decimation
ratios required. This filter tries to approach the problem more directly, but
at the expense of 2-pass operation.

Another typical application is removing 3 out of 4 frames for clips that were
rendered at 120fps to properly present hybrids of film and video.

This version supports YUY2 only for Avisynth 2.5. YV12 will be added when the
functionality stabilizes.

Initial Setup
-------------

Place MultiDecimate.exe and ProcessMD.exe somewhere in your DOS executable
path. Put the MultiDecimate.dll in your Avisynth plugins folder (or load it
manually in your scripts). If you are clueless about DOS paths, just put
MultiDecimate.exe and ProcessMD.exe in your script directory.

Process
-------

**Step1.** Make your script for pass 1:
::

    ...
    MultiDecimate(pass=1)

Load the script and play it straight through from the beginning to completion
(do not jump around on the timeline first!). This makes the metrics file,
*mfile.txt*, in your script directory. If you want to examine your video for
duplicate patterns, comment out the MultiDecimate() call, do not try to do it
during pass 1, as any timeline jumps will corrupt the metrics file. Finally,
after pass 1 completes, immediately exit VirtualDub; do not scroll around.

It is a waste of time to include any filters after the MultiDecimate() call
in the first pass. Comment them out for faster performance. Comment them back
in for the second pass.

**Step 2.** Execute MultiDecimate.exe to run the GUI. Browse to select the
*mfile.txt* file. Select the desired mode (see below for a description of the
modes). Set the remaining configuration boxes as required (see below). Then
click on the Create button. This makes the *cfile.txt* and *dfile.txt* files
in your script directory.

**Step 3.** Edit the script to change pass=1 to pass=2. Now you can load the
script and it will have the correct decimated video, and timeline random
access navigation will be supported. Pass 2 reads the *dfile.txt* file into
memory and uses it to determine which frames to deliver.

GUI Decimation Modes and Configuration
--------------------------------------

The following modes are available:

-   "Remove duplicates: Global: Naive": This mode will remove all frames
    determined to be duplicates as defined by the configured Threshold.
    Frames with a metric less than threshold will be considered to be
    duplicates. Examine *mfile.txt* to examine the frame metrics.


-   "Remove duplicates: Global: Protect static scenes": This mode will
    remove all frames determined to be duplicates as defined by the threshold
    configuration, but static scenes will not be removed. A static scene is
    defined to be one with a run of duplicates equal to or greater than the
    configured Run Length.


-   "Remove duplicates: Cycle-based: Naive": This mode will remove N out
    of every M frames, where N is defined by the configured Cycle, and M is
    defined by the configured Remove.


-   "Remove duplicates: Cycle-based: Protect static scenes": This mode
    will remove N out of every M frames, as above, but static scenes are
    "protected". In naive mode, a static scene within a cycle may be
    preferentially decimated. This may be bad because you may lose too much
    of the static scene, and because taking extra duplicates from the static
    scene means that extra dups are being left in the action scenes.

    To address this, use the "protect static scenes" mode. When you
    choose this mode from the drop down list, two extra edit boxes appear.
    Threshold is the metric below which duplicates are declared. The metrics
    are the same as those used in the *mfile.txt* file, so you can use that
    to help you. The fields in *mfile.txt* are as follows:

        FrameNum IsADup Metric

    Run Length is the number of duplicates (thus defined) in a row that
    defines a static scene subject to protection. A protected static scene is
    decimated by the ratio remove/cycle, no more and no less (subject to
    integer truncation).


Avisynth Filter Parameters
--------------------------

Following is the syntax for MultiDecimate (replace *parameter_list* with your
comma-separated list of named parameters):

``MultiDecimate`` (parameter_list)

*pass* (1 or 2, default 1): This parameter defines the processing pass as
described above.

*quality* (0-3, default 2) This option allows the user to trade off quality of
difference detection against speed. Following are the possibilities:

- quality = 0: Subsampled for speed and chroma not considered (fastest).
- quality = 1: Subsampled for speed and chroma considered.
- quality = 2: Fully sampled and chroma not considered.
- quality = 3: Fully sampled and chroma considered (slowest).

*show* (true/false, default false) enables information to be displayed on the
frame. Also displays the software version.

*debug* (true/false, default false) enables information to be printed via
OutputDebugString(). A utility called DebugView is available for catching
these strings. The information displayed is the same as shown by the show
option above.

Acknowledgments
---------------

Thanks to Tom Daniel ('manono') for suggesting this new decimation
functionality.

Copyright
---------

| Copyright (C) 2003, Donald A. Graft, All Rights Reserved.
| Feedback/inquiries to neuron2 at attbi.com.

For updates and other filters/tools, visit my web site:
`<http://neuron2.net/>`_

$Date: 2004/08/13 21:57:25 $

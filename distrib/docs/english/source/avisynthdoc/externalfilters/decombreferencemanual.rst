
Decomb Plugin (Version 5.2.1) for Avisynth 2.5 Reference Manual
===============================================================

by Donald A. Graft
::::::::::::::::::

.. toctree::
    :maxdepth: 3

--------


Introduction
------------

This package of plugin functions for Avisynth provides the means for removing
combing artifacts from telecined progressive streams, interlaced streams, and
mixtures thereof. Functions can be combined to implement inverse telecine
(IVTC) for both NTSC and PAL streams.

The term "combing" is used as a general term versus "interlacing" because
interlacing is usually associated with nonprogressive streams. What look like
interlacing artifacts can be seen on telecined progressive streams, so I use
the term combing to refer to all such artifacts, regardless of the type of
the stream.

The package automatically adapts to any telecine/capture patterns, and
mixtures thereof, although if you want to decimate the recovered progressive
stream, you have to specify the desired decimation ratio if it differs from
the default 1-in-5 used for NTSC 3:2 telecining. None of the functions
introduce a delay in the audio or video streams.

For many applications, there are few parameters to set, but best results are
achieved with careful setting of parameters. Decomb allows the user to define
his own default parameters if desired (defaults are the values assumed if the
parameter is not specified explicitly).

Many of Decomb's decisions can be manually overridden as desired using text
override files. This allows meticulous users to achieve the best possible
quality for their encodes.

This is the reference manual for Decomb. Refer to the Decomb Tutorial for
user-friendly procedures for using Decomb, and to the Decomb FAQ for answers
to frequently asked questions.

--------


Functions Overview
------------------

The package consists of the following functions:

-   **Telecide()**: Recovers progressive frames (by finding and aligning
    matching fields) but does not remove resulting duplicates and does not
    change the frame rate or frame count. Do not use Telecide on streams that
    do not contain telecined progressive frames, such as pure interlaced
    video. Note that a stream of PAL progressive frames that are shifted by
    one field will exhibit combing and can be considered a telecined stream
    for purposes of recovery; Telecide will easily handle this situation.

    By default, Telecide runs *postprocessing* on the recovered frames.
    This postprocessing checks each frame to see if it is combed (some combed
    frames may come through the field-matching process [see below]), and if a
    frame is combed, it is deinterlaced, otherwise it is not touched. The
    deinterlacing algorithm is a space-adaptive one, that is, only the
    portions of a frame that are combed are deinterlaced. This means that
    full detail is retained in static picture areas. The algorithm for
    distinguishing between combed and progressive frames is effective but not
    perfect.

    To achieve inverse telecine, apply Telecide followed by Decimate.
    Appendix A explains why this works.

    Telecide supports an optional *pattern guidance mode*. Enabling this
    option allows Telecide to overrule the calculated field match with its
    predicted match based on the recent clip history and future. A threshold
    can be defined so that a large enough discrepancy between the predicted
    and calculated matches will reset the pattern.

-   **FieldDeinterlace()**: This filter provides functionality similar to
    the postprocessing function of Telecide. You can use it for pure
    interlaced streams (that is, those not containing telecined progressive
    frames). (The name refers to the fact that field mode differencing is
    used.) Do not use FieldDeinterlace after Telecide because the same
    functionality is built into Telecide.

    FieldDeinterlace provides an option that allows you to specify
    whether all frames are to be deinterlaced or whether just frames detected
    to be combed are deinterlaced.

-   **Decimate()**: Decimate 1 frame in every N, where N is a parameter
    and can range from 2 to 25. Decimate removes one duplicate frame in every
    group of N frames. The frame rate and count are adjusted appropriately.
    Decimate properly supports VirtualDub random access (timeline
    navigation).

    Decimate provides several special modes that are useful when dealing
    with film/video hybrid material.

-   **IsCombed()**: This is a utility filter useful in Avisynth
    scripting. It enables a script to distinguish progressive versus
    interlaced frames. It is designed for use within ConditionalFilter().

--------


Input Requirements
------------------

Telecide requires that the input width be a multiple of 2. Deviation from
this will cause Telecide to throw an exception.

Do not resize vertically before applying Decomb. Decomb needs to see the
original line spacing to properly detect combing.

Telecide requires YUY2 or YV12 color format as input. The DLL shipped with
this version of Decomb will work only with Avisynth version 2.5 and beyond.

It is preferable not to convert between YV12 and YUY2 (either way) before
using Decomb. Apply Decomb in the native color space. Such conversions can
cause artifacts due to incorrect chroma upsampling/downsampling.

For YV12 operation, the only supported version of "mpeg2dec" is MarcFD's
mpeg2dec3 (version 0.94 and beyond). Other versions may produce incorrect
output.

--------


Important Speed Tips
--------------------

| **Avoid Converting the Color Space**
| Be aware that converting to YUY2 from
  RGB is very time expensive, so don't save your AVIs in RGB. The popular
  HUFFYUV codec has an option to convert RGB to YUY2 and you should enable that
  when generating AVIs with HUFYUV that are destined for Decomb processing.

| **Don't Postprocess Unnecessarily**
| Try Telecide without postprocessing if
  you think there is a chance that you have a nice clean input stream. If you
  do have such a stream, you'll find that processing runs faster without
  postprocessing.

| **Don't Invoke an Avisynth Strangeness**
| This one is very important and
  can easily cost you a 25% speed penalty! It appears that Avisynth has a
  strangeness that causes it to waste enormous amounts of time when there are
  no parentheses with the commands. For example, this script:

| ``Telecide``
| ``Decimate``

...will run much slower than this one:

| ``Telecide()``
| ``Decimate()``

If you doubt this, try it both ways and see. It is critical, therefore, to
always include at least one parameter or the set of empty parentheses. That
is why the scenarios below all are coded that way.

| **Use Fast Recompress If Possible**
| If you are serving into VirtualDub for
  transcoding, and you don't need to do any filtering or other processing in
  VirtualDub, then use VirtualDub's Fast Recompress mode.

| **Disable Frame Displays**
| When sending Decomb output to VirtualDub (and
  similar applications), disable the display of the input and output frames
  during processing. This will noticeably decrease processing time.

--------


Typical Scenarios
-----------------

These examples are intended only to illustrate the main features of Decomb.
Refer to the User Manual for detailed procedures for determining the correct
settings to apply, as best results are achieved with careful tweaking of
parameters.

| **Simple Deinterlacing**
| If you have some nonfilm (interlaced) source, you simply deinterlace it as follows:

::

  LoadPlugin("decomb.dll")
  AVISource("nonfilm.avi")
  FieldDeinterlace()

| **Progressive Frame Recovery**
| If you have telecined film (progressive)
  source and want to recover the progressive frames but not change the frame
  rate by decimating, you proceed as follows:

::

  LoadPlugin("decomb.dll")
  AVISource("film.avi")
  Telecide(order=1)

The order parameter must be set to correctly specify the field order of the
clip. Note that here Telecide does postprocessing of the recovered frames to
clean up any combed frames that might have come through the field-matching
process (see "Notes on Field Matching" below).

You can use the show option, or the debug option in conjunction with the
DebugView utility, to see the metrics and decisions that Telecide generates.
This will assist you in tweaking the parameters.

| **Inverse Telecine (IVTC)**
| If you want to do the same thing but decimate
  the result to remove duplicated frames (which amounts to performing an
  inverse telecine [IVTC] operation), you proceed as follows [NTSC 3:2 uses
  Decimate(cycle=5)]:

::

  LoadPlugin("decomb.dll")
  AVISource("film.avi")
  Telecide(order=1)
  Decimate(cycle=5)

| **Disabling Postprocessing**
| If your telecined source material is very
  clean, you may want to disable postprocessing to reduce processing time.
  Proceed as follows:

::

  LoadPlugin("decomb.dll")
  AVISource("mixed.avi")
  Telecide(order=1,post=0)
  Decimate(cycle=5)

Here the third parameter, *post*, is set to 0 to disable postprocessing.

| **Inverse 3:2 Telecine with Pattern Guidance**
| If your telecined source
  material is NTSC 3:2 pulldown, you can enable pattern guidance, which can
  make the field matching more accurate for some clips. Proceed as follows:

::

  LoadPlugin("decomb.dll")
  AVISource("mixed.avi")
  Telecide(order=1,guide=1)
  Decimate(cycle=5)

Refer to the syntax description for Telecide() below for more details.

| **Processing Hybrid Material**
| If you have a clip that contains both 3:2
  pulldown (film) and pure video, you can do a good job with it like this:

::

  LoadPlugin("decomb.dll")
  AVISource("hybrid.avi")
  Telecide(order=1,guide=1)
  Decimate(mode=3,threshold=2.0)

Refer to APPENDIX B for further details on handling hybrid material.

--------


Notes on Field Matching
-----------------------

Telecide normally does an excellent job at recovering progressive frames by
field matching. There are four known source stream conditions that can cause
Telecide to output frames with combing:

-   *Missing Fields*. If a field is missing due to a bad edit, then its
    partner field in the source will not have a good field to match with. Use
    the default postprocessing to clean up the output stream.

-   *Blended Fields*. Some streams have fields that are blends of two
    original film progressive pictures! Some NTSC/PAL conversions can cause
    this, for example. Usually such a stream has a lot of these and the
    solution is to run postprocessing with blend=true to clean up the output
    stream. Using blend=true mode seems to work best and the blended fields
    appear as blended frames, which lends a kind of motion blur and reflects
    the "intent" of the input stream.

    In cases where there are a lot of blended frames, there is no point
    in using Telecide at all. Just use FieldDeinterlace() to treat the clip
    as interlaced video.

    Finally, how do you tell if your stream has blended fields? Simply
    use the Avisynth SeparateFields function to split the fields apart and
    then serve the result to VirtualDub. Step through the fields and see if
    there are any fields that are blends of more than one picture.

-   *Nonfilm Frames (Hybrid Clips)*. Some streams, especially those
    captured from live broadcasts, have periods of film and periods of
    nonfilm. For such streams, you can either use Decimate() with mode=1 or
    mode=3 as described in the scenarios section above. Mode=1 will leave the
    clip at 30fps while mode=3 will leave it at 24fps. It is your (difficult)
    decision about whether you want the final stream at 24fps or 30fps.

-   *Hybrid Frames*. Sometimes graphics, credits, etc., will be rendered
    at final frame rate and then overlayed on the telecined content,
    resulting in frames that have both progressive and nonprogressive
    content. Or more rarely, one encounters clips that have different layers
    that are telecined with differnt phases and then composited. For streams
    with hybrid frames, you usually must rely upon Telecide's postprocessing
    to clean up the output stream.

    By now you should be getting the idea that postprocessing is
    generally a good thing to do to ensure that no combed frames sneak
    through.

--------


General Function Syntax
-----------------------

The Decomb functions use named parameters. That means you do not have to
worry about the ordering of parameters. You can simply refer to them by name
and put them in any order in the list of parameters. If you omit a parameter
it takes its default value. For example, if you want to run Telecide with
order tff, postprocessing, and with debug enabled, you can simply say:

``Telecide(order=1,post=2,debug=true)``

Any combination and order of named parameters is allowed. Remember, however,
that you should always include empty parentheses if you are not specifying
any parameters.

--------


Customizing Default Parameter Values
------------------------------------

If you do not like the defaults as documented below, you can set your own
standard defaults. To override the defaults, create defaults files as
required in the Avisynth plugins directory. For example, to set the default
post=4 for Telecide(), make a file called Telecide.def and put this line in
it:

``post=4``

You can list as many parameter assignments as you like, one per line. Those
not specified assume the default values documented below. Of course, you can
always override the defaults in your scripts when you invoke the functions.
NOTE: The lines in the defaults file must not contain any spaces or tabs.

--------


Overriding Decomb Decisions
---------------------------

On occasion you may have close to a perfect encode except for a few frames
that Decomb decides wrong about. In such cases you can use Decomb's manual
override functionality. This is an advanced feature that should be used only
by experts who need the highest possible quality encodes.

| **Overriding Telecide()**
| For example, suppose we find that frame 100 is
  not being field-matched correctly. We inspect Telecide()'s debug output and
  find that it is matching to the previous frame ("[using p]"). We want to try
  forcing a match to current and next to see if a correct match can be found
  (refer to APPENDIX A for an explanation of the terminology). First, we make a
  file in the same directory as the script file called "tango.tel" (you can use
  any filename). Then we place this line in the file:

``100 c``

This declares that frame 100 is to be matched to current (use "p" for
previous frame and "n" for next).

Now we direct Telecide to use this overrides file:

``Telecide(ovr="tango.tel")``

Now process the script as usual; the specified override to current will be
used. If this match fails we can try replacing "c" with "n" to try next.

A frame range can also be used, such as:

``100,500 c``

We can add additional lines defining any other overrides as required. Note
that the frame numbers must be in ascending order (from lowest to highest).

You can also specify full patterns for your matching in the Telecide
overrides file. For example, suppose you wanted to force a pattern of ncccn
for frames 100 through 185. Your overrides file line would be:

``100,185 ncccn``

You can put as many specifiers in the pattern as you like. For example, this
would be legal too:

``100,185 nc``

The specified pattern will be repeated beginning at the starting frame and
ending at the ending frame of the range.

This capability allows for flexible and convenient manual control of field
matching. You can do your whole film manually if you like!

Telecide()'s postprocessing decisions can also be overridden. To force a
frame to be considered combed, use a line with a '+' like one of these:

| ``100 +``
| ``100,500 +``

To force a frame to be considered NOT combed, use lines like this:

| ``100 -``
| ``100,500 -``

You can set different *vthresh* values for different parts of your clip. For
example, to set *vthresh* to 25 for frames 200 through 500, use:

``200,500 v 25``

Note that the 'v' override requires a frame range. To specify a single frame,
set both the starting and ending frames of the range to the desired frame
number.

You can specify the matching mode (back=0/1/2) for a frame or range of
frames:

| ``100 b 2``
| ``200,500 b 1``

The first line above enables triple matching for frame 100. The second line
enables combing-driven backward matching for frames 200 through 500.

Lines for overriding field matching and for overriding postprocessing can be
used together in the same overrides file as long as the ascending frame
number requirement is met.

| **Overriding FieldDeinterlace()**
| FieldDeinterlace() supports the '+' and
  '-' overrides as described for Telecide(). Of course, field matching
  overrides do not apply and should not be used. You might want to call the
  overrides file "tango.fd".

| **Overriding Decimate()**
| For Decimate(), we do things the same way but we
  use lines of the following form, where the numbers are frame numbers of the
  clip entering Decimate() that will be forced to be decimated:

| ``150``
| ``175``

In the above example, frames 150 and 175 will be force decimated. Again, an
appropriate name for the overrides file would be "tango.dec".

--------


Detailed Function Syntax
------------------------

Following is the syntax for the Decomb functions (replace *parameter_list*
with your comma-separated list of named parameters):

-------


Telecide(parameter_list)
~~~~~~~~~~~~~~~~~~~~~~~~

Commonly used parameters
........................

**order** (0-1, default none!) defines the field order of the clip. It is
very important to set this correctly. The User Manual specifies a reliable
procedure for doing so. Use order=0 for bottom field first (bff). Use order=1
for top field first (tff). You must specify order; Decomb throws an exception
if you omit this parameter.

**guide** (0-3, default 0) can be used to improve field matching when the
source clip is known to be PAL or NTSC telecined material. To disable this
option (blind field matching), set guide=0. For NTSC 24fps->30fps telecine
guidance, set guide=1. For simple PAL guidance (tries to maintain lock to the
field phase), set guide=2. For NTSC 25fps->30fps telecine guidance, set
guide=3.

When this option is enabled, Telecide() can overrule a field match decision
and use a predicted match based on the recent clip past and future. The
*gthresh* parameter (below) is used to define how small a discrepancy between
the predicted and calculated field matches is required to accept and use the
prediction. Do not enable this option unless you know that the source clip
corresponds to the selected guidance mode. If in doubt, leave guide=0.

Note that this feature uses Avisynth random frame access, so it works fine
when the user uses random timeline navigation.

**gthresh** (0.0-100.0, default 10.0) defines how large a discrepancy (in
percent) between the predicted and calculated field matches is required to
reset the pattern. Use the show option, if required, to appropriately tweak
this threshold. The show and/or debug output will indicate which matches have
been overridden. Overridden matches are denoted with an asterisk, e.g., "in-
pattern*".

Do not set *gthresh* too high, as it can create bad matches. Anything over
about 10-15 is starting to get dangerous.

**post** (0-5, default 2) controls whether and how Telecide performs
postprocessing to clean up frames that come through the field-matching still
combed:

-   post=0: Use this to totally disable postprocessing.
-   post=1: Use this to enable the metrics calculation and to display the
    *vmetric* values but not perform deinterlacing.
-   post=2: Use this to enable deinterlacing. Note that in this mode, the
    field matching occurs normally and the best matched frame is deinterlaced
    and delivered.
-   post=3: This is the same as post=2 except that the deinterlacing
    motion map is displayed in white on the deinterlaced frames.
-   post=4: This is the same as post=2 except that instead of using the
    best field match for the frame, the original frame is deinterlaced. You
    would use this to pass video sequences through when you have a hybrid
    clip.
-   post=5: This is the same as post=4 except that the deinterlacing
    motion map is displayed in white on the deinterlaced frames.

**vthresh** (0.0-255.0, default 50.0) sets the combed frame detection
threshold for the postprocessing. You may want to increase this value if too
many good frames are being deinterlaced, or reduce it if some combed frames
are not getting caught. The default is a reasonable general purpose value.
Note that this threshold determines whether a frame is considered combed and
needs to be deinterlaced; it is not the threshold you might be familiar with
in Smart Deinterlacer. That threshold is determined by *dthresh* (below); it
is the threshold for deinterlacing the frames *detected as combed*.

**dthresh** (0.0-255.0, default 7.0) sets the threshold for deinterlacing
frames detected as combed. Note that this threshold is the threshold you
might be familiar with in Smart Deinterlacer.

**blend** (true/false, default false) enables blending instead of
interpolating in combed areas. Interpolating is faster.

**show** (true/false, default false) enables metrics to be displayed on the
frame to assist with tweaking of thresholds. Also displays the software
version.

Advanced parameters
...................

**chroma** (true/false, default true) determines whether chroma combing is
included in the decision made during postprocessing as to whether a frame is
combed or not. If chroma=true, then chroma combing is included, otherwise it
is not included. Note that chroma is always deinterlaced; this parameter
affects only the decision about whether a frame is combed. It is useful for
clips which have a large amount of luma/chroma interference, as might result
from a poor comb filter. The interference can cause frames that are not
combed to be detected as combed when chroma=true. By setting chroma=false,
the effect of the interference can be eliminated.

**back** (0-2, default 0) selects the matching mode. When back=0, the
backward match is never tested. This means that at bad edit cuts, a
progressive frame may not be found and the resulting frame must be
deinterlaced by postprocessing. When back=1, if a frame is still combed after
field matching (according to a test against 'bthresh' -- see below), then the
backward match is tried. This requires post > 0 and may allow a good
progressive frame to be found at bad edit cuts. When back=2, the backward
match is always considered. Some clips, especially those with a lot of
blended fields, may be handled better with back=2, which tests for matching
with the previous, current, and next frames. But use it with caution, because
it can make some clips jerky.

**bthresh** (0.0-255.0, default 50) sets the combing detection threshold
for conditional backward matching (back=1). The backward match will be
considered if the candidate match has combing greater than 'bthresh'.
Typically you set a high 'vthresh' to catch stray combed frames, together
with a lower 'bthresh' to catch bad edits.

**nt** (integer, default 10) defines the noise tolerance threshold. It
should usually not be necessary to adjust this parameter. If you have a noisy
capture and are experiencing matching failures, however, you might usefully
try increasing it.

**y0** and **y1** (integer, default 0) define an exclusion band for the
field matching. If y0 is not equal to y1 this feature is enabled. Rows in the
image between lines y0 and y1 (inclusive) are excluded from consideration
when the field matching is decided. This feature is typically used to ignore
subtitling, which might otherwise throw off the matching. y0 and y1 must both
be positive integers and y0 must be less than or equal to y1; if this is
violated an exception will be thrown.

**hints** (true/false, default true) enables Telecide() to pass hints to
Decimate(). Decimate uses these hints to optimize its behavior in modes 1 and
3.

**ovr** (string, default "") enables specification of an overrides file
(see the section above called "Overriding Decomb Decisions"). The file must
be in the same directory as the script file (the Avisynth current directory)
and the filename must be enclosed in quotation marks, e.g., ovr="tango.tel".

**debug** (true/false, default false) enables logging/debugging information
about the filter's decisions to be printed via OutputDebugString(). A utility
called DebugView is available for catching these strings.

-------


FieldDeinterlace(parameter_list)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**full** (true/false, default true) chooses whether to process all frames
or just the frames that are detected as combed. Use full=true to process all
frames.

**threshold** (0-255, default 20) sets the combed frame detection
threshold. When running with full=false, you may want to increase this value
if too many good frames are being deinterlaced, or reduce it if small combed
areas are not getting caught. The default is a good general purpose value.
Note that this threshold determines whether a frame is considered combed and
needs to be deinterlaced; it is not the threshold you might be familiar with
in Smart Deinterlacer. That threshold is determined by *dthreshold* (below);
it is the threshold for deinterlacing the frames *detected as combed*. When
full=true, threshold is ignored, but dthreshold remains functional.

**dthreshold** (0-255, default 7) sets the threshold for deinterlacing
frames detected as combed. Note that this threshold is the threshold you
might be familiar with in Smart Deinterlacer.

**blend** (true/false, default true) enables blending instead of
interpolating in combed areas.

**map** (true/false, default false) enables display of the combing
detection map (motion map). If full=true, the map is shown for all frames. If
full=false, the map is shown only for frames detected as combed; non-combed
frames are displayed normally. The map shows combed areas as bright cyan;
non-combed areas are copied from the source frame and blended with gray.

**chroma** (true/false, default false) determines whether chroma combing is
included in the decision made during postprocessing as to whether a frame is
combed or not. If chroma=true, then chroma combing is included, otherwise it
is not included. Note that chroma is always deinterlaced; this parameter
affects only the decision about whether a frame is combed. It is useful for
clips which have a large amount of luma/chroma interference, as might result
from a poor comb filter. The interference can cause frames that are not
combed to be detected as combed when chroma=true. By setting chroma=false,
the effect of the interference can be eliminated. This option has no effect
when full=true because all frames are considered combed.

**ovr** (string, default "") enables specification of an overrides file
(see the section above called "Overriding Decomb Decisions"). The file must
be in the same directory as the script file (the Avisynth current directory)
and the filename must be enclosed in quotation marks, e.g., ovr="tango.fd".

**show** (true/false, default false) enables metrics to be displayed on the
frame to assist with tweaking of thresholds. Also displays the software
version.

**debug** is the same as described for Telecide.

-------


Decimate(parameter_list)
~~~~~~~~~~~~~~~~~~~~~~~~

**cycle** (2-25, default 5) selects the decimation ratio, that is, decimate
1 frame in every cycle frames.

**mode** (0-3, default 0) determines how Decimate() deals with the extra
frame in the cycle.

If mode=0, Decimate discards the frame in the cycle determined to be most
similar to its predecessor.

If mode=1, instead of discarding the most similar frame, Decimate() will
either replace it with a frame interpolated between the current frame
(usually a duplicate of the preceding frame) and the following frame, or it
will pass the frame through as is. The choice between these two depends on
two things: 1) whether the cycle of frames is from 3:2 pulldown material or
from pure video, and 2) the threshold parameter setting and on how different
the frame is from its preceding frame (see below). The first requires guide=1
for Telecide (to declare the pulldown pattern).

Decimate(mode=1) is useful for hybrid clips having mostly video. It allows
you to leave the frame rate at video rates (to get smooth video sequences)
but also to ameliorate the effect of duplicate frames that are emitted by
Telecide() for film sequences (frames that are normally removed with mode=0).

Refer to APPENDIX B for more details and a guide to processing hybrid
material.

If mode=2, Decimate() deletes a frame from the longest run of duplicates.
This mode is the most reliable with anime and other material where the motion
may occur only in every second, third, or fourth (etc.) frames. If you use
mode=0 on such clips, there is a danger that incorrect decimation may occur,
causing jerkiness. Clips such as those described usually derive from 8fps or
12fps animation, as well as normal 24fps animation where slow motion results
in repeated duplicates. Mode=2 is able to delete the correct duplicates in
all these cases.

If mode=3, instead of discarding the most similar frame, Decimate() will
apply different decimation strategies for 3:2 pulldown material (film) and
for pure video (nonfilm) cycles. Film cycles are decimated in the normal way.
Nonfilm cycles are decimated by applying a frame blending decimation of the
cycle from 5 frames to 4 frames. The resulting frame rate is the film rate
(decimated by 1/5).

There are two factors that affect the decision about how to decimate: 1)
whether the cycle of frames is from 3:2 pulldown material or from pure video,
and 2) the threshold parameter setting and on the difference metric for the
most similar frame. The first requires guide=1 for Telecide (to declare the
pulldown pattern).

Decimate(mode=3) is useful for hybrid clips having mostly film. It allows you
to decimate the clip to film rates and treat the film normally while doing a
blend decimation of video sequences to retain their smoothness.

Refer to APPENDIX B for more details and a guide to processing hybrid
material.

**threshold** (decimal, default 0.0) When mode=1, frames determined to be
the most similar to their preceding frame can be treated in two possible
ways: 1) they can be blended as described above, or 2) they can be passed
through if the threshold parameter is non-zero and the difference metric
exceeds the threshold. By setting an appropriate threshold, you can have
duplicates get blended while passing through frames that have new content,
i.e., which differ significantly from the previous frame. This allows hybrid
film/nonfilm clips to be dealt with intelligently: the film portions will
have blends and the nonfilm portions will not. The threshold parameter has no
effect when mode=0. Use show=true (or debug=true in conjunction with the
DebugView utility) to view the difference metrics and thereby determine an
appropriate threshold for your clip.

the threshold parameter also affects mode=3 in a similar way. Refere to
APPENDIX B for details.

**threshold2** (decimal, default 3.0) When mode=2, Decimate() deletes a
frame from the longest run of duplicates as described above. The threshold2
parameter controls how close two frames must be to be considered duplicates.
If threshold2 is raised, a larger frame difference is tolerated while still
declaring them as duplicates. The default setting works well for most clips.
Use show=true (or debug=true in conjunction with the DebugView utility) to
view the difference metrics and thereby determine an appropriate threshold
for your clip.

**quality** (0-3, default 2) This option allows the user to trade off
quality of difference detection against speed. Following are the
possibilities:

-   quality=0: Subsampled for speed and chroma not considered (fastest).
-   quality=1: Subsampled for speed and chroma considered.
-   quality=2: Fully sampled and chroma not considered.
-   quality=3: Fully sampled and chroma considered (slowest).

**ovr** (string, default "") enables specification of an overrides file
(see the section above called "Overriding Decomb Decisions"). The file must
be in the same directory as the script file (the Avisynth current directory)
and the filename must be enclosed in quotation marks, e.g., ovr="tango.dec".

**show** (true/false, default false) enables metrics to be displayed on the
frame to assist with tweaking of thresholds. Also displays the software
version.

**debug** (true/false, default false) enables logging/debugging information
about the filter's decisions to be printed via OutputDebugString(). A utility
called DebugView is available for catching these strings. The information
displayed is the same as shown by the show option above.

-------


IsCombed(parameter_list)
~~~~~~~~~~~~~~~~~~~~~~~~

This utility filter is designed for use within ConditionalFilter(). It
returns a boolean to indicate whether a frame is combed (interlaced).

**threshold** (0-255, default 20) sets the amount of combing required to
declare a frame combed. It is analogous to the *threshold* parameter of
FieldDeinterlace(). You may have to tweak this for best performance with your
specific material.

--------


APPENDIX A. Telecide() Theory of Operation
------------------------------------------

Here's what Telecide does by default. When it receives a request for a frame
it gets access to the next frame and the requested one (called the current
frame). Here is what he sees (where N=next, C=current, t=top field, b=bottom
field):

| ``Ct Nt``
| ``Cb Nb``

Telecide makes 2 combinations of frames from the available fields. Here are
the combinations:

| ``Nt``
| ``Cb``

| ``Ct``
| ``Cb``

Now both of these frames are checked for combing. The least combed frame is
the output frame (assuming no pattern guidance). It is that simple.

If pattern guidance is enabled, the decision described above can be
overridden by a predicted match based on the perceived pattern surrounding
the current frame. The prediction is ignored if the difference between the
matches is too large, as that implies a possible change of phase of the
pattern, to which Telecide() should re-sync.

Consider now a cycle of frames from a 3:2 pulldown sequence (top field
first):

| ``a a b c d``
| ``a b c c d``

Telecide will produce matched output frames as follows:

| ``a b c c d``
| ``a b c c d``

It can be seen that a duplicate frame has been produced. Decimate removes the
duplicate frame by finding and removing the frame most similar to its
predecessor. It is easy to see now why Telecide followed by Decimate produces
an inverse 3:2 telecine (IVTC) function.

--------


APPENDIX B. Dealing with Hybrid Clips
-------------------------------------

Most real-world clips in NTSC environments are mixtures of 3:2 pulldown
material (film) and pure video (nonfilm) material. This presents a difficulty
for encoding because our formats require a specification of frame rate as
either 29.97fps or 23.976fps. If we choose the video rate, the video
sequences will be OK, but the film sequences will not be decimated and will
emit duplicates, appearing jumpy.

On the other hand, if we choose the film rate, the film sequences will be OK,
but the video sequences will be decimated, appearing jumpy.

Decomb provides two special decimation modes to better handle hybrid clips.

**Mostly Film Clips (mode=3)**

Let's first consider the case where the clip is mostly film. In this case, we
want to decimate the film portions normally so they will be smooth. For the
nonfilm portions, we want to reduce their frame rate by blend decimating each
cycle of frames from 5 frames to 4 frames. Video sequences so rendered appear
smoother than when they are decimated as film.

Here is a typical script to enable this mode of operation:
::

    Telecide(guide=1)
    Decimate(mode=3,threshold=1.0)

There are 2 factors that enable Decimate to treat the film and nonfilm
portions appropriately. First, when Telecide declares guide=1, it is able to
pass information to Decimate about which frames are derived from film and
which from video. For this mechanism to work, Decimate must immediately
follow Telecide. Clearly, the better job you do with pattern locking in
Telecide (by tweaking parameters as required), the better job Decimate can
do.

The second factor is the threshold. If a cycle of frames is seen that does
not have a duplicate, then the cycle is treated as video. The threshold
determines what percentage of frame difference is considered to be a
duplicate. Note that threshold=0 disables the second factor.

**Mostly Video Clips (mode=1)**

Now let's consider the case where the clip is mostly video. In this case, we
want avoid decimating the video portions be smooth. For the film portions, we
want to leave them at the video rate but change the duplicated frame into a
frame blend so it is not so obvious.

Here is a typical script to enable this mode of operation:
::

    Telecide(guide=1)
    Decimate(mode=1,threshold=1.0)

There are 2 factors that enable Decimate to treat the film and nonfilm
portions appropriately. First, when Telecide declares guide=1, it is able to
pass information to Decimate about which frames are derived from film and
which from video. For this mechanism to work, Decimate must immediately
follow Telecide. Clearly, the better job you do with pattern locking in
Telecide (by tweaking parameters as required), the better job Decimate can
do.

The second factor is the threshold. If a cycle of frames is seen that does
not have a duplicate, then the cycle is treated as video. The threshold
determines what percentage of frame difference is considered to be a
duplicate. Note that threshold=0 disables the second factor.

--------


Acknowledgements
----------------

I'd like to thank Thomas Daniel ('manono') for his valuable assistance during
the development of this software. He not only made several valuable
suggestions and pointed out useful resources, but he performed torture
testing on very difficult streams and ran head-to-head tests against other
available decombing software.

Darryl Andrews provided valuable assistance in specifying and testing the new
features in Version 4.00.

Klaus Post ('sh0dan') provided some code and ideas for low-level optimization
of the YV12 code.

Members of the forum at doom9.org provided much help and encouragement.

--------

Copyright (C) 2003, Donald A. Graft, All Rights Reserved.

For updates and other filters/tools, visit my web site:
`<http://neuron2.net/>`_

$Date: 2004/08/13 21:57:25 $

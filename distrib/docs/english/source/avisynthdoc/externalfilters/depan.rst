
DePan
=====


Abstract
--------

| **author:** Alexander G. Balakhnin aka Fizick
| **version:** 1.7
| **download:** `<http://avisynth.org.ru>`_
| **category:** Misc Plugins
| **requirements:** YV12 or YUY2 Colorspace
| **license:** closed source


Introduction
------------

DePan plugin has tools (functions) for estimation of global motion (pan) in
frames, and for full or partial global motion compensation.

The DePan tools can be used for:

-   global motion compensation of neighbor frames for strong temporal
    denoising and clip (film) restoration.
-   recreating of damaged frames by motion interpolation,
-   creating series of intermediate frames for frequency (fps) changing.
-   doing partial motion stabilization.

The DePan plugin replaced my experimental GenMotion C-plugin (which uses
motion data from VirtualDub Deshaker plugin log file).

The DePan works in one pass, but consist of server part (function) and one or
more clients parts (functions or its instances). The server function
estimates frames motion data and gives it to client functions on inquiry.
Current version of DePan uses a special service clip as the motion data
container.


DePan plugin functions
----------------------

| ``DePanEstimate`` (server) - estimates frames global motion data and write it to
  special service clip
| ``DePan`` (client) - make full or partial global motion compensation
| ``DePanInterleave`` (client) - generates long motion compensated interleaved clip
| ``DePanStabilize`` (client) - stabilizes motion
| ``DePanScenes`` (client) - scene change indication


DePanEstimate function
~~~~~~~~~~~~~~~~~~~~~~

This function uses phase-shift method (by fast Fourier transform) for global
motion estimation. It uses some central region of every frame (or field) as
FFT window to find the inter-frames correlation and to calculate the most
appropriate values of vertical and horizontal shifts, which fit current frame
to previous one. The some relative correlation parameter is used as trust
measure and for scene change detection. In zoom mode, plugin uses left and
right sub-windows to estimate both displacements and zoom. Output is special
service clip with coded motion data in frames, and optional log file.


Function call
:::::::::::::

``DePanEstimate`` ( clip, int range, float trust, int winx, int winy, int
dxmax, int dymax, float zoommax, bool improve, float stab, float pixaspect,
bool info, string log, bool debug, bool show, bool fftw)


Parameters of DePanEstimate
:::::::::::::::::::::::::::

| *clip* - input clip
| *range* - number of previous (and also next) frames (fields) near requested
  frame to estimate motion (integer value >=0, default=1)
| *trust* - limit of relative maximum correlation difference from mean value at
  scene change (0.0 to 100.0, default=4.0)
| *winx* - number of columns (width) of fft window (must be power of 2  if not
  fftw, default = maximum within frame width).
| *winy* - number of rows (height) of fft window (must be power of 2  if not
  fftw, default = maximum within frame height).
| *dxmax* - limit of x shift (default = winx/4)
| *dymax* - limit of y shift (default = winy/4)
| *zoommax* - maximum zoom factor (if = 1 (default), zoom is not estimated)
| *improve* - improve zoom estimation by iteration (default = false). Since v1.6
  this mode is disabled.
| *stab* - decreasing of calculated trust for large shifts ( factor dxmax/(dxmax
  + stab*abs(dx)) ):
|    = 0.0 (default)- not decrease,
|    = 1.0 - half at dxmax, dymax.
| *pixaspect* - pixel aspect (default = 1.0)
| *info* - show motion info on frame (default = false)
| *log* - output log filename with motion data (in the VirtualDub Deshaker plugin
  format) (default none, not write)
| *debug* - output data for debugview utility (default = false)
| *show* - show correlation surface (default = false)
| *fftw* - use external FFTW library (default = false, use internal Ooura FFT2D
  code)

DePan function
~~~~~~~~~~~~~~

It generates the clip with motion compensated frames, using motion data
previously calculated by DePanEstimate.


Function call
:::::::::::::

``DePan`` (clip, clip data, float offset, int subpixel, float pixaspect, bool
matchfields, int mirror, int blur, bool info, string inputlog)


Parameters of DePan
:::::::::::::::::::

| *clip* - input clip (the same as input clip for DePanEstimate)
| *data* - special service clip with coded motion data, produced by DePanEstimate
| *offset* - value of compensation offset for all input frames (fields) (from -
  10.0 to 10.0, default =0)
|    = 0 is null transform.
|    = -1.0 is full backward motion compensation of next frame (field) to current,
|    = 1.0 is full forward motion compensation of previous frame (field),
|    = -0.5 is backward semi-compensation of next frame (field),
|    = 0.5 is forward semi-compensation of previous frame (field),
|    = 0.3333333 is forward one-third compensation of previous frame (field),
|    = -1.5 is backward semi-compensation of next next frame (field),
|    = 2.0 is full forward motion compensation of previous previous frame (field),
|    and so on.
| *subpixel* - pixel interpolation accuracy (default = 2)
|    0 - pixel accuracy (at nearest pixel), no interpolation (fast),
|    1 - subpixel accuracy with bilinear interpolation,
|    2 - subpixel accuracy with bicubic interpolation (best).
| *pixaspect* - pixel aspect (default = 1.0)
| *matchfields* - match vertical position of interlaced fields for preserve
  fields order, better denoising etc (default=true)
| *mirror* - fill empty borders with mirrored from frame edge pixels (instead of
  black):
|    0 - no mirror (default);
|    1 - top;
|    2 - bottom;
|    4 - left;
|    8 - right;
|    sum any of above - combination (15 - all ).
| *blur* -  blur mirrored zone by using given max blur length (default=0,  not
  blur;   the good values is above 30)
| *info* - show motion info on frame (default=false).
| *inputlog* - name of input log file in Deshaker format (default - none, not
  read)

Note: The offset parameter of DePan is extended version of delta parameter of
GenMotion.


DePanInterleave
~~~~~~~~~~~~~~~

It generates long interleaved clipwith series of group of previous frames
motion compensated (within some range), original frame, and motion
compensated next frames (within range), and same groups for every following
frames. In fact, it combines DePan function and ``Interleave`` function
(AviSynth internal) for easy following temporal denoising, with following
SelectEvery(prev+next+1, prev) function for selecting only cleaned source
frames.


Function call
:::::::::::::

``DePanInterleave`` (clip, clip data, int prev, int next, int subpixel, float
pixaspect, bool matchfields, int mirror, int blur, bool info, string
inputlog)


Parameters of DePanInterleave similar to Depan
::::::::::::::::::::::::::::::::::::::::::::::

| *clip* - input clip (the same as input clip for DePanEstimate)
| *data* - special service clip with coded motion data, produced by DePanEstimate
| *prev* - number of previous frames (fields) in group to compensate (integer>0,
  default=1)
| *next* - number of next frames (fields) in group to compensate (integer>0,
  default=1)
| *subpixel* - pixel interpolation accuracy (default = 1)
|     0 - pixel accuracy (at nearest pixel), no interpolation (fast),
|     1 - subpixel accuracy with bilinear interpolation, (optimal for denoising)
|     2 - subpixel accuracy with bicubic interpolation (best but slow).
| *pixaspect* - pixel aspect (default = 1.0)
| *matchfields* - match vertical position of interlaced fields for better
  denoising etc (default=true)
| *mirror* - fill empty borders with mirrored from frame edge pixels (instead of black):
|     0 - no mirror (default);
|     1 - top;
|     2 - bottom;
|     4 - left;
|     8 - right;
|     sum any of above - combination (15 - all ).
| *blur* -  blur mirrored zone by using given max blur length (default=0,  not
  blur; the good values is above 30)
| *info* - show motion info on frame (default=false).
| *inputlog* - name of input log file in Deshaker format (none default, not read)

DePanStabilize
~~~~~~~~~~~~~~

This function make some motion stabilization (deshake) by smoothing of global
motion. Inertial filtering method is used (probably similar to Digistudio
VirtualDub plugin).


Function call
:::::::::::::

``DePanStabilize`` (clip, clip data, float cutoff, float damping, float
initzoom, bool addzoom, int prev, int next, int mirror, int blur, int dxmax,
int dymax, float zoommax, float rotmax, int subpixel, float pixaspect,  int
fitlast, float tzoom, bool info, string inputlog)


Parameters of DePanStabilize
::::::::::::::::::::::::::::

| *clip* - input clip (the same as input clip for DePanEstimate);
| *data* - special service clip with coded motion data, produced by
  DePanEstimate;
| *cutoff* - vibration frequency cutoff , Hertz (default = 1.0);
| *damping* - damping ratio (default = 1.0);
| *initzoom* - initial (minimal) zoom to fill borders (default = 1.0);
| *addzoom* - use additional adaptive zoom (default=false);
| *prev* - lag of some previous frame to fill empty borders (instead of black):
|     0 - not fill (default ),
|     1 - use nearest previous (n-1) frame to fill current frame (n) edges,
|     2 - use previous (n-2) frame to fill (not all in range !),
|     and so on.
| *next* - lag of some next frame to fill empty borders (instead of black):
|     0 - not fill (default ),
|     1 - use nearest next (n+1) frame to fill current frame (n) edges,
|     2 - use next (n+2) frame to fill (not all in range !),
|     and so on.
| *mirror* - fill empty borders with mirrored from frame edge pixels (instead of
  black):
|     0 - no mirror (default);
|     1 - top;
|     2 - bottom;
|     4 - left;
|     8 - right;
|     sum any of above - combination (15 - all ).
| *dxmax* - limit of horizontal correction, in pixels (default = 60);
| *dymax* - limit of vertical correction, in pixels (default = 30);
| *zoommax* - limit of zoom correction (only adaptive zoom, default = 1.05);
| *rotmax* - limit of rotation correction, in degrees (default = 1.0);
  these values limit the correction (since v1.7 - approximately, not
  strictly )
| *subpixel* - pixel interpolation accuracy (default = 2):
|     0 - pixel accuracy (at nearest pixel), no interpolation (fast);
|     1 - subpixel accuracy with bilinear interpolation;
|     2 - subpixel accuracy with bicubic interpolation (best).
| *pixaspect* - pixel aspect (default = 1.0);
| *fitlast* - fit some last frames range to original position (integer range,
  default=0)
| *tzoom* - adaptive zoom rise time, sec (float, default=3.0)
| *info* - show motion info on frame (default=false).
| *inputlog* - name of input log file in Deshaker format (none default, not read)

DePanScenes function
~~~~~~~~~~~~~~~~~~~~

Generate clip with pixel values =255 for defined plane at scenechange and
pixel values =0 at rest frames,
using motion data previously calculated by DePanEstimate.

May be used by AverageLuma function for conditional processing.


Function call
:::::::::::::

``DePanScenes`` ( clip, string inputlog, int plane)


Parameters of DePanScenes
:::::::::::::::::::::::::

| *clip* - input clip (special service clip with coded motion data, produced by
  DePanEstimate)
| *inputlog* - name of input log file in Deshaker format (default - none, not
  read)
| *plane* - code of plane to mark (1 - Y, 2 - U, 4 - V, sum - combination,
  default=1)

Features and limitations of current version of DePan plugin
-----------------------------------------------------------

1. Works only in YV12 and YUY2 color formats.
2. Uses only pan and zoom motion (no rotation), but it gives advance in
   speed and stability. Estimation in zoom mode is not very precise.
3. The source clip  must be same length as motion data clip.
4. Directly works only with progressive clips. For interlaced sources,
   you must use AviSynth following function ``SeparateFields`` and followed
   ``Weave`` (after motion compensation and denoising), with ``AssumeTTF`` and
   ``AssumeBFF`` (both may be needed for odd fields offset). Plugin estimates
   and calculates motion from one field to neighbor (by time) field (from same
   or neighbor frame). For preserving fields order (dominance) and best
   denoising, set parameter MatchFields=true.
5. Mirror mode is unique but slightly strange :-). The blur is some
   workaround to hide sharp mirrored details.
6. Not very fast, not assembler optimized.
7. Tested with Avisynth 2.5.3 and 2.55.
8. The plugin function DePanEstimate uses free FFT2D code by Takuya Ooura
   (`<http://momonga.t.u-tokyo.ac.jp/~ooura/index.html>`_)

   Since version 1.0, DePan can also use more fast FFTW library version 3
   (`<http://www.fftw.org>`_)
   as Windows binary DLL (compiled with gcc under MinGW by Alessio Massaro),
   which support for threads and have AMD K7 (3dNow!) support in addition to
   SSE/SSE2.

   It may be downloaded from `<ftp://ftp.fftw.org/pub/fftw/fftw3win32mingw.zip>`_
   For fftw using, you must put FFTW3.DLL file from this package to some
   directory in path (for example, ``C:\WINNT``).
9. For best results, you may temporary add Info parameter, analyze info
   and tune some parameters (Trust, dxmax etc).
10. You may use not strictly same clips for motion estimation and
    compensation, for example try add some brightness-contrast adjusting,
    pre-filtering, masking, cropping  to input clip used for motion
    estimation only (and use different processing for output compensated-
    stabilized results).


DePan Using
-----------


Using DePan for preparation of interleaved motion compensated clip with following strong temporal denoising
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

1. Load original (input) clip (I),
2. Make clip (F) with full forward motion compensation,
3. Make clip (B) with full backward motion compensation,
4. Make a interleave clip, with compensated frames before and after every
   original frame;
   We will get a long clip (with triple length), with every 3 successive frames
   corresponded to same time.
5. Apply some temporal filter that uses pixel differences between
   previous, current and next frames, for example Fluxsmooth filter.
6. Select every third (original non-compensated but cleaned) frame to
   output.
   The cleaned clip will not have a lot of artifacts, produced by global motion
   with denoising, and the denoising will be more strong in most areas (camera's
   pan will be compensated.)

Notes: with ``DePanInterleave``, stages 2,3,4 combined to single. Moreover,
the range may be large than 1.


Simple sample script for progressive clip
:::::::::::::::::::::::::::::::::::::::::

::

    AviSource("input.avi")
    LoadPlugin("depan.dll")
    LoadPlugin("fluxsmooth.dll")

    i = ConvertToYV12()
    mdata = DePanEstimate(i)
    DePanInterleave(i, data=mdata)
    FluxSmooth()
    SelectEvery(3, 1)

For best results, you may temporary add Info parameter, analyze info and tune
some parameters (Trust, dxmax etc)


Sample script for interlaced clip
:::::::::::::::::::::::::::::::::

::

    LoadPlugin("depan.dll")
    LoadPlugin("fluxsmooth.dll")

    AviSource("input.avi")

    AssumeTFF()
    SeparateFields()
    i = ConvertToYV12()
    mdata = DePanEstimate(i, range=1, trust=5.5, log="depan.log")
    DePanInterleave(i,data=mdata, prev=1, next=1, matchfields=true)
    FluxSmooth()
    SelectEvery(3, 1)
    Weave()


Some suitable temporal denoising filters:

-   CTMedian (Conditional Temporal Median) by Kevin Atkinson
-   and its new version renamed to DeSpot (by Fizick) - for spot removing
-   STMedianFilter by Tom Barry - trbarry@trbarry.com
-   FluxSmooth by Ross Thomas <ross@grinfinity.com>
-   RemoveDirt by Rainer Wittmann gorw@gmx.de
-   DeGrainMedian by Fizick

Please, make a tests to add filters to the list!

For proposed denoising method with using of the DePan (previously with
GenMotion), such temporal filter must compare pixel with previous and next
frame, and make some smoothing if difference between previous and next frame
is small. These filters also may make additional internal (small) local
motion compensation (as Dust filter, which may get some speed increasing due
to well global motion compensation).


Using DePan for framerate change
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

DePan may be used as a tool for framerate converting and similar tasks.

For example, to change framerate with factor=1.5, from 16.6 fps progressive
(old 8 mm film) to 25 fps, use script.

::

    LoadPlugin("depan.dll")
    AviSource("kino.avi")
    i = ConvertToYV12()

    data = DePanEstimate(i, range=1, trust=5)
    f1_3 = DePan(i, data, offset=1./3)
    b1_3 = DePan(i, data, offset=-1./3)
    Interleave(f1_3, i, b1_3)
    SelectEvery(6, 0, 1, 2)

It may by written as a function:
::

    function fps2to3(clip) {
    # change FPS from 2 to 3 (or 16.66 to 25, or 20 to 30 and so on), i.e. with factor=3/2
    # uses global motion compensation
    # input must be YV12 or YUY2 progressive (or separated fields probably ?)
    data = DePanEstimate(clip)
    f1_3 = DePan(clip, data, offset=1./3)
    b1_3 = DePan(clip, data, offset=-1./3)
    Interleave(f1_3, clip, b1_3)
    SelectEvery(6, 0, 1, 2)
    }

    LoadPlugin("depan.dll")
    AviSource("e:\video.avi")
    ConvertToYV12()
    fps2to3()

Here is a possible function for framerate converting (progressive) with factor=5/3, for example from 15 fps to 25 fps:
::

    function fps3to5(clip) {
    # change FPS from 3 to 5 (or 15 to 25, or 18 to 30 and so on), i.e.
    with factor=5/3
    # uses global motion compensation
    # input must be YV12 or YUY2 progressive (or separated fields
    probably ?)
    data = DePanEstimate(clip)
    t3_5 = DePan(clip, data, offset=-2./5)
    t6_5 = DePan(clip, data, offset=1./5).trim(2,0)
    t9_5 = DePan(clip, data, offset=-1./5).trim(1,0)
    t12_5 = DePan(clip, data, offset=2./5).trim(3,0)
    Interleave(clip, t3_5, t6_5, t9_5, t12_5)
    SelectEvery(15,0,1,2,3,4)
    }

    LoadPlugin("depan.dll")
    AviSource("e:\video.avi")
    ConvertToYV12()
    fps3to5()

Notes. There is more simple and general alternative method: try ``ChangeFPS``
with following ``DePanStabilize``.


Using DePan for motion stabilization
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

DePan may be used as a tool for smoothing of global motion. Inertial
filtering method is used in current version.

Simple sample script for progressive clip:

::

    LoadPlugin("depan.dll")
    AviSource("input.avi")
    i = ConvertToYV12()
    mdata = DePanEstimate(i)
    DePanStabilize(i, data=mdata)

We may add and tune parameters cutoff, dxmax, edges filling method etc,
corresponded to your clip and you.


Using log files
~~~~~~~~~~~~~~~

DepanEstimate function may write optional log file with motion data, in
Deshaker - compatible format. Moreover, Depan function may read such log
files (in this mode it works as GenMotion, without DepanEstimate, data clip
is ignored, and source clip may be used as dummy data clip). Deshaker log may
be loaded in Depan and vice versa. Depan can compensate zoom and rotation
too. Therefore you may load similar AVS script files in VirtualDub, and run
second pass of Deshaker for anvanced image stabilization (and coding) of
filtered clip. Of course, before you must run first pass of DePanEstimate to
make Depan.log file, which must be selected in Deshaker. Instead of that, you
may add DePanStabilize(i,data) function to script and run all in one pass !


Deshaker log file format (after Gunnar Thalin)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

During Pass 1 the Deshaker plugin tries to find the panning, rotation and
zoom that, when applied to the current image, makes it look like the previous
image (almost). The values on each line in the file are (from left to right):
frame number (or fields number), x- and y-panning (in pixels), rotation (in
degrees) and zoom factor. You can edit the log file manually (but use fixed
line format). You can delete lines that got completely wrong (and that you
don't care to try to fix in a better way). Gaps in the frames numbers are
treated as zero-panning, zero rotation and no scaling. If a frame exists more
than once in the log file, the last line is used.

Note: For interlaced source, info is for every field (A - first, B - second
by time)


DePan client-server framebuffer format (mostly for programmers)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Depan use framebuffer of special clip for storing of motion data. When client
(Depan) requests the motion data for frame ``n`` from this clip, server
(DepanEstimate) creates frame and writes such data (from start of
framebuffer): one header record, and several frame motion data records, from
``n-range`` to ``n+range`` (``nframes = 2*range+1`` for non-edge frames).
Definition of motion data parameters is same as in Deshaker log.

In all versions from 0.6, I use this structures:
::

    #define DEPANSIGNATURE "depan06"

    typedef struct depanheaderstruct { // structure of header depandata
    in framebuffer
    char signature[8]; // signature for check
    int reserved; // for future using
    int nframes; // number of records with frames motion data in current
    framebuffer
    } depanheader;

    typedef struct depandatastruct { // structure of every frame motion
    data record in framebuffer
    int frame; // frame number
    float dx; // x shift (in pixels) for this frame
    float dy; // y shift (in pixels, corresponded to pixel aspect = 1)
    float zoom; // zoom
    float rot; // rotation (in degrees), (now =0 - no rotation estimated
    data in current version)
    } depandata;

Note 1. Depan uses dx=0.0 as mark of scenechange.

Note 2. DepanEstimate output is cropped if not in show or info mode.


Alternative method for Global motion estimation
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Recently I added to local motion estimation ``MVTools`` plugin by Manao (to
version 0.9.8.2) a new function ``MVDepan`` for global motion estimation. It
is based on local block motion vectors analysis, similar to first pass of
DeShaker plugin. ``MVDepan`` function can be used instead of
``DepanEstimate`` function. It can estimate pan, zoom and rotation, but at
present it is still experimental beta version. Search for newest ``MVTools``
and its documentation at Manao's site `<http://manao4.free.fr/>`_. Discussion
is at AviSynth doom9 forum.


More info about Depan
~~~~~~~~~~~~~~~~~~~~~

Some discussion about GenMotion and DePan plugins may be found in AviSynth
forum at
`<http://forum.doom9.org/forumdisplay.php?s=&forumid=33>`_
in particular in thread `<http://forum.doom9.org/showthread.php?s=&threadid=66686>`_


Acknowledgments
~~~~~~~~~~~~~~~

Thanks to Gunnar Thalin for detailed info about Deshaker log file format and
very useful discussions.

Thanks to Takuya Ooura for free and fast FFT2D code.

Thanks to scharfis_brain and many others for useful discussions and bug
reports.

+--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| Version changes                                                                                                                                                                                  |
+========+===============================+=========================================================================================================================================================+
| v0.1   | April 25, 2004                | first public (beta!).                                                                                                                                   |
+--------+-------------------------------+---------------------------------------------------------------------------------------------------------------------------------------------------------+
| v0.2   | April 27, 2004                | fixed bug for non-integer Offset values.                                                                                                                |
+--------+-------------------------------+---------------------------------------------------------------------------------------------------------------------------------------------------------+
| v0.3   | May 15, 2004                  || fixed bug with DePanStabilize input parameters, set subpixel=2 as default;                                                                             |
|        |                               || added zoom estimation and ZoomMax parameter, zoom and rotation compensation, log file input, MatchFields parameter.                                    |
+--------+-------------------------------+---------------------------------------------------------------------------------------------------------------------------------------------------------+
| v0.4   | May 16, 2004                  || fixed bug with MatchFields for big Offset;                                                                                                             |
|        |                               || fixed bug with pixel position for nearest and bilinear interpolation;                                                                                  |
|        |                               || set default MatchFields=true, add pixel aspect, Russian doc.                                                                                           |
+--------+-------------------------------+---------------------------------------------------------------------------------------------------------------------------------------------------------+
| v0.5   | May 22, 2004                  || fixed some bug with rotation in Depan                                                                                                                  |
|        |                               || improved DepanStabilize: changed stabilization method to inertial in wide range;                                                                       |
|        |                               || added parameters freqmax, dxmax, dymax, zoommax, rotmax, inputlog.                                                                                     |
+--------+-------------------------------+---------------------------------------------------------------------------------------------------------------------------------------------------------+
| v0.6   | May 28, 2004                  || minor changed and documented client-server format.                                                                                                     |
|        |                               || DepanEstimate: added stab parameter, scenechange at sharp trust changes,                                                                               |
|        |                               || range may be 0, show correlation, crop output.                                                                                                         |
|        |                               || DepanStabilize: Freqmax is renamed to Cutoff, add adaptive zoom, Fill.                                                                                 |
+--------+-------------------------------+---------------------------------------------------------------------------------------------------------------------------------------------------------+
| v0.7   | May 30, 2004                  | DepanEstimate: added improve zoom estimation.                                                                                                           |
+--------+-------------------------------+---------------------------------------------------------------------------------------------------------------------------------------------------------+
| v0.8   | June 06, 2004                 | DepanInterleave: replaced "Range" parameter to "Prev" and "Next".                                                                                       |
+--------+-------------------------------+---------------------------------------------------------------------------------------------------------------------------------------------------------+
| v0.9   | June 13, 2004                 | all clients: added "Mirror" parameter to fill empty borders.                                                                                            |
+--------+-------------------------------+---------------------------------------------------------------------------------------------------------------------------------------------------------+
| v0.9.1 | August 24, 2004               | Fixed bugs with zoom estimation and compensation.                                                                                                       |
+--------+-------------------------------+---------------------------------------------------------------------------------------------------------------------------------------------------------+
| v1.0   | September 3, 2004             | Added option for using of external FFTW library (more fast) .                                                                                           |
+--------+-------------------------------+---------------------------------------------------------------------------------------------------------------------------------------------------------+
| v1.1   | September 16, 2004            | Added experimental DepanScenes function.                                                                                                                |
+--------+-------------------------------+---------------------------------------------------------------------------------------------------------------------------------------------------------+
| v1.1.1 | October 07, 2004              || fixed bug with compensation near right bottom and top corners,                                                                                         |
|        |                               || changed from FFTW_MEASURE to FFTW_ESTIMATE for more short init, without speed change (for power-2 windows),                                            |
|        |                               || compiled without /G7 flag (as before v.1.0), added FPS script functions to doc. (not public)                                                           |
+--------+-------------------------------+---------------------------------------------------------------------------------------------------------------------------------------------------------+
| v1.1.2 | October 09, 2004              | delayed loading of  fftw3.dll (now optional).                                                                                                           |
+--------+-------------------------------+---------------------------------------------------------------------------------------------------------------------------------------------------------+
| v1.1.3 | November 16, 2004             | fixed bug with infinite shift in DePanStabilize.                                                                                                        |
+--------+-------------------------------+---------------------------------------------------------------------------------------------------------------------------------------------------------+
| v1.1.4 | December 15, 2004             || damping parameter in DePanStabilize is now variable (was accidentally fixed =0.9 in all previous versions :-),                                         |
|        |                               || added notes about MVDepan to documentation.                                                                                                            |
+--------+-------------------------------+---------------------------------------------------------------------------------------------------------------------------------------------------------+
| v1.1.5 | December 31, 2004             | bug fixed in DepanEstimate (erroneous motion data) for fftw=true with show=false and info=false                                                         |
+--------+-------------------------------+---------------------------------------------------------------------------------------------------------------------------------------------------------+
| v1.2   | April 1, 2005                 | added fitlast parameter to fit some last frames range to original position                                                                              |
+--------+-------------------------------+---------------------------------------------------------------------------------------------------------------------------------------------------------+
| v1.3   | April 29, 2005                || added blur parameter to somewhat hide the sharp mirrored details;                                                                                      |
|        |                               || blur is horizontal only, at left and right border.                                                                                                     |
+--------+-------------------------------+---------------------------------------------------------------------------------------------------------------------------------------------------------+
| v1.4   | May 7, 2005 (released May 29) || DePanStabilize: Zoom adaptive mode addzoom is improved.                                                                                                |
|        |                               || Adaptive zoom decreasing rate is slower than zoom increasing rate now. Thus, the black empty borders are decreased, and zoom value is more stable now. |
+--------+-------------------------------+---------------------------------------------------------------------------------------------------------------------------------------------------------+
| v1.4.1 | May 30, 2005                  | DepanEstimate: fixed bug with log file (A and B symbols swapped) for BFF (all previous versions). Thanks to eugvas for report.                          |
+--------+-------------------------------+---------------------------------------------------------------------------------------------------------------------------------------------------------+
| v1.5   | June 4, 2005                  | improved adaptive zoom; added tzoom parameter for the zoom rise time (was equal to 1/cutoff)                                                            |
+--------+-------------------------------+---------------------------------------------------------------------------------------------------------------------------------------------------------+
| v1.6   | August 5, 2005                | added YUY2 support; disabled improved=true mode (was broken); changed default subpixel=1 for DepanInterleave as sufficient for denoising and more fast  |
+--------+-------------------------------+---------------------------------------------------------------------------------------------------------------------------------------------------------+
| v1.7   | September 5, 2005             || DePanStabilize: added parameter initzoom - minimal zoom;                                                                                               |
|        |                               || changed limits dxmax, dymax, zoommax, rotmax from hard to soft with larger slope non-linearity.                                                        |
|        |                               || Changed cache.                                                                                                                                         |
+--------+-------------------------------+---------------------------------------------------------------------------------------------------------------------------------------------------------+


License
~~~~~~~

This program is freeware, but WITHOUT ANY WARRANTY.

$Date: 2006/09/17 17:41:38 $

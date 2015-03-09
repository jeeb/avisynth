
MVTools
=======


Abstract
::::::::

| **author:** Manao & Fizick
| **version:** 1.4.11
| **download:** `<http://avisynth.org.ru/>`_
| **category:** Misc Plugins
| **requirements:** YV12 or YUY2 Colorspace
| **license:** GPL

--------


.. contents:: Table of contents
    :depth: 3

.. sectnum::
    :depth: 3
    :suffix: .


About MVTools
-------------

MVTools plugin for AviSynth 2.5 is collection of functions for estimation and
compensation of objects motion in video clips. Motion compensation may be
used for strong temporal denoising, advanced framerate conversions, image
restoration and other tasks.

The plugin contains the motion estimation server-function MVAnalyse to find
the motion vectors and several motion compensation client-functions
(MVDenoise, MVCompensate, MVMask and others) which use these vectors.

Plugin uses block-matching method of motion estimation (similar methods are
used in MPEG2, MPEG4, etc). At analysis stage plugin divides frames by small
blocks and try to find for every block in current frame the most similar
(matching) block in second frame (previous or next). The relative shift of
these blocks is motion vector. The main measure of block similarity is sum of
absolute differences (SAD) of all pixels of these two blocks compared. SAD is
a value which says how good the motion estimation was.

The output of MVAnalyse (server) is special clip with motion vector
information in some format.

At compensation stage the plugin client functions read the motion vectors and
use them to move blocks and form motion compensated frame (or realize some
other full or partial motion compensation or interpolation function). Every
object (block) in this (fully) compensated frame is placed in the same
position as this object in current frame. So, we may (for example) use strong
temporal denoising even for quite fast moving objects without producing
annoying artefactes and ghosting (object's features and edges are coincide if
compensation is perfect). Plugin can create compensated neighbor frames for
every current frame, and denoise it by internal function (MVDenoise).
Alternatively, you can use compensated and original frames to create
interleaved clip, denoise it by any external temporal filter, and select
central cleaned original frames for output (see examples).

Of course, the motion estimation and compensation is not ideal and precise.
In some complex cases (video with fading, ultra-fast motion, or periodic
structures) the motion estimation may be completely wrong, and compensated
frame will be blocky and (or) ugly. Severe difficulty is also due to objects
mutual screening (occlusion) or reverse opening. Complex Avisynth scripts
with many motion compensation functions may eat huge amount of memory and
result in very slow processing. It is not simple but quite advanced plugin.
Use it for appropriate cases only, and try tune its parameters. There are
many discussions about motion compensation using at doom9 Avisynth forum. In
particular see `MVTools thread.`_ Try read postings in addition to this
documentation and ask for support there. If you really interested in motion
estimation and compensation topics, you can easy find numerous scientific
publications (use WWW search).

Notes: The plugin is still under development. Current version has some
limitations. Only progressive YV12, YUY2 video is supported. Use color format
conversion and try use (smart) bob-deinterlace for interlaced video.


Function descriptions
---------------------


Common parameters
~~~~~~~~~~~~~~~~~

Filters that use motion vectors have common parameters. Those are the scene-
change detection thresholds, and the mmx / isse flags. They also use one or
several vectors stream, which are produced by ``MVAnalyse``.

int *thSCD1* : threshold which decides whether a block has changed between the
previous frame and the current one. When a block has changed, it means for me
that motion estimation for him isn't relevant at all. It occurs for example
at scene changes. So it is one of the thresholds used to tweak the scene
changes detection engine. Raising it will lower the number of blocks detected
as changed. It may be useful for noisy or flickered video. The threshold is
compared to the SAD (sum of absolute difference, a value which says how good
the motion estimation was ) value. Suppose we have two compared 8x8 blocks
with every pixel different by 5. It this case SAD will be 8x8x5 = 320 (block
will not detected as changed for thSCD1=400). If you use 4x4 blocks, SAD will
be 320/4. If you use 16x16 blocks, SAD will be 320*4. Really this parameter
is scaled internally, and you must always use reduced to block size 8x8
value. Default is 400 (since v.1.4.1).

int *thSCD2* : threshold which sets how many blocks have to change for the
frame to be considered as a scene change. It is ranged from 0 to 255, 0
meaning 0 %, 255 meaning 100 %. Default is 130 ( which means 51 % ).

bool *isse* : flags which allows to disable ISSE and MMX optimizations if set
to false (for debugging). Default is true. If your processor doesn't support
ISSE MMX optimizations, it will be disabled anyway ( and you won't be able to
activate them )

bool *mmx*  : flags which allows you to disable mmx optimizations. Default is
true. This parameter is ignored in current and all (?) previous versions.

MVAnalyse
~~~~~~~~~

``MVAnalyse`` (clip, int "blksize", int "pel", int "level", int "search", int
"searchparam", bool "isb", int "lambda", bool "chroma", int "delta", int
"idx", bool "truemotion", int "lsad", int "pnew", int "plevel", bool
"global", int "overlap", string "outfile", int "sharp")

Estimate motion by block-matching method and produce special output clip with
motion vectors data (used by other functions).
Some hierarchical multi-level search methods are implemented (from coarse
image scale to finest). Function uses zero vector and neighbors blocks
vectors as a predictors for current block. Firstly difference (SAD) are
estimated for predictors, then candidate vector changed by some values to
some direction, SAD is estimated, and so on. The accepted new vector is the
vector with minimal SAD value (with some penalty for motion coherence).

*blksize* : Size of a block. It's either 4, 8 or 16 ( default is 8 ). Larger
blocks are less sensitive to noise, are faster, but also less accurate.

*pel* : it is the accuracy of the motion estimation. Value can only be 1 or 2.
1 means a precision to the pixel. 2 means a precision to half a pixel,
produced by spatial bilinear interpolation (better but slower). Default is 2
since v1.4.10.

*level* : it is the number of levels NOT used in the hierarchical analysis made
while searching for motion vectors. The lower the better. It is kept variable
for study's purposes only. Default : 0 (all levels are used).

*search*, *searchparam* : search decides the type of search, and searchparam is
an additional setting for this search :

-   search = 0 : 'OneTimeSearch'. searchparam is the step between each
    vectors tried ( if searchparam is superior to 1, step will be
    progressively refined ).
-   search = 1 : 'NStepSearch'. N is set by searchparam. It's the most
    well known of the MV search algorithm.
-   search = 2 : Logarithmic search, also named Diamond Search.
    searchparam is the initial step search, there again, it is refined
    progressively. It's the default search ( with searchparam = 2 )
-   search = 3 : Exhaustive search, searchparam is the radius. It is
    slow, but it gives the best results, SAD-wise.

*isb* : allows to choose between a forward search ( between the current frame
and the previous one ) and a backward one ( between the current frame and the
following one ). isb = false means forward search ( isb stands for "IS
Backward" ).

*chroma* : set to true, it allows to take chroma into account when doing the
motion estimation (false - luma only). Default is true.

*delta* : set the frame interval between the reference frame and the current
frame. By default, it's 1, which means that the motion vectors are searched
between the current frame and the previous ( or next ) frame. Setting it to 2
will allow you to search mvs between the frame n and n-2 or n+2 ( depending
on the isb setting ).

*idx* : allows the filter to store the interpolation he made during the motion
estimation, in order them to be reused by another instance of the filter on
the same clip for more fast processing. It allows for example, when doing a
forward & backward search on the same clip, to avoid to compute twice the
bilinear interpolation, if pel = 2. By default, a unique negative number is
given (unique for each filter). If you use it, you should always use
positive values, and you should only use the same value for filters which
work on the same clip (else, the analysis won't work properly).

There are few advanced parameters which set coherence of motion vectors for
so called true motion estimation. Some matched blocks from other frame may be
most similar to sample blocks of current frame by intensity criterion (SAD),
but not correspond to true object motion. For example, they may belong to
other similar object in different corner of the frame or belong to some
periodic structure. "True motion" parameters try maintain the motion field
more coherent, instead of some random vectors distribution. It is especially
important for partial motion compensation and interpolation. Some parameters
are experimental and may be removed (replaced) in next versions after
testing. Please report your conclusions.

*truemotion* is a preset of these parameters values. It allows easy to switch
default values of all "true motion" parameters at once. Set it true for true
motion search (high vector coherence), set it false to search motion vectors
with best SAD. Default is true since v1.4.10. In any case you can tune each
parameter individually.

*lambda* : set the coherence of the field of vectors. The higher, the more
coherent. However, if set too high, some best motion vectors can be missed.
Values around 400 - 2000 (for block size 8) are strongly recommended.
Internally it is a SAD penalty coefficient for vector squared difference from
predictor (neighbors), scaled by 256.
Default is 0 for truemotion=false and 1000*blksize*blksize/64 for
truemotion=true.

*lsad*: SAD limit for lambda using. Local lambda is reset to 0 for vector
predictor (formed from neighbor blocks) with greater then the limit SAD
value. It prevents bad predictors using but destroy motion coherence. Values
above 1000 (for block size=8) are strongly recommended for true motion.
Default is 400*blksize*blksize/64 for truemotion=false and
1200*blksize*blksize/64 for truemotion=true.

*pnew*: penalty to SAD cost for new candidate vector. SAD (cost) of new
candidate vector must be better than predictor by this value to be accepted
as new vector. Values about 50-100 (for block size 8) are recommended for
true motion. It prevent replacing of quite good predictors by new vector with
a little better SAD but different length and direction.

pnew value is also used as upper limit of predictors SAD to skip further
search.

Default is 0 for truemotion=false and 75*blksize*blksize/64 for
truemotion=true.

| *plevel*: penalty factor lambda level scaling mode. Value=0 - no scaling, 1 -
  linear, 2 - quadratic dependence from hierarchical level size. Note, that
  vector length is smaller at lower level.
| Default is 0 for truemotion=false and 1 for truemotion=true

*global*: estimate global motion (at every level) and use it as an additional
predictor. Only pan shift is estimated (no zoom and rotation). Use false to
disable, use true to enable. Default is false for truemotion=false and true
for truemotion=true.

*overlap*: block overlap value. Must be even and less than block size. Overlap
is both vertical and horizontal. The step between blocks for motion
estimation is equal to (blksize-overlap). N blocks cover the size ((blksize-
overlap)*N + overlap) on frame. It is experimental option. Try use overlap
value from blksize/4 to blksize/2. The greater overlap, the more blocks
number, and the lesser the processing speed. Default value is 0.

Functions with overlap support: MVFlow, MVFlowInter, MVFlowFps1/2, MVShow,
MVVMask, MVCompensate, MVDeGrain1/2.

*outfile*: name of file to write motion vectors data. This data may be used by
some external program or may be by next MVTools versions for second pass
coding, etc.

| Produced binary file has a header (MVAnalysisData structure, see
  MVInterface.h source code), and the data sequence:
| frame number, vector data (Vx, Vy, SAD) of every block, next valid frame
  number, this frame vector data, and so on.
| Default - empty string, do not write.

| *sharp*: subpixel interpolation method for pel=2.
| Use 0 for soft interpolation (bilinear), 1 for bicubic interpolation (4 tap
  Catmull-Rom), 2 for sharper Wiener interpolation (6 tap, similar to Lanczos).
| Default is 2.

Try use ``MVShow`` function to check estimated motion field and tune
parameters.


MVCompensate
~~~~~~~~~~~~

``MVCompensate`` (clip, clip "vectors", bool "scbehavior", int "mode", int
"idx")

Do a full motion compensation of the frame. It means that the blocks pointed
by the mvs in the reference frame will be moved along the vectors to reach
their places in the current frame.

*scbehavior* ( by default true ), decide which frame will be kept on a scene
change. If true, the frame is left unchanged. Else, the reference frame is
copied into the current one.

*mode* can be either 0, 1 (default) or 2. Mode=0 (faster) means it uses the
compensation made during the vectors search. Mode=1 means it recomputes
compensation from vectors data ( because you may want to apply vectors to a
different clip that the one on which you searched ). Mode=2 means it
recomputes the compensation, but it does it in-loop, meaning that the vectors
will be applied to the last frame computed. Results will be ugly, and that
mode shouldn't be used except if you know what you're doing.

*idx* works the same way as idx in MVAnalyse. It is used only with mode = 1.

Overlaped blocks processing is implemented in mode=1 as window block
summation (like FFT3DFilter, overlap value up to blksize/2) for blocking
artefactes decreasing, and still as sequential covering by blocks from left
to right from top to bottom for others modes 0,2.


MVDenoise
~~~~~~~~~

``MVDenoise`` (clip, clip mvs [,...], bool "Y", bool "U", bool "V", int
"thT", int "thSAD", int "thMV")

Makes a temporal denoising with motion compensation. Reference frames are
motion compensated and then merged into the current frame.

The first threshold, *thT*, decides whether the pixel which comes from the
previous or the following frame has to be taken into account. If this pixel
differs more than thT from the pixel of the current frame, it is not used.

The second one, *thSAD*, decides whether the block has changed or not (same
principle as thSCD1). If it has changed, the pixels aren't merged with
those of the previous or following frame.

*thMV* is the vector's length over which the block isn't used for denoising.

Finally, *Y*, *U* and *V* tell which planes should be denoised.

Defaults are : Y, U and V are true, thT = 10, thSAD = 200 and thMV=30.


MVMask
~~~~~~

``MVMask`` (clip, clip "vectors", float "ml", float "gamma", int "kind", int
"Ysc")

Creates mask clip from motion vectors data. Mask is defined by blocks data,
but is interpolated to fit full frame size. The mask is created both on the
luma and on chroma planes. Mask values may be from 0 (min) to 255 (max).

*kind* parameter defines kind of mask.

Mode kind=0 creates motion mask from the motion vectors' length. It builds a
better mask than :ref:`MotionMask` ( ` MaskTools`_ ) because motion vectors are
a lot more reliable than the algorithm of MotionMask. Mask value 0 means no
motion at all ( the length of the motion vector is null ). The longer vector
length, the larger mask value (saturated to 255), the scale is defined by ml.

kind=1 allows to build a mask of the SAD (sum of absolute differences) values
instead of the vectors' length. It can be useful to find problem areas with
bad motion estimation. (Internal factor blocksize*blocksize/4 is used for
normalization of scale ml.)

kind=2 allows to build a occlusion mask (bad blocks due to rupture, tensile).
Currently, some normalized sum of positive blocks motion differences is used.
It can be scaled with ml.

*ml* parameter defines the scale of motion mask. When the vector's length (or
other kind value) is superior or equal to ml, the output value is saturated
to 255. The lesser value results to lesser output.

*gamma* is used to defined the exponent of relation output to input. gamma =
1.0 implies a linear relation, whereas gamma = 2.0 gives a quadratic
relation.

And finally, *Ysc* is the value taken by the mask on scene change

Defaults are : kind = 0, ml = 100, gamma = 1.0, and Ysc = 0.


MVSCDetection
~~~~~~~~~~~~~

``MVSCDetection`` (clip, clip "vectors", int "Ysc")

Creates scene detection mask clip from motion vectors data. The mask is
created both on the luma and on chroma planes. Output without scene change is
0.

*Ysc* is the value taken by the mask on scene change, default is 255.


MVShow
~~~~~~

``MVShow`` (clip, clip "vectors", int "scale", int "sil", int "tol", bool
"showsad")

Shows the motion vectors.

*scale* allows to enlarge the motion vectors, in order for example to gain in
accuracy ( when pel > 1 and scale = 1, you can't see a variation of less than
one pixel ).

*sil* allows to see a different level of analysis ( when searching for motion
vectors, a hierarchal analysis is done, and it may be interesting to see what
happens at higher levels ).

*tol* is a tolerance threshold. If the distortion induced by the motion vector
is over tol the vector isn't shown.

Finally, *showsad* allows to show the mean SAD after compensating the picture.

Defaults are : scale = 1, sil = 0, tol = 20000 and showsad = false ( which
shows all vectors ).


MVChangeCompensate
~~~~~~~~~~~~~~~~~~

``MVChangeCompensate`` (clip vectors, clip)

Allows to change the compensation stored into the mvs stream.


MVIncrease
~~~~~~~~~~

``MVIncrease`` (clip, clip "vectors", int "horizontal", int "vertical", int
"idx")

It allows to use vectors computed on a reduced version of the clip in order
to make a compensation on a clip with the original size.

*horizontal* is the horizontal ratio between the width of the clip and the
width of the reduced clip.

*vertical* is the vertical ratio between the height of the clip and the height
of the reduced clip.

*idx* works the same as in ``MVAnalyse``


MVDepan
~~~~~~~

``MVDepan`` (clip, clip "vectors", bool "zoom", bool "rot", float
"pixaspect", float "error", bool "info", string "log")

Get the motion vectors,  estimate global motion and put data to output frame
in special format for ``DePan`` plugin (by Fizick).

Inter-frame global motion (pan, zoom, rotation) is estimated by iterative
procedure, with good blocks only.

Rejected blocks: 1) near frame borders; 2) with big SAD (by thSCD1
parameter); 3) with motion different from global.

*zoom* and *rot* parameters switch zoom and rotation estimation,  pixaspect
is pixel aspect (1.094 for standard PAL, 0.911 for standard NTSC),  error is
maximum mean motion difference.

The frame estimated global motion is switched to null for big motion error or
at scene change  (by thSCD1, thSCD2 parameters).

*info* parameter allows to type global motion info for debug.

*log* parameter allows to set log file name in DeShaker, Depan format.

Defaults are : zoom = true, rot = true, pixaspect = 1.0, error = 15.0, *info*
= false.

For global motion estimation of interlaced source, you must separate fields
(for both MVAnalyse and MVDepan).


MVFlow
~~~~~~

``MVFlow`` (clip, clip "vectors", float "time", int "mode", int "idx")

Do a motion compensation of the frame not by blocks (like MVCompensation),
but by pixels. Motion vector for every pixel is calculated by bilinear
interpolation of motion vectors of current and neighbor blocks (according to
pixel position). It means that the pixels pointed by the vector in the
reference frame will be moved (flow) along the vectors to reach their places
in the current frame. This flow motion compensation method does not produce
any blocking artefactes, and is good for denoising, but sometimes can create
very strange deformed pictures :). True motion estimation is strongly
recommended for this function. Motion compensation may be full or partial (at
intermediate time).

Limitation: vectors with components above 127 will be reset to zero length.

*time*: percent of motion compensation (default=100.0, full compensation),
define time moment between reference and current frame.

*mode* can be either 0 ( default ), or 1.

- mode=0 - fetch pixels to every place of destination. It is main producing mode.
- mode=1 - shift pixels from every place of source (reference). It is debug
  (learning) mode with some empty spaces (with null intensity). It can be used
  for occlusion mask creation.

*idx* (may be) works the same way as idx in MVAnalyse.


MVFlowInter
~~~~~~~~~~~

``MVFlowInter`` (clip, clip "mvbw", clip "mvfw", float "time", float "ml",
int "idx")

Motion interpolation function. It is not the same (but similar) as
MVInterpolate function of older MVTools version. It uses backward "mvbw" and
forward "mvfw" motion vectors to create picture at some intermediate time
moment between current and next frame. It uses pixel-based (by MVFlow method)
motion compensation from both frames. Internal forward and backward occlusion
masks (MVMask kind=2 method) and time weighted factors are used to produce
the output image with minimal artefactes. True motion estimation is strongly
recommended for this function.

*time*: interpolation time position between frames (in percent, default=50.0,
half-way)

| *ml*: mask scale parameter. The greater values are corresponded to more weak
  occlusion mask (as in MVMask function, use it to tune and debug).
| Default=100.

*idx* (may be) works the same way as idx in MVAnalyse for speed increasing.


MVFlowFps
~~~~~~~~~

``MVFlowFps`` (clip, clip "mvbw", clip "mvfw", int "num", int "den", float
"ml", int "idx")

Will change the framerate (fps) of the clip (and number of frames). The
function can be use for framerate conversion, slow-motion effect, etc. It
uses backward "mvbw" and forward "mvfw" motion vectors to create interpolated
pictures at some intermediate time moments between frames. The function uses
pixel-based motion compensation (as MVFlow, MVFlowInter). Internal forward
and backward occlusion masks (MVMask kind=2 method) and time weighted factors
are used to produce the output image with minimal artefactes. True motion
estimation is strongly recommended for this function.

*num*: fps numerator (default=25)

*den*: fps denominator (default=1). Resulted fps = num/den. For example, use
30000/1001 for 29.97 fps

| *ml*: mask scale parameter. The greater values are corresponded to more weak
  occlusion mask (as in MVMask function, use it to tune and debug).
| Default=100.

*idx* (may be) works the same way as idx in MVAnalyse for speed increasing.


MVFlowFps2
~~~~~~~~~~

``MVFlowFps2`` (clip, clip "mvbw", clip "mvfw", clip "mvbw2", clip "mvfw2",
int "num", int "den", float "ml", int "idx", int "idx2")

Will change the framerate (fps) of the clip (and number of frames) like
MVFlowFps, but with a little better quality (and slower processing). In
addition to backward "mvbw" and forward "mvfw" motion vectors of original
source clip, the function uses backward "mvbw2" and forward "mvfw2" motion
vectors of second (modified) source clip. Second clip must be produced from
original source clip by cropping (i.e. diagonal shift) by half block size. It
must be done with command ``Crop(a,a,-b,-b)``, where a=b=4 must be used for
blksize=8, a=b=8 for blksize=16, and a=2, b=6 for blksize=4 (see example).
Blocks boundaries will be at different parts of objects. MVFlowFps2 reverses
the shift internally and averages motion vectors from these two sources to
decrease motion estimation errors. The function uses pixel-based motion
compensation (as MVFlow, MVFlowInter). Internal forward and backward
occlusion masks (MVMask kind=2 method) and time weighted factors are used to
produce the output image with minimal artefactes. True motion estimation is
strongly recommended for this function.

*num*: fps numerator (default=25)

*den*: fps denominator (default=1). Resulted fps = num/den.

*ml*: mask scale parameter. The greater values are corresponded to more weak
occlusion mask (as in MVMask function, use it to tune and debug).
Default=100.

*idx* (may be) works the same way as idx in MVAnalyse for speed increasing.

*idx2* is MVAnalyse index of second (shifted) clip (must not coincide with
first idx).


MVFlowBlur
~~~~~~~~~~

``MVFlowBlur`` (clip, clip "mvbw", clip "mvfw", float "blur", int "prec", int
"idx")

Experimental simple motion blur function. It may be used for FILM-effect (to
simulate finite shutter time). It uses backward "mvbw" and forward "mvfw"
motion vectors to create and overlay many copies of partially compensated
pixels at intermediate time moments in some blurring interval around current
frame. It uses pixel-based motion compensation (as MVFlow). True motion
estimation is strongly recommended for this function.

*blur*: blur time interval between frames, open shutter time (in percent,
default=50.0)

*prec*: blur precision in pixel units. Maximal step between compensated blurred
pixels. Default =1 (most precise).

*idx* (may be) works the same way as idx in MVAnalyse for speed increasing.


MVDeGrain1 and MVDeGrain2
~~~~~~~~~~~~~~~~~~~~~~~~~

``MVDeGrain1`` (clip, clip "mvbw", clip "mvfw", int "thSAD", int "plane", int
"idx")

``MVDeGrain2`` (clip, clip "mvbw", clip "mvfw", clip "mvbw2", clip
"mvfw2",int "thSAD", int "plane", int "idx")

Makes a temporal denoising with motion compensation. Blocks of previous and
next frames are motion compensated and then averaged with current frame with
weigthing factors depended on block differences from current (SAD). Functions
support overlapped blocks mode.

Overlaped blocks processing is implemented as window block summation (like
FFT3DFilter, overlap value up to blksize/2) for blocking artefactes
decreasing.

MVDeGrain1 has temporal radius 1 (uses vectors of previous mvfw and next mvbw
frames).

MVDeGrain2 has temporal radius 2 (uses vectors of two previous mvfw2, mvfw
and two next mvbw,mvbw2 frames). It is slower, but produces a little better
results.

| The filtering strength is controlled by "thSAD" parameter. It defines
  threshold of block sum absolute differences. You must enter thSAD value
  reduced to block size 8x8. The greater the SAD, the lesser the weight. Block
  with SAD above threshold thSAD have a zero weigtht.
| Default thSAD=400.

*plane* parameter set procesed color plane:

- 0 - luma
- 1 - chroma U
- 2 - chroma V
- 3 - both chromas
- 4 - all.

Default is 4.

*idx* (may be) works the same way as idx in MVAnalyse for speed increasing.




Examples
--------

To show the motion vectors ( forward ) :

::

    vectors = source.MVAnalyse(isb = false)
    return source.MVShow(vectors)

To show the backward one :

::

    vectors = source.MVAnalyse(isb = true)
    return source.MVShow(vectors)

To use MVMask :

::

    vectors = source.MVAnalyse(isb = false)
    return source.MVMask(vectors)

To denoise :

::

    backward_vec2 = source.MVAnalyse(isb = true, lambda = 1000, delta = 2)
    backward_vec1 = source.MVAnalyse(isb = true, lambda = 1000, delta = 1)
    forward_vec1 = source.MVAnalyse(isb = false, lambda = 1000, delta = 1)
    forward_vec2 = source.MVAnalyse(isb = false, lambda = 1000, delta = 2)
    return source.MVDenoise(backward_vec2,backward_vec1,forward_vec1,forward_vec2,tht=10,thSAD=300)

To deblock the compensation stored into a mvs stream

::

    vectors = source.MVAnalyse(isb = false, lambda = 1000)
    compensation = source.MVCompensate(vectors, mode = 0)
    compensation = compensation.Deblock() # use DeBlock function
    vectors = vectors.MVChangeCompensate(compensation)

To denoise with pel = 2, efficiently :

::

    backward_vec2 = source.MVAnalyse(isb = true, lambda = 1000, delta = 2, pel = 2, idx = 1)
    backward_vec1 = source.MVAnalyse(isb = true, lambda = 1000, delta = 1, pel = 2, idx = 1)
    forward_vec1 = source.MVAnalyse(isb = false, lambda = 1000, delta = 1, pel = 2, idx = 1)
    forward_vec2 = source.MVAnalyse(isb = false, lambda = 1000, delta = 2, pel = 2, idx = 1)
    return source.MVDenoise(backward_vec2,backward_vec1,forward_vec1,forward_vec2,tht=10,thSAD=300)

To use MVIncrease :

::

    vectors = source.reduceby2().mvanalyse(isb = true)
    return source.MVIncrease(vectors, horizontal = 2, vertical = 2)

To use MVDepan with `Depan`_ plugin for interlaced source (DepanStabilize
function example):

::

    source = source.AssumeTFF().SeparateFields() # set correct fields order
    vectors = source.MVAnalyse(isb = false)
    globalmotion = source.MVDepan(vectors, pixaspect=1.094, thSCD1=400)
    DepanStabilize(source, data=globalmotion, cutoff=2.0, mirror=15,
    pixaspect=1.094)
    Weave()


To blur problem (blocky) areas of compensated frame with occlusion mask:

::

    vectors = source.MVAnalyse(isb = false, lambda = 1000)
    compensation = source.MVCompensate(vectors) # or use MVFlow function here
    # prepare blurred frame with some strong blur or deblock function:
    blurred = compensation.DeBlock(quant=51) # Use DeBlock function here
    badmask = source.MVMask(vectors, kind = 2, ml=50)
    overlay(compensation,blurred,mask=badmask) # or use faster MaskedMerge function of MaskTools


To recreate bad frames by interpolation with MVFlowInter:

::

    backward_vectors = source.MVAnalyse(isb = true, truemotion=true, pel=2, delta=2, idx=1)
    forward_vectors = source.MVAnalyse(isb = false, truemotion=true, pel=2, delta=2, idx=1)
    inter = source.MVFlowInter(backward_vectors, forward_vectors, time=50, ml=70, idx=1)
    # Assume bad frames 50 and 60
    source.trim(0,49) ++ inter.trim(49,-1) \
     ++ source.trim(51,59) ++ inter.trim(59,-1) ++ source.trim(61,0)


To change fps with MVFlowFps:

::

    # assume progressive PAL 25 fps source
    backward_vec = source.MVAnalyse(isb = true, truemotion=true, pel=2, idx=1)
    # we use explicit idx for more fast processing
    forward_vec = source.MVAnalyse(isb = false, truemotion=true, pel=2, idx=1)
    return source.MVFlowFps(backward_vec, forward_vec, num=50, den=1, ml=100, idx=1) # get 50 fps


To change fps with MVFlowFps2:

::

    # Assume progressive PAL 25 fps source. Lets try convert it to 50.
    backward_vec = source.MVAnalyse(isb = true, truemotion=true, pel=2, idx=1)
    # we use explicit idx for more fast processing
    forward_vec = source.MVAnalyse(isb = false, truemotion=true, pel=2, idx=1)
    cropped = source.crop(4,4,-4,-4) # by half of block size 8
    backward_vec2 = cropped.MVAnalyse(isb = true, truemotion=true, pel=2, idx=2)
    forward_vec2 = cropped.MVAnalyse(isb = false, truemotion=true, pel=2, idx=2)
    return source.MVFlowFps2(backward_vec,forward_vec,backward_vec2,forward_vec2,num=50,idx=1,idx2=2)


To generate nice motion blur with MVFlowBlur:

::

    backward_vectors = source.MVAnalyse(isb = true, truemotion=true)
    forward_vectors = source.MVAnalyse(isb = false, truemotion=true)
    return source.MVFlowBlur(backward_vectors, forward_vectors, blur=15)


To denoise with some external denoiser filter (which uses 3 frames: prev,
cur, next):

::

    backward_vectors = source.MVAnalyse(isb = true, truemotion=true, delta = 1, idx = 1, thSCD1=500)
    # we use explicit idx for more fast processing
    forward_vectors = source.MVAnalyse(isb = false, truemotion=true,
    delta = 1, idx = 1, thSCD1=500)
    forward_compensation = source.MVFlow(forward_vectors, idx=1) # or use MVCompensate function
    backward_compensation = source.MVFlow(backward_vectors, idx=1) # or use MVCompensate function
    # create interleaved 3 frames sequences
    interleave(forward_compensation, source, backward_compensation)

    DeGrainMedian() # place your preferred temporal (spatial-temporal) denoiser here

    return selectevery(3,1) # return filtered central (not-compensated) frames only


To use prefiltered clip for more reliable motion estimation, but compensate
motion of not-prefiltered clip (denoising example)

::

    # Use some denoiser (blur) or deflicker for prefiltering
    prefiltered = source.DeGrainMedian()
    backward_vectors = prefiltered.MVAnalyse(isb = true, truemotion=true, delta = 1, idx = 1)
    # we use explicit idx for more fast processing
    forward_vectors = prefiltered.MVAnalyse(isb = false, truemotion=true, delta = 1, idx = 1)
    # use not-prefiltered clip for motion compensation (with other idx)
    forward_compensation = source.MVFlow(forward_vectors, idx=2) # or use MVCompensate(mode=1)
    backward_compensation = source.MVFlow(backward_vectors, idx=2) # or use MVCompensate(mode=1)
    # create interleaved 3 frames sequences
    interleave(forward_compensation, source, backward_compensation)

    DeGrainMedian() # place your preferred temporal (spatial-temporal) denoiser here

    return selectevery(3,1) # return filtered central (not-compensated) frames only


To denoise by MVDegrain2 with overlapped blocks (blksize=8) and subpixel
precision:

::

    backward_vec2 = source.MVAnalyse(isb = true, delta = 2, pel = 2, overlap=4, sharp=1, idx = 1)
    backward_vec1 = source.MVAnalyse(isb = true, delta = 1, pel = 2, overlap=4, sharp=1, idx = 1)
    forward_vec1 = source.MVAnalyse(isb = false, delta = 1, pel = 2, overlap=4, sharp=1, idx = 1)
    forward_vec2 = source.MVAnalyse(isb = false, delta = 2, pel = 2, overlap=4, sharp=1, idx = 1)
    source.MVDegrain2(backward_vec1,forward_vec1,backward_vec2,forward_vec2,thSAD=400,idx=1)

To denoise interlaced source by MVDegrain1 with overlapped blocks (blksize=8)
and subpixel precision:

::

    fields=source.AssumeTFF().SeparateFields() # or AssumeBFF
    backward_vec2 = fields.MVAnalyse(isb = true, delta = 2, pel = 2, overlap=4, sharp=1, idx = 1)
    forward_vec2 = fields.MVAnalyse(isb = false, delta = 2, pel = 2, overlap=4, sharp=1, idx = 1)
    fields.MVDegrain1(backward_vec2,forward_vec2,thSAD=400,idx=1)
    Weave()

Disclaimer
----------

This plugin is released under the GPL license. You must agree to the terms of
'Copying.txt' before using the plugin or its source code. Please donate for
support.


Revisions
---------

+-----------------------------------------------------------------------------------------------------------------------------------------------+
| Changelog                                                                                                                                     |
+============+=============+========+===========================================================================================================+
| 1.4.11     | 06.09.2006  | Fizick | - Corrected vector predictors interpolation (from coarse to fine scale) for overlap>0.                    |
|            |             |        | - Fixed bug with pitch for overlap=0, YV12 in MVDegrain1 (thanks to Boulder for report)                   |
+------------+-------------+--------+-----------------------------------------------------------------------------------------------------------+
| 1.4.10     | 18.08.2006  | Fizick | - Corrected right and bottom borders processing in MVCompensate for arbitrary frame sizes.                |
|            |             |        | - Changed defaults in MVAnalyse: pel=2, truemotion=true, sharp=2.                                         |
+------------+-------------+--------+-----------------------------------------------------------------------------------------------------------+
| 1.4.9      | 14.08.2006  | Fizick | - Fixes a bug in MMX optimization of overlap mode in MVDeGrain,                                           |
|            |             |        |   MVCompensate for YUY2 with blksize=8 (thanks to TSchniede for report).                                  |
+------------+-------------+--------+-----------------------------------------------------------------------------------------------------------+
| 1.4.8      | 31.07.2006  | Fizick | - Added some MMX optimization of overlap mode in MVDeGrain, MVCompensate.                                 |
|            |             |        | - Fixed a bug with last (not processed) rows in MVDeGrain.                                                |
+------------+-------------+--------+-----------------------------------------------------------------------------------------------------------+
| 1.4.7      | 25.07.2006  | Fizick | - Decreased overlap gridness in MVDeGrain1, MVDeGrain2, MVCompensate.                                     |
|            |             |        | - Added example with MVDeGrain1 for interlaced.                                                           |
+------------+-------------+--------+-----------------------------------------------------------------------------------------------------------+
| 1.4.6      | 24.07.2006  | Fizick | - Decreased denoising in MVDeGrain1, MVDeGrain2.                                                          |
|            |             |        | - Plane parameter in MVDeGrain1, MVDeGrain2 now works :)                                                  |
+------------+-------------+--------+-----------------------------------------------------------------------------------------------------------+
| 1.4.5      | 22.07.2006  | Fizick | - Added plane parameter to MVDeGrain1, MVDeGrain2.                                                        |
+------------+-------------+--------+-----------------------------------------------------------------------------------------------------------+
| 1.4.4      | 19.07.2006  | Fizick | - Corrected default thSAD=400 in MVDeGrain1, MVDeGrain2.                                                  |
|            |             |        | - Fixed a bug with V color plane in MVChangeCompensate.                                                   |
+------------+-------------+--------+-----------------------------------------------------------------------------------------------------------+
| 1.4.3      | 17.07.2006  | Fizick | - Decrease overlap gridness in MVDeGrain1, MVDeGrain2.                                                    |
+------------+-------------+--------+-----------------------------------------------------------------------------------------------------------+
| 1.4.2      | 16.07.2006  | Fizick | - Fixed a memory access bug in MVDeGrain1, MVDeGrain2. Thanks to krieger2005 for report.                  |
+------------+-------------+--------+-----------------------------------------------------------------------------------------------------------+
| 1.4.1      | 23.06.2006  | Fizick | - Changed MVDeGrain1, MVDeGrain2 mode to SAD weigthing.                                                   |
|            |             |        | - Chanded thSCD1 default from 300 to 400.                                                                 |
+------------+-------------+--------+-----------------------------------------------------------------------------------------------------------+
| 1.4.0      | 19.06.2006  | Fizick | - Added MVDeGrain1, MVDeGrain2 limited averaging denoisers.                                               |
|            |             |        | - Corrected thSAD scale in MVDenoise.                                                                     |
|            |             |        | - Corrected documentation about SAD.                                                                      |
+------------+-------------+--------+-----------------------------------------------------------------------------------------------------------+
| 1.3.1      | 11.06.2006  | Fizick | - Added bicubic subpixel interpolation method for pel=2 (with iSSE optinization for sharp=1,2).           |
|            |             |        | - Assembler iSSE speed optimization for overlapped block compensation.                                    |
+------------+-------------+--------+-----------------------------------------------------------------------------------------------------------+
| 1.3.0      | 05.06.2006  | Fizick | - Implemented overlapped block motion compensation to MVCompensation(mode=1).                             |
|            |             |        | - Changed default to mode=1 in MVCompensation as the most universal.                                      |
|            |             |        | - Added sharp subpixel interpolation method for pel=2.                                                    |
|            |             |        | - Fixed bug for blksize=16 with YUY2.                                                                     |
|            |             |        | - (To-do list: assembler SSE speed optimization for new compensation and interpolation methods.)          |
+------------+-------------+--------+-----------------------------------------------------------------------------------------------------------+
| 1.2.6 beta | 21.05.2006  | Fizick | - Added option to write motion vectors data to log file as requested by Endre.                            |
+------------+-------------+--------+-----------------------------------------------------------------------------------------------------------+
| 1.2.5      | 08.05.2006  | Fizick | - Decreased zero vector weight, iteration accuracy in MVDepan                                             |
+------------+-------------+--------+-----------------------------------------------------------------------------------------------------------+
| 1.2.4      | 07.04.2006  | Fizick | - Fixed bug v.1.2.3 with info mode in MVDepan                                                             |
+------------+-------------+--------+-----------------------------------------------------------------------------------------------------------+
| 1.2.3      | 31.03.2006  | Fizick | - Implemented MVDepan for interlaced source separated by fields;                                          |
|            |             |        | - added optional MVDepan log file.                                                                        |
+------------+-------------+--------+-----------------------------------------------------------------------------------------------------------+
| 1.2.2 beta | 01.03.2006  | Fizick | - Fixed frame shift bug of v1.2.1 with mmx YUY2 conversion (thanks to **WorBry** for bug report)          |
+------------+-------------+--------+-----------------------------------------------------------------------------------------------------------+
| 1.2.1 beta | 20.02.2006  | Fizick | - Fixed bug of v1.2,                                                                                      |
|            |             |        | - Speed restored,                                                                                         |
|            |             |        | - mmx YUY2 conversion (from avisynth 2.6 function by sh0dan)                                              |
|            |             |        | - But it seems, overlap mode still does not work properly                                                 |
+------------+-------------+--------+-----------------------------------------------------------------------------------------------------------+
| 1.2 beta   | 17.02.2006  | Fizick | - YUY2 format support (besides MVIncrease), no optimization                                               |
+------------+-------------+--------+-----------------------------------------------------------------------------------------------------------+
| 1.1.1      | 16.02.2006  | Fizick | - Removed DeBlock and Corrector filters (will be separate plugins)                                        |
|            |             |        | - Documented old MVSCDetection function.                                                                  |
|            |             |        | - Cleaned project from unused source files.                                                               |
+------------+-------------+--------+-----------------------------------------------------------------------------------------------------------+
| 1.1        | 09.01.2006  | Fizick | - non-public build                                                                                        |
|            |             |        | - Quite large revision (beta). New option for overlapped block motion                                     |
|            |             |        |   estimation in MVAnalyse for usage in MVFlow, MVFlowInter, MVFlowFps for                                 |
|            |             |        |   improved motion compensation.                                                                           |
|            |             |        | - Lookup tables for motion interpolation.                                                                 |
|            |             |        | - Small correction of displacement value in MVFlowFps2.                                                   |
+------------+-------------+--------+-----------------------------------------------------------------------------------------------------------+
| 1.0.3      | 30.12.2005  | Fizick | - Fixed bug with displacement in MVFlowInter, MVFlowFps (introduced in v1.0.2).                           |
+------------+-------------+--------+-----------------------------------------------------------------------------------------------------------+
| 1.0.2      | 28.12.2005  | Fizick | - Corrected value of displacement in MVFlow (a little).                                                   |
+------------+-------------+--------+-----------------------------------------------------------------------------------------------------------+
| 1.0.1      | 24.12.2005  | Fizick | - Fixed memory leakage bug in MVAnalyse with global motion (thanks to **AI** for report).                 |
|            |             |        | - Removed penalty for zero vector predictor in MVAnalyse (was introduced in v1.0).                        |
|            |             |        | - Changed chroma=true as default in MVAnalyse.                                                            |
+------------+-------------+--------+-----------------------------------------------------------------------------------------------------------+
| 1.0        | 29.11.2005  | Fizick | - I'm tired of long version numbers :). But the plugin is still experimental :(.                          |
|            |             |        | - Restored zero vector predictor in MVAnalyse.                                                            |
|            |             |        | - Changed blur time scale in MVFlowBlur (100 is fully open shutter now) as **Mug Funky** requested.       |
+------------+-------------+--------+-----------------------------------------------------------------------------------------------------------+
| 0.9.13.3   | 27.11.2005  | Fizick | - Added global motion (simple method) vector predictor to MVAnalyse.                                      |
|            |             |        | - Vector search is skipped (for speed) if good predictor was found (with SAD < pnew).                     |
|            |             |        | - Parameter scale in MVShow works properly now.                                                           |
|            |             |        | - Disabled some debug and profiling info output (for speed increasing).                                   |
|            |             |        | - Changed default prec=1 (was 2) in MVFlowBlur.                                                           |
+------------+-------------+--------+-----------------------------------------------------------------------------------------------------------+
| 0.9.13.2   | 22.11.2005  | Fizick | - Fixed bug in MVFlowFps, MVFlowFps2 for non-integer fps.                                                 |
+------------+-------------+--------+-----------------------------------------------------------------------------------------------------------+
| 0.9.13.1   | 21.11.2005  | Fizick | - Fixed bug in MVFlowFps, MVFlowFps2, MVFlowInter, MVFlowBlur (introduced in v0.9.13).                    |
|            |             |        | - Removed plen parameter from MVAnalyse as not useful.                                                    |
+------------+-------------+--------+-----------------------------------------------------------------------------------------------------------+
| 0.9.13     | 20.11.2005  | Fizick | - Added truemotion preset to MVAnalyse.                                                                   |
|            |             |        | - Added experimental MVFlowFps2.                                                                          |
|            |             |        | - Change interpolated vector rounding method in all MVFLow... functions.                                  |
|            |             |        | - Edited documentation a little.                                                                          |
+------------+-------------+--------+-----------------------------------------------------------------------------------------------------------+
| 0.9.12.4   | 15.11.2005  | Fizick | - Changed type of ml parameter in MVMask, MVFlowInter, MVFlowFps from int to float.                       |
|            |             |        | - Added bound check of ml, time, blur parameters.                                                         |
|            |             |        | - Small possible bug fixed (emms).                                                                        |
|            |             |        | - Partially updated documentation. But I am not sure that **sh0dan**                                      |
|            |             |        |   requested **exactly such** updating :). It is still not user guide but                                  |
|            |             |        |   functions reference.                                                                                    |
+------------+-------------+--------+-----------------------------------------------------------------------------------------------------------+
| 0.9.12.3   | 14.11.2005  | Fizick | - Fixed bug for chroma for width not divisible by 16 in MVMask (introduced in v0.9.11).                   |
|            |             |        | - Some speed optimizing of MVFlowFps.                                                                     |
|            |             |        | - Reset size of internal frames buffer to original value 10. Try various versions.                        |
+------------+-------------+--------+-----------------------------------------------------------------------------------------------------------+
| 0.9.12.2   | 13.11.2005  | Fizick | - Added experimental precise but slow MVFlowBlur function as **scharfis_brain** requested.                |
|            |             |        | - Temporary changed size of internal frames buffer to 5.                                                  |
+------------+-------------+--------+-----------------------------------------------------------------------------------------------------------+
| 0.9.12.1   | 12.11.2005  | Fizick | - Added experimental MVFlowFps function.                                                                  |
|            |             |        | - Disabled MVInter function.                                                                              |
|            |             |        | - Temporary changed size of internal frames buffer from 10 to 3 for                                       |
|            |             |        |   memory usage decreasing. Speed must be tested for complex scripts.                                      |
+------------+-------------+--------+-----------------------------------------------------------------------------------------------------------+
| 0.9.12     | 09.11.2005  | Fizick | - Added MVFlowInter function. MVInter function will be removed in next release (it is worse).             |
|            |             |        | - Changed scale of ml parameter for kind=2 of MVMask to more optimal default.                             |
|            |             |        | - Fixed small bug in Bilinear.asm (strange pixels near right border for pel=2).                           |
+------------+-------------+--------+-----------------------------------------------------------------------------------------------------------+
| 0.9.11.1   | 06.11.2005  | Fizick | - Added half-pel support to MVFlow.                                                                       |
|            |             |        | - Increased max quant from 51 to 60 in DeBlock for very strong deblocking .                               |
|            |             |        | - Corrected documentation.                                                                                |
+------------+-------------+--------+-----------------------------------------------------------------------------------------------------------+
| 0.9.11     | 04.11.2005  | Fizick | - Improved MVMask: Replaced boolean showsad parameter to integer kind,                                    |
|            |             |        |   added occlusion mask option. Changed bilinear resize code to more correct                               |
|            |             |        |   and fast SimpleResize.                                                                                  |
+------------+-------------+--------+-----------------------------------------------------------------------------------------------------------+
| 0.9.10.1   | 01.11.2005  | Fizick | - Fixed bug with chroma and luma small changes in MVInter (rounding error).                               |
|            |             |        | - Vector interpolation in MVFlow and mask in MVInter are correct now (due to fixing bug in SimpleResize). |
+------------+-------------+--------+-----------------------------------------------------------------------------------------------------------+
| 0.9.10     | 31.10.2005  | Fizick | - Added some true motion (smoothed) estimation options to MVAnalyse                                       |
|            |             |        | - Added function MVFlow for per-pixel motion compensation                                                 |
|            |             |        | - Added function MVInter for motion interpolation (very experimental)                                     |
+------------+-------------+--------+-----------------------------------------------------------------------------------------------------------+
| 0.9.9.1    | 20.01.2005  | Manao  | - No need anymore of stlport_vcxxxx.dll                                                                   |
+------------+-------------+--------+-----------------------------------------------------------------------------------------------------------+
| 0.9.9      |             |        | - Filter added : Corrector                                                                                |
|            |             |        | - Filter added : MVIncrease                                                                               |
|            |             |        | - New available blocksize : 16                                                                            |
|            |             |        | - New parameter in MVAnalyse : chroma                                                                     |
|            |             |        | - Changes in the core                                                                                     |
+------------+-------------+--------+-----------------------------------------------------------------------------------------------------------+
| 0.9.8.5    |             |        | - ``MVCompensate`` changed : a new parameter, idx, which works as idx                                     |
|            |             |        |   in ``MVAnalyse``, and which allows speed up when compensating the same                                  |
|            |             |        |   frames several times.                                                                                   |
+------------+-------------+--------+-----------------------------------------------------------------------------------------------------------+
| 0.9.8.4    |             |        | - ME takes into account the chroma now, as requested by tsp.                                              |
+------------+-------------+--------+-----------------------------------------------------------------------------------------------------------+
| 0.9.8.3    |             |        | - Added ``Corrector`` function, as requested by scharfi.                                                  |
+------------+-------------+--------+-----------------------------------------------------------------------------------------------------------+
| 0.9.8.2    |             |        | - New function ``MVDepan`` (added by Fizick) for Depan plugin.                                            |
+------------+-------------+--------+-----------------------------------------------------------------------------------------------------------+
| 0.9.8.1    |             |        | - Several bugfixes                                                                                        |
+------------+-------------+--------+-----------------------------------------------------------------------------------------------------------+
| 0.9.8      |             |        | - Yet another little changes in the filters' syntax. The core changed a                                   |
|            |             |        |   lot, in order to gain speed were it was possible. However, by default,                                  |
|            |             |        |   the speed gain won't be visible, you'll need to configure correctly the                                 |
|            |             |        |   analysis filter through its "idx" parameter in order to gain speed (in                                  |
|            |             |        |   the mode "pel" = 2).                                                                                    |
|            |             |        | - Bugfixes in MVDenoise, and chroma denoising in MVDenoise.                                               |
|            |             |        | - Now, the filters down the filter's chain tell to the analysis filter                                    |
|            |             |        |   if they need the compensation, so you don't have to worry about that at                                 |
|            |             |        |   the analysis stage.                                                                                     |
+------------+-------------+--------+-----------------------------------------------------------------------------------------------------------+
| 0.9.7      |             |        | - Yet again, a lot of rewriting. Interpolating filters are disabled (for the moment),                     |
|            |             |        |   all the other filters work and should be considered as                                                  |
|            |             |        |   stable. Syntax has changed a lot, and will change again before reaching                                 |
|            |             |        |   1.0 (if it's reached one day). Changes mainly affect ``MVAnalyse``. New                                 |
|            |             |        |   filter : ``MVChangeCompensate``.                                                                        |
+------------+-------------+--------+-----------------------------------------------------------------------------------------------------------+
| 0.9.6.2    |             |        | - Fixed bug in ``MVMask`` parameters.                                                                     |
+------------+-------------+--------+-----------------------------------------------------------------------------------------------------------+
| 0.9.6.1    |             |        | - Lot of bugfixes for the existing filters. MVMask, MVShow,                                               |
|            |             |        |   MVCompensate, MVDenoise and MVSCDetection, and MVAnalyse should work .                                  |
|            |             |        |   Other may crash unexpectedly.                                                                           |
|            |             |        | - Now, for the three new filters. Two have nothing to do with motion                                      |
|            |             |        |   compensation, but I didn't want to put them in separate binaries, since                                 |
|            |             |        |   they'll mainly be used with filters from this package. The third one uses                               |
|            |             |        |   vectors, and integrates somehow the two others.                                                         |
|            |             |        | - QDeQuant(clip c, int quant, int level) : takes a clip and quantizes                                     |
|            |             |        |   it, using an approximation of the H264 DCT. It filters the three planes (                               |
|            |             |        |   4x4 blocks for each of them, so the chroma isn't processed as in H264 ).                                |
|            |             |        |   It's not exactly the H264 DCT because at q1, it's lossless, and a q51                                   |
|            |             |        |   it's not that bad, but you can raise quant over 51. Level is the                                        |
|            |             |        |   reference level of the picture. By default it's zero, but it can be set,                                |
|            |             |        |   for example, to 128. The picture is then treates as if pixels were                                      |
|            |             |        |   ranging from -128 to 127, hence avoiding errors around 128.                                             |
|            |             |        | - Deblock(clip c, int quant, int aOffset, int bOffset) : takes a clip,                                    |
|            |             |        |   and deblock it using H264 deblocking, as if the picture was made only of                                |
|            |             |        |   inter blocks. This time, quant ranges from 0 to 51 as in H264, and has                                  |
|            |             |        |   the same impact. aOffset and bOffset allow to raise / lower the quant                                   |
|            |             |        |   when deciding for some internal thresholds. They are set by default to 0.                               |
|            |             |        |   Be warned that the filter should do nothing at quant < 16, if aOffset and                               |
|            |             |        |   bOffset are both zero. It's a wanted behavior (thus it respect the                                      |
|            |             |        |   partially the norm).                                                                                    |
|            |             |        | - EncDenoise(clip c, clip vectors, bool scbehavior, int quant, int                                        |
|            |             |        |   aOffset, int bOffset, int thSCD1, int thSCD2) : it merges Deblock,                                      |
|            |             |        |   QDeQuant and MVCompensate, taking from them the name and behavior of                                    |
|            |             |        |   their parameters. It basically does a h264 encode as if all blocks were                                 |
|            |             |        |   8x8 inter blocks. Reference frame is the previous frame output by the                                   |
|            |             |        |   filter (if it is the correct one, else it's the previous frame of the                                   |
|            |             |        |   source), mvs are those given by mvanalyse on the source. The reference                                  |
|            |             |        |   frame is compensated by the vectors, then the residual difference is                                    |
|            |             |        |   quantized / dequantized and added to the result of the motion                                           |
|            |             |        |   compensation. Finally, the frame is deblocked, and serves as reference                                  |
|            |             |        |   for the next one.                                                                                       |
+------------+-------------+--------+-----------------------------------------------------------------------------------------------------------+
| 0.9.5.3    |             |        | - Mainly a bugfixe (several filters were affected by a silly bug), and                                    |
|            |             |        |   MVCompensate now do padded motion compensation, and compensate also the                                 |
|            |             |        |   chroma (though it rounds the vector to odd coordinates to do so)                                        |
|            |             |        | - A new feature, as asked by Fizick, for mvcompensate : "scbehavior", a                                   |
|            |             |        |   boolean set to true by default, will allow you to keep the previous frame                               |
|            |             |        |   over a scenechange if you set it to false.                                                              |
+------------+-------------+--------+-----------------------------------------------------------------------------------------------------------+
| 0.9.5.2    |             |        | - MVMask has two new parameters : showsad (bool) which allows to build                                    |
|            |             |        |   the mask of the SAD values instead of the mask of the vectors' length.                                  |
|            |             |        |   Ysc allows you to set the value taken by the mask on scene changes                                      |
|            |             |        | - MVCompensate : behavior modification on scene changes. Now, the                                         |
|            |             |        |   current frame is left untouched if a scene change was detected.                                         |
|            |             |        | - New filter : MVSCDetection, with one parameter, Ysc, which works as previously.                         |
|            |             |        | - MVInterpolate, MVConvertFPS and MVBlur are enabled, but may be buggy                                    |
|            |             |        |   (though I wasn't able to make MVConvertFPS crash)                                                       |
+------------+-------------+--------+-----------------------------------------------------------------------------------------------------------+
| 0.9.5      |             |        | - Huge rewritting of the core engine. Vectors are now searched with a                                     |
|            |             |        |   precision up to the pixel (because no other filters can use yet more                                    |
|            |             |        |   precise vectors, except MVShow). The search engine is now fast (which                                   |
|            |             |        |   doesn't mean necessarily that the filters which use it are fast)                                        |
|            |             |        | - A new parameter for MVAnalyse : lambda. See the documentation of the                                    |
|            |             |        |   filter to see how it works                                                                              |
|            |             |        | - MVDenoise works better.                                                                                 |
+------------+-------------+--------+-----------------------------------------------------------------------------------------------------------+
| 0.9.4      |             |        | - Vectors can be saved to a file. In order to do so, add the parameter                                    |
|            |             |        |   filename=``"C:\foo.bar"`` to the filter MVAnalyse. If the file doesn't exist,                           |
|            |             |        |   vectors will be saved into it. If it exists, vectors will be read from                                  |
|            |             |        |   it. But, be warned:                                                                                     |
|            |             |        |                                                                                                           |
|            |             |        |   - The file for a whole movie will be around 500 MB                                                      |
|            |             |        |   - Saving / reading from a file need for the moment a linear access                                      |
|            |             |        |     to the frames, so it has to be used only when encoding the movie, not                                 |
|            |             |        |     when doing random access in it.                                                                       |
|            |             |        |   - The speed gain is not as great as one may hope, because SADs                                          |
|            |             |        |     can't be saved (it would take too much space) and so have to be                                       |
|            |             |        |     recomputed.                                                                                           |
|            |             |        |                                                                                                           |
|            |             |        | - The filter MVDenoise now works on 5 frames, and its parameters are                                      |
|            |             |        |   now "thT" and "sadT" (have a look in the documentation to see how they                                  |
|            |             |        |   work). It works nice (very good for heavy denoising)                                                    |
|            |             |        | - The scene change detection thresholds have slightly changed. Now, a                                     |
|            |             |        |   block has changed if its SAD it over thSCD1. The default for thSCD1 is                                  |
|            |             |        |   300, and for thSCD2 it is 130. It orks well (better than the previous                                   |
|            |             |        |   SCD engine).                                                                                            |
+------------+-------------+--------+-----------------------------------------------------------------------------------------------------------+
| 0.9.3      |             |        | - Last cleanings in the search of the motion vectors. It should be slightly faster                        |
|            |             |        | - More search parameters can be set by the user, especially the search                                    |
|            |             |        |   algorithm. See the documentation                                                                        |
|            |             |        | - Server / client implemented. You now first have to use MVAnalyse, and                                   |
|            |             |        |   then the filter you want. Look at the documentation and at the examples                                 |
|            |             |        |   I'll give alter.                                                                                        |
|            |             |        | - MVCompensate is separated from MVShow (it's more logic that way).                                       |
|            |             |        |   For the moment, it doesn't move the chroma (same behavior as MVShow in                                  |
|            |             |        |   the latest releases)                                                                                    |
|            |             |        | - Some cleaning in MVBlur / MVInterpolate / MVConvertFPS, but still                                       |
|            |             |        |   some work to do. Now, MVBlur blurs around the frame, not between the                                    |
|            |             |        |   frame and the previous one.                                                                             |
|            |             |        | - Half of the work is done for writing vectors to a file. But the                                         |
|            |             |        |   resulting file will be large ( around 500 MB - 1 GB I guess ).                                          |
|            |             |        | - MVDenoise is slightly faster ( at least it should )                                                     |
|            |             |        | - Copies are optimized inside the filter, thanks to avisynth's copy functions.                            |
|            |             |        | - MVShow can display the mean SAD of the compensation (using showsad = true)                              |
+------------+-------------+--------+-----------------------------------------------------------------------------------------------------------+
| 0.9.2.1    |             |        | - MVInterpolate makes its come back.                                                                      |
|            |             |        | - MVConvertFPS should work on the last few frames of the clip                                             |
+------------+-------------+--------+-----------------------------------------------------------------------------------------------------------+
| 0.9.2      |             |        | - MVInterpolate doesn't exist anymore.                                                                    |
|            |             |        | - MVBlur and MVConvertFPS have been improved. They also have got new                                      |
|            |             |        |   parameters, have a look at the documentation.                                                           |
|            |             |        | - MVShow gets back its compensate mode ( MVShow(cm = true) )                                              |
+------------+-------------+--------+-----------------------------------------------------------------------------------------------------------+
| 0.9.1      | 12.05.2004  |        | - First version renamed to MVTools.                                                                       |
+------------+-------------+--------+-----------------------------------------------------------------------------------------------------------+
| 0.1-0.6    | 24.01.2004- | Manao  | - First public versions MotionVectors (Motion.dll).                                                       |
|            | 01.04.2004  |        |                                                                                                           |
+------------+-------------+--------+-----------------------------------------------------------------------------------------------------------+

$Date: 2006/09/17 17:47:05 $

.. _MVTools thread.:
    http://forum.doom9.org/showthread.php?s=&threadid=76041
.. _MaskTools: masktools.rst
.. _Depan: depan.rst

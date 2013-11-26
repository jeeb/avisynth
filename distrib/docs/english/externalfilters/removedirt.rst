
RemoveDirt
==========


Abstract
--------

| **author:** Rainer Wittmann (aka kassandro)
| **version:** 0.6.1
| **download:** `<http://www.removedirt.de.tf/>`_
| **category:** Temporal Smoothers
| **requirements:**

-   YV12 & YUY2 Colorspace
-   SSEMMX support
-   width and the height of the clip must be a multiple of 8

--------


Overview
--------

``RemoveDirt`` is a so called temporal cleaner for Avisynth 2.5x. If you
don't want to go through the documentation, use it without setting any
variables. For an average movie this should take out about half the dirt
without creating visible artifacts. Though the default setting is quite
conservative, very fast motion bears a moderate artifact risk. However,
because postprocessing should remove all non-smooth artifacts, these rare
artifacts are usually smooth and very short in time, whence hard to catch for
the human eye. Clips are very different and tuning the variables, which
control the behavior of RemoveDirt, can make a big difference. Two different
debugging tools can be used for this purpose. The variables themself can be
explained best by discussing the underlying algorithm. If you don't
understand a variable, don't change its default value and post a question in
the `RemoveDirt thread`_ of the doom9 forum rather than sending me email.

No automatic cleaner can catch all dirt without destroying the movie by
artifacts. The amount of cleaning depends heavily on the amount of motion. To
avoid artifacts cleaning must be suppressed in the *moving* areas of a clip.
A moving camera is clearly the worst case. The first and the last frame of a
clip are never cleaned and there will hardly be any cleaning at a sharp scene
switch, because RemoveDirt requires that the previous and the subsequent
frame are existent and fit together well. Because ``RemoveDirt`` - unlike
other cleaners - tries to remove dirt but never blurs dirt, it is adviceable
to use it together with a slight softener for the remaining dirt, in order to
obtain better compression (such a filter should be *after* ``RemoveDirt``).
If you use divx 5.1.1 the builtin preprocessing filter is a good choice, but
based on my personal tests I would recommend only *light* preprocessing. In
its default configuration ``RemoveDirt`` can hardly remove any big pieces of
dirt, but it usually removes a lot of small dirt and dust and after being
processed by ``RemoveDirt`` the clip should look significantly cleaner.

``RemoveDirt`` supports the two most important color spaces YV12 and YUY2.
Support for other YUV color spaces can be added, but the design makes it
impossible to support RGB color spaces (convert to YUY2 with Avisynth's
internal filter ConvertToYUY2 if you stumble over such color spaces). The
width and the height of the clip must be a multiple of 8.

As can be seen from the source code and the descriptions below,
``RemoveDirt`` is quite sophisticated. To achieve reasonable performance it
was necessary to program all low level routines in assembly language using
extensively Intel's integer SSE instructions. Thus a modern CPU with integer
SSE capability is necessary for using this plugin.


Installation
------------

Beginning with version 0.5 the binary package of RemoveDirt contains two
versions of the plugin. RemoveDirt.dll (dynamically linked, hence small) and
RemoveDirtS.dll (staticly linked, hence big). Try first the small dll and
copy it to Avisynth's plugin directory. If it doesn't work, this is probably
due to missing msvcr70.dll library. Either you install this library in
``C:\windows\system32`` or you delete RemoveDirt.dll and replace it by
RemoveDirtS.dll. Please do not put both dlls into the plugin directory. If
you fill the plugin directory with all kinds of superfluous dlls, then you
only slow down the start of any application which uses Avisynth. RemoveDirt
is safe with respect to multithreading and may be used arbitrarily often
within a single Avisynth script. There should be no conflict with other
filters.


Dirt Detection
--------------

1. **A dirty spot or scratch occurs only on one frame and not on a whole
   sequence of frames.** This basic property is exploited by any *temporal cleaner*.
   Thus scratches caused by bad film projectors at the same
   position on a series of frames *cannot* be corrected by a temporal
   cleaner. In our program design the basic property is made precise as
   follows: a pixel is *potentially dirty* if cf[x,y] is not between
   min(pf[x,y],nf[x,y]) and max(pf[x,y],nf[x,y]), where cf[x,y] denotes the
   grey value of the pixel with coordinates x and y on the current frame and
   similarily pf[x,y] and nf[x,y] denote the grey value of the same pixel on
   the previous and the subsequent frame. In other words a potentially dirty
   pixel is out of the range spanned by the previous and the subsequent
   frame. We use the word potentially, because, nnfortunately, not only dirt
   but also some kind of motion are causing a pixel to get out of range. If
   one does not distinguish between both, dirt removal will ruin rather than
   clean a clip. This unpleasant fact makes automatic true dirt removal with
   low artifact risk so challenging. The extreme example is a moving thin
   black line with a white background. Because of motion any black pixel is
   white on the previous and the subsequent frame, whence in this example *all*
   black pixel are *always* out of range. On the other hand, if the
   width of the moving object is large in the direction of motion (this is
   the reason why the line has to be thin in the above example) and if the
   pixels of the moving object have similar color, then the pixels of the
   moving object are not significantly out of range. Hence from the point of
   view of cleaning there is *good* and *bad* motion. Cleaning bad motion
   causes artifacts, good motion does not.

2. **If a cleaned block has one edge in common with a block, which has
   not been cleaned because of some reason, then cleaning should not
   increase the difference of the two adjacent border lines of both blocks
   by too much**, otherwise the cleaning has to be considered as incorrect
   and must be undone. In other words cleaned blocks should fit reasonably
   well to blocks, which are not cleaned. This is a *postprocessing*
   technique. As technical as it may look, it is an extremely powerful idea
   to avoid ugly artifacts. Frankly, I think that automatic true dirt
   removal with low artifact risk is hardly possible without this idea. For
   postprocessing ``RemoveDirt`` uses the variable  pthreshold and
   cthreshold. If the total luma difference of the two adjacent border line
   increases by more than  pthreshold, then cleaning of the block is undone
   by ``RemoveDirt``. Similarily cleaning is undone, if the chroma
   difference exceedes cthreshold. The maximal difference of 8 pixels is
   8*255. If cthreshold is larger than this value, then chroma
   postprocessing is disabled. For YUY2 chroma postprocessing is always
   disabled because it is not yet implemented. If both,  pthreshold and
   cthreshold, are larger than the maximal value, then postprocessing is
   completely disabled.  pthreshold = 20 is the default value and if nothing
   else is specified cthreshold has the same value as  pthreshold. Clearly
   luma postprocessing is much more important than chroma postprocessing. If
   cthreshold and especially  pthreshold, then rather unpleasant blocky
   artifacts become visible. These are much more likely in areas with very
   flat contrast. Of course, there must be motion as well to get such
   artifacts. If you see these typical blocky artifacts, you should lower
   the thresholds. Postprocessing should only be disabled if all the cleaned
   frames are checked for artifacts. By the very nature of the algorithm no
   postprocessing will occur if all blocks were cleaned. There must be at
   least one block, which has not been cleaned to trigger postprocessing.
   The postprocessing algorithm loops through all the blocks as long as it
   can find blocks to be restored, nevertheless it is quite efficient. It is
   the basic philosophy of ``RemoveDirt``, that motion detection needs only
   detect at least one but not all blocks of a moving object. The rest is
   then taken care by postprocessing.


Motion Detection
----------------

Any procedure which allows to distinguish between dirt and motion can be
considered as some kind of motion detection. From this point of view the
previous section already contains two such procedures. Both are dirt specific
and can be considered as nonconventional. In this section we consider general
*dirt independent* motion detection. The basic idea to decide whether a
block is a *motion block* or not, is to measure the difference of this block
with its counterpart on the previous or the subsequent frame. However, for
the purpose of cleaning this is not appropriate, because if the difference
with its counterpart in a neighbour frame is low, then it cannot contain much
dirt either. Instead the principal idea is to measure the difference of the
blocks in the previous and the subsequent frame but not the current frame.
Thus for the basic motion detection, the current frame (i.e. the frame to be
cleaned) is *not used at all*, only the previous and the subsequent frame
are used for this purpose. Within ``RemoveDirt`` the behavior of basic motion
detection is controlled primarily by the  mthreshold variable. Though a block
contains 64 pixels the difference function used by ``RemoveDirt`` returns
only values between 0 (identical blocks) and 8*255 (difference between an
absolutely black and an absolutely white block). If this difference exceeds
the value of  mthreshold, then the block is marked as a motion block.
mthreshold=150 is the default value. The higher the value of  mthreshold the
more cleaning (but also with a higher artifact risk). In addition to the
absolute threshold represented by the variable mthreshold, ``RemoveDirt`` can
use also adaptive thresholds which vary from block to block (actually
``RemoveDirt`` uses a block specific threshold only if it is smaller than
mthreshold). We do not describe the somewhat complicated algorithm here. The
adaptive thresholds are controlled by the athreshold variable. It may very
well attain negative values, but if athreshold < -mthreshold/2, then there
will be no cleaning at all. athreshold=50 is the default value. The higher
the athreshold variable the more blocks are cleaned and the higher the
artifact risk. If  mthreshold and athreshold are both > 8*255, then *all*
blocks are cleaned. In the presence of bad motion this choice will creat
massive artifacts and should only be used for selected frames using range
files as decribed below. There are two further variables  dist and tolerance,
which control basic motion detection. dist=1 and tolerance=12 are the default
values. A block B is considered a neighbor of a block A by RemoveDirt, if
both horizontally and verticaly both blocks are only dist blocks apart. For
dist=0 a block has only one neighbor, the block itself. If dist=1, then a
block has 9 neighbors, if it is not located at the boundary. If dist=2, then
each inner block has 25 neighbors and if dist=3 then it has 49 neighbors
etc.. Now for a given block ``RemoveDirt`` counts all the neighbor blocks
which are marked as motion blocks. If the percentage of motion blocks among
all neighbor blocks exceeds the value of tolerance, then the block is not
cleaned. Thus in the default case of 9 neighbor blocks and tolerance=12 one
motion block is allowed and cleaning will nevertheless be allowed. In
particular, a motion block is cleaned, if it has no other motion blocks as
neighbors. This is reasonable, because motion rarely occurs on one tiny block
alone. On the other, if motion blocks have a certain density then also the
neighbors should not be cleaned. This is the idea behind the variables dist
and tolerance. A higher value of dist results in less cleaning. The higher
the value of tolerance, the more cleaning. If tolerance >= 100 then all
blocks are cleaned. Using the default values ``RemoveDirt`` will be able to
distinguish a moving thin black line from dirt, if it doesn't move too fast.
Motion detection only needs to catch parts of a moving area. Then the rest is
usually taken care by postprocessing. The disadvantage of the basic motion
detection is that it doesn't distinguish between good and bad motion. In
fact, good motion is easier to catch than bad motion. I hope that one fine
day I can do better in this respect.

The above method of motion detection has a serious drawback, though. If a
block on a frame has a substantial amount of dirt, then the same block on the
previous and on the subsequent frame will falsely be detected as a motion
block. Consequently, if a lot of small dirt is scattered all over a sequence
of frames, cleaning performance of RemoveDirt may degrade. The idea to fix
this problem is, to take the two neighbor frames, but not the current frame,
from an already cleaned version of the clip. An Avisynth script with this
kind of *double processing* will look like the following

::

    input = MPEG2Source("input.d2v")
    firstpass = RemoveDirt(input)
    cleaned = RemoveDirt(input, neighbour=firstpass)

Here  neighbour is a variable, which takes *clips* as values (the default
value is the clip specified in the first argument). This technique can be
iterated further, but of course the script executes slower and slower with
each iteration. Normally the number of cleaned blocks increases by far less
than 1% using iteration and this may not be enough reward for doubling the
execution time. All kind of debugging should be disabled for the first pass
if a debugging chaos is not desired. All the other variables should probably
have identical values, but there is room for many experiments. Of course, one
can do a lot of nonsense with the neighbour variable. ``RemoveDirt`` only
rejects a neighbour clip, if its format is so different from the format of
the primary clip, such that it would compromise stable execution of the
plugin.


Cleaning Modes
--------------

Currently three cleaning modes are implemented in ``RemoveDirt``. They are
accessed through the mode variable. mode = 0 is a very simple averaging mode.
If a block is marked as cleanable, then the pixels of this block are replaced
by the average of the corresponding pixels in the previous and the subsequent
frame. If postprocessing is disabled, it very easily generates artifacts. The
default mode = 2 is much smarter but also a little slower. If cf[x,y] is the
grey value of a pixel in a cleanable block and if pf[x,y], nf[x,y] are the
corresponding grey values in the previous and the subsequent frames, then
cf[x,y]is left unchanged if min(pf[x,y],nf[x,y]) <= cf[x,y] <=
max(pf[x,y],nf[x,y]). If cf[x,y] < min(pf[x,y],nf[x,y]) then cf[x,y] is
replaced by min(pf[x,y],nf[x,y]) . Finally, if cf[x,y] >
max(pf[x,y],nf[x,y]), then cf[x,y] is replaced by max(pf[x,y],nf[x,y]). The
chroma values are handled in the same way. This is the method I am using all
time and probably yields the best results. Due to SSE it is not much slower
than the simple averaging method. Finally there is the experimental mode = 1,
which is somewhere in the middle between mode 0 and 2 but closer to mode 2.
Mode 1 is the most complicated and slowest mode. Initially I had hoped to
obtain the better compression with mode = 1, but a first test was
discouraging. Further tests will show, whether this mode should be dropped or
not.


Higher Speed for Black&White Clips
----------------------------------

If you have a movie which requires cleaning it is likely to be old whence
often black and white. Of course the chroma of b&w clips doesn't need to be
processed and cpu time can be saved. To accomplish this, ``RemoveDirt``
should be used with the variable grey = true (the default is of course grey =
false). Then instead of simply copying the old chroma to the new one, the new
chroma is simply assigned the constant value 128. This is faster than copying
and, as a byproduct, erases all chroma noise. In other words, if there was
any color before, it is erased (Avisynth's builtin filter `greyscale`_ has
exactly the same effect). grey = true also disables chroma post processing,
which doesn't make any sense for b&w clips. Because there cannot be any
chroma postprocessing,  pthreshold should be somewhat lower, say 30 or 40.


Debugging
---------

The boolean variable debug and the integer variable show are used for
debugging. If ``RemoveDirt`` is used with show = 1, then all blocks, which
are cleaned are colored red. If ``RemoveDirt`` is used with show = 2, then
all blocks, which are not cleaned, are colored red. The default value show =
0, of course, implies that no coloring is done. Of course the show mode can
only be used for previewing and is useful to see quickly which areas are
caught by ``RemoveDirt`` and which it cannot clean. Using VirtualDubMod's F5
key, one can nicely see how changes of variables effect cleaning. The show
mode only works for YUY2 clips. However, this is an inconvenience rather than
a restriction. One has simply to put Avisynth's internal filter
`ConvertToYUY2`_ in front of ``RemoveDirt``. Of course this should only be
done for previewing. If the script is compressed, ConvertToYUY2 should be
removed to avoid unnecessary color space conversions.

If debug = true then ``RemoveDirt`` sends output of the following kind to the
debugview utility:

::

    [1120] [36536] RemoveDirt: 5779 blocks cleaned (93%), 229 motion blocks (3%), 0 blocks restored, 1 loops
    [1120] [36537] RemoveDirt: 5745 blocks cleaned (93%), 246 motion blocks (3%), 3 blocks restored, 2 loops
    [1120] [36538] RemoveDirt: 5348 blocks cleaned (86%), 378 motion blocks (6%), 17 blocks restored, 5 loops
    [1120] [36539] RemoveDirt: 4772 blocks cleaned (77%), 778 motion blocks (12%), 0 blocks restored, 1 loops
    [1120] [36540] RemoveDirt: 4396 blocks cleaned (71%), 1028 motion blocks (16%), 1 blocks restored, 2 loops
    [1120] [36541] RemoveDirt: 4695 blocks cleaned (76%), 820 motion blocks (13%), 3 blocks restored, 2 loops
    [1120] [36542] RemoveDirt: 5362 blocks cleaned (86%), 384 motion blocks (6%), 48 blocks restored, 3 loops
    [1120] [36543] RemoveDirt: 4038 blocks cleaned (65%), 1275 motion blocks (20%), 6 blocks restored, 2 loops
    [1120] [36544] RemoveDirt: 3780 blocks cleaned (61%), 1522 motion blocks (24%), 3 blocks restored, 2 loops
    [1120] [36545] RemoveDirt: 3943 blocks cleaned (63%), 1359 motion blocks (22%), 6 blocks restored, 2 loops
    [1120] [36546] RemoveDirt: 4104 blocks cleaned (66%), 1225 motion blocks (19%), 3 blocks restored, 2 loops
    [1120] [36547] RemoveDirt: 4287 blocks cleaned (69%), 1099 motion blocks (17%), 1 blocks restored, 2 loops
    [1120] [36548] RemoveDirt: 4041 blocks cleaned (65%), 1261 motion blocks (20%), 0 blocks restored, 1 loops
    [1120] [36549] RemoveDirt: 3901 blocks cleaned (63%), 1409 motion blocks (22%), 2 blocks restored, 3 loops
    [1120] [36550] RemoveDirt: 3757 blocks cleaned (60%), 1507 motion blocks (24%), 4 blocks restored, 2 loops
    [1120] [36551] RemoveDirt: 3799 blocks cleaned (61%), 1502 motion blocks (24%), 6 blocks restored, 2 loops

The first number on the left hand side is the id of the process, which runs
the script, the second is the frame number, the *blocks cleaned* number
explains itself, the *motion block* number is the number of motion blocks,
which were found by the various motion detection routines, *blocks restored*
is the number which were restored by the postprocessing routine and the loops
number is the number of *loops* which were needed by the postprocessing
routine to complete its task. If dist = 0 and  tolerance < 100, then the
relation

    blocks cleaned + motion blocks + restored blocks = total number of
    blocks = (frame width / 8) * (frame height / 8)

holds. If a block is cleaned, it doesn't imply that there was dirt. It would
be too time consuming exclude clean blocks from cleaning and using all the
artifact protection discussed before, cleaning a clean block should not make
a visable difference and should improve compression slightly. The *blocks restored*
number is of particular importance. The average *blocks restored*
number should be 1-2% of all blocks. If it is higher, then the setting is
probably too aggressive, if it is lower a more aggressive setting can be
used. If a sharp scene switch occurs, then there are always two frames with
very low *blocks cleaned* numbers. the first frame is the last frame of the
old scene and the second frame is the first frame of the new scene. This is a
rather characteristic pattern. In general the numbers are quite motion
dependent. Here are some extraordinary frames, which I observed recently,
while processing a rather dirty old b&w movie:

::

    [1008] [2652] RemoveDirt: 5495 blocks cleaned (89%), 13 motion blocks (0%), 587 blocks restored, 32 loops
    [1008] [22269] RemoveDirt: 4196 blocks cleaned (68%), 729 motion blocks (11%), 445 blocks restored, 20 loops
    [1008] [24016] RemoveDirt: 1783 blocks cleaned (29%), 1957 motion blocks (32%), 1017 blocks restored, 16 loops

Frame 2652 is particularily remarkable: though there were only 13 motion
blocks, postprocessing did catch a whopping 587 blocks with a whopping 32
loops. I had never seen such numbers before and I thought that they are only
theoretically possible. Though it costs some time, I still run ``RemoveDirt``
always with debug = true and if I see exceptional numbers as above, then I
always make a visual inspection of such exceptional frames. In the above
case, all frames were without visual artifacts.


Range Files
-----------

Unfortunately the amount of dirt per frame usually varies throughout a movie.
For instance, the first minutes of a film are often particular dirty, because
these first minutes are located on the outside of the film roll. Also many
particularly bad single frames are scattered all over a film. These
situations cannot be handled optimally by a single variable setting. For this
purpose *range files* were implemented in ``RemoveDirt``. One may use up to
9 range files, which are specified with the range1, range2, ... , range9
variable. Each range file has its own set of variables, which have the same
name as the global variables but with the range number attached at the end.
For instance, if ``RemoveDirt`` is used with
::

    RemoveDirt(range3= "myrange", mthreshold3=200, athreshold3=300,
    tolerance3=0, dist3=2)

then ``RemoveDirt`` uses mthreshold = 200, athreshold = 300 etc. for all the
frames specified in the range file "myrange", which is expected to be located
in the directory of the Avisynth script, but also different locations can be
used by specifying the full path. For all the other frames RemoveDirt just
uses the default values, because none of the global variables has been
changed. A range file like "myrange" contains either single frame numbers
like 13054 or ranges of frames like 13756-64, which is a shortcut for
13756-13764 (these numbers can be found by previewing the clip
VirtualDubMod). The frame numbers in a range file must be *increasing*. This
agreement makes certain shortcuts possible, which can be seen from the
subsequent example. The various ranges must also be separated by a white
space. Here we have a typical example:

::

    0-43 67 287 9
    1211-39 387 1432-544
    11780-2833

The above range file covers the followings frames: 0-43, 67, 287, 289,
1211-1239, 1387, 1432-1544, 11780-12833. If a frame is specified in more than
one range file, then the range file with the highest range number has
precedence. For instance, if a frame is specified in range2 and range4, then
the variables for range4 are used. In the future, I will use values a bit
more agressive than the default values for the global variables. Then I will
have a range file with significantly more aggressive values for the first
minutes and perhaps some other scenes, if there is not too much motion and
finally I will have a range file with variables set for total cleaning, which
will be used for single extremely dirty or damaged frames. Though I already
use RemoveDirt for real work (over time I have filled an entire 120 GB
exchangeable hard disk with dirty old b&w movies of DVB origin and this hard
disk is now gradually worked down), testing is still an important issue and
this is the only reason, why I am not yet using range files.


Optimal Usage
-------------

1. **Never crop after RemoveDirt.** Modern codecs divide the frames in
   the same way as ``RemoveDirt`` into a grid of 8x8 pixel blocks (codec
   experts, could you please confirm this?) to perform the crucial
   *discrete cosine transform* for such blocks. Now if the clip is cropped
   after ``RemoveDirt``, then the grid of ``RemoveDirt`` and the codec are
   likely to be different resulting in subpar compression. There is one
   exception, though: cropping afterwards does not hurt, if all four sides
   are cropped by a multiple of 8. For instance, Crop(8,64,0,-72) is ok.

2. `Crop only with "align=true"`_. ``RemoveDirt`` heavily uses SSE
   instructions. If you crop without "align=true" before ``RemoveDirt``,
   then the data on the frames may not be properly aligned and RemoveDirt
   will execute substantially slower. As a consequence you should always
   crop with Avisynth and not with DVD2AVI.

3. **Telecined movies must be inverse telecined before RemoveDirt.** If
   a film is telecined some fields are doubled in order to increase the
   frame rate from 24fps to 30fps. Hence on such doubled fields the basic
   property of dirt, described above, is no more valid and no temporal
   cleaner can ever spot dirt on such doubled fields. On the other hand,
   after an inverse telecine usually every fourth frame is composed of
   fields, which originate from two different frames. Visually these two
   fields fit together well but both are from a different *compression context*,
   which can mislead ``RemoveDirt`` to false motion detection. In
   extreme cases, one field may be from an I- or P-frame, while the other is
   from a B-frame. But even if the fields are from from frames of identical
   type, the different compression context has a substantial effect.
   Consequently ``RemoveDirt`` performes less well on inverse telecined
   movies than on natively progressive movies. By the same reason also
   compression of inverse telcined movies is worse than of natively
   progressive movies. We in Europe should thank god every day that we are
   not getting telecined. However, here in Germany we have digital tv
   broadcasters, which like to comb progressive films (about 5% of all
   progressive movies from ARD and especially ZDF are combed). Fortunately
   these idiots are not able to double fields, so RemoveDirt should work,
   but on combed films the dirt is always split over two frames which
   clearly hurts ``RemoveDirt``. On the other hand, if these combed films
   are uncombed, then we have the compression context problem for any frame
   and not only for any fourth frame. This is also the reason why Trbarry's
   uncomb filter doesn't work well in practice. The same question arises for
   interlaced movies. Should we deinterlace before or after ``RemoveDirt``?
   On the other hand, dirt and scratches usually occur only on photographic,
   hence progressive, film. Thus this question shouldn't matter much. From
   the above discussion it should be clear that for measuring the quality of
   RemoveDirt only progressive clips should be used, which have never been
   telecined, and which have never been messed up by stupid digital tv
   providers. My plugin *AlignFields* can be used to decide with near
   absolute certainty, whether a clip fullfills these quality constraints or
   not.

4. **Put other filters after RemoveDirt.** Except those filters
   mentioned before, like crop and inverse telecine, all other filters
   should be put after ``RemoveDirt`` in the Avisynth script, because most
   filters have a negative rather than a positive impact on dirt detection.


All Variables at a Glance
-------------------------

+------------+---------+---------------+---------------------+------------------+
| Name       | Type    | Default Value | Remarks             | Section          |
+============+=========+===============+=====================+==================+
| neighbour  | Clip    | primary clip  |                     | Motion Detection |
+------------+---------+---------------+---------------------+------------------+
| dist       | Integer | 1             | >=0                 | Motion Detection |
+------------+---------+---------------+---------------------+------------------+
| mthreshold | Integer | 150           | >=0                 | Motion Detection |
+------------+---------+---------------+---------------------+------------------+
| athreshold | Integer | 50            |                     | Motion Detection |
+------------+---------+---------------+---------------------+------------------+
| pthreshold | Integer | 20            | >=0                 | Dirt Detection   |
+------------+---------+---------------+---------------------+------------------+
| cthreshold | Integer | pthreshold    | >=0,                | Dirt Detection   |
|            |         |               | only for YV12 clips |                  |
+------------+---------+---------------+---------------------+------------------+
| tolerance  | Integer | 12            | 0<=tolerance<=100   | Motion Detection |
+------------+---------+---------------+---------------------+------------------+
| mode       | Integer | 2             | only 0,1,2          | Cleaning Modes   |
+------------+---------+---------------+---------------------+------------------+
| grey       | Boolean | false         |                     | Black&White      |
+------------+---------+---------------+---------------------+------------------+
| debug      | Boolean | false         |                     | Debugging        |
+------------+---------+---------------+---------------------+------------------+
| show       | Integer | 0             | only 0,1,2,         | Debugging        |
|            |         |               | only for YUY2 clips |                  |
+------------+---------+---------------+---------------------+------------------+

All these variables can be specified for range files by attaching the number
of the range file at the end of the variable. For instance, *mode5* is the
mode variable for range5.


RemoveDirt.ini
--------------

For a filter with many variables like ``RemoveDirt`` (together with all the
variables for range files, RemoveDirt has more than 100 variables), where the
settings depend strongly on the input clip, it desirable to have more than
one default setting. Rather than hard wiring these default settings into the
binary, these default settings are placed into the file RemoveDirt.ini. Now,
if we have RemoveDirt(default="anime") in the script, then RemoveDirt looks
for the file RemoveDirt.ini in the directory of the script. If it cannot find
this file it terminates with an error message. Of course, without the
*default* option, RemoveDirt.ini is not needed. Then ``RemoveDirt`` looks for
the string *anime*. If it cannot find this string, it terminates with an
error message. Finally it reads all the variables **after** *anime* and
replaces the internal default values by the values in RemoveDirt.ini. The
format is
::

    name of variable = value of variable

If the variable is a string variable, then the value has to be enclosed in
quotes. The string must not contain the quote character ". If the comment
character # appears in the script (outside a string value, of course), then
everything after # until the end of the line is considered as a comment. Let
us look at the following concrete RemoveDirt.ini example:

::

    clensing # total clensing, only for selected frames specified in clensing.rmd
    range1 = "clensing.rmd"
    pthreshold1 = 5000
    mthreshold1 = 5000
    grey = false
    bw # black & white mode
    grey = true
    pthreshold = 20
    mthreshold =180
    anime # mode for anime videos
    pthreshold = 40
    cthreshold = 30
    mthreshold = 200

In this example RemoveDirt.ini contains three default modes: **clensing**,
**bw**, **anime**. The default mode clensing defines a range1 file
clensing.rmd (it must exist if this default mode is used) and the variables
pthreshold1 and mthreshold1 are chosen such that for all frames specified in
clensing.rmd all pixels are cleaned. It also uses grey = false, pthreshold =
20, mthreshold = 180 and cthreshold = 30 for the other frames, because these
variables are specified after clensing. For all the the other variables the
internal default values are taken. In the case RemoveDirt(default="bw") grey
= true, pthresholdd = 20, mthreshold =180 and cthreshold = 30 is used (of
course, cthreshold is ignored because grey = true). In the case
RemoveDirt(default= "anime") the values pthreshold = 40, cthreshold = 30 and
mthreshold = 200 are used. If there are syntax errors in RemoveDirt.ini, then
``RemoveDirt`` terminates with an error message, which displays the line
number with the first syntax error.


ImproveSceneSwitch
------------------

By the very nature of the underlying algorithm ``RemoveDirt`` cannot clean
at sharp scene switches. Only very few random blocks are cleaned. On the
other hand the first frame after a scene switch should be encoded as an
I-frame, and if this I-frame is particularily dirty, compression will suffer.
Similarily the last frame before a scene switch should be encoded as a
P-frame and dirt will be negative for compression as well. Most other
temporal filters have problems at sharp scene switches as well.
``ImproveSceneSwitch`` is a simple filter to improve this situation. The
difference d(n) of frame n and n+1 plays a fundamental role for this filter.
It is based on the SSE intruction *psadbw*. If YV12 is the color space of the
clip, then only the luma is used for calculating the difference, otherwise
luma and chroma are used. The reason for this difference is computational
convenience. If field = 0 (this is the default) then the entire frame is used
for calculating d(n). If field = 1, then only the top field is used for
calculating the difference. If field = 2, then only the bottom field is used
for calculating the difference. This variable is important for deinterlacers
like AlignFields. We determine a (sharp) scene switch between frame n and n+1
if d(n) > ratio*d(n-1) and d(n)>ratio*d(n+1). Here ratio is an integer
variable > 1 (ratio = 7 is the default). This variable determines the scene
switch sensitivity of ``ImproveSceneSwitch``. It should not be larger than
100 to avoid arithmetic overflow. Now, if we have a sequence n-1,,n,n+1,n+2
of frames with a scene switch between n and n+1, then ``ImproveSceneSwitch``
replaces the sequence n-1,n,n+1 by n-1,n-1,n+2,n+2. Of course this kind of
frame doubling should only occur at sharp scene switches and then the viewer
should hardly notice the trick. ``ImproveSceneSwitch`` is used as follows
::

    ImproveSceneSwitch(clip, orig, ratio = integer, extrapolate = boolean, \
       first = boolean, last = boolean, field=0,1,2, debug = boolean)

Here  clip is the output and  orig is the input of the temporal filter, which
should be improved by ``ImproveSceneSwitch``. orig is only used for
determining scene switches. Flashes and similar effects (very common in music
videos) may mislead ``ImproveSceneSwitch``. For such clips either
``ImproveSceneSwitch`` should not be used at all or a very high ratio above
10 should be chosen. By default both, the last frame of the old scene and the
first frame of the new scene are replaced by ``ImproveSceneSwitch``. However,
for some temporal filters like AlignFields only one of these frames should be
replaced. For this purpose we have the boolean variables  first and last. If
first = false, then the frame remains unchanged, even if it is detected as
the first frame of a new scene. Similarily, if last = false, then the frame
remains unchanged even if it is detected as the last frame of a scene. first
= true and last = true are the deafult values. ``ImproveSceneSwitch`` should
be applied as follows:

::

    input = MPEG2Source("input.d2v")
    dein = RemoveDirt(input)
    ImproveSceneSwitch(dein, input)

If  debug = true, then scene switch information is send to debugview. If
extrapolate = true (false is the default), then instead of simple frame
doubling extrapolation is used to calculate the scene switch frames. Though
SSE can be very nicely used for this purpose, extrapolation is obvious much
slower than frame doubling and compression is not as good. However, motion is
handled better by extrapolation. Hence ratio may be chosen somewhat lower if
extrapolate = true. On the other hand, if ratio is not too small, then the
viewer should hardly be able recognize frame doubling. I would very much
appreciate feedback in the forum about the question, whether to extrapolate
or not to extrapolate. As soon as a new version AlignField is released,
``ImproveSceneSwitch`` will be removed from RemoveDirt and moved to the
AlignField plugin. In the AlignField the scene switch code will be removed,
because ``ImproveSceneSwitch`` is better and faster. Currently I combine
``ImproveSceneSwitch`` with AlignFields through the following Avisynth
function

::

    function AF3(clip input)
    {
    dein = AlignFields(input,mode=3, scene=0, topfield=true, tff=true)
    return ImproveSceneSwitch(dein, input, ratio=5, first=true, last=false, field=2, debug=true)
    }

If topfield = true and tff = false, then first = false, last = true, field =
2 has to be chosen instead. If topfield = false and tff = true, then first =
false, last = true, field = 1 has to be chosen instead. Finally, if topfield
= false and tff = false, then first = true, last = false, field = 1 has to be
chosen instead. To determine the correct values for other deinterlacers, one
has to look at the source code. By the nature of the algorithm, the first two
and the last two frames are always left unchanged. ``ImproveSceneSwitch`` is
optimised for the situation, when the frames of the clip are requested
sequentially. This is typical for an encoding process. It is almost 4 times
slower if the frames are requested randomly or in reverse order.

By Rainer Wittmann "gorw at gmx.de"

$Date: 2004/08/17 20:31:19 $

.. _RemoveDirt thread: http://forum.doom9.org/showthread.php?s=&threadid=70856
.. _greyscale: ../corefilters/greyscale.rst
.. _ConvertToYUY2: ../corefilters/convert.rst
.. _Crop only with "align=true": ../corefilters/crop.rst

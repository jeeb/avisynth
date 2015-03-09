
Blockbuster
===========


Abstract
--------

| **author:** Ross Thomas
| **version:** 0.7
| **download:** `<http://www.avisynth.org/warpenterprises/>`_
| **category:** Misc Plugins
| **requirements:** YV12 & YUY2 Colorspace

--------


Description
-----------

Blockbuster is an Avisynth filter designed to reduce or eliminate DCT blocks
from an enocode. DCT blocks, also known as "dark blocks" when they appear in
low-luma areas of a frame, are ugly, distracting artifacts that MPEG encoders
like to scatter liberally over our otherwise flawless encodes.

While the severity of the problem varies between the different versions of
MPEG, with MPEG-1 exhibiting the most DCT blocks and MPEG-4 the least, they
do occur with all versions. MPEG-1 produces them almost anywhere there is a
flat surface with a low detail level (such as a wall), and MPEG-2 shows them
in the same areas but less frequently (they're particularly visible at lower
bitrates, as anyone familiar with digital satellite can testify). MPEG-4 has
signficantly less of a problem with low detail levels, but does still produce
DCT blocks, especially in dark areas (see `this Doom9 thread`_ for more on
these blocks and MPEG-4).

(Incidentally, DCT stands for "discrete cosine transform", and is one of the
techniques MPEG uses to do its compression. They differ from macroblocks,
which are most noticible in high-motion areas when the bitrate is
insufficient to describe the motion accurately, and also from mosquito noise,
which is an artifact that tends to appear around edges and areas of high
contast.)

As hinted at above, the cause of these blocks seems to be a lack of detail in
areas of the picture, which the encoder fails to "see" and so applies too
much compression. In the process what details were there are smoothed away
and the area turns into a DCT block.

The aim of this filter is to attempt to make those areas more "noticible" to
the encoder so that it allocates more bits and thus doesn't need to compress
so much. The astute amongst you will realise this filter is, basically,
designed to increase the bitrate (and decrease the compressibility) of your
clip. While this is considered a cardinal sin by most, there are at least two
valid reasons for doing so.

First, while the highest possible compression ratio is of course desirable,
one has to consider the cost. No-one would accept a movie with a resolution
of 120x90, even though everyone knows it would have an excellent compression
ratio. A similar effect could be achieved by smoothing the clip until it
looks like you smeared your monitor screen with vasoline, but again no-one
would accept this kind of image.

It could be argued that DCT blocks are similarly unacceptable in a good-
quality encode. Peoples' opinion on this differs greatly, but an increasing
number, myself included, are looking for a sure-fire way to get rid of them.
This filter is my contribution to the effort.

Second, as compression technology evolves and improves it is sometimes
necessary to re-encode old movies into new, more advanced formats. Removing
artifacts from existing movies is much more difficult than stopping them
appearing in the first place. Thus removing DCT blocks from your encodes now
will make it easier to re-compress your movies later.


How it works
------------

The filter splits each frame into a series of blocks (of user-configurable
dimensions) and checks each block in turn for the amount of detail it
contains. If the detail level is within the defined range then the block is
processed according to the *method* parameter (see the :ref:`Usage` section
below).


.. _Usage:

Usage
-----

There are several methods that can potentially reduce or eliminate DCT
blocks: adding noise, sharpening, and blurring. These different approaches
are accessed with the *method* parameter, combined with a set of parameters
shared by all methods as well as parameters specific to each one. There is an
additional method called "show" which highlights each block that will be
affected by the filter.

The general form of usage is

``Blockbuster`` (clip, method="...", shared parameters, method-specific
parameters)

The method parameter can be either "noise", "dither", "sharpen", "blur" or
"show". Parameters specific to each method will be listed in the :ref:`Methods`
section below. Parameters common to all methods are:

+------------------+----------------------------------------------+---------------------+
| Parameter        | Description                                  | Default             |
+==================+==============================================+=====================+
| *block_size*     | Determines the size of the blocks into which | 8                   |
|                  | each frame is split before processing. The   |                     |
|                  | value represents both the width and height   |                     |
|                  | of the block, and cannot be less than 3.     |                     |
+------------------+----------------------------------------------+---------------------+
| *detail_min*,    | Determines the amount of detail that must be | *detail_min=1*,     |
| *detail_max*     | present in a block for it to be processed.   | *detail_max=10*     |
|                  | This value is a percentage, and can be       |                     |
|                  | between 1 and 100. It represents the         |                     |
|                  | percentage of unique brightness levels       |                     |
|                  | within the block.                            |                     |
|                  |                                              |                     |
|                  | For example, if *block_size* is set to 8,    |                     |
|                  | each block contains 64 pixels. If            |                     |
|                  | *detail_min* is 1 and *detail_max* is 50, a  |                     |
|                  | block will only be processed if it contains  |                     |
|                  | between 1 and 32 unique brightness levels.   |                     |
|                  | each block contains 64 pixels. If            |                     |
|                  | *detail_min* is 1 and *detail_max* is 50, a  |                     |
|                  | block will only be processed if it contains  |                     |
|                  | between 1 and 32 unique brightness levels.   |                     |
|                  |                                              |                     |
|                  | A setting of *detail_min=1*,                 |                     |
|                  | *detail_max=100* will process the entire     |                     |
|                  | frame.                                       |                     |
+------------------+----------------------------------------------+---------------------+
| *luma_offset*,   | Luma pixels in the range 0-*luma_threshold*  | *luma_offset=0*,    |
| *luma_threshold* | will be offset by *luma_offset* within       | *luma_threshold=25* |
|                  | processed blocks.                            |                     |
|                  |                                              |                     |
|                  | For example, if *luma_threshold* is 30 and   |                     |
|                  | *luma_offset* is -2, dark pixels (those with |                     |
|                  | a luma between 0 and 30 inclusive) will have |                     |
|                  | 2 subtracted from them, making them slightly |                     |
|                  | darker still.                                |                     |
|                  |                                              |                     |
|                  | (Rationale: It was discovered that Marc FD's |                     |
|                  | mpeg2dec plugin's *lumoff* parameter seems   |                     |
|                  | to reduce the appearance of MPEG-4 "dark     |                     |
|                  | blocks" -- DCT blocks in dark areas of the   |                     |
|                  | frame -- further. These parameters are       |                     |
|                  | designed to do the same thing, but constrain |                     |
|                  | the brightness change only to areas that     |                     |
|                  | seem to need it.)                            |                     |
+------------------+----------------------------------------------+---------------------+


.. _Methods:

Methods
-------


method="noise"
~~~~~~~~~~~~~~

This method adds normally distributed -- also known as Gaussian -- noise to
the clip. Testing has shown that Gaussian noise is far more suitable for this
filter's purposes than uniformly distributed noise.

With uniformly distributed noise, each possible value is as likely to occur
as any other. That is, if you generate a sequence of numbers in the range
1-100, at any point in the sequence you are as likely to generate a 5 as a
95.

"Normally distributed" means the chance of each value occurring is not equal.
Let's say you generate normally distributed numbers with a mean of 0 and a
variance of 1 (for an explanation of these terms see the links at the end of
the paragraph). The generator can in theory spit out *any* number that can
fit into a double-precision floating point, but taken as a whole the numbers
will average out to zero (that's what mean=0, er, means). With these
parameters about 68% of the values will be between -1 and 1, about 95%
between -2 and 2, and about 99% between -3 and 3. The probability of
generating numbers that are significantly higher or lower is very small
indeed, with the probability getting smaller the further away from zero you
get. You'd need to make billions of Gaussian random numbers with mean 0,
variance 1 before you saw, for example, the value 9 being generated. You can
read more about normal distribution `here`_ and `at this site`_.

Gaussian noise very much tends to concentrate around the specified mean, and
is thus more "natural" than uniformly distributed noise. Most things in
nature (including spring precipitation, calorific intake, and, of course,
noise) cluster around a "normal" value, with progressively less frequent
occurrances as you get further from that norm.

You can read more about adding noise to eliminate DCT blocks in `this Ars
Technica thread`_ (edit by Wilbert: I can't find the relevant thread
anymore).
::

    Blockbuster(clip, method="noise", common parameters, float "mean", float "variance", int "cache", int "seed")

+------------+------------------------------------------------------+------------------------+
| Parameter  | Description                                          | Default                |
+============+======================================================+========================+
| *mean*,    | The mean and variance of the generated random noise. | *mean=0*, *variance=1* |
| *variance* |                                                      |                        |
+------------+------------------------------------------------------+------------------------+
| *cache*    | Because the generation of Gaussian numbers is very   | 256                    |
|            | slow, to achieve reasonable performance Blockbuster  |                        |
|            | creates a cache of random numbers at startup. This   |                        |
|            | parameter spcifies the size of the cache in          |                        |
|            | kilobytes. While you can set this value to any       |                        |
|            | positive number, it's best to keep it fairly large   |                        |
|            | in order to maintain properly random noise.          |                        |
|            |                                                      |                        |
|            | While the default may seem quite big, consider this: |                        |
|            | If you have a block size of 8, then each block has   |                        |
|            | an area of 64 pixels. Each pixel requires 2 bytes    |                        |
|            | to describe it, so a processed block will consume    |                        |
|            | 128 bytes of random data. If 10% of the blocks in    |                        |
|            | a 640x480 frame are processed, this will require     |                        |
|            | nearly 62kb of random numbers.                       |                        |
+------------+------------------------------------------------------+------------------------+
| *seed*     | By default the pseudo-random number generator will   | 0                      |
|            | use the system's current time as its seed value.     |                        |
|            | For certain applications where predictable results   |                        |
|            | are desired, the seed parameter can be used to       |                        |
|            | override this default, thus providing the same       |                        |
|            | "random" noise each time the filter is run.          |                        |
|            |                                                      |                        |
|            | The seed can be any number from 0 to                 |                        |
|            | 2,147,483,647. When zero, the system's current       |                        |
|            | time is used.                                        |                        |
+------------+------------------------------------------------------+------------------------+

method="dither"
~~~~~~~~~~~~~~~

This method is very similar to the "noise" method, with the only difference
being this method will add the same noise to each frame of the clip, whereas
the "noise" method will add different noise to each frame. The effect of this
is hard to describe, but easy to see, so try it for yourself with a high
variance. The closest comparison I can think of is that this method produces
an effect similar to watching the movie through speckled glass.

The reason I added this method is because I found when using method = "noise"
that the constantly changing nature of the noise produced motion in otherwise
static areas of the frame, particularly when using a low bit rate. It is my
hope that using the same noise for each frame will prevent artificial motion
in static areas. This should be considered experimental.
::

    Blockbuster(clip, method="dither", common parameters, float "mean", float "variance", int "seed")

For a description of this method's parameters, see the "noise" method above.
Note however that the "dither" method has no cache parameter since it always
generates only enough noise for one frame.


method="sharpen"
~~~~~~~~~~~~~~~~

This method applies a basic (and fast) sharpening filter to processed blocks.
In this way it "amplifies" detail already present in the block rather than
adding new noise.
::

    Blockbuster(clip, method="sharpen", common parameters, int "strength")

+------------+-------------------------------------------+---------+
| Parameter  | Description                               | Default |
+============+===========================================+=========+
| *strength* | Specifies the strength of the sharpening, | 25      |
|            | from 1-100.                               |         |
+------------+-------------------------------------------+---------+

method="blur"
~~~~~~~~~~~~~

This method applies a 3x3 blur to processed blocks. It is currently
experimental, since in theory reducing the frequency of already low-frequency
blocks will not reduce the appearance of DCT blocks. It's worth a try, though
:).
::

    Blockbuster(clip, method="blur", common parameters, int "strength")

+------------+-----------------------------------------+---------+
| Parameter  | Description                             | Default |
+============+=========================================+=========+
| *strength* | Specifies the strength of the blurring, | 25      |
|            | from 1-100.                             |         |
+------------+-----------------------------------------+---------+

method="show"
~~~~~~~~~~~~~

This method highlights blocks that will be processed using the specified
block-related common parameters (*block_size*, *detail_min*, and
*detail_max*). It is mainly useful as a visual aid in setting the detail
thresholds to the desired range.

This method has no additional parameters.

TODO
----

-   MMX/SSE optimizations (I have to learn x86 assembler first...).
-   Unsharp mask method.


Author
------

Ross Thomas <ross(at)grinfinity.com>

$Date: 2004/08/13 21:57:25 $

.. _this Doom9 thread:
    http://forum.doom9.org/showthread.php?s=&threadid=31301
.. _here: http://www.robertniles.com/stats/stdev.shtml
.. _at this site: http://davidmlane.com/hyperstat/A16252.html
.. _this Ars Technica thread: http://arstechnica.infopop.net/OpenTopic/page?a=tpc&s=50009562&f=67909965&m=3890938134&r=5470927074


NoMoSmooth
==========


Abstract
--------

| **author:** Sansgrip (Ross Thomas)
| **version:** 0.1b
| **download:** `<http://www.avisynth.org/warpenterprises/>`_
| **category:** Spatio-Temporal Smoothers
| **requirements:** YUY2 Colorspace

--------


Description
-----------

When it comes to smoothers (as opposed to smoothies) there are two main
varieties: temporal smoothers and spatial smoothers. Temporal smoothers work
by looking at frames surrounding the current one and averaging corresponding
pixels together if they're deemed similar enough (and thus likely to be
noise). Spatial smoothers work in a similar way, except they look at nearby
pixels within the same frame. A few filters are a hybrid of the two,
including this one and the excellent Convolution3D.

Each approach has its advantages and disadvantages. The great thing about
temporal smoothers is that they do a fantastic job getting rid of noise when
you set them up just right, but they can also generate some very ugly
artifacts, particularly when dealing with motion. Spatial smoothers, on the
other hand, can be quite destructive to the details in your image, but are
great with high-motion areas because those have few details and are moving
too quickly to notice artifacts anyway.

Ideally, then, one would like to apply a temporal smoother to relatively
static areas and a spatial smoother to moving ones. That's what this filter
tries to do, and the aim is for a higher quality result (i.e. less noticible
smoothing) than Convolution3D but with correspondingly less of an impact on
compressibility.

In addition to this motion-based approach, NoMoSmooth employs another
technique to try to retain as much existing detail as possible: only pixels
that are "fluctuating" are smoothed. In other words, given three frames in a
row with the current one in the centre, the pixel being examined will only be
considered for either type of smoothing if it is either greater than both the
corresponding pixels in the previous and next frames, or less than both. This
could be considered a very simplistic "noise detector" and seems to help
reduce temporal smoothing on important details such as those in skin tones.


Usage
-----

``NoMoSmooth`` (clip, int "motion_threshold", int "temporal_radius", int
"temporal_threshold", int "spatial_radius", int "spatial_threshold", bool
"show")

+----------------------+---------------------------------------+---------+
| Parameter            | Meaning                               | Default |
+======================+=======================================+=========+
| *motion_threshold*   | Controls how sensitive the motion     | 40      |
|                      | detector is to, er, motion, with      |         |
|                      | lower values being more sensitive     |         |
|                      | (seeing more motion) and higher being |         |
|                      | less sensitive. I might go into the   |         |
|                      | algorithm in more detail in a later   |         |
|                      | version of this file, but for now if  |         |
|                      | you're curious look at the source.    |         |
+----------------------+---------------------------------------+---------+
| *temporal_radius*    | Determines how far the temporal       | 1       |
|                      | smoother will venture into the clip   |         |
|                      | to do its work. If set to 2 a total   |         |
|                      | of 5 frames are examined, 2 on each   |         |
|                      | "side" of the current one.            |         |
+----------------------+---------------------------------------+---------+
| *temporal_threshold* | When the temporal smoother is         | 6       |
|                      | averaging it will only include values |         |
|                      | within this threshold of the pixel in |         |
|                      | the current frame.                    |         |
+----------------------+---------------------------------------+---------+
| *spatial_radius*     | Determines how many pixels the        | 1       |
|                      | spatial smoother will consider when   |         |
|                      | doing it's job. If set to 2 a total   |         |
|                      | of 25 pixels are examined, with the   |         |
|                      | current pixel in the centre.          |         |
+----------------------+---------------------------------------+---------+
| *spatial_threshold*  | The spatial smoother will only        | 3       |
|                      | include values within this threshold  |         |
|                      | when it is averaging.                 |         |
+----------------------+---------------------------------------+---------+
| *show*               | If true no smoothing will be carried  | false   |
|                      | out, but areas deemed to be in motion |         |
|                      | (according to the *motion_threshold*  |         |
|                      | parameter) will be highlighted in a   |         |
|                      | charming light grey.                  |         |
+----------------------+---------------------------------------+---------+

Known Issues
------------

- Pixels within *spatial_radius* of the edges get passed through.
- Frames within *temporal_radius* of the ends of the clip get passed through.
- While the algorithm is fairly well optimized it's still fairly slow,
  but very brief tests indicate it might be slightly faster than
  Convolution3D in preset="movieHQ" mode.


CopyRight
---------

There is no copyright on this code, and there are no conditions on its
distribution or use. Do with it what you will.


TODO
----

- Optimize both algorithmically and by implementing assembly code
  versions of critical parts of the algorithm.
- Improve motion detector such that it is more sensitive to motion in
  low-contrast areas.


Author
------

Ross Thomas <ross at grinfinity.com>


History
-------

+---------+---------------------------------------------------------------------------+
| Version | Description                                                               |
+=========+===========================================================================+
| 0.1b    | Compiled for AviSynth v2.5 by Wilbert. [YUY2 required!]                   |
+---------+---------------------------------------------------------------------------+
| 0.1a    | Fixed off-by-one error that caused an access violation when height > 462. |
+---------+---------------------------------------------------------------------------+
| 0.1     | Total rewrite with a number of algorithmic improvements, much more        |
|         | verbose comments and widespread use of assertions. Made brief             |
|         | preparations for an eventual YV12 version. Implemented "noise detector".  |
|         | First official (yet still alpha) release.                                 |
+---------+---------------------------------------------------------------------------+
| 0.0c    | Back on track. Rewrote again, this time making a conscious effort         |
|         | not to duplicate code already in the core. Implemented a motion detector  |
|         | so that areas in motion get spatially softened and static areas get       |
|         | softened temporally. The name "NoMoSmooth" makes less sense every time I  |
|         | read it.                                                                  |
+---------+---------------------------------------------------------------------------+
| 0.0b    | Highly embarrassing version algorithmically identical to                  |
|         | TemporalSoften which must never be spoken of again.                       |
+---------+---------------------------------------------------------------------------+
| 0.0a    | Rewrote from scratch and optimized the algorithm somewhat, so now is      |
|         | slightly faster. A change in the algorithm both removed the need for      |
|         | *noise_threshold* and made *show* mode extremely difficult to implement.  |
+---------+---------------------------------------------------------------------------+
| 0.0     | Proof of concept code. Hard hats must be worn in this area.               |
+---------+---------------------------------------------------------------------------+

$Date: 2006/12/15 19:29:25 $

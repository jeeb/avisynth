
Convolution3D
=============


Abstract
--------

| **author:** Vlad59
| **versions:**

- 1.01 (YUY2 version)
- beta 4 (YV12 version with temporal influence disabled)

| **download:** `<http://www.avisynth.org/warpenterprises/>`_
| **category:** Spatio-Temporal Smoothers
| **requirements:**

-   YV12 or YUY2 (dependent on version) colorspace
-   ISSE support

--------


Description
-----------

Convolution3D is an avisynth filter that will apply a 3D convolution to all
pixel of a frame.

How to use it
-------------

``Convolution3d`` (matrix=0, ythresh=3, cthresh=4, t_ythresh=3, t_cthresh=4,
influence=3, debug=0)

**Matrix choice** :

| 0 : original matrix :
| ``1 2 1   2 4 2   1 2 1``
| ``2 4 1   4 8 4   2 4 1``
| ``1 2 1   2 4 2   1 2 1``

This matrix is useful for normal movie (not anime) because it keep more
details

| 1 : bb idea of full 1 matrix (great idea)
| ``1 1 1   1 1 1   1 1 1``
| ``1 1 1   1 1 1   1 1 1``
| ``1 1 1   1 1 1   1 1 1``

This matrix is much usefull with animes or bad quality sources because it
blur a little more (so removing more noise)

| **Temporal influence** :
| It's used especially to speed up a little this filter and to avoid using
  temporal informations when not needed
  (scene change, fade, ...)
| I first build a limit = Temporal Luma Threshold * Temporal influence
| For each 2 pixel computed (due to MMX, 2 pixel at the same time), I first
  check this :

::

    if
        (Abs (Y0 - Y0[Previous frame]) +
        Abs (Y0 - Y0[Next frame]) +
        Abs (Y1 - Y1[Previous frame]) +
        Abs (Y1 - Y1[Next frame])) > limit
    then
        do Spacial work (only 3*3 matrix)
    Else
        do Spacial and Temporal work (3*3*3 matrix)

| The lower it is -> the faster will be the filter but compressibility should
  be lower
| The higher it is -> the slower will be the filter but compressibility should
  be higher
| if temporal influence is set to -1 then only spatial work is done (high
  speed).
| This parameter is a float.


Parameters sample
-----------------

I build the following presets to make things easier :
::

    Convolution3d (preset="movieHQ") # Movie Hi Quality (good DVD source)

is an alias for ``Convolution3D(0, 3, 4, 3, 4, 2.8, 0)``
::

    Convolution3d (preset="movieLQ") # Movie Low Quality (noisy DVD source)

is an alias for ``Convolution3D(0, 6, 10, 6, 8, 2.8, 0)``
::

    Convolution3d (preset="animeHQ") # Anime Hi Quality (good DVD source)

is an alias for ``Convolution3D(0, 6, 12, 6, 8, 2.8, 0)``
::

    Convolution3d (preset="animeLQ") # Anime Low Quality (noisy DVD source)

is an alias for ``Convolution3D(1, 8, 16, 8, 8, 2.8, 0)``
::

    Convolution3d (preset="animeBQ") # Anime Bad Quality (???)

is an alias for ``Convolution3D(1, 12, 22, 8, 8, 2.8, 0)``
::

    Convolution3d (preset="vhsBQ") # VHS capture Bad Quality (???)

is an alias for ``Convolution3D(0, 32, 128, 16, 64, 10, 0)``

I had to test Convolution3d with bad quality TV capture and in this case
you'll have to higher especially the chroma tresholds (causing some ghosting
but the overall quality seems to be better).

I personnaly use these parameters :

::

    Convolution3D (0, 32, 128, 32, 128, 10, 0)

The thresholds of Convolution3d are only here to take care of edges and scene
change. You can increase the spatial one (especially the chroma threshold)
but stop as soon as you see some blurring around the edges (if you want
quality). With the settings proposed you shouldn't have this problem.
The Temporal one should be left below 10 to avoid ghosting.
You should especially take care of the threshold with matrix 1, because with
this matrix the current frame has less weight so it's easier to have
ghosting.

You can find some informations about how it works in :
`<http://forum.doom9.org/showthread.php?s=&threadid=29829>`_


Current limitations or known problems
-------------------------------------

-   Work only with YUV2, CHECKED.
-   requires a Integer SSE capable CPU (no PII and K6-II), CHECKED.

WARNING : it's slow, I know it and I try to make it faster so don't rush me
about it, thanks in advance.


Credits
-------

| Thanks to
| bb for the original idea and a lot of tests
| iago, Koepi and TheReal for real full length movie (or capture) tests
| Tom Barry, Dividee and Sh0dan for their usefull technicals informations or
  ideas
| Ctrl-Alt-Suppr for a french tutorial
| Defiler for hosting Convolution3d
| all Convolution3D users

Vlad59 (babas.lucas@laposte.net)


License
-------

| Copyright (c) 2002 Sebastien LUCAS. All rights reserved.
| babas.lucas@laposte.net

This file is subject to the terms of the GNU General Public License as
published by the Free Software Foundation. A copy of this license is included
with this software distribution in the file COPYING. If you do not have a
copy, you may obtain a copy by writing to the Free Software Foundation, 675
Mass Ave, Cambridge, MA 02139, USA.

This software is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details


$Date: 2004/08/13 21:57:25 $

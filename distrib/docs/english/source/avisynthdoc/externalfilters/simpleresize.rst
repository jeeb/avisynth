
SimpleResize
============


Abstract
--------

| **author:** Tom Barry
| **version:** 0.3.3.0
| **download:** `<http://mywebpages.comcast.net/trbarry/downloads.htm>`_
| **category:** Resizers
| **requirements:** YV12 & YUY2 Colorspace

--------


Description
-----------

SimpleResize is an Avisynth filter that will do a very simple 2 tap linear
interpolation. It is unfiltered which means it will not soften much.

``InterlacedResize`` & ``InterlacedWarpedResize`` are designed to work on
interlaced materiel (more at the end of the doc)

It's main advantage is that it will run somewhat faster than some of the
others.


To use it just
~~~~~~~~~~~~~~

In you Avisynth file use command similar to

::

    LoadPlugin("F:\SimpleResize\SimpleResize.dll")
    Avisource("D:\wherever\myfile.avi")
    SimpleResize(width, height)

Of course replace the file and directory names with your own and supply the
integer values for width & height.

``SimpleResize`` should run on all MMX machines. It has also has some added
(optional) code for SSE2 instructions for when it is running on a P4 and some
SSEMMX for P3 & Athlon, though it could maybe use some more. But sorry, no
plans for extra 3DNow code.


WarpedResize
~~~~~~~~~~~~

``WarpedResize`` is also included in the SimpleResize.dll. ``WarpedResize``
will do a non-linear stretch/squeeze in both the horizontal and vertical
dimensions. This can be useful when you want to change the aspect ratio of a
video clip and have it mostly distorted at the top, bottom, and side edges.
This is mostly experimental but I added it because it required few code
changes and almost Zero performance penalty. Use as:

::

    LoadPlugin("F:\SimpleResize\SimpleResize.dll")
    AviSource("D:\wherever\myfile.avi")
    WarpedResize(width, height, hWarp, vWarp)

where hWarp & vWarp are the horizontal and vertical warp factors. These are
real number, usually between .8 and 1.3 that determine how non-linear you
really want the output to be.

Values above 1.0 cause the output image to be stretched more in the middle
and squeezed at the edges. Values below 1.0 will do the reverse. Specifying
1.0 for either of them will do a linear resize in that dimension, just as you
would get using ``SimpleResize``. Values far from 1.0 will give some very
strange results. See the "Notes to Mathematicians" below.

One reason to use ``WarpedResize`` would be when you have a clip with a 16:9
aspect ratio and want to resize it for a 4:3 aspect ratio display without
either clipping off valuable info or having to display black bars. (or vice
versa)

An example image of using ``WarpedResize`` for this can be found (for now) at
`<http://www.trbarry.com/warptest.jpg>`_. This image was from a short HDTV digital
capture that was at a 1280x720 resolution, a 16:9 aspect ratio. It was
downsized and warped to a 640x480 4:3 aspect ratio using the following script
command:

::

    WarpedResize(640,480,1.15,0.95))

Also, for an example of a 4:3 capture warped to fit on a 16:9 screen see
`<http://www.trbarry.com/Warptest2.jpg>`_


Current limitations, for SimpleResize/WarpedResize
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

1) Only YUY2 color mode is currently supported. Add a ConvertToYuY2
   command first if needed.

2) The target width NO LONGER must be a multiple of 4 pixels.

3) It will run faster on SSE2 machines if the target width is a multiple
   of 8 pixels and if the data starts on an 8 pixel boundary. I don't know
   if prior Clip() commands affect this or not.

4) If anyone knows how to make a DirectShow filter out of this I'd sure
   like to have one. ;-)


Notes to Mathematicians: (and questions)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Imagine the screen was dimensions that went from -1.0 to 1.0. We'll only
consider the horizontal dimension for the moment and only the right hand half
of the screen. Assume we want to calculate the value of an output pixel at
location x, where 0 <= x <=1.

The output value will be the source value from the input screen with the same
dimensions, at location s. Right now I'm just calculating s = (1-w) * x^3 + w
* x, where w is the warp factor mentioned above (Later note: w = 2 - warp
factor, for compat with first release). This gets the job done and produces
smooth numbers from 0 to one, without too much distortion as long as w is
close to 1.0.

The same formula is reflected for the left half of the screen.

The warp equations are designed to:

* Always be rising but yield results from 0 to 1

* Have a first derivative that doesn't go to 0 or infinity, at least close to
  the center of the screen

* Have a curvature (absolute val of 2nd derivative) that is small in the
  center and smoothly rises towards the edges. We would like the curvature to
  be everywhere = 0 when the warp factor = 1

Now, as near as I can tell the curvature is more or less just the absolute
value of the second derivative. So if we wanted the curvature to be small
when x = 0 and to grow toward the edges, what could be a useful warp
function? The above function already represents a change since V 0.1 but I'm
still not so sure it's the best.

It is easy to drop in another warp function. And there is no performance
penalty either way because it's just calculated and tabled at startup. After
that it runs at the same speed as SimpleResize.

Anyone have any ideas? (Anybody care about this part?)


InterlacedResize and InterlacedWarpedResize
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

If you want to resize an interlace source you can instead use:

``InterlacedResize`` (width, height)

or

``InterlacedWarpedResize`` (width, height, hWarp, vWarp)

These have the same paramaters as before but are designed to operate on
interlaced source, without either blending even/odd data or messing up the
coordinates because of the even/odd line offsets. Theoretically these can
lose a small amount of vertical detail or confuse a subsequent deinterlace or
IVTC function but so far in my own testing I have not found it to be a
problem.

So if you intend to keep your video in interlaced form, certainly use these.
And if you are downsizing you may find that doing the InterlacedResize first
before a more costly deinterlacing step can give you a small performance
advantage on other material. But this is still experimental. YMMV.

$Date: 2005/06/09 20:43:30 $

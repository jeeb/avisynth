
TComb
=====


Abstract
--------

| **author:** tritical
| **version:** 0.9.3
| **download:** `<http://bengal.missouri.edu/~kes25c/>`_
| **category:** Broadcast Video Plugins
| **requirements:**

-   YV12 & YUY2 Colorspace
-   NTSC

**license:** GPL

--------


Description
-----------

TComb is a temporal comb filter (it reduces cross-luminance (rainbowing) and
cross-chrominance (dot crawl) artifacts in static areas of the picture). It
will ONLY work with NTSC material, and WILL NOT work with telecined material
where the rainbowing/dotcrawl was introduced prior to the telecine process!
In terms of what it does it is similar to guavacomb/dedot.


Syntax
------

``TComb`` (clip, int "hfThreshL", int "hfThreshC", int "fCorrL", int
"fCorrC", bool "edge", int "mode", int "map")


PARAMETERS
----------

*hfThreshL/hfThreshC* - (high frequency thresholds)

These settings are only used if "edge" is set to true. They are the
thresholds that are used to create edge maps in luma/chroma that are then
mapped from chroma->luma and luma->chroma to identify areas where cross-
luminance and cross-chrominance are likely to occur. hfThreshL is the
threshold used when detecting edges in luma and hfThreshC is the threshold
used to detect edges in chroma. The only real reason for using
"edge/hfThreshL/hfThreshC" is to reduce the risk of artifacts (i.e. limit
filtering to only high frequency areas). However, better results are usually
produced with "edge" set to false and artifacts are usually rare unless you
use high fCorrL/fCorrC settings.

**NOTE: edges are detected on a low pass filtered version of the input image
so lower thresholds should be used then on an ordinary edge map. To see what
areas are being detected use the "map" option.**

**NOTE: By default "edge" is set to false, so hfThreshL/hfThreshC are not used.**

- default:

  - hfThreshL -> 5 (int)
  - hfThreshC -> 4

*fCorrL/fCorrC* - (filtered correlation thresholds)

TComb determines whether or not to use filtered values based the correlation
of those filtered values over the length of the kernel. If all the values are
within fCorrL (for luma) or fCorrC (for chroma) then the filtered values will
be used. Larger values for fCorrL/fCorrC will mean  more pixels will be
replaced with filtered values (will be more effective at removing
rainbowing/dotcrawl), but it will create more artifacts. Smaller values will
produce less artifacts, but will be less effective in removing
rainbowing/dotcrawl. A good range of values is between 4 and 9.

- default:

  - fCorrL -> 7 (int)
  - fCorrC -> 7

*edge* - (limit filtering to high frequency areas)

If set to true, then filtering is only performed in edge regions identified
via the hfThreshL and hfThreshC parameters. If set to false, filtering is
performed on the entire image (much
faster, and usually better).

- default:

  - false (bool)

*mode* - (limit processing to luma or chroma only)

Controls whether both luma/chroma are processed or only one or the other.
Possible settings:

- 0 - process both luma/chroma
- 1 - process luma only
- 2 - process chroma only

- default:

  - 0 (int)

*map* -

Identifies areas of the frame that are detected as edges via
hfThreshL/hfThreshC (option 1) or
identifies pixels that are being replaced with filtered values (option 2).
Possible settings:

- 0 - no map
- 1 - pixels thare are detected as edges via hfThreshL/hfThreshC will be
  set to 255, pixels that aren't will be set to 0
- 2 - pixels that would be replaced with filtered values will be set to
  255, pixels that wouldn't will be set to 0

- default:

  - 0 (int)

+-----------------------------------------+
| Changelog                               |
+========+============+===================+
| v0.9.0 | 06/24/2005 | - Initial Release |
+--------+------------+-------------------+

$Date: 2005/07/10 16:11:01 $

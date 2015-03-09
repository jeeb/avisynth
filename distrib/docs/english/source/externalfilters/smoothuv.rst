
SmoothUV
========


Abstract
--------

| **author:** Kurosu
| **version:** 1.4.0
| **download:** `<http://www.avisynth.org/warpenterprises/>`_
| **category:** Spatial Smoothers
| **requirements:**

- YV12 Colorspace

--------


Functions
---------

- SmoothUV
- SSHiQ


Syntax
------

1) ``SSHiQ`` (clip, int rY, int rC, int tY, int tC, int str, bool HQY,
   bool HQC, bool field)

-   rY and rC are respectively the radii for luminance and chrominance
    (how far from the current pixel neighbours are used). The greater the
    more smooth.
-   tY and tC are thresholds (details sensibility). The greater the more
    smooth (not exactly the same as fpr other SSHiQ filters).
-   str is the strength (only used in HiQ mode), ie how much of the
    smoothed version should be used at best for calculating the new pixel
-   HQY and HQC tell whether to use the HiQ mode, where the edge
    information automatically reduce the strength value.

(HQ == 0) ? smoothed_pixel : ( (strength-edge)*smoothed_pixel + (256-(strength-edge))*pixel_in )/256

-   field allows you to process independently fields (risks of worse
    blurring/color bleeding)

| 2) SmoothUV (used to reduce rainbows, as done by smartsmoothIQ)
| ``SmoothUV`` (clip, int radius, int threshold, bool field)

-   radius is the range of operation
-   threshold is the one used in smartsmoothIQ
-   field is the same as above

This filter doesn't process the luma at all, and can be seen as a shorter
version of SSHiQ(rY, radius, -1, threshold, 255, false, false, field)

$Date: 2004/08/17 20:31:19 $

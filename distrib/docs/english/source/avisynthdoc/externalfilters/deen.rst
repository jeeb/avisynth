
Deen
====


Abstract
--------

| **author:** MarcFD
| **version:** 1.0 beta 1
| **download:** `<http://ziquash.chez.tiscali.fr/>`_
| **category:** Spatio-Temporal Smoothers
| **requirements:**

-   YV12 Colorspace
-   ISSE support

**license:** closed source

--------


Description
-----------

deen is a set of assembly-optimised denoisers.

usage
~~~~~

``deen`` (clip, string "mode", int "rad", int "thrY", int "thrUV", int
"tthY", int "tthUV", float "min", float "scd", string "fcf")

| *mode* = "c3d":
| "c2d", "c3d", "w2d", "w3d", "a2d", "a3d"
| 2d = spatial / 3d = spatial-temporel; default : "c3d"

The c mode is the same as the w mode without reduction. In practice, the c
mode is less aggressive on textures, which is in theory better for deringing
and movies. See explanation of min parameter for further details.

*rad* = 1: radius (1-4 for 3d / 1-7 for 2d)

*thrY* = 7, thrUV = 9: spatial thresholds

*tthY* = 4, tthUV = 6: temporal thresholds (only for 3d)

*min* = 0.5: min parameter (float) 1.0 mean no reduction, 0.0 means maximum
reduction
in "w" mode it reduces the weight of pixels depending on how far from the
center they are.
in "a" mode it reduces the threshold for a pixel depending on how far from
the center it is.

*scd* = 9: when triggered deen falls back in an equivalent 2d mode. It triggers
when movement metrics is beyond the parameter (it's a threshold). For scd=-1
deen outputs the movement metrics in debugview (which only works for 3d
modes).

*fcf* = "" (empty string): full control file (there's a demo in the zip), thus
disabling fcf overrides parameters with an external (.fcf for instance) file.

$Date: 2005/10/01 23:44:54 $

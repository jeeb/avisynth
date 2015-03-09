
ReverseFieldDominance
=====================


Abstract
--------

| **author:** Simon Walters
| **version:** 1.2
| **download:** `<http://www.geocities.com/siwalters_uk/fnews.html>`_
| **category:** Misc Plugins
| **requirements:** YUY2 & RGB Colorspace

--------


Description
-----------

This filter is intended to reverse the field dominance (I refer the read to
`here`_ for the best explanation of field order on the web) of PAL DV video.

My Matrox G400 TV-out outputs the top field of an avi first and then the
bottom field.

But PAL DV is always stored as bottom field first and this filter reverses
the field order by simply moving each line up (or down) by one line and
duplicating the bottom(or top) line.

Check out `this`_ thread in Doom9 to see how it came about and to get Donald
Graft's VirtualDub version.


Syntax
------

``ReverseFieldDominance`` (bool shiftup = true)

+-------------------------------------------------------------------------------------------------------------------------------+
| Version History                                                                                                               |
+======+=================+======================================================================================================+
| v1.2 | 3rd August 2004 | Correctly shifts RGB material in the right direction - thanks to Wilbert for pointing out the error. |
+------+-----------------+------------------------------------------------------------------------------------------------------+
| v1.1 | 25th Feb 2003   | Adds the shiftup parameter                                                                           |
+------+-----------------+------------------------------------------------------------------------------------------------------+
| v1   | 23rd Feb 2003   | 1st attempt - works in RGB and YUY2 colourspaces only.                                               |
+------+-----------------+------------------------------------------------------------------------------------------------------+

$Date: 2005/06/19 07:23:09 $

.. _here: http://www.lurkertech.com/lg/dominance.html
.. _this: http://forum.doom9.org/showthread.php?s=&postid=268353#post268353

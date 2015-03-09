
Filters with multiple input clips
=================================

There are some functions which combine two or more clips in different ways.
How the video content is calculated is described for each function, but here
is a summary which properties the result clip will have.

The input clips must always have the same color format and - with the
exception of `Layer`_ and `Overlay`_ - the same dimensions.

+--------------------------------------------------------+----------------------+----------------------+------------------------+---------------------+
| filter                                                 | framerate            | framecount           | audio content          | audio sampling rate |
+========================================================+======================+======================+========================+=====================+
| `AlignedSplice`_, `UnalignedSplice`_                   | first clip           | sum of all clips     | see filter description | first clip          |
+--------------------------------------------------------+                      +----------------------+                        |                     |
| `Dissolve`_                                            |                      | sum of all clips     |                        |                     |
|                                                        |                      | minus the overlap    |                        |                     |
+--------------------------------------------------------+                      +----------------------+------------------------+                     |
| `Merge`_, `MergeLuma`_, `MergeChroma`_, `Merge(A)RGB`_ |                      | first clip `1`:sup:  | first clip             |                     |
+--------------------------------------------------------+                      |                      |                        |                     |
| `Layer`_                                               |                      |                      |                        |                     |
+--------------------------------------------------------+                      +----------------------+                        |                     |
| `Subtract`_                                            |                      | longer clip `2`:sup: |                        |                     |
+--------------------------------------------------------+                      |                      |                        |                     |
| `StackHorizontal`_, `StackVertical`_                   |                      |                      |                        |                     |
+--------------------------------------------------------+----------------------+----------------------+                        |                     |
| `Interleave`_                                          || (fps of first clip) || N x frame-count     |                        |                     |
|                                                        || x                   || of longer clip      |                        |                     |
|                                                        || (number of clips)   || `2`:sup:            |                        |                     |
+--------------------------------------------------------+----------------------+----------------------+------------------------+---------------------+

| `1`:sup: (the last frame of the shorter clip is repeated until the end of the clip)
| `2`:sup: (the last frame of the shorter clip is repeated until the end of the clip)

As you can see the functions are not completely symmetric but take some attributes from the FIRST clip.

$Date: 2008/07/19 15:17:14 $

.. _Layer: corefilters/layer.rst
.. _Overlay: corefilters/Overlay.rst
.. _AlignedSplice: corefilters/splice.rst
.. _UnalignedSplice: corefilters/splice.rst
.. _Dissolve: corefilters/dissolve.rst
.. _Merge: corefilters/merge.rst
.. _MergeLuma: corefilters/merge.rst
.. _MergeChroma: corefilters/merge.rst
.. _Merge(A)RGB: corefilters/mergergb.rst
.. _Subtract: corefilters/subtract.rst
.. _StackHorizontal: corefilters/stack.rst
.. _StackVertical: corefilters/stack.rst
.. _Interleave: corefilters/Interleave.rst

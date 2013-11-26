
AviSynth Syntax - Internal functions
====================================

In addition to `internal filters`_ AviSynth has a fairly large number of
other (non-clip) internal functions. The input or/and output of these
functions are not clips, but some other variables which can be used in a
script. They are roughly classified as follows:

-   `Boolean functions`_

They return true or false, if the condition that they test holds or not,
respectively.

-   `Control functions`_

They facilitate flow of control (loading of scripts, arguments checks, global
settings adjustment, etc.).

-   `Conversion functions`_

They convert between different types.

-   `Numeric functions`_

They provide common mathematical operations on numeric variables.

-   `Runtime functions`_

These are internal functions which are evaluated at every frame. They can be
used inside the scripts passed to runtime filters (`ConditionalFilter`_,
`ScriptClip`_, `FrameEvaluate`_) to return information for a frame.

-   `Script functions`_

They provide AviSynth script information.

-   `String functions`_

They provide common operations on string variables.

-   `Version functions`_

They provide AviSynth version information.

$Date: 2011/01/16 12:24:09 $

.. _internal filters: corefilters.rst
.. _Boolean functions: syntax_internal_functions_boolean.rst
.. _Control functions: syntax_internal_functions_control.rst
.. _Conversion functions: syntax_internal_functions_conversion.rst
.. _Numeric functions: syntax_internal_functions_numeric.rst
.. _Runtime functions: syntax_internal_functions_runtime.rst
.. _ConditionalFilter: corefilters/conditionalfilter.rst
.. _ScriptClip: corefilters/conditionalfilter.rst
.. _FrameEvaluate: corefilters/conditionalfilter.rst
.. _Script functions: syntax_internal_functions_script.rst
.. _String functions: syntax_internal_functions_string.rst
.. _Version functions: syntax_internal_functions_version.rst

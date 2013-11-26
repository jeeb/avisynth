
The script execution model
==========================

This section is a walkthrough to the internals of the AviSynth script engine.
Its aim is to provide a better understanding of how AviSynth transforms
script commands to actual video frames and help a user that has already
grasped the basics of AviSynth scripting to start writing better and
optimised scripts.

The following subsections present the various parts that when combined
together form what can be called the AviSynth's "script execution model":

-   `Sequence of events`_

A detailed description of the sequence of events that occur when you execute
(ie load and render to your favorite encoder) an AviSynth script.

-   `The (implicit) filter graph`_

A glance at the basic internal data structure that holds the representation
of a parsed AviSynth script.

-   `The fetching of frames (from bottom to top)`_

How the AviSynth engine requests frames from filters.

-   `Scope and lifetime of variables`_

The interplay of variables' scope and lifetime with the other features of
AviSynth `syntax`_.

-   `Evaluation of runtime scripts`_

The details of runtime scripts' evaluation.

-   `Performance considerations`_

Various performance-related issues and advice on how to optimise your
AviSynth scripts and configuration.

--------

Back to `scripting reference`_.

$Date: 2008/04/20 19:07:33 $

.. _Sequence of events:
    script_ref_execution_model_sequence_events.rst
.. _The (implicit) filter graph:
    script_ref_execution_model_filter_graph.rst
.. _The fetching of frames (from bottom to top):
    script_ref_execution_model_fetching_frames.rst
.. _Scope and lifetime of variables:
    script_ref_execution_model_lifetime_variables.rst
.. _syntax: http://avisynth.org/mediawiki/AviSynth_Syntax
.. _Evaluation of runtime scripts:
    script_ref_execution_model_eval_scripts.rst
.. _Performance considerations:
    script_ref_execution_model_perf_cons.rst
.. _scripting reference:
    http://avisynth.org/mediawiki/Scripting_reference

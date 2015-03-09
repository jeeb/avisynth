
Scripting reference
===================

This section contains information that goes beyond scripting basics. It
presents the internals of AviSynth script processing, their influence on
script performance as well as advanced techniques for using productively the
features of the AviSynth script language. Before reading further it is
recommended that you first become familiar with basic concepts of the
`AviSynth syntax`_.

-   `The script execution model`_

The steps behind the scenes from the script to the final video clip output.
The filter graph. Scope and lifetime of variables. Evaluation of runtime
scripts.

-   `User functions`_

How to effectively write and invoke user functions; common pitfalls to avoid;
ways to organise your function collection and create libraries of functions,
and many more.

-   `Block statements`_

Techniques and coding idioms for creating blocks of AviSynth script
statements.

-   `Arrays`_

Using arrays and array operators for manipulating collections of data in a
single step.

-   `Runtime environment`_

How to unravel the power of runtime filters and create complex runtime
scripts that can perform interesting (and memory/speed efficient)
editing/processing operations and effects.

$Date: 2008/04/20 19:07:33 $

.. _AviSynth syntax: http://avisynth.org/mediawiki/AviSynth_Syntax
.. _The script execution model: script_ref_execution_model.rst
.. _User functions: script_ref_user_functions.rst
.. _Block statements: script_ref_block_statements.rst
.. _Arrays: script_ref_arrays.rst
.. _Runtime environment: syntax_runtime_environment.rst

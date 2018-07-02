AviSynth+
=========

**AviSynth+** is an improved version of the [AviSynth frameserver](http://avisynth.nl/index.php/Main_Page), with improved
features and developer friendliness. 

Visit our [forum thread](http://forum.doom9.org/showthread.php?t=168856) for compilation instructions and support.
The original [homepage of the project](http://avs-plus.net) is not functional at the moment (June, 2018).
Development branch: https://github.com/pinterf/AviSynthPlus/tree/MT

Building the documentation:
---------------------------
(note, that the bundled documentation was not following the numerous changes in Avisynth+. 
You can always check the online documentation at http://avisynth.nl/index.php/Main_Page)

AviSynth+'s documentation can be generated into HTML by using Sphinx.

### Set-up:

Make sure that Sphinx is installed. This requires that Python is already
installed and the pip tool is available.  Sphinx 1.3 is the recommended
version.

>pip install sphinx

For various Linux distributions, a Sphinx package should be available
in the distro's repositories.  Often under the name 'python-sphinx'
(as it is in Ubuntu's repositories).

There is currently a fallback so that distros that only provide
Sphinx 1.2 can still build the documentation.  It will look
different than when built with Sphinx 1.3, because the theme
used with Sphinx 1.3 (bizstyle) had not yet been added to the main
Sphinx package.

### Building the documentation

Once Sphinx is installed, we can build the documentation.

>cd distrib/docs/english

>make html


Libav users:
------------

CMake isn't really suited to doing header-only installs, so a GNUmakefile is
provided specifically for this purpose.

### Using GNUmakefile:

#### To install:

>make install

#### To install to a non-standard location:

>make install PREFIX=/path/to/location

#### To uninstall:

>make uninstall PREFIX=/path/to/location
AviSynth+
=========

**AviSynth+** is an improved version of the [AviSynth frameserver](http://avisynth.nl/index.php/Main_Page), with improved
features and developer friendliness. 

Visit our [homepage](http://avs-plus.net) for more information and binaries, or our
[forum thread](http://forum.doom9.org/showthread.php?t=168856) for compilation instructions and support.


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

Using AviSynth+ on POSIX systems
================================

As of version 3.5, AviSynth+ can now be built and used natively
on Linux, macOS, and BSD.

.. toctree::
    :maxdepth: 3

.. contents:: Table of contents



AviSynth+ prerequisites
-----------------------

Depending on your OS or distribution, the commands to fetch
the necessary prerequisites for building AviSynth+ differ.

At a bare minimum:

* CMake 3.8 or higher.
* GCC 8 or higher.

Linux
^^^^^

Ubuntu 19.10 or higher
~~~~~~~~~~~~~~~~~~~~~~

::

    sudo apt-get install build-essential cmake git ninja-build checkinstall


::

    git clone git://github.com/AviSynth/AviSynthPlus.git && \
    cd AviSynthPlus && \
    mkdir avisynth-build && \
    cd avisynth-build && \

    cmake ../ -G Ninja && \
    ninja && \
        sudo checkinstall --pkgname=avisynth --pkgversion="$(grep -r \
        Version avs_core/avisynth.pc | cut -f2 -d " ")-$(date --rfc-3339=date | \
        sed 's/-//g')-git" --backup=no --deldoc=yes --delspec=yes --deldesc=yes \
        --strip=yes --stripso=yes --addso=yes --fstrans=no --default ninja install


Ubuntu 18.04 LTS
~~~~~~~~~~~~~~~~

18.04 ships with GCC 7, which is not sufficient to
build AviSynth+.  Adding the following repository will allow
installing GCC 9:

::

    sudo add-apt-repository ppa:ubuntu-toolchain-r/test
    sudo apt-get update
    sudo apt-get install build-essential cmake git ninja-build gcc-9 g++-9 checkinstall


::

    git clone git://github.com/AviSynth/AviSynthPlus.git && \
    cd AviSynthPlus && \
    mkdir avisynth-build && \
    cd avisynth-build && \

    CC=gcc-9 CXX=gcc-9 LD=gcc-9 cmake ../ -G Ninja && \
    ninja && \
        sudo checkinstall --pkgname=avisynth --pkgversion="$(grep -r \
        Version avs_core/avisynth.pc | cut -f2 -d " ")-$(date --rfc-3339=date | \
        sed 's/-//g')-git" --backup=no --deldoc=yes --delspec=yes --deldesc=yes \
        --strip=yes --stripso=yes --addso=yes --fstrans=no --default ninja install


Distributions without checkinstall
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Not all Linux distributions have checkinstall in their repositories, either due to
a lack of checkinstall working with their package management system or simply due
to omission.  In these cases, the install step is a little different:

::

    sudo ninja install
    sudo ldconfig


macOS
^^^^^

| Requires Homebrew:
| `<https://brew.sh/>`_

::

    brew install cmake ninja gcc


GCC isn't strictly necessary for AviSynth+, but it can side-step
the need to use `an external implementation`_ on High Sierra and
Mojave.


10.13 High Sierra and 10.14 Mojave
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Apple's libc++ doesn't support the C++17 filesystem functionality
on either of these versions of macOS, so we have to resort to
using `an external implementation`_ as a submodule.

::

    git clone --recursive git://github.com/AviSynth/AviSynthPlus.git && \
    cd AviSynthPlus && \
    mkdir avisynth-build && \
    cd avisynth-build

    cmake ../ -G Ninja && \
    ninja && \
    sudo ninja install


10.15 Catalina
~~~~~~~~~~~~~~

C++17 filesystem support is available on Catalina, so it can
be built with the default Clang installation.  GCC is potentially
useful later on, though, so install that too.

::

    cmake ../ -G Ninja && \
    ninja && \
    sudo ninja install


FreeBSD
^^^^^^^

Tested on FreeBSD 12.1.

::

    pkg install cmake git gmake ninja

    git clone git://github.com/AviSynth/AviSynthPlus.git && \
    cd AviSynthPlus && \
    mkdir avisynth-build && \
    cd avisynth-build


Building AviSynth+ (GNU Make)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

::

    cmake ../ && \
    gmake -j$(nproc) && \
    gmake install


Building AviSynth+ (Ninja)
~~~~~~~~~~~~~~~~~~~~~~~~~~

::

    cmake ../ -G Ninja && \
    ninja && \
    sudo ninja install


FFmpeg support
--------------

On all of these OSes, AviSynth+ can interface with FFmpeg.
This change was applied to the FFmpeg git master branch on
2020-04-05.

To compile a basic build of FFmpeg that supports
AviSynth+, the following steps will suffice:

Prerequisites
^^^^^^^^^^^^^

Linux
~~~~~

Ubuntu
......

First, enable the Sources repository by either enabling it
using the Software Sources dialog or by uncommenting the
right lines in /etc/apt/sources.list.

::

    sudo apt-get build-dep ffmpeg
    sudo apt-get install nasm libsdl2-dev


macOS
~~~~~

Homebrew prerequisites:

::

    brew install xz sdl2 pkg-config nasm


FreeBSD
~~~~~~~

::

    pkg install nasm sdl2


Building FFmpeg
^^^^^^^^^^^^^^^

::

    git clone git://git.ffmpeg.org/ffmpeg.git
    cd ffmpeg


Linux
~~~~~

Ubuntu
......

::

        ./configure --prefix=$HOME/ffavx_build --enable-gpl --enable-version3 \
        --disable-doc --disable-debug --enable-pic --enable-avisynth && \
    make -j$(nproc) && \
    make install


Installing FFmpeg to the system can be done by leaving out the `--prefix`
option and then using the following checkinstall command:

::

    sudo checkinstall --pkgname=ffmpeg --pkgversion="7:$(git rev-list \
    --count HEAD)-g$(git rev-parse --short HEAD)" --backup=no --deldoc=yes \
    --delspec=yes --deldesc=yes --strip=yes --stripso=yes --addso=yes \
    --fstrans=no --default


macOS
~~~~~

::

        ./configure --prefix=$HOME/ffavx_build --enable-gpl --enable-version3 --disable-doc \
        --disable-debug --enable-avisynth
    make -j$(nproc)
    make install

On Catalina, `--extra-cflags="-fno-stack-check"` is necessary when using AppleClang as the compiler.

FreeBSD
~~~~~~~

::

        ./configure --prefix=$HOME/ffavx_build --enable-gpl --enable-version3 --disable-doc \
        --disable-debug --enable-pic --enable-avisynth --cc=cc
    gmake -j$(nproc)
    gmake install


Testing the installation
------------------------

FFplay can be used to preview scripts in a pinch; if mpv or VLC is built against the patched
version of FFmpeg, those can be used to play back scripts in a more comfortable player
experience.

The easiest two scripts to test the installation are Version or Colorbars/ColorbarsHD.

::

    Version()


::

    Colorbars() # or ColorbarsHD()


And running this script in the test build of FFmpeg:

::

    cd ~/ffavx_build/bin
    [create the script in this directory, for ease of testing]

    # to play the script:
    ./ffplay -i test.avs

    # to convert as usual:
    ./ffmpeg -i test.avs [encoding options]


Loading actual video sources will require a source filter.  FFMS2 doesn't require any porting
to these OSes, making it the most straightforward option at the moment.


Building FFMS2
--------------

FFMS2 doesn't require any additional prerequisites, so it can be
built straight away.

::

    git clone git://github.com/ffms/ffms2.git && \
    cd ffms2


Linux
^^^^^

Ubuntu
~~~~~~

::

        PKG_CONFIG_PATH=$HOME/ffavx_build/lib/pkgconfig \
        CPPFLAGS="-I/usr/local/include/avisynth" \
        ./autogen.sh --enable-shared --enable-avisynth && \
    make -j$(nproc) && \
        sudo checkinstall --pkgname=ffms2 --pkgversion="1:$(./version.sh)-git" \
        --backup=no --deldoc=yes --delspec=yes --deldesc=yes --strip=yes --stripso=yes \
        --addso=yes --fstrans=no --default


macOS
^^^^^

::

    brew install autoconf automake libtool m4

        CC=gcc-9 CXX=g++-9 LD=gcc-9 PKG_CONFIG_PATH=$HOME/ffavx_build/lib/pkgconfig \
        CPPFLAGS="-I/usr/local/include/avisynth" \
        ./autogen.sh --enable-shared --enable-avisynth && \
    make -j$(nproc) && \
    sudo make install


FreeBSD
^^^^^^^

::

    pkg install autoconf automake libtool m4

        PKG_CONFIG_PATH=$HOME/ffavx_build/lib/pkgconfig \
        CPPFLAGS="-I/usr/local/include/avisynth" \
        ./autogen.sh --enable-shared --enable-avisynth && \
    gmake -j$(nproc) && \
    gmake install


Plugin autoloading
------------------

AviSynth+ will use two directories for autoloading:
the `avisynth/` subdirectory where libavisynth.so was installed,
and `$HOME/.avisynth`.  The latter of which can hold
plugins (and symlinks to plugins) or AVSI files without needing
root permissions.

On FreeBSD, procfs needs to be mounted first in order for
autoloading to function.


Back to the :doc:`main page <../../index>`

$ Date: 2020-05-20 12:50:44-04:00 $

.. _an external implementation: https://github.com/gulrak/filesystem

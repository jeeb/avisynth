
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

    cmake ../ -G Ninja -DCMAKE_CXX_FLAGS="-fpermissive" && \
    ninja && \
        sudo checkinstall --pkgname=avisynth --pkgversion="$(grep -r \
        Version avs_core/avisynth.pc | cut -f2 -d " ")-$(date --rfc-3339=date | \
        sed 's/-//g')-git" --backup=no --deldoc=yes --delspec=yes --deldesc=yes \
        --strip=yes --fstrans=no --default ninja install


Ubuntu 18.04 LTS
~~~~~~~~~~~~~~~~

18.04 ships with GCC 7, which is not sufficient to
build AviSynth+.  Adding the following repository will allow
installing GCC 9:

::

    sudo add-apt-repository ppa:ubuntu-toolchain-r/test
    sudo apt-get update
    sudo apt-get install build-essential cmake git ninja-build gcc-9 g++-9


::

    git clone git://github.com/AviSynth/AviSynthPlus.git && \
    cd AviSynthPlus && \
    mkdir avisynth-build && \
    cd avisynth-build && \

    CC=gcc-9 CXX=gcc-9 LD=gcc-9 cmake ../ -G Ninja -DCMAKE_CXX_FLAGS="-fpermissive" && \
    ninja && \
        sudo checkinstall --pkgname=avisynth --pkgversion="$(grep -r \
        Version avs_core/avisynth.pc | cut -f2 -d " ")-$(date --rfc-3339=date | \
        sed 's/-//g')-git" --backup=no --deldoc=yes --delspec=yes --deldesc=yes \
        --strip=yes --fstrans=no --default ninja install


macOS
^^^^^

Tested on both 15.3 High Sierra and 15.4 Mojave.

| Requires Homebrew:
| `<https://brew.sh/>`_

::

    brew install cmake ninja gcc


Building AviSynth+
~~~~~~~~~~~~~~~~~~

::

    git clone git://github.com/AviSynth/AviSynthPlus.git && \
    cd AviSynthPlus && \
    mkdir avisynth-build && \
    cd avisynth-build && \

    CC=gcc-9 CXX=g++-9 LD=gcc-9 cmake ../ -G Ninja -DCMAKE_CXX_FLAGS="-fpermissive" && \
    ninja && \
    sudo ninja install


Ninja is preferred, since it obviates the need to use
CMAKE_SHARED_LINKER_FLAGS to link in libstdc++ (also true
in BSD, but getting GNU make working on Mac seems to be
more complicated than installing gmake on BSD).


FreeBSD
^^^^^^^

Tested on FreeBSD 12.1.

::

    pkg install cmake git gmake ninja gcc


Building AviSynth+ (GNU Make)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

::

    git clone git://github.com/AviSynth/AviSynthPlus.git && \
    cd AviSynthPlus && \
    mkdir avisynth-build && \
    cd avisynth-build && \

    CC=gcc CXX=g++ LD=gcc cmake ../ -DCMAKE_CXX_FLAGS="-fpermissive" -DCMAKE_SHARED_LINKER_FLAGS="-lstdc++" && \
    gmake -j$(nproc) && \
    gmake install


Building AviSynth+ (Ninja)
~~~~~~~~~~~~~~~~~~~~~~~~~~

::

    git clone git://github.com/AviSynth/AviSynthPlus.git && \
    cd AviSynthPlus && \
    mkdir avisynth-build && \
    cd avisynth-build && \

    CC=gcc CXX=g++ LD=gcc cmake ../ -G Ninja -DCMAKE_CXX_FLAGS="-fpermissive" && \
    ninja && \
    sudo ninja install


FFmpeg support
--------------

On all of these OSes, AviSynth+ can interface with FFmpeg.
As of this time of writing (2020-03-02), FFmpeg uses
AvxSynth to support non-Windows OSes, so to support
AviSynth+, a patched version of FFmpeg is necessary.

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

    git clone -b avsplus_linux git://github.com/qyot27/FFmpeg.git
    cd FFmpeg


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
built straight away.  As of the current time of writing (2020-03-02)
upstream FFMS2 hasn't been patched to allow building the AviSynth
plugin on non-Windows OSes, so the C-plugin repository will have
to suffice for now:

::

    git clone -b patches_plusvp9av1 git://github.com/qyot27/ffms2_cplugin.git
    cd ffms2_cplugin


Linux
^^^^^

Ubuntu
~~~~~~

::

        PKG_CONFIG_PATH=$HOME/ffavx_build/lib/pkgconfig ./configure --enable-shared \
        --enable-pic --enable-avisynth-cpp --enable-vapoursynth
    make -j$(nproc)
        sudo checkinstall --pkgname=ffms2 --pkgversion="1:$(./version.sh)-git" \
        --backup=no --deldoc=yes --delspec=yes --deldesc=yes --strip=yes --stripso=yes \
        --addso=yes --fstrans=no --default


macOS
^^^^^

::

        CC=gcc CXX=g++ LD=gcc PKG_CONFIG_PATH=$HOME/ffavx_build/lib/pkgconfig \
        ./configure --enable-shared --enable-pic --enable-avisynth-cpp --enable-vapoursynth
    make -j$(nproc)
    sudo make install


FreeBSD
^^^^^^^

::

        CC=gcc CXX=g++ LD=gcc PKG_CONFIG_PATH=$HOME/ffavx_build/lib/pkgconfig \
        ./configure --enable-shared --enable-pic --enable-avisynth-cpp --enable-vapoursynth
    gmake -j$(nproc)
    gmake install


Using FFMS2 as a source filter
------------------------------

AddAutoloadDir can be used to set the directory to load plugins
from.

::

    AddAutoloadDir("/usr/local/lib/avisynth")
    FFmpegSource2("source") # or any additional options as usual


If AddAutoloadDir doesn't work, try to fall back to loading the
plugin manually:

::

    LoadPlugin("/usr/local/lib/avisynth/libffms2.so")
    FFmpegSource2("source") # or any additional options as usual


Back to the :doc:`main page <../../index>`

$ Date: 2020-03-04 15:09:23-05:00 $

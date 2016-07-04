
Compiling AviSynth+
===================

This guide uses a command line-based compilation methodology, because
it's easier to provide direct instructions for this that can just be copy/pasted.

`MSys2 <https://msys2.github.io/>`_ and `7zip <http://www.7-zip.org/>`_ should
already be installed, and msys2's bin directory should have been added to Windows'
%PATH% variable.

.. toctree::
    :maxdepth: 3

.. contents:: Table of contents



AviSynth+ prerequisites
-----------------------

AviSynth+ requires Visual Studio [Express] 2012 SP1 or higher.  This guide will
use Visual Studio 2013, since Wine currently does not binaries built with VS2015.

| Download and install Visual Studio Express 2013 for Windows Desktop:
| `<https://www.microsoft.com/en-us/download/details.aspx?id=48131>`_

| Install the latest version of CMake:
| `<http://www.cmake.org/cmake/resources/software.html>`_

Add CMake's bin directory to the system %PATH% manually if the installer won't.
Also add 7zip and upx to the %PATH%.


DirectShowSource Prerequisites
------------------------------

DirectShowSource requires a couple of big pieces here that building the AviSynth+
core itself does not. It's not a requirement to build DirectShowSource, especially
with the options of using either FFmpegSource2 or LSMASHSource, but the guide
wouldn't be complete if I didn't cover it.


C++ Base Classes library
~~~~~~~~~~~~~~~~~~~~~~~~

DirectShowSource requires strmbase.lib, the C++ Base Classes library, which for some
reason isn't included in a standard install of Visual Studio.  The source code for
the library is provided with the Windows SDK, and requires the user to build it first.

| Download the Windows SDK 7.1:
| `<http://www.microsoft.com/en-US/download/details.aspx?Id=8442>`_

| Download the following ISO for 32-bit Windows installations:
| GRMSDK_EN_DVD.iso

| Download the following ISO for 64-bit Windows installations:
| GRMSDKX_EN_DVD.iso

The ISO you download is based on the version of Windows you're actually running,
*not* on the Windows installs you're targetting.  Both ISOs include the correct
tools to build for either 32-bit or 64-bit targets.

| Verify the 32-bit ISO against CRC32 or SHA1:
| CRC#: 0xBD8F1237
| SHA1: 0xCDE254E83677C34C8FD509D6B733C32002FE3572

| Verify the 64-bit ISO against CRC32 or SHA1:
| CRC#: 0x04F59E55
| SHA1: 0x9203529F5F70D556A60C37F118A95214E6D10B5A

For convenience (and on computers without an optical drive), you can use Pismo File Mount
(if you've already got it installed for AVFS) to mount the ISO. Then just launch
setup.exe and follow the wizard.

Install only the Samples, uncheck everything else.

| Open VS2013 Express, and open the .sln file in the 7.1 SDK, at
| ``C:\Program Files\Microsoft SDKs\Windows\v7.1\Samples\multimedia\directshow\baseclasses``

Allow VS2013 Express to convert the project, switch it over to ``Release``, and enter the
project Properties by right-clicking on the solution name and selecting ``Properties``.

Select the ``Visual Studio 12 (v120_xp)`` option on the main Properties page under ``Toolset``,
and on the ``C/C++ Code Generation`` page select *Disabled* or *SSE* from the
``Enhanced Instruction Set`` option (IMO, it's safer to disable it for system support
libraries like strmbase.lib), and finally, exit back to the main screen.

Now select ``Build``. That's it.

For 64-bit, change to ``Release x64`` and ``Build``. The SSE2 note isn't relevant here, since
64-bit CPUs are required to have SSE2 support.


DirectX SDK
~~~~~~~~~~~

| Download the DirectX SDK (June 2010):
| `<https://www.microsoft.com/en-us/download/details.aspx?id=6812>`_

Only install the headers and libs from it. Uncheck everything else. Unlike the
strmbase.lib stuff, there's no post-install stuff required.


Miscellaneous
~~~~~~~~~~~~~

To make the AviSynth+ build instructions more concise, we'll set a couple of environment
variables.  After starting msys2, open the file /etc/profile in Wordpad:
::

    write /etc/profile

and copy the following three lines into it somewhere:
::

    export STRMBASELIB="C:/Program Files/Microsoft SDKs/Windows/v7.1/Samples/multimedia/directshow/baseclasses/Release/strmbase.lib"
    export STRMBASELIB64="C:/Program Files/Microsoft SDKs/Windows/v7.1/Samples/multimedia/directshow/baseclasses/x64/Release/strmbase.lib"
    export DIRECTXSDK="C:/Program Files/Microsoft DirectX SDK (June 2010)/Include"

(64-bit Windows users should use ``Program Files (x86)``, but you probably already knew that ;P)

Thankfully, all of this setup only needs to be done once.


Building AviSynth+
------------------

Start the Visual Studio 2013 Command Prompt.

You can use Visual Studio's stuff from MSys by launching MSys from the Visual Studio
Command Prompt. So type 'msys' and hit Enter.

Note: in the instructions below, the ``\`` character means the command spans more than
one line.  Make sure to copy/paste all of the lines in the command.

Download the AviSynth+ source:
::

    git clone git://github.com/AviSynth/AviSynthPlus.git && \
    cd AviSynthPlus

Set up the packaging directory for later:
::

    AVSDIRNAME=avisynth+_r$(git rev-list --count HEAD)-g$(git rev-parse --short HEAD)-$(date --rfc-3339=date | sed 's/-//g') && \
    cd .. && \
    mkdir -p avisynth_build $AVSDIRNAME/32bit/dev $AVSDIRNAME/64bit/dev && \
    cd avisynth_build

Now, we can build AviSynth+.


Using MSBuild
~~~~~~~~~~~~~

For 32-bit:
::

    cmake ../AviSynthPlus -DDSHOWSRC_BASECLASSES_LIB="$STRMBASELIB" \
    -DDSHOWSRC_DX_INCLUDE_PATH="$DIRECTXSDK" && \

    cmake --build . --config Release


Copy the .dlls to the packaging directory:
::

    cp Output/AviSynth.dll Output/system/DevIL.dll Output/plugins/DirectShowSource.dll \
    Output/plugins/ImageSeq.dll Output/plugins/Shibatch.dll Output/plugins/TimeStretch.dll \
    Output/plugins/VDubFilter.dll ../$AVSDIRNAME/32bit

Copy the .libs to the packaging directory:
::

    cp avs_core/Release/AviSynth.lib plugins/DirectShowSource/Release/DirectShowSource.lib \
    ../AviSynthPlus/plugins/ImageSeq/lib/DevIL_x86/DevIL.lib plugins/ImageSeq/Release/ImageSeq.lib \
    plugins/Shibatch/PFC/Release/PFC.lib plugins/Shibatch/Release/Shibatch.lib \
    plugins/TimeStretch/Release/TimeStretch.lib plugins/TimeStretch/SoundTouch/Release/SoundTouch.lib \
    plugins/VDubFilter/Release/VDubFilter.lib ../$AVSDIRNAME/32bit/dev


Undo the upx packing on the 32-bit copy of DevIL.dll:
::

    upx -d ../$AVSDIRNAME/32bit/DevIL.dll


For 64-bit:
::

    cmake ../AviSynthPlus -G "Visual Studio 12 2013 Win64" \
    -DDSHOWSRC_BASECLASSES_LIB="$STRMBASELIB64" -DDSHOWSRC_DX_INCLUDE_PATH="$DIRECTXSDK" && \

    cmake --build . --config Release

Copy the .dlls to the packaging directory:
::

    cp Output/AviSynth.dll Output/system/DevIL.dll Output/plugins/DirectShowSource.dll \
    Output/plugins/ImageSeq.dll Output/plugins/Shibatch.dll Output/plugins/TimeStretch.dll \
    Output/plugins/VDubFilter.dll ../$AVSDIRNAME/64bit

Copy the .libs to the packaging directory:
::

    cp avs_core/Release/AviSynth.lib plugins/DirectShowSource/Release/DirectShowSource.lib \
    ../AviSynthPlus/plugins/ImageSeq/lib/DevIL_x64/DevIL.lib plugins/ImageSeq/Release/ImageSeq.lib \
    plugins/Shibatch/PFC/Release/PFC.lib plugins/Shibatch/Release/Shibatch.lib \
    plugins/TimeStretch/Release/TimeStretch.lib plugins/TimeStretch/SoundTouch/Release/SoundTouch.lib \
    plugins/VDubFilter/Release/VDubFilter.lib ../$AVSDIRNAME/64bit/dev


Finishing up
------------

Packaging up everything can be quickly done with 7-zip:
::

    cd ..
    7z a -mx9 $AVSDIRNAME.7z $AVSDIRNAME


Back to the :doc:`main page <../../index>`

$ Date: 2016-07-04 17:45:09 -04:00 $

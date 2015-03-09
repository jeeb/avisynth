
MPEG2Dec3
=========


Abstract
--------

| **authors:** MarcFD, Nic, trbarry, Sh0dan and others
| **version:** 1.10.1
| **category:** MPEG Decoder (source) Plugins
| **download:** `<http://www.avisynth.org/warpenterprises/>`_
| **requirements:**

--------


Description
-----------

This filter is able to output in the RGB, YUY2 and YV12 colorformats.

It is a MPEG-2 decoder and it is able to decode any MPEG-2 streams readable
by dvd2avi 1.76 / 1.77.3 / 1.83x (and others). It also supports hdtv streams,
although you need `dvd2avi 1.83.5`_ for this. (This is a modified version of
MPEG2Dec2 from save-oe (smart audio video encoder).)

Additional features are (for example): YV12, YUY2 and RGB32 output (although
it is recommended to use AviSynth's color conversion routines), interlacing
control, integrated PostProcessing, Luminance Filtering, etc.

Examples Usage of MPEG2Dec3 package filters
-------------------------------------------

| (all the avisynth script lines here are only exemples)
| First, add the following line in your .avs script:

::

    LoadPlugin("MPEG2Dec3.dll")

Basic MPEG2Dec3 usage
~~~~~~~~~~~~~~~~~~~~~

To do plain YV12 decoding.
::

    MPEG2Source("dvd.d2v")

To use Post Processing : here, deblocking only.
::

    MPEG2Source("dvd.d2v", cpu=4)

it's better to use Field Based Post Processing for interlaced sources.
::

    MPEG2Source("dvd.d2v", cpu=4, iPP=true)

if you have a intel Pentium 4 cpu, you can force sse2 idct.
::

    MPEG2Source("dvd.d2v", idct=5)

To use custom post processing setting : only deringing here.
::

    MPEG2Source("dvd.d2v", cpu2="ooooxx")

Colorspace convertions
~~~~~~~~~~~~~~~~~~~~~~

To convert to YUY2.
::

    MPEG2Source("dvd.d2v")
    YV12toYUY2()

To convert to YUY2 a progressive source.

::

    MPEG2Source("dvd.d2v")
    YV12toYUY2(interlaced=false)

To convert to RGB24.

::

    MPEG2Source("dvd.d2v")
    YV12toRGB24()
    FlipVertical() #YV12->BGR24 convertion natively flips image

To convert to RGB24 a progressive source.

::

    MPEG2Source("dvd.d2v")
    YV12toRGB24(interlaced=false)
    FlipVertical() #YV12->BGR24 convertion natively flips image

Usefull additionnal YV12 Filters
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

To Darken Luminosity.
::

    LumaFilter(-10, 0.9)

To Ligthen Luminosity.
::

    LumaFilter(+10, 1.1)

Of course you can tweak the settings how you want.
See the Syntax part for more information about it.


Syntax of MPEG2Dec3 package filters
-----------------------------------


MPEG2Source
~~~~~~~~~~~

``MPEG2Source`` (string "d2v", int "cpu" int "idct" bool "iPP", int
"moderate_h", int "moderate_v", bool "showQ", bool "fastMC", string "cpu2")

*d2v* :
Your DVD2AVI project file. (``*.d2v``)

| *cpu* : 0 to 6.
| DivX decoder like cpu level setting.

- 0 : No PP
- 1 : DEBLOCK_Y_H
- 2 : DEBLOCK_Y_H, DEBLOCK_Y_V
- 3 : DEBLOCK_Y_H, DEBLOCK_Y_V, DEBLOCK_C_H
- 4 : DEBLOCK_Y_H, DEBLOCK_Y_V, DEBLOCK_C_H, DEBLOCK_C_V
- 5 : DEBLOCK_Y_H, DEBLOCK_Y_V, DEBLOCK_C_H, DEBLOCK_C_V, DERING_Y
- 6 : DEBLOCK_Y_H, DEBLOCK_Y_V, DEBLOCK_C_H, DEBLOCK_C_V, DERING_Y, DERING_C

| (Y=luma C=chroma, H=horizontal V=vertical)
| default : 0

| *idct* : 1 to 5.
| iDCT : iDCT algo used.

- 0 : Default value (in .d2v file)
- 1 : 32 bit MMX
- 2 : 32 bit SSEMMX
- 3 : 64 bit FPU
- 4 : 64 bit IEEE-1180 Reference
- 5 : 32 bit SSE2 (for P4)
- 6 : Skal's SSEMMX iDCT (fastest)
- 7 : SimpleiDCT (Very accurate iDCT from XviD project)

default : 0

| *iPP* :
| To use Field-Based Post-Processing.
| it' better if you want to deinterlace

- True : Field based
- False : Image based (default)

| *moderate_h*, *moderate_v* :
| Post Processing strength fine tunning.
| smaller values are stronger. use with care.
| default : moderate_h=20, moderate_v=40

| *showQ* :
| To see the quantizers used for each MB.
| a fun tool to play with ^^
| default : false

| *fastMC* :
| Vlad's Fast Motion Compensation code.
| very small speedup, and degraded accuracy
| it's here for testing purposes, and would probably be removed in next versions
| for ssemmx capable cpu only.
| default : false

| *cpu2* :
| Custom cpu settings
| you need to enter a 6 charachter string. each cross (x)
| would enable the corresponding Post Processing feature :

::

    example :
    "oxoxox"
    123456
    would enable chroma only PP

- 1 : luma horizontal deblocking
- 2 : luma vertical deblocking
- 3 : chroma horizontal deblocking
- 4 : chroma vertical deblocking
- 5 : luma deringing
- 6 : chroma deringing

default : " " (disabled)


LumaFilter
~~~~~~~~~~

``LumaFilter`` (clip, integer "lumoff", float "lumgain")

the transfomation is : ``yy = (y*lumgain)+lumoff``

| *lumoff* :
| Luminosity offset.
| default = -2 (for iago ^^)

| *lumgain* :
| Luminosity gain.
| default = 1


YV12toRGB24
~~~~~~~~~~~

``YV12toRGB24`` (clip, bool "interlaced", bool "TVscale")

| YV12->BGR24 convertion natively flips image
| Use `FlipVertical`_ after

| *interlaced* :
| set this to true if your source is interlaced,
  to interlace chroma correctly.
| if you have a progressive stream, using false will
  give sharper and real colors
| default : true

| *TVscale* :
| The same setting as in DVD2AVI.
| it may be inversed. try with and without and keep your favorite
| default : false


YV12toYUY2
~~~~~~~~~~

``YV12toYUY2`` (clip, bool "interlaced", bool "tff")

| *interlaced* :
| set this to true if your source is interlaced,
  to interlace chroma correctly.
| if you have a progressive stream, using false will
  give sharper and real colors
| default : true

| *tff* :
| Top Field First.
| set this to false if you have Bottom Field First.
| default : true


BlindPP
~~~~~~~

``BlindPP`` (clip, int "quant", int "cpu", str "cpu2", bool "iPP", int
"moderate_h", int "moderate_v")

| To Deblock and Dering on any kind of DCT-encoded source.
| Of course, less accurate than decoder intergrated PP, but still very efficient
| need YV12 input.

| *quant* :
| Emulated Quantizer
| use higher value to increase aggressivity
| using a value close to the source will allow very accurate postprocessing
| default : 2

| *cpu*, *cpu2*, *iPP*, *moderate_h*, *moderate_v* :
| same settings as MPEG2Source's PP.
| defaults : cpu=6, cpu2="", iPP=false, moderate_h=20, moderate_v=40


+-------------------------------------------------------------------------------------------------------------------------------------+
| **Version History**                                                                                                                 |
+-------------------------------------------------------------------------------------------------------------------------------------+
| based on MPEG2Dec2 (save-oe CVS 28.09.2002)                                                                                         |
+====================+=========+======================================================================================================+
|| betas versions    |         | - Added Nic's Post Processing with Field-Based PP                                                    |
|| (1 to 6)          |         | - Overrided iDCT / luma filtering choice                                                             |
|                    |         | - Fixed Luma filtering MMX code (3 bugs at least)                                                    |
|                    |         | - YV12->YUY2 Convertion optimised (+10 % speed)                                                      |
|                    |         | - a PP bug fixed. a bit slower now.                                                                  |
|                    |         | - trbarry's SSE2 optimisation disabled.                                                              |
|                    |         | - Added showQ debugging trigger                                                                      |
|                    |         | - Added vlad's new MC (3dnow/ssemmx) / re-writed ssemmx                                              |
|                    |         | - Added working MMX memory transfer for seeking (+3% speed)                                          |
|                    |         | - Added Interlaced Upsampling support                                                                |
+---------+----------+---------+------------------------------------------------------------------------------------------------------+
| v0.9    | 09.11.02 |         | - heavy code cleaning                                                                                |
|         |          |         | - redesigned the whole Avisynth interface                                                            |
|         |          |         | - YV12 support                                                                                       |
|         |          |         | - RGB24 support                                                                                      |
|         |          |         | - other misc stuff                                                                                   |
+---------+----------+---------+------------------------------------------------------------------------------------------------------+
| v0.91   | 10.11.02 |         | - cleaned a bit more the source                                                                      |
|         |          |         | - added MPEG2Dec3.def default settings loading (like don's filters)                                  |
|         |          |         | - bff mode in SeparateFieldsYV12                                                                     |
+---------+----------+---------+------------------------------------------------------------------------------------------------------+
| v0.92   | 17.11.02 |         | - code released                                                                                      |
|         |          |         | - blindPP implemented                                                                                |
+---------+----------+---------+------------------------------------------------------------------------------------------------------+
| v0.93   | 25.11.02 |         | - total YV12 code convertion...                                                                      |
|         |          |         | - ...who fixed YV12 bugs                                                                             |
|         |          |         | - less memory is needed                                                                              |
|         |          |         | - fast MMX copy (faster seeking)                                                                     |
+---------+----------+---------+------------------------------------------------------------------------------------------------------+
| v0.94   | 08.12.02 |         | - very little bugfix                                                                                 |
+---------+----------+---------+------------------------------------------------------------------------------------------------------+
| v1.00   | 19.01.03 |         | - final version                                                                                      |
|         |          |         | - i squashed all bugs i were aware of                                                                |
+---------+----------+---------+------------------------------------------------------------------------------------------------------+
| v1.01   | unknown  | trbarry | - Fixed HDTV bug (0x21 PID hardcoded)                                                                |
+---------+----------+---------+------------------------------------------------------------------------------------------------------+
| v1.02   | 12.05.03 | Nic     | - aligned malloc done different                                                                      |
+---------+----------+---------+------------------------------------------------------------------------------------------------------+
| v1.03   | 12.05.03 | Nic     | - Now supports both DVD2AVI 1.77.3 D2V Files and 1.76 ones                                           |
+---------+----------+---------+------------------------------------------------------------------------------------------------------+
| v1.04   | 12.05.03 | Nic     | - Removed another memory leak, slightly quicker                                                      |
+---------+----------+---------+------------------------------------------------------------------------------------------------------+
| v1.05a  | 12.05.03 | trbarry | - trbarry test version for optimisations                                                             |
+---------+----------+---------+------------------------------------------------------------------------------------------------------+
| v1.06   | 24.05.03 |         | - Nic: Added 2 new iDCT's Skal's (fastest!, idct=6) & SimpleiDCT (very accurate, idct=7)             |
|         |          |         | - Nic: Support for external use of MPEG2Dec3.dll without AviSynth added back in                      |
|         |          |         |   (See Source code for example.zip and GetPic example)                                               |
|         |          |         | - trbarry: Added new Add_Block optimisations as well as optimised Block Decoding for SSE2 machines   |
|         |          |         | - sh0dan: Uses AviSynth's fast BitBlt for mem copys where possible                                   |
|         |          |         | - Nic: General optimisations :) Faster now on all machines tested.                                   |
+---------+----------+---------+------------------------------------------------------------------------------------------------------+
| v1.07   | 06.06.03 |         | - Nic & Sh0dan: Bug Fixes, better stability on broken streams                                        |
+---------+----------+---------+------------------------------------------------------------------------------------------------------+
| v1.08   | 08.06.03 |         | - trbarry: Optimised Simple_iDCT, lots faster now :)                                                 |
|         |          |         | - Nic: added CPUCheck elsewhere, forgot to fix Lumafilter last time                                  |
|         |          |         |   (Thanx ARDA!), robUx4 helped me make simple_idct into a fastcall                                   |
+---------+----------+---------+------------------------------------------------------------------------------------------------------+
| v1.09   | 26.07.03 |         | - Nic: Now skal's Sparse iDCT is used instead for idct=6 (fastest!)                                  |
|         |          |         | - Nic: Added the Luminance_Filter from DVD2AVI 1.77.3, for when Luminance_Filter is used in the .d2v |
+---------+----------+---------+------------------------------------------------------------------------------------------------------+
| v1.10   | 28.07.03 |         | - Nic: Damn! There was a problem with the Luminance filter and 1.77.3 D2V files. Fixed!              |
+---------+----------+---------+------------------------------------------------------------------------------------------------------+
| v1.10.1 | 23.05.05 |         | - Kassandro: removed assumption "luma pitch = luma width". Needed for AviSynth v2.57 and beyond.     |
+---------+----------+---------+------------------------------------------------------------------------------------------------------+


Credits
-------

| Chia-chen Kuo, author of DVD2AVI
| Peter Gubanov, author of the MMX/SSEMMX iDCT
| Dmitry Rozhdestvensky, author of the SSE2 iDCT
| Miha Peternel, author of the Floating Point and Reference iDCT
| Mathias Born, author of MPEG2Dec

Special thanks to Nic, for the Post Processing who made MPEG2Dec3 possible

| 1.01 and above: Nic, trbarry, sh0dan
| Thanks to Skal for the use of his iDCT (http://skal.planet-d.net)

$Date: 2005/08/11 21:11:50 $

.. _dvd2avi 1.83.5: http://www.trbarry.com/DVD2AVIT3.zip
.. _FlipVertical: ../corefilters/flip.rst

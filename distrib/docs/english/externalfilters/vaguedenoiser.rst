
VagueDenoiser
=============


Abstract
--------

| **author:** Lefungus, Kurosu, Fizick
| **version:** 0.35.1
| **download:** `<http://bag.hotmail.ru/>`_
| **category:** Spatial Smoothers
| **requirements:**

-   YV12 & YUY2 & RGB Colorspace

**license:** GPL


Introduction
------------

This is a Wavelet based Denoiser.
Basically, it transforms each frame from the video input into the wavelet
domain, using various wavelet filters. Then it applies some filtering to the
obtained coefficients. It does an inverse wavelet transform after. Due to
wavelet properties, it should gives a nice smoothed result, and reduced
noise, without blurring picture features. This wavelet transform could be
done on each plane of the colorspace. This filter uses a wavelets from
Brislawn tutorial.


Syntax of VagueDenoiser filter
------------------------------

``VagueDenoiser`` (clip, int "threshold", int "method", int "nsteps", float
"chromaT", bool "debug", bool "interlaced", int "wavelet", bool "Wiener",
float "wratio", integer "percent", clip "auxclip")


Parameters
----------

*threshold*: a float (default=0)
    Filtering strength. The higher, the more filtered the clip will be.
    Hard thresholding can use a higher threshold than Soft thresholding
    before the clip looks overfiltered.
    If set < 0, then luminosity denoising will be disabled
    If set = 0, then threshold is estimated automatically (adaptive)

*method*: -1 to 3 (default=3)
    The filtering method the filter will use.
    -1 : No thresholding (debug purpose)
    0 : Hard Thresholding. All values under the threshold will be zeroed.
    1 : Soft Thresholding. All values under the threshold will be zeroed.
    All values above will be reduced by the threshold.
    2 : Adaptive thresholding (NormalSrink method). Scale input threshold
    depending on local wavelet data (local = wavelet decomposition subband
    level).
    3 : Qian's (garrote) thresholding. Scales or nullifies coefficients -
    intermediary between (more) soft and (less) hard thresholding.
    4 : Uniformly smooth shrinking function.

*nsteps*: (default=4)
    Number of times, the wavelet will decompose the picture. High values
    can be slower but results will be better.
    Suggested values are 3-6.
    Picture can't be decomposed beyond a particular point (typically, 8
    for a 640x480 frame - as 2^9 = 512 > 480)

*chromaT*: a float (default=-1)
    Set threshold value for Chroma filtering. It is slower but give
    better results
    If set < 0, then Chroma denoising will be disabled (default mode)
    If set = 0, then threshold is estimated automatically (adaptive)

*debug*: true or false
    Deactivates the inverse transform for direct display (but not
    normalize coefficients now).
    Also enables output for Debugview utility

*interlaced*: true or false (default=false)
    Try to process separately fields clip.

*wavelet*: an integer (default=1)
    Select wavelet type:
    1 - Cohen-Daubechies-Feauveau 9/7 - popular, probably the best
    2 - Brislawn 10/10 (sharp, used as only in all previous versions from
    0.23 to 0.29)
    3 - Villasenor-Belzer-Liao 6/10

*Wiener*: true or false (default=false)
    Activates WienerChop two-pass denoising mode for strong denoising
    with high threshold (slow).
    First pass is noise estimation by transform with first wavelet
    (different from selected wavelet) with selected settings,
    the second pass is optimal Wiener denoising with second (selected)
    wavelet.
    Second wavelet 1 is used with first (estimation) wavelet 3,
    Second wavelet 2 is used with first (estimation) wavelet 1,
    Second wavelet 3 is used with first (estimation) wavelet 1.

*wratio*: a float (default=0.5)
    Noise std deviation value for Wiener denoising as relative ratio to
    threshold value.

*percent*: an integer from 0 to 100 (default=85)
    Partial of full denoising (limited coefficients shrinking).

*auxclip*: auxiliary clip for first pass of Wiener mode (default=none, source)
    Prefiltered source clip, preferably with temporal or spatial-temporal
    filter (such as DeGrainMedian) for best denoising and artifacts
    suppression.



Examples Usage of VagueDenoiser filter
--------------------------------------

(all the avisynth script lines here are only examples)


Some settings Lefungus (sometime) liked
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

For light filtering on a movie. (medium->threshold=1-1.5;
strong->threshold=2-3)

::

    VagueDenoiser(threshold=0.8, method=1, nsteps=6, chromaT=0.8)

For light filtering on an anime. (medium->threshold=2; strong->threshold=4)

::

    VagueDenoiser(threshold=1.5, method=1, nsteps=6, chromaT=2.0)

Quian's thresholding is preferred method as the more similar to optimal
(Bayesian).


Some strong (but slow) settings by Fizick
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

For strong filtering (with a high threshold) on a noisy interlaced analog
video. In this case, the "small drop" artifactes is likely appear, especially
for hard thresholding (Quian mode is optimal). This is because of fast
decimated wavelet transform usage, as wee as "hot" pixels pulses. Use
optinmal Wiener fiter. Firstly make poewr density estimation for signal and
noise in firdt wavlet basis  Then use another wavelet basis (almost
uncorrelared) for filtering, i.e. weak value decreasing (using previuusly
made estimation). In addition, decrease the "hot pixel" influence by median
temporal prefiltering (undot, degrainmedian), and use the result as auxiliary
clip for noise estimation. This aux clip can be strongly filtered since we do
not use it as input for Vaguedenoiser. (By the way, we get not strictly
spatial smoothing now). See example (slow):

::

    LoadPlugin("vaguedenoiser.dll")
    LoadPlugin("degrainmedian.dll")
    avisource("input.avi")
    aux=DeGrainMedian(mode=0, limity=7, interlaced=true)
    VagueDenoiser(threshold=7, auxclip=aux, interlaced=true, wiener=true)

+------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| Changelog                                                                                                                                                                                                              |
+===========+====================+======================+================================================================================================================================================================+
| v0.12     |                    |                      | - First Release                                                                                                                                                |
|           |                    |                      | - Precision problems corrected, 0 for threshold is now lossless.                                                                                               |
+-----------+--------------------+----------------------+----------------------------------------------------------------------------------------------------------------------------------------------------------------+
| v0.13     |                    |                      | - Hard thresholding method enabled.                                                                                                                            |
|           |                    |                      | - Cosmetic changes.                                                                                                                                            |
|           |                    |                      | - Avisynth parameters changed.                                                                                                                                 |
+-----------+--------------------+----------------------+----------------------------------------------------------------------------------------------------------------------------------------------------------------+
| v0.2      |                    |                      | - Implemented many new wavelet filters, thanks to the wavelet transform coder construction kit. look at `<http://www.geoffdavis.net/>`_ for more informations. |
|           |                    |                      | - Implemented nsteps parameter, that allow you to use n steps in the selected wavelet tranform.                                                                |
|           |                    |                      | - Some nsteps values could produce unvalid results, reducing this value generally solve the problem.                                                           |
|           |                    |                      | - Little optimizations from Shodan and Bidoche.                                                                                                                |
+-----------+--------------------+----------------------+----------------------------------------------------------------------------------------------------------------------------------------------------------------+
| v0.22     |                    |                      | - YUY2 colorspace support.                                                                                                                                     |
|           |                    |                      | - Optionnal chroma filtering, (chroma=true/false).                                                                                                             |
|           |                    |                      | - Cosmetic changes, code cleaned.                                                                                                                              |
|           |                    |                      | - Html documentation.                                                                                                                                          |
+-----------+--------------------+----------------------+----------------------------------------------------------------------------------------------------------------------------------------------------------------+
| v0.23     |                    |                      | - Code cleaned, filters class removed.                                                                                                                         |
|           |                    |                      | - Better parameters for compilation. Should really works on every cpu now.                                                                                     |
|           |                    |                      | - Removed all filters except Brislawn 10/10, so filter parameter has been removed.(it's like filter=7).                                                        |
|           |                    |                      | - A little speed increase (3-5fps on a 640x256 frame with nsteps=6).                                                                                           |
+-----------+--------------------+----------------------+----------------------------------------------------------------------------------------------------------------------------------------------------------------+
| v0.24     |                    |                      | - Another speed increase, some critical loops unrolled (To infinity and beyond!)                                                                               |
+-----------+--------------------+----------------------+----------------------------------------------------------------------------------------------------------------------------------------------------------------+
| v0.241    |                    |                      | - height and width must be mod4, added errors messages if not                                                                                                  |
|           |                    |                      | - Cleaned include thanks to Kurosu                                                                                                                             |
+-----------+--------------------+----------------------+----------------------------------------------------------------------------------------------------------------------------------------------------------------+
| v0.242    |                    |                      | - Removed restrictions on width/height, fixed bugs                                                                                                             |
|           |                    |                      | - Some improvements from Kurosu                                                                                                                                |
+-----------+--------------------+----------------------+----------------------------------------------------------------------------------------------------------------------------------------------------------------+
| v0.243    |                    |                      | - Code cleaned                                                                                                                                                 |
+-----------+--------------------+----------------------+----------------------------------------------------------------------------------------------------------------------------------------------------------------+
| v0.25     |                    |                      | - Little speed increase (due to some little profiles and vectorizations)                                                                                       |
|           |                    |                      | - added defaults                                                                                                                                               |
|           |                    |                      | - added a visual.net compiled dll for compatibility purposes. This dll is slower                                                                               |
+-----------+--------------------+----------------------+----------------------------------------------------------------------------------------------------------------------------------------------------------------+
| v0.26     |                    | Kurosu               | - Merged (C++ frenzy) all assembly parts from Kurosu's version into one dll                                                                                    |
|           |                    |                      | - Hence, speed increase                                                                                                                                        |
|           |                    |                      | - Undone previous vectorization optimizations                                                                                                                  |
|           |                    |                      | - All improvements are available for YV12 only                                                                                                                 |
+-----------+--------------------+----------------------+----------------------------------------------------------------------------------------------------------------------------------------------------------------+
| v0.26.1   |                    | Kurosu               | - Merge from Sh0dan on copy                                                                                                                                    |
|           |                    |                      | - Cleaner and safer management from Bidoche                                                                                                                    |
|           |                    |                      | - Fixed a crash that may have affected P4 users (Athlons with SSE support weren't affected as 3DNow! code, being the fatest, is always selected).              |
|           |                    |                      | - Added debug output and NOOP threshold method (for debug purpose)                                                                                             |
+-----------+--------------------+----------------------+----------------------------------------------------------------------------------------------------------------------------------------------------------------+
| v0.27.0   |                    | Kurosu               | - Continued integration and added framework for Haar wavelet (still not functionnal, deactivated)                                                              |
|           |                    |                      | - All optimizations proposed by Sh0dan, ARDA and Bidoche                                                                                                       |
|           |                    |                      | - Some registers reuse (AMD-64 gonna rock) for some more speed (5%)                                                                                            |
|           |                    |                      | - Added Qian thresholding                                                                                                                                      |
+-----------+--------------------+----------------------+----------------------------------------------------------------------------------------------------------------------------------------------------------------+
| v0.27.1   |                    | Kurosu               | - Fixed YUY2 mode                                                                                                                                              |
|           |                    |                      | - Added RGB24 and RGB32 mode                                                                                                                                   |
|           |                    |                      | - The 3 above modes are unlikely to get any speed optimization                                                                                                 |
+-----------+--------------------+----------------------+----------------------------------------------------------------------------------------------------------------------------------------------------------------+
| v0.28.0   |                    | Kurosu               | - Fixed all modes to properly process what needs to be processed (small speedup)                                                                               |
|           |                    |                      | - Unrolled float2byte conversion, little speed-up                                                                                                              |
|           |                    |                      | - Added interlaced (see option with that name) support to YV12 colorspace                                                                                      |
|           |                    |                      | - Workspace compatible with ICL7. dll isn't compiled in that mode for legal reasons (I don't own ICL)                                                          |
|           |                    |                      | - Decteted a major slowdown for MOD64 width. Avoid them, as applying this filter before resizing might be faster in fact (particularly true for widths of 512) |
+-----------+--------------------+----------------------+----------------------------------------------------------------------------------------------------------------------------------------------------------------+
| v0.28.1   |                    | Kurosu               | - MOD64 width slowdown is due to something needing a dreadfull rewrite, so no fix                                                                              |
|           |                    |                      | - Implemented cleaner support of YUY2 and RGB24/32                                                                                                             |
|           |                    |                      | - Interlaced mode now works in all mode (processing in interlaced mode is a little bit faster but you loose precision)                                         |
+-----------+--------------------+----------------------+----------------------------------------------------------------------------------------------------------------------------------------------------------------+
| v0.28.2   |                    | Kurosu               | - Ported to nasm the assembly parts                                                                                                                            |
+-----------+--------------------+----------------------+----------------------------------------------------------------------------------------------------------------------------------------------------------------+
| v0.29     |                    | Kurosu               | - More nasm                                                                                                                                                    |
|           |                    |                      | - More bug fixes                                                                                                                                               |
+-----------+--------------------+----------------------+----------------------------------------------------------------------------------------------------------------------------------------------------------------+
| v0.30     |                    | Fizick               | - Added (restored) CDF 9/7 wavelet (C version only) and wavelet selection option                                                                               |
|           |                    |                      | - Added WienerChop two-pass denoising mode using two different wavelet basises.                                                                                |
|           |                    |                      | - Replaced boolean "chroma" parameter to float "chromaT" as threshold value for chroma planes                                                                  |
|           |                    |                      | - Added automatic (adaptive) threshold estimation                                                                                                              |
|           |                    |                      | - 3DNow mode of hard thesholding temporary replaced by SSE or C versions.                                                                                      |
|           |                    |                      | - Fixed possible memory leakage bug for non YV12 modes                                                                                                         |
+-----------+--------------------+----------------------+----------------------------------------------------------------------------------------------------------------------------------------------------------------+
| v0.31     |                    | Fizick               | - Added noise ratio parameter for Wiener pass                                                                                                                  |
|           |                    |                      | - Fixed interlaced mode. Seems it now work.                                                                                                                    |
|           |                    |                      | - 3DNow mode of Qian thesholding temporary replaced by SSE or C versions.                                                                                      |
+-----------+--------------------+----------------------+----------------------------------------------------------------------------------------------------------------------------------------------------------------+
| v0.32     | July 09, 2004      | Released by Fizick,  | - Added Villasenor-Belzer-Liao 6/10 wavelet (not optimized C version only)                                                                                     |
|           |                    | but part of work was | - Changed estimation wavelets to more optimal pairs for WienerChop mode.                                                                                       |
|           |                    | done by Kurosu       | - 3DNow optimized version of WienerChop and AutoThreshold - thanks to Kurosu.                                                                                  |
|           |                    |                      | - 3DNow mode of hard and Qian thesholding re-enabled after some bugs were fixed by Kurosu and Fizick.                                                          |
|           |                    |                      | - Small speed increasing mainly due to copy reverse order                                                                                                      |
|           |                    |                      | - But big slowdown for mod64 width (especially 512!) still exists.                                                                                             |
|           |                    |                      | - SSE float-byte conversion has bug for some width, temporary replaced to C version.                                                                           |
|           |                    |                      | - Ported to NASM memcopy assembly function (no more non-NASM assembly).                                                                                        |
+-----------+--------------------+----------------------+----------------------------------------------------------------------------------------------------------------------------------------------------------------+
| v0.32.1   | July 10, 2004      | Fizick               | - Fixed bug in copy function, introduced in v.0.32                                                                                                             |
|           |                    |                      | - Re-enabled SSE optimized float-byte and byte-float conversion after fixing some bugs                                                                         |
+-----------+--------------------+----------------------+----------------------------------------------------------------------------------------------------------------------------------------------------------------+
| v0.33     | July 11, 2004      | Fizick               | - Fixed slowdown for mod64 width by padding (thanks to MfA)                                                                                                    |
|           |                    |                      | - Add partial denoising mode (by blending with source)                                                                                                         |
|           |                    |                      | - Fixed bug with AutoThreshold for 3DNow.                                                                                                                      |
|           |                    |                      | - AutoThreshold is now also dependent from "wratio" parameter.                                                                                                 |
|           |                    |                      | - Added messages for Debugview utility.                                                                                                                        |
|           |                    |                      | - Change some parameters default values to more optimal (for me?): thresh=0 (auto), method=3, nsteps=4, wavelet=1, percent=75.                                 |
+-----------+--------------------+----------------------+----------------------------------------------------------------------------------------------------------------------------------------------------------------+
| v0.33.1   | July 13, 2004      | Fizick               | - Fixed bug with mod64 width for SSE (by pad increasing from 2 to 4)                                                                                           |
+-----------+--------------------+----------------------+----------------------------------------------------------------------------------------------------------------------------------------------------------------+
| v0.33.2   | July 17, 2004      | Fizick               | - Fixed bug with default values.                                                                                                                               |
+-----------+--------------------+----------------------+----------------------------------------------------------------------------------------------------------------------------------------------------------------+
| v0.33.3   | July 21, 2004      | Fizick               | - Fixed bug with YUY2 (introduced in v.0.32.1)                                                                                                                 |
+-----------+--------------------+----------------------+----------------------------------------------------------------------------------------------------------------------------------------------------------------+
| v0.33.4   | August 23, 2004    | Fizick               | - Fixed bug with mirrored padded pixels (thanks to Eugen65 for report).                                                                                        |
+-----------+--------------------+----------------------+----------------------------------------------------------------------------------------------------------------------------------------------------------------+
| v0.33.5   | September 28, 2004 | Fizick               | - Fixed bug with AutoThreshold for Interlaced Wiener mode (thanks to Viperzahn and LigH for report).                                                           |
+-----------+--------------------+----------------------+----------------------------------------------------------------------------------------------------------------------------------------------------------------+
| v0.33.6   | October 13, 2004   | Fizick               | - Nsteps parameter now is auto-limited to max admissible value if input too big or =0 (don't worry, Viperzahn :-)                                              |
|           |                    |                      | - Some improving of exception handling (try-catch-throw).                                                                                                      |
+-----------+--------------------+----------------------+----------------------------------------------------------------------------------------------------------------------------------------------------------------+
| v0.33.7   | October 17, 2004   | Fizick               | - Decreased max admissible value of nsteps to fix some internal bug (or feature) (thanks to Viperzahn for insistent report :-)                                 |
+-----------+--------------------+----------------------+----------------------------------------------------------------------------------------------------------------------------------------------------------------+
| v0.34     | November 24, 2004  | Fizick               | - Added auxiliary (some prefiltered) clip for first pass of Wiener mode.                                                                                       |
+-----------+--------------------+----------------------+----------------------------------------------------------------------------------------------------------------------------------------------------------------+
| v0.34.1   | December 19, 2004  | Fizick               | - Some fix for mem_set compatibility with old CPU (P2).                                                                                                        |
+-----------+--------------------+----------------------+----------------------------------------------------------------------------------------------------------------------------------------------------------------+
| v0.34.2   | March 11, 2005     | Fizick               | - Fixed small bug (blue dot) for chromaT>=0 in YV12 for Athlon (Thanks to Pavico for report).                                                                  |
|           |                    |                      | - Added pitch for internal buffers.                                                                                                                            |
+-----------+--------------------+----------------------+----------------------------------------------------------------------------------------------------------------------------------------------------------------+
| v0.34.2.0 | June 13, 2005      | Fizick               | - Updated documentation.                                                                                                                                       |
+-----------+--------------------+----------------------+----------------------------------------------------------------------------------------------------------------------------------------------------------------+
| v0.34.3   | September 11, 2005 | Fizick               | - Fixed AutoThreshold algorithm.                                                                                                                               |
+-----------+--------------------+----------------------+----------------------------------------------------------------------------------------------------------------------------------------------------------------+
| v0.35     | September 17, 2005 | Fizick               | - Improving soft method=1 - do not filter lowest level anymore, mean picture intensity is not changed now .                                                    |
|           |                    |                      | - Re-enabled method=2 of multilevel subband adaptive thresholding, implemented as NormalShrink method;                                                         |
|           |                    |                      | - Added method=4 uniformly smooth shrinking function;                                                                                                          |
|           |                    |                      | - Replaced partial denoising blend mode by limited shrinking of small coefficients for all thresholding functions;                                             |
|           |                    |                      | - Changed default percent=85;                                                                                                                                  |
|           |                    |                      | - Implemented new SSE versions for method=0,3,4 and C versions for rest (3DNow disabled with 4% speed decreasing);                                             |
|           |                    |                      | - Added 3DNow optimized versions for transform of wavelet 1 and 3 (10% speed).                                                                                 |
+-----------+--------------------+----------------------+----------------------------------------------------------------------------------------------------------------------------------------------------------------+
| v0.35.1   | September 26, 2005 | Fizick               | - Added YUY2 and RGB format for auxclip                                                                                                                        |
+-----------+--------------------+----------------------+----------------------------------------------------------------------------------------------------------------------------------------------------------------+


Credits
-------

* Everyone at Doom9.org for their counsels.
* MarcFD for his mpegdec3 html documentation. This html is the same but with
  modified contents.
* Lefungus for his VagueDenoiser html documentatiion. This html is the same
  but with modified contents. :)
* Kurosu for his VagueDenoiser html documentatiion. This html is the same but
  with modified contents. :-) (but partially reformated now)
* Geoff Davis, author of the wavelet transform coder construction kit.
* Lefungus, VagueDenoiser's creator.
* Kurosu, code reorganization and optimizations.
* Fizick, some code deorganization and deoptimizations. :-)


Code Distribution
-----------------

This is a free sofware distribued under the terms of the GNU-GPL v2 .


Contact
-------

You can e-mail Lefungus to: lefungus (at) altern (dot) org for most
suggestions, bug report, feature request, or whatever.

Lefungus web page: `<http://perso.wanadoo.fr/reservoir/avisynth.html>`_

Optimization matters are Kurosu's matters.

Yet it should be obvious, you may reach Kurosu here: *kurosu (at) inforezo
(dot) org*

Fizick is not responsible for anything, but usually strives for perfection :)

Fizick is accessible at: *bag (at) hotmail (dot) ru*, and his web page with
latest versions must be at: `<http://bag.hotmail.ru>`_ or its mirror.

Goto `<http://forum.doom9.org/showthread.php?s=&threadid=56871>`_ for support.

$Date: 2005/10/05 18:12:43 $

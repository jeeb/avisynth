
`AviSynth FilterSDK`_
=====================

AviSynth external Filter SDK is a package for developers to create your own
filters (plugins) for AviSynth.

The package consists of:

-   this documentation files in HTML format;
-   the header file 'avisynth.h' (recent version) with all declarations
    to include in plugin source code;
-   SimpleSample plugin source codes;
-   may be some extra files in 'Extra' folder.

You must have some :doc:`necessary software. <SDKNecessaries>`

:doc:`Bens AviSynth Docs <BensAviSynthDocs>` is the documentation written for AviSynth 1.0 by Ben
Rudiak-Gould, in its original form.

See more about the modifications for AviSynth 2.5 in the :doc:`AviSynth Two-Five SDK <AviSynthTwoFiveSDK>`.

Also, :doc:`Simple Sample <SimpleSample>` has some very simple examples covering development of
a filter that does virtually nothing through to one that draws a variable
sized square, in the middle of the screen, in all :doc:`Color Spaces <ColorSpaces>`.

One thing not covered in SimpleSample, is how to :doc:`Change Frame Size <ChangeFrameSize>` in a
filter.

Also have a look at :doc:`Getting started with Audio <GettingStartedWithAudio>`.

See :doc:`Non-clip Sample <Non-ClipSample>` how to create runtime AviSynth functions.

There are several different Colorspaces in AviSynth. See more information
about :doc:`Color Spaces <ColorSpaces>` and :doc:`Working With Images <WorkingWithImages>`.

Read more about the :doc:`Internal Functions <InternalFunctions>` in AviSynth.

You can also browse various topic on :doc:`Assembler Optimizing <AssemblerOptimizing>`.

Once you've got the basics down on AVS development (Ben's text is quite
good), the `[SDK]`_ for VirtualDub is also a good read. Good news is you
won't have to worry about writing `[function pointers]`_ and `[raw Win32]`_;
meanwhile, Avery knows his stuff when it comes to video & CPU optimization
techniques, so you best pay attention.

Some video related ebooks (PDF) can be downloaded freely from `[Snell & Wilcox]`_.

Please read AviSynth :doc:`FilterSDK license <SDKLicense>` and :doc:`SDK History. <SDKHistory>`

And welcome to `[AviSynth Development forum]`_!

$Date: 2007/12/01 20:53:28 $

Latest online mediaWiki version is at
http://avisynth.org/mediawiki/Filter_SDK

.. _AviSynth FilterSDK: http://www.avisynth.org/FilterSDK
.. _[SDK]: http://virtualdub.org/filtersdk
.. _[function pointers]: http://function-pointer.org/
.. _[raw Win32]: http://www.charlespetzold.com/pw5/index.html
.. _[Snell & Wilcox]: http://www.snellwilcox.com/reference.html
.. _[AviSynth Development forum]:
    http://forum.doom9.org/forumdisplay.php?s=&f=69

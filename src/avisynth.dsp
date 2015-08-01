# Microsoft Developer Studio Project File - Name="avisynth" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=avisynth - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "avisynth.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "avisynth.mak" CFG="avisynth - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "avisynth - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "avisynth - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "avisynth - Win32 RelSym" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "avisynth - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W4 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "AVISYNTH_EXPORTS" /YX /FD /c
# ADD CPP /nologo /G6 /MD /W4 /GX /Zd /O2 /Op /Ob2 /D "NDEBUG" /D "INC_OLE2" /D "STRICT" /D "WIN32" /D "_WIN32" /D "_MT" /D "_DLL" /D "_MBCS" /D "_USRDLL" /D "AVISYNTH_C_EXPORTS" /D "AVISYNTH_CORE" /Fr /Yu"stdafx.h" /FD /Gs /GF /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 ../distrib/lib/DevIL.lib $(IntDir)/SoundTouch.lib $(IntDir)/pfc.lib $(IntDir)/softwire.lib msacm32.lib vfw32.lib kernel32.lib advapi32.lib version.lib user32.lib gdi32.lib ole32.lib uuid.lib winmm.lib oleaut32.lib /nologo /dll /map /machine:I386 /nodefaultlib:"LIBC"
# SUBTRACT LINK32 /pdb:none /debug
# Begin Custom Build
InputPath=.\Release\avisynth.dll
SOURCE="$(InputPath)"

"$(SystemRoot)\system32\avisynth.dll" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\distrib\upx -9 release\avisynth.dll 
	copy Release\avisynth.dll $(SystemRoot)\system32 
	
# End Custom Build
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Cmds=copy ..\distrib\bin\devil.dll $(SystemRoot)\system32
# End Special Build Tool

!ELSEIF  "$(CFG)" == "avisynth - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W4 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "AVISYNTH_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /G6 /MDd /W4 /Gm /GX /ZI /Od /D "IL_DEBUG" /D "_DEBUG" /D "DEBUG" /D "INC_OLE2" /D "STRICT" /D "WIN32" /D "_WIN32" /D "_MT" /D "_DLL" /D "_MBCS" /D "_USRDLL" /D "AVISYNTH_C_EXPORTS" /D "AVISYNTH_CORE" /Fr /Yu"stdafx.h" /FD /GZ /GF /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 ../distrib/lib/DevIL.lib $(IntDir)/SoundTouch.lib $(IntDir)/pfc.lib $(IntDir)/softwire.lib msacm32.lib vfw32.lib kernel32.lib advapi32.lib version.lib user32.lib gdi32.lib ole32.lib uuid.lib winmm.lib oleaut32.lib /nologo /dll /map /debug /machine:I386 /nodefaultlib:"LIBC" /pdbtype:sept
# SUBTRACT LINK32 /pdb:none /incremental:no /nodefaultlib
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Cmds=copy Debug\avisynth.dll $(SystemRoot)\system32	copy ..\distrib\bin\devil.dll $(SystemRoot)\system32
# End Special Build Tool

!ELSEIF  "$(CFG)" == "avisynth - Win32 RelSym"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "RelSym"
# PROP BASE Intermediate_Dir "RelSym"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "RelSym"
# PROP Intermediate_Dir "RelSym"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W4 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "AVISYNTH_EXPORTS" /YX /FD /c
# ADD CPP /G6 /MD /W4 /GX /Zd /O2 /Op /Ob2 /D "NDEBUG" /D "INC_OLE2" /D "STRICT" /D "WIN32" /D "_WIN32" /D "_MT" /D "_DLL" /D "_MBCS" /D "_USRDLL" /D "AVISYNTH_C_EXPORTS" /D "AVISYNTH_CORE" /FAs /FR /Yu"stdafx.h" /FD /Gs /GF /c
# SUBTRACT CPP /nologo
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /D "NDEBUG" /mktyplib203 /win32
# SUBTRACT MTL /nologo
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 ../distrib/lib/DevIL.lib $(IntDir)/SoundTouch.lib $(IntDir)/pfc.lib $(IntDir)/softwire.lib msacm32.lib vfw32.lib kernel32.lib advapi32.lib version.lib user32.lib gdi32.lib ole32.lib uuid.lib winmm.lib oleaut32.lib /nologo /dll /map /debug /debugtype:both /machine:I386 /nodefaultlib:"LIBC"
# SUBTRACT LINK32 /pdb:none

!ENDIF 

# Begin Target

# Name "avisynth - Win32 Release"
# Name "avisynth - Win32 Debug"
# Name "avisynth - Win32 RelSym"
# Begin Group "Avisynth core"

# PROP Default_Filter ""
# Begin Group "Parser"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\core\parser\expression.cpp
# End Source File
# Begin Source File

SOURCE=.\core\parser\expression.h
# End Source File
# Begin Source File

SOURCE=.\core\parser\script.cpp
# End Source File
# Begin Source File

SOURCE=.\core\parser\script.h
# End Source File
# Begin Source File

SOURCE=.\core\parser\scriptparser.cpp
# End Source File
# Begin Source File

SOURCE=.\core\parser\scriptparser.h
# End Source File
# Begin Source File

SOURCE=.\core\parser\tokenizer.cpp
# End Source File
# Begin Source File

SOURCE=.\core\parser\tokenizer.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\core\alignplanar.cpp
# End Source File
# Begin Source File

SOURCE=.\core\alignplanar.h
# End Source File
# Begin Source File

SOURCE=.\core\avisynth.cpp
# End Source File
# Begin Source File

SOURCE=.\core\avisynth.def
# End Source File
# Begin Source File

SOURCE=.\core\avisynth.h
# End Source File
# Begin Source File

SOURCE=.\core\avisynth.rc
# End Source File
# Begin Source File

SOURCE=.\core\avisynth_c.cpp
# End Source File
# Begin Source File

SOURCE=.\core\avisynth_c.h
# End Source File
# Begin Source File

SOURCE=.\core\cache.cpp
# End Source File
# Begin Source File

SOURCE=.\core\cache.h
# End Source File
# Begin Source File

SOURCE=.\core\clip_info.h
# End Source File
# Begin Source File

SOURCE=.\core\Error.h
# End Source File
# Begin Source File

SOURCE=.\core\info.h
# End Source File
# Begin Source File

SOURCE=.\core\interface.cpp
# End Source File
# Begin Source File

SOURCE=.\internal.h
# End Source File
# Begin Source File

SOURCE=.\core\main.cpp
# End Source File
# Begin Source File

SOURCE=.\core\memcpy_amd.cpp
# End Source File
# Begin Source File

SOURCE=.\core\memcpy_amd.h
# End Source File
# Begin Source File

SOURCE=.\core\plugins.cpp
# End Source File
# Begin Source File

SOURCE=.\core\softwire_helpers.cpp
# End Source File
# Begin Source File

SOURCE=.\core\softwire_helpers.h
# End Source File
# End Group
# Begin Group "Filters"

# PROP Default_Filter ""
# Begin Group "Audio"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\audio\audio.cpp
# End Source File
# Begin Source File

SOURCE=.\audio\audio.h
# End Source File
# Begin Source File

SOURCE=".\audio\avs-soundtouch.cpp"
# End Source File
# Begin Source File

SOURCE=".\audio\avs-soundtouch.h"
# End Source File
# Begin Source File

SOURCE=.\audio\convertaudio.cpp
# End Source File
# Begin Source File

SOURCE=.\audio\convertaudio.h
# End Source File
# Begin Source File

SOURCE=.\audio\dbesi0.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\audio\math_shared.h
# End Source File
# Begin Source File

SOURCE=.\audio\paramlist.h
# End Source File
# Begin Source File

SOURCE=".\audio\ssrc-convert.cpp"
# End Source File
# Begin Source File

SOURCE=".\audio\ssrc-convert.h"
# End Source File
# Begin Source File

SOURCE=.\audio\ssrc.cpp
# End Source File
# Begin Source File

SOURCE=.\audio\ssrc.h
# End Source File
# Begin Source File

SOURCE=.\audio\supereq.cpp
# End Source File
# Begin Source File

SOURCE=.\audio\supereq.h
# End Source File
# End Group
# Begin Group "Video"

# PROP Default_Filter ""
# Begin Group "Overlay"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\filters\overlay\444convert.cpp
# End Source File
# Begin Source File

SOURCE=.\filters\overlay\444convert.h
# End Source File
# Begin Source File

SOURCE=.\filters\overlay\blend_asm.cpp
# End Source File
# Begin Source File

SOURCE=.\filters\overlay\blend_asm.h
# End Source File
# Begin Source File

SOURCE=.\filters\overlay\imghelpers.h
# End Source File
# Begin Source File

SOURCE=.\filters\overlay\OF_add.cpp
# End Source File
# Begin Source File

SOURCE=.\filters\overlay\OF_blend.cpp
# End Source File
# Begin Source File

SOURCE=.\filters\overlay\OF_darken.cpp
# End Source File
# Begin Source File

SOURCE=.\filters\overlay\OF_difference.cpp
# End Source File
# Begin Source File

SOURCE=.\filters\overlay\OF_exclusion.cpp
# End Source File
# Begin Source File

SOURCE=.\filters\overlay\OF_lighten.cpp
# End Source File
# Begin Source File

SOURCE=.\filters\overlay\OF_lumaChroma.cpp
# End Source File
# Begin Source File

SOURCE=.\filters\overlay\OF_multiply.cpp
# End Source File
# Begin Source File

SOURCE=.\filters\overlay\OF_softhardlight.cpp
# End Source File
# Begin Source File

SOURCE=.\filters\overlay\OF_subtract.cpp
# End Source File
# Begin Source File

SOURCE=.\filters\overlay\overlay.cpp
# End Source File
# Begin Source File

SOURCE=.\filters\overlay\overlay.h
# End Source File
# Begin Source File

SOURCE=.\filters\overlay\overlayfunctions.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\filters\color.cpp
# End Source File
# Begin Source File

SOURCE=.\filters\color.h
# End Source File
# Begin Source File

SOURCE=.\filters\combine.cpp
# End Source File
# Begin Source File

SOURCE=.\filters\combine.h
# End Source File
# Begin Source File

SOURCE=.\filters\convolution.cpp
# End Source File
# Begin Source File

SOURCE=.\filters\convolution.h
# End Source File
# Begin Source File

SOURCE=.\filters\edit.cpp
# End Source File
# Begin Source File

SOURCE=.\filters\edit.h
# End Source File
# Begin Source File

SOURCE=.\filters\field.cpp
# End Source File
# Begin Source File

SOURCE=.\filters\field.h
# End Source File
# Begin Source File

SOURCE=.\filters\focus.cpp
# End Source File
# Begin Source File

SOURCE=.\filters\focus.h
# End Source File
# Begin Source File

SOURCE=.\filters\fps.cpp
# End Source File
# Begin Source File

SOURCE=.\filters\fps.h
# End Source File
# Begin Source File

SOURCE=.\filters\greyscale.cpp
# End Source File
# Begin Source File

SOURCE=.\filters\greyscale.h
# End Source File
# Begin Source File

SOURCE=.\filters\histogram.cpp
# End Source File
# Begin Source File

SOURCE=.\filters\histogram.h
# End Source File
# Begin Source File

SOURCE=.\filters\layer.cpp
# End Source File
# Begin Source File

SOURCE=.\filters\layer.h
# End Source File
# Begin Source File

SOURCE=.\filters\levels.cpp
# End Source File
# Begin Source File

SOURCE=.\filters\levels.h
# End Source File
# Begin Source File

SOURCE=.\filters\limiter.cpp
# End Source File
# Begin Source File

SOURCE=.\filters\limiter.h
# End Source File
# Begin Source File

SOURCE=.\filters\merge.cpp
# End Source File
# Begin Source File

SOURCE=.\filters\merge.h
# End Source File
# Begin Source File

SOURCE=.\filters\misc.cpp
# End Source File
# Begin Source File

SOURCE=.\filters\misc.h
# End Source File
# Begin Source File

SOURCE=.\filters\planeswap.cpp
# End Source File
# Begin Source File

SOURCE=.\filters\planeswap.h
# End Source File
# Begin Source File

SOURCE=.\filters\resample.cpp
# End Source File
# Begin Source File

SOURCE=.\filters\resample.h
# End Source File
# Begin Source File

SOURCE=.\filters\resample_functions.cpp
# End Source File
# Begin Source File

SOURCE=.\filters\resample_functions.h
# End Source File
# Begin Source File

SOURCE=.\filters\resize.cpp
# End Source File
# Begin Source File

SOURCE=.\filters\resize.h
# End Source File
# Begin Source File

SOURCE=".\filters\text-overlay.cpp"
# End Source File
# Begin Source File

SOURCE=".\filters\text-overlay.h"
# End Source File
# Begin Source File

SOURCE=.\filters\transform.cpp
# End Source File
# Begin Source File

SOURCE=.\filters\transform.h
# End Source File
# Begin Source File

SOURCE=.\filters\turn.cpp
# End Source File
# Begin Source File

SOURCE=.\filters\turn.h
# End Source File
# Begin Source File

SOURCE=.\filters\turnfunc.cpp
# End Source File
# Begin Source File

SOURCE=.\filters\turnfunc.h
# End Source File
# End Group
# Begin Group "Conditional"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\filters\conditional\conditional.cpp
# End Source File
# Begin Source File

SOURCE=.\filters\conditional\conditional.h
# End Source File
# Begin Source File

SOURCE=.\filters\conditional\conditional_functions.cpp
# End Source File
# Begin Source File

SOURCE=.\filters\conditional\conditional_functions.h
# End Source File
# Begin Source File

SOURCE=.\filters\conditional\conditional_reader.cpp
# End Source File
# Begin Source File

SOURCE=.\filters\conditional\conditional_reader.h
# End Source File
# End Group
# Begin Group "Other"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\filters\debug.cpp
# End Source File
# Begin Source File

SOURCE=.\filters\debug.h
# End Source File
# End Group
# End Group
# Begin Group "Sources"

# PROP Default_Filter ""
# Begin Group "Vdub AVI"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\sources\avi\AudioSource.cpp
# End Source File
# Begin Source File

SOURCE=.\sources\avi\AudioSource.h
# End Source File
# Begin Source File

SOURCE=.\sources\avi\AVIIndex.cpp
# End Source File
# Begin Source File

SOURCE=.\sources\avi\AVIIndex.h
# End Source File
# Begin Source File

SOURCE=.\sources\avi\AVIReadHandler.cpp
# End Source File
# Begin Source File

SOURCE=.\sources\avi\AVIReadHandler.h
# End Source File
# Begin Source File

SOURCE=.\sources\avi\clip_info.h
# End Source File
# Begin Source File

SOURCE=.\sources\avi\cpuaccel.cpp
# End Source File
# Begin Source File

SOURCE=.\sources\avi\DubSource.cpp
# End Source File
# Begin Source File

SOURCE=.\sources\avi\DubSource.h
# End Source File
# Begin Source File

SOURCE=.\sources\avi\FastReadStream.cpp
# End Source File
# Begin Source File

SOURCE=.\sources\avi\FastReadStream.h
# End Source File
# Begin Source File

SOURCE=.\sources\avi\File64.cpp
# End Source File
# Begin Source File

SOURCE=.\sources\avi\File64.h
# End Source File
# Begin Source File

SOURCE=.\sources\avi\Fixes.h
# End Source File
# Begin Source File

SOURCE=.\sources\avi\list.cpp
# End Source File
# Begin Source File

SOURCE=.\sources\avi\list.h
# End Source File
# Begin Source File

SOURCE=.\sources\avi\VD_Audio.cpp
# End Source File
# Begin Source File

SOURCE=.\sources\avi\VD_Audio.h
# End Source File
# Begin Source File

SOURCE=.\sources\avi\VD_misc.cpp
# End Source File
# Begin Source File

SOURCE=.\sources\avi\VD_misc.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\sources\avi_source.cpp
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\sources\ImageSeq.cpp
# End Source File
# Begin Source File

SOURCE=.\sources\ImageSeq.h
# End Source File
# Begin Source File

SOURCE=.\sources\msvcpXX.h
# End Source File
# Begin Source File

SOURCE=.\sources\source.cpp
# End Source File
# End Group
# Begin Group "Conversion"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\convert\convert.cpp
# End Source File
# Begin Source File

SOURCE=.\convert\convert.h
# End Source File
# Begin Source File

SOURCE=.\convert\convert_matrix.cpp
# End Source File
# Begin Source File

SOURCE=.\convert\convert_matrix.h
# End Source File
# Begin Source File

SOURCE=.\convert\convert_planar.cpp
# End Source File
# Begin Source File

SOURCE=.\convert\convert_planar.h
# End Source File
# Begin Source File

SOURCE=.\convert\convert_rgb.cpp
# End Source File
# Begin Source File

SOURCE=.\convert\convert_rgb.h
# End Source File
# Begin Source File

SOURCE=.\convert\convert_rgbtoy8.cpp
# End Source File
# Begin Source File

SOURCE=.\convert\convert_rgbtoy8.h
# End Source File
# Begin Source File

SOURCE=.\convert\convert_yuy2.cpp
# End Source File
# Begin Source File

SOURCE=.\convert\convert_yuy2.h
# End Source File
# Begin Source File

SOURCE=.\convert\convert_yuy2torgb.cpp
# End Source File
# Begin Source File

SOURCE=.\convert\convert_yuy2torgb.h
# End Source File
# Begin Source File

SOURCE=.\convert\convert_yv12.cpp
# End Source File
# Begin Source File

SOURCE=.\convert\convert_yv12.h
# End Source File
# End Group
# Begin Source File

SOURCE=..\distrib\AviSynth.ico
# End Source File
# Begin Source File

SOURCE=.\initguid.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\core\resource.h
# End Source File
# Begin Source File

SOURCE=.\stdafx.cpp
# ADD CPP /W3 /Yc"stdafx.h"
# End Source File
# Begin Source File

SOURCE=.\stdafx.h
# End Source File
# End Target
# End Project

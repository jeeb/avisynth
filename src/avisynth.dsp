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
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "AVISYNTH_EXPORTS" /YX /FD /c
# ADD CPP /nologo /G6 /MD /W3 /GX /Zi /O2 /Ob2 /D "NDEBUG" /D "INC_OLE2" /D "STRICT" /D "WIN32" /D "_WIN32" /D "_MT" /D "_DLL" /D "_MBCS" /D "_USRDLL" /D "AVISYNTH_EXPORTS" /Fr /Yu"stdafx.h" /FD /Gs /GF /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 ../distrib/lib/pfc.lib ../distrib/lib/devil.lib ../distrib/lib/softwire.lib msacm32.lib quartz.lib ddraw.lib amstrmid.lib vfw32.lib kernel32.lib advapi32.lib version.lib user32.lib gdi32.lib ole32.lib uuid.lib winmm.lib strmbase.lib oleaut32.lib /nologo /dll /debug /machine:I386
# SUBTRACT LINK32 /pdb:none
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
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "AVISYNTH_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /G6 /MDd /W3 /Gm /GX /ZI /Od /D "_DEBUG" /D "DEBUG" /D "INC_OLE2" /D "STRICT" /D "WIN32" /D "_WIN32" /D "_MT" /D "_DLL" /D "_MBCS" /D "_USRDLL" /D "AVISYNTH_EXPORTS" /Fr /Yu"stdafx.h" /FD /GZ /GF /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 ../distrib/lib/debug/pfc.lib ../distrib/lib/devil.lib ../distrib/lib/softwire.lib msacm32.lib quartz.lib ddraw.lib amstrmid.lib vfw32.lib kernel32.lib advapi32.lib version.lib user32.lib gdi32.lib ole32.lib uuid.lib winmm.lib strmbasd.lib oleaut32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# SUBTRACT LINK32 /pdb:none /incremental:no /nodefaultlib
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Cmds=copy Debug\avisynth.dll $(SystemRoot)\system32	copy ..\distrib\bin\debug\devil-d.dll $(SystemRoot)\system32
# End Special Build Tool

!ENDIF 

# Begin Target

# Name "avisynth - Win32 Release"
# Name "avisynth - Win32 Debug"
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

SOURCE=.\internal.h
# End Source File
# Begin Source File

SOURCE=.\core\main.cpp
# SUBTRACT CPP /YX /Yc /Yu
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

SOURCE=.\audio\dbesi0.c
# SUBTRACT CPP /YX /Yc /Yu
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

SOURCE=.\sources\directshow_source.cpp
# End Source File
# Begin Source File

SOURCE=.\sources\directshow_source.h
# End Source File
# Begin Source File

SOURCE=.\sources\ImageSeq.cpp
# End Source File
# Begin Source File

SOURCE=.\sources\ImageSeq.h
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

SOURCE=.\convert\convert_a.asm

!IF  "$(CFG)" == "avisynth - Win32 Release"

# PROP Ignore_Default_Tool 1
# Begin Custom Build - Assembling $(InputPath)...
IntDir=.\Release
InputPath=.\convert\convert_a.asm
InputName=convert_a

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	ml.exe -c -coff -Cx -Fo$(IntDir)\$(InputName).obj .\convert\$(InputName).asm

# End Custom Build

!ELSEIF  "$(CFG)" == "avisynth - Win32 Debug"

# PROP Ignore_Default_Tool 1
# Begin Custom Build - Assembling $(InputPath)...
IntDir=.\Debug
InputPath=.\convert\convert_a.asm
InputName=convert_a

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	ml.exe -c -coff -Cx -Zi -Fo$(IntDir)\$(InputName).obj .\convert\$(InputName).asm

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\convert\convert_xvid.cpp
# End Source File
# Begin Source File

SOURCE=.\convert\convert_xvid.h
# End Source File
# Begin Source File

SOURCE=.\convert\convert_yuy2.cpp
# End Source File
# Begin Source File

SOURCE=.\convert\convert_yuy2.h
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

SOURCE=.\stdafx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# Begin Source File

SOURCE=.\stdafx.h
# End Source File
# End Target
# End Project

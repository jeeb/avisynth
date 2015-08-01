# Microsoft Developer Studio Project File - Name="SoundTouch" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=SoundTouch - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "SoundTouch.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "SoundTouch.mak" CFG="SoundTouch - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "SoundTouch - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "SoundTouch - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE "SoundTouch - Win32 Relsym" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "SoundTouch - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\..\..\src\Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
F90=df.exe
MTL=midl.exe
# ADD BASE CPP /nologo /W4 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /G6 /MD /W4 /GX /Zi /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /FR /FD /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x40b /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "SoundTouch - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\..\..\src\Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
F90=df.exe
MTL=midl.exe
# ADD BASE CPP /nologo /W4 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /G6 /MDd /W4 /Gm /Gi /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /FR /FD /I /GZ /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x40b /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "SoundTouch - Win32 Relsym"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Relsym"
# PROP BASE Intermediate_Dir "Relsym"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\..\..\src\Relsym"
# PROP Intermediate_Dir "Relsym"
# PROP Target_Dir ""
F90=df.exe
MTL=midl.exe
# ADD BASE CPP /nologo /G6 /MD /W4 /GX /Zi /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /FR /FD /c
# SUBTRACT BASE CPP /YX
# ADD CPP /nologo /G6 /MD /W4 /GX /Zi /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /FAs /FR /FD /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ENDIF 

# Begin Target

# Name "SoundTouch - Win32 Release"
# Name "SoundTouch - Win32 Debug"
# Name "SoundTouch - Win32 Relsym"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\AAFilter.cpp
# End Source File
# Begin Source File

SOURCE=.\cpu_detect_x86_win.cpp
# End Source File
# Begin Source File

SOURCE=.\FIFOSampleBuffer.cpp
# End Source File
# Begin Source File

SOURCE=.\FIRFilter.cpp
# End Source File
# Begin Source File

SOURCE=.\InterpolateCubic.cpp
# End Source File
# Begin Source File

SOURCE=.\InterpolateLinear.cpp
# End Source File
# Begin Source File

SOURCE=.\InterpolateShannon.cpp
# End Source File
# Begin Source File

SOURCE=.\RateTransposer.cpp
# End Source File
# Begin Source File

SOURCE=.\SoundTouch.cpp
# End Source File
# Begin Source File

SOURCE=.\sse_optimized.cpp
# End Source File
# Begin Source File

SOURCE=.\TDStretch.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\AAFilter.h
# End Source File
# Begin Source File

SOURCE=.\cpu_detect.h
# End Source File
# Begin Source File

SOURCE=.\FIFOSampleBuffer.h
# End Source File
# Begin Source File

SOURCE=.\FIFOSamplePipe.h
# End Source File
# Begin Source File

SOURCE=.\FIRFilter.h
# End Source File
# Begin Source File

SOURCE=.\InterpolateCubic.h
# End Source File
# Begin Source File

SOURCE=.\InterpolateLinear.h
# End Source File
# Begin Source File

SOURCE=.\InterpolateShannon.h
# End Source File
# Begin Source File

SOURCE=.\RateTransposer.h
# End Source File
# Begin Source File

SOURCE=.\SoundTouch.h
# End Source File
# Begin Source File

SOURCE=.\STTypes.h
# End Source File
# Begin Source File

SOURCE=.\TDStretch.h
# End Source File
# End Group
# End Target
# End Project

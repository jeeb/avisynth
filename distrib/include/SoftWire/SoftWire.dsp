# Microsoft Developer Studio Project File - Name="SoftWire" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=SoftWire - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "SoftWire.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "SoftWire.mak" CFG="SoftWire - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "SoftWire - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "SoftWire - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE "SoftWire - Win32 Relsym" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "SoftWire - Win32 Release"

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
# ADD BASE CPP /nologo /W4 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /G6 /MD /W4 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /FR /FD /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x813 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "SoftWire - Win32 Debug"

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
# ADD BASE CPP /nologo /W4 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /G6 /MDd /W4 /Gm /Gi /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /FR /FD /GZ /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x813 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "SoftWire - Win32 Relsym"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "SoftWire___Win32_Relsym"
# PROP BASE Intermediate_Dir "SoftWire___Win32_Relsym"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\..\..\src\Relsym"
# PROP Intermediate_Dir "Relsym"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /G6 /MD /W4 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /FR /FD /c
# SUBTRACT BASE CPP /YX
# ADD CPP /nologo /G6 /MD /W4 /GX /Zi /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /FAs /FR /FD /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"SoftWire.lib"
# ADD LIB32 /nologo

!ENDIF 

# Begin Target

# Name "SoftWire - Win32 Release"
# Name "SoftWire - Win32 Debug"
# Name "SoftWire - Win32 Relsym"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\Assembler.cpp
# End Source File
# Begin Source File

SOURCE=.\CodeGenerator.cpp
# End Source File
# Begin Source File

SOURCE=.\Emulator.cpp
# End Source File
# Begin Source File

SOURCE=.\Encoding.cpp
# End Source File
# Begin Source File

SOURCE=.\Error.cpp
# End Source File
# Begin Source File

SOURCE=.\Instruction.cpp
# End Source File
# Begin Source File

SOURCE=.\InstructionSet.cpp
# End Source File
# Begin Source File

SOURCE=.\Linker.cpp
# End Source File
# Begin Source File

SOURCE=.\Loader.cpp
# End Source File
# Begin Source File

SOURCE=.\Operand.cpp
# End Source File
# Begin Source File

SOURCE=.\Optimizer.cpp
# End Source File
# Begin Source File

SOURCE=.\RegisterAllocator.cpp
# End Source File
# Begin Source File

SOURCE=.\Synthesizer.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\Assembler.hpp
# End Source File
# Begin Source File

SOURCE=.\CodeGenerator.hpp
# End Source File
# Begin Source File

SOURCE=.\Emulator.hpp
# End Source File
# Begin Source File

SOURCE=.\Encoding.hpp
# End Source File
# Begin Source File

SOURCE=.\Error.hpp
# End Source File
# Begin Source File

SOURCE=.\Instruction.hpp
# End Source File
# Begin Source File

SOURCE=.\InstructionSet.hpp
# End Source File
# Begin Source File

SOURCE=.\Intrinsics.hpp
# End Source File
# Begin Source File

SOURCE=.\Link.hpp
# End Source File
# Begin Source File

SOURCE=.\Linker.hpp
# End Source File
# Begin Source File

SOURCE=.\Loader.hpp
# End Source File
# Begin Source File

SOURCE=.\Operand.hpp
# End Source File
# Begin Source File

SOURCE=.\Optimizer.hpp
# End Source File
# Begin Source File

SOURCE=.\RegisterAllocator.hpp
# End Source File
# Begin Source File

SOURCE=.\String.hpp
# End Source File
# Begin Source File

SOURCE=.\Synthesizer.hpp
# End Source File
# End Group
# End Target
# End Project

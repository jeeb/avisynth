!packhdr tempfile.exe "upx --best --q tempfile.exe"

!DEFINE VERSION 2.5.5
!DEFINE DATE 270104

SetCompressor lzma
!include "MUI.nsh"
!include WinMessages.nsh
!include Sections.nsh
!define MUI_ICON   "avisynth.ico"
!define MUI_UNICON "un_avisynth.ico"
!define MUI_ABORTWARNING
;!define MUI_WELCOMEFINISHPAGE_BITMAP "${NSISDIR}\Welcome.bmp"
!define MUI_COMPONENTSPAGE_SMALLDESC
!define MUI_FINISHPAGE_LINK "Visit the AviSynth website for the latest news and support"
!define MUI_FINISHPAGE_LINK_LOCATION "http://www.avisynth.org/"
!define MUI_INSTFILESPAGE_COLORS "C5DEFB 000000"
!define MUI_INSTFILESPAGE_PROGRESSBAR "colored"

;Pages------------------------------

;  !insertmacro MUI_PAGE_WELCOME
  !insertmacro MUI_PAGE_LICENSE "GPL.txt"
  !insertmacro MUI_PAGE_COMPONENTS
  !insertmacro MUI_PAGE_DIRECTORY
  !insertmacro MUI_PAGE_INSTFILES
  !insertmacro MUI_PAGE_FINISH
  !insertmacro MUI_UNPAGE_CONFIRM
  !insertmacro MUI_UNPAGE_INSTFILES
  
;----------------------------------

!insertmacro MUI_LANGUAGE "English"
NAME "AviSynth"
BRANDINGTEXT "AviSynth ${VERSION} -- [${DATE}]"
OutFile "AviSynth_${DATE}.exe"
SetOverwrite ON
Caption "AviSynth ${VERSION}"
SetOverwrite try
ShowInstDetails nevershow
CRCCheck ON

ComponentText "AviSynth - the premiere frameserving tool available today.$\nCopyright © 2000 - 2004."

InstallDir "$PROGRAMFILES\AviSynth 2.5"
InstallDirRegKey HKLM SOFTWARE\AviSynth ""

InstType Standard

Section "!AviSynth Base (required)" Frameserving
SectionIn RO 

ClearErrors
  SetOutPath $SYSDIR
  File "..\src\release\AviSynth.dll"
	File "bin\devil.dll"
	File "bin\avisynth_c.dll"

IfFileExists "$SYSDIR\msvcp60.dll" msvc60_exists
  File "bin\msvcp60.dll"
msvc60_exists:

IfErrors dll_not_ok

  SetOutPath $INSTDIR
  File "GPL.txt"

  WriteRegStr HKLM "SOFTWARE\AviSynth" "" "$INSTDIR"

  ReadRegStr $0 HKEY_LOCAL_MACHINE "SOFTWARE\AviSynth" "plugindir2_5"
	StrCmp "$0" "" No_Plugin_exists Plugin_exists
No_Plugin_exists:
	CreateDirectory "$INSTDIR\plugins"
	StrCpy $0 "$INSTDIR\plugins"
Plugin_exists:
ClearErrors

  SetOutPath $0
  File "..\src\plugins\DirectShowSource\Release\DirectShowSource.dll"

  WriteRegStr HKEY_LOCAL_MACHINE "SOFTWARE\AviSynth" "plugindir2_5" "$0"
  WriteRegStr HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\AviSynth" "DisplayName" "AviSynth 2.5"
  WriteRegStr HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\AviSynth" "UninstallString" '"$INSTDIR\Uninstall.exe"' 
  WriteRegStr HKLM "SOFTWARE\Classes\.avs" "" "avsfile"
  WriteRegStr HKCR ".avs" "" "avs_auto_file"
  WriteRegStr HKCR "CLSID\{E6D6B700-124D-11D4-86F3-DB80AFD98778}" "" "AviSynth"
  WriteRegStr HKCR "CLSID\{E6D6B700-124D-11D4-86F3-DB80AFD98778}\InProcServer32" "" AviSynth.dll   
  WriteRegStr HKCR "avifile\Extensions\avs" "" "{E6D6B700-124D-11D4-86F3-DB80AFD98778}"
  WriteRegStr HKCR "Media Type\Extensions\.avs" "" ""
  WriteRegStr HKCR "Media Type\Extensions\.avs" "Source Filter" "{D3588AB0-0781-11CE-B03A-0020AF0BA770}"
  WriteRegStr HKCR "CLSID\{E6D6B700-124D-11D4-86F3-DB80AFD98778}\InProcServer32" "ThreadingModel" "Apartment"
  WriteRegStr HKCR ".avs" "" "avsfile"
  WriteRegStr HKCR "avsfile" "" "AviSynth Script"
  WriteRegStr HKCR "avsfile\DefaultIcon" "" $SYSDIR\AviSynth.dll,0
IfErrors reg_not_ok
  goto reg_ok
reg_not_ok:
  MessageBox MB_OK "You need administrator rights to install AviSynth!\r\n\r\n(Could not write to registry)"
  Abort
reg_ok:

CreateDirectory  "$SMPROGRAMS\AviSynth 2.5"
  
  CreateShortCut "$SMPROGRAMS\AviSynth 2.5\Uninstall AviSynth.lnk" "$INSTDIR\Uninstall.exe"
  CreateShortCut "$SMPROGRAMS\AviSynth 2.5\License.lnk" "$INSTDIR\GPL.txt"
  CreateShortCut "$SMPROGRAMS\AviSynth 2.5\Plugin Directory.lnk" "$INSTDIR\Plugins"
  WriteINIStr    "$SMPROGRAMS\AviSynth 2.5\AviSynth Online.url" "InternetShortcut" "URL" "http://www.avisynth.org"
  WriteINIStr    "$SMPROGRAMS\AviSynth 2.5\Download Plugins.url" "InternetShortcut" "URL" "http://www.avisynth.org/warpenterprises/"

 Delete $INSTDIR\Uninstall.exe 
  WriteUninstaller $INSTDIR\Uninstall.exe
  goto dll_ok
dll_not_ok:
  MessageBox MB_OK "Could not copy avisynth.dll to system directory - Close down all applications that use Avisynth, and sure to have write permission to the system directory, and try again."
  Abort
dll_ok:


Delete $INSTDIR\Uninstall.exe 
WriteUninstaller $INSTDIR\Uninstall.exe


SectionEnd

Subsection "Documentation" Documentation

Section "English Documentation" English
SectionIn 1

  SetOutPath $INSTDIR\Docs
  File "..\Docs\*.*"
  SetOutPath $INSTDIR\Docs\english
  File "..\Docs\english\*.*"
  SetOutPath $INSTDIR\Docs\english\corefilters
  File "..\Docs\english\corefilters\*.*"
  SetOutPath $INSTDIR\Docs\english\externalfilters
  File "..\Docs\english\externalfilters\*.*"
  SetOutPath $INSTDIR\Docs\pictures\corefilters
  File "..\Docs\pictures\corefilters\*.*"
  SetOutPath $INSTDIR\Docs\pictures\externalfilters
  File "..\Docs\pictures\externalfilters\*.*"

  SetOutPath $INSTDIR\Examples
  File "Examples\*.*"

  CreateShortCut "$SMPROGRAMS\AviSynth 2.5\AviSynth Documentation.lnk" "$INSTDIR\Docs\english\index.htm"
  CreateShortCut "$SMPROGRAMS\AviSynth 2.5\Example Scripts.lnk" "$INSTDIR\Examples"
SectionEnd


Section /o "German Documentation" German
  SetOutPath $INSTDIR\Docs
  File "..\Docs\*.*"
  SetOutPath $INSTDIR\Docs\german
  File "..\Docs\german\*.*"
  SetOutPath $INSTDIR\Docs\german\corefilters
  File "..\Docs\german\corefilters\*.*"
  SetOutPath $INSTDIR\Docs\german\externalfilters
  File "..\Docs\german\externalfilters\*.*"
  SetOutPath $INSTDIR\Docs\pictures\corefilters
  File "..\Docs\pictures\corefilters\*.*"
  SetOutPath $INSTDIR\Docs\pictures\externalfilters
  File "..\Docs\pictures\externalfilters\*.*"


  SetOutPath $INSTDIR\Examples
  File "Examples\*.*"
  
CreateShortCut "$SMPROGRAMS\AviSynth 2.5\Deutsche AviSynth Dokumentation.lnk" "$INSTDIR\Docs\german\index.htm"

SectionEnd

Subsectionend


SubSection /e "Select Association" SelectAssociation

Section /o "Associate AVS files with Notepad" Associate1
WriteRegStr HKCR "avsfile\shell\open\command" "" 'notepad.exe "%1"'
SectionEnd

Section /o "Associate AVS files with Media Player 6.4" Associate2
WriteRegStr HKCR "avsfile\shell\open\command" "" 'mplayer2.exe "%1"'
WriteRegStr HKCR "avsfile\shell\open\command" "" '$PROGRAMFILES\Windows Media Player\mplayer2.exe "%1"'
SectionEnd

Section ""

SectionEnd

SubSectionEnd




  !insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN    
  !insertmacro MUI_DESCRIPTION_TEXT  ${Frameserving} "Install the main files for frameserving via AviSynth"
  !insertmacro MUI_DESCRIPTION_TEXT  ${Documentation} "Install help and example files"
  !insertmacro MUI_DESCRIPTION_TEXT  ${English} "Install English help and example files"
  !insertmacro MUI_DESCRIPTION_TEXT  ${German} "Install German help and example files"
  !insertmacro MUI_DESCRIPTION_TEXT  ${SelectAssociation} "Select only ONE of these for association!"
  !insertmacro MUI_DESCRIPTION_TEXT  ${Associate1} "Open AVS files directly with Notepad"
  !insertmacro MUI_DESCRIPTION_TEXT  ${Associate2} "Open AVS files directly with Media Player 6.4"
  !insertmacro MUI_FUNCTION_DESCRIPTION_END
	

Function un.onUninstSuccess
    MessageBox MB_OK "Uninstall has been successfully completed."
  FunctionEnd
                                                
Section "Uninstall"
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\AviSynth"
  Delete "$SYSDIR\devil.dll"
  Delete "$SYSDIR\AviSynth.dll"
  DeleteRegKey HKLM "Software\Classes\avs"
  DeleteRegKey HKCR ".avs"
  DeleteRegKey HKCR "CLSID\{E6D6B700-124D-11D4-86F3-DB80AFD98778}"
  DeleteRegKey HKCR "Media Type\Extensions\.avs"
  DeleteRegKey HKCR "avifile\Extensions\avs"
  DeleteRegKey HKCR "avsfile\DefaultIcon"
  DeleteRegValue HKCR "avsfile\shell\open\command 'notepad.exe "%1"'"
  DeleteRegValue HKCR "avsfile\shell\open\command 'mplayer2.exe "%1"'"
  Delete "$SMPROGRAMS\AviSynth 2.5\*.*"
  RMDir  "$SMPROGRAMS\AviSynth 2.5"
  Delete "$INSTDIR\GPL.txt"
  Delete "$INSTDIR\Examples\*.*"
  RMDir  "$INSTDIR\Examples"

  Delete "$INSTDIR\Docs\English\corefilters\*.*"
  RMDir  "$INSTDIR\Docs\English\corefilters"
  Delete "$INSTDIR\Docs\English\externalfilters\*.*"
  RMDir  "$INSTDIR\Docs\English\externalfilters"
  Delete "$INSTDIR\Docs\English\*.*"
  RMDir  "$INSTDIR\Docs\English"

  Delete "$INSTDIR\Docs\German\corefilters\*.*"
  RMDir  "$INSTDIR\Docs\German\corefilters"
  Delete "$INSTDIR\Docs\German\externalfilters\*.*"
  RMDir  "$INSTDIR\Docs\German\externalfilters"
  Delete "$INSTDIR\Docs\German\*.*"
  RMDir  "$INSTDIR\Docs\German"

  Delete "$INSTDIR\Docs\Pictures\corefilters\*.*"
  Delete "$INSTDIR\Docs\Pictures\externalfilters\*.*"
  Delete "$INSTDIR\Docs\Pictures\*.*"
  RMDir  "$INSTDIR\Docs\Pictures\corefilters"
  RMDir  "$INSTDIR\Docs\Pictures\externalfilters"
  RMDir  "$INSTDIR\Docs\Pictures"
  Delete "$INSTDIR\Docs\*.*"
  RMDir  "$INSTDIR\Docs"
  Delete "$INSTDIR\Uninstall.exe"

IfFileExists $INSTDIR 0 Removed
    MessageBox MB_YESNO|MB_ICONQUESTION \
      "Do you want to remove pointer to plugin directory (no files will be removed)?" IDNO Removed
    DeleteRegKey HKLM "Software\AviSynth"
  Removed:

SectionEnd
!packhdr tempfile.exe "upx --best --q tempfile.exe"

!DEFINE VERSION 2.5.6
!DEFINE DATE 120705

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

ComponentText "AviSynth - the premiere frameserving tool available today.$\nCopyright © 2000 - 2005."

InstallDir "$PROGRAMFILES\AviSynth 2.5"
InstallDirRegKey HKLM SOFTWARE\AviSynth ""

InstType Standard

Section "!AviSynth Base (required)" Frameserving
SectionIn RO

ClearErrors
  SetOutPath $SYSDIR
  File "..\src\release\AviSynth.dll"
  File "bin\devil.dll"

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
  File "..\src\plugins\TCPDeliver\Release\TCPDeliver.dll"
  File "color_presets\colors_rgb.avsi"

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

SetShellVarContext Current
CreateDirectory  "$SMPROGRAMS\AviSynth 2.5"
  CreateShortCut "$SMPROGRAMS\AviSynth 2.5\Uninstall AviSynth.lnk" "$INSTDIR\Uninstall.exe"

SetShellVarContext All
CreateDirectory  "$SMPROGRAMS\AviSynth 2.5"
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

SetOutPath $INSTDIR\Examples
File "Examples\*.*"
CreateShortCut "$SMPROGRAMS\AviSynth 2.5\Example Scripts.lnk" "$INSTDIR\Examples"

Delete $INSTDIR\Uninstall.exe
WriteUninstaller $INSTDIR\Uninstall.exe


SectionEnd

Subsection "Documentation" Documentation

Section "English Documentation" English
SectionIn 1

  SetOutPath $INSTDIR\Docs
  File "..\..\Docs\*.css"
  SetOutPath $INSTDIR\Docs\english
  File "..\..\Docs\english\*.*"
  SetOutPath $INSTDIR\Docs\english\advancedtopics
  File "..\..\Docs\english\advancedtopics\*.*"
  SetOutPath $INSTDIR\Docs\english\corefilters
  File "..\..\Docs\english\corefilters\*.*"
  SetOutPath $INSTDIR\Docs\english\externalfilters
  File "..\..\Docs\english\externalfilters\*.*"
  SetOutPath $INSTDIR\Docs\english\pictures\advancedtopics
  File "..\..\Docs\english\pictures\advancedtopics\*.*"
  SetOutPath $INSTDIR\Docs\english\pictures\corefilters
  File "..\..\Docs\english\pictures\corefilters\*.*"
  SetOutPath $INSTDIR\Docs\english\pictures\externalfilters
  File "..\..\Docs\english\pictures\externalfilters\*.*"
  
  SetOutPath $INSTDIR\Examples
  File "Examples\*.*"

SetShellVarContext All
CreateShortCut "$SMPROGRAMS\AviSynth 2.5\AviSynth Documentation.lnk" "$INSTDIR\Docs\english\index.htm"

SectionEnd


Section /o "German Documentation" German
  SetOutPath $INSTDIR\Docs
  File "..\..\Docs\*.css"
  SetOutPath $INSTDIR\Docs\german
  File "..\..\Docs\german\*.*"
  SetOutPath $INSTDIR\Docs\german\corefilters
  File "..\..\Docs\german\corefilters\*.*"
  SetOutPath $INSTDIR\Docs\german\externalfilters
  File "..\..\Docs\german\externalfilters\*.*"

SetShellVarContext All
CreateShortCut "$SMPROGRAMS\AviSynth 2.5\Deutsche AviSynth Dokumentation.lnk" "$INSTDIR\Docs\german\index.htm"

SectionEnd

Section /o "French Documentation" French
  SetOutPath $INSTDIR\Docs
  File "..\..\Docs\*.css"
  SetOutPath $INSTDIR\Docs\french
  File "..\..\Docs\french\*.*"
  SetOutPath $INSTDIR\Docs\french\corefilters
  File "..\..\Docs\french\corefilters\*.*"
  SetOutPath $INSTDIR\Docs\french\externalfilters
  File "..\..\Docs\french\externalfilters\*.*"

SetShellVarContext All
CreateShortCut "$SMPROGRAMS\AviSynth 2.5\French AviSynth Documentation.lnk" "$INSTDIR\Docs\french\index.htm"

SectionEnd

Section /o "Italian Documentation" Italian
  SetOutPath $INSTDIR\Docs
  File "..\..\Docs\*.css"
  SetOutPath $INSTDIR\Docs\italian
  File "..\..\Docs\italian\*.*"
  SetOutPath $INSTDIR\Docs\italian\corefilters
  File "..\..\Docs\italian\corefilters\*.*"
  SetOutPath $INSTDIR\Docs\italian\externalfilters
  File "..\..\Docs\italian\externalfilters\*.*"
  SetOutPath $INSTDIR\Docs\italian\pictures\corefilters
  File "..\..\Docs\italian\pictures\corefilters\*.*"

SetShellVarContext All
CreateShortCut "$SMPROGRAMS\AviSynth 2.5\Italian AviSynth Documentation.lnk" "$INSTDIR\Docs\italian\index.htm"

SectionEnd

Section /o "Russian Documentation" Russian
  SetOutPath $INSTDIR\Docs
  File "..\..\Docs\*.css"
  SetOutPath $INSTDIR\Docs\russian
  File "..\..\Docs\russian\*.*"
  SetOutPath $INSTDIR\Docs\russian\advancedtopics
  File "..\..\Docs\russian\advancedtopics\*.*"
  SetOutPath $INSTDIR\Docs\russian\corefilters
  File "..\..\Docs\russian\corefilters\*.*"
  SetOutPath $INSTDIR\Docs\russian\externalfilters
  File "..\..\Docs\russian\externalfilters\*.*"
  SetOutPath $INSTDIR\Docs\russian\pictures\advancedtopics
  File "..\..\Docs\russian\pictures\advancedtopics\*.*"
  SetOutPath $INSTDIR\Docs\russian\pictures\corefilters
  File "..\..\Docs\russian\pictures\corefilters\*.*"
  SetOutPath $INSTDIR\Docs\russian\pictures\externalfilters
  File "..\..\Docs\russian\pictures\externalfilters\*.*"

SetShellVarContext All
CreateShortCut "$SMPROGRAMS\AviSynth 2.5\Russian AviSynth Documentation.lnk" "$INSTDIR\Docs\russian\index.htm"

SectionEnd

Subsectionend


SubSection /e "Select Association" SelectAssociation

Section /o "Associate AVS files with Notepad (open)" Associate1
WriteRegStr HKCR "avsfile\shell\open\command" "" 'notepad.exe "%1"'
SectionEnd

Section /o "Associate AVS files with Media Player 6.4 (play)" Associate2
WriteRegStr HKCR "avsfile\shell\play\command" "" '"$PROGRAMFILES\Windows Media Player\mplayer2.exe" /Play "%L"'
SectionEnd

Section ""

SectionEnd

SubSectionEnd


  !insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
  !insertmacro MUI_DESCRIPTION_TEXT  ${Frameserving} "Install the main files for frameserving via AviSynth"
  !insertmacro MUI_DESCRIPTION_TEXT  ${Documentation} "Install help"
  !insertmacro MUI_DESCRIPTION_TEXT  ${English} "Install English help"
  !insertmacro MUI_DESCRIPTION_TEXT  ${German} "Install German help"
  !insertmacro MUI_DESCRIPTION_TEXT  ${French} "Install French help"
  !insertmacro MUI_DESCRIPTION_TEXT  ${Italian} "Install Italian help"
  !insertmacro MUI_DESCRIPTION_TEXT  ${Russian} "Install Russian help"
  !insertmacro MUI_DESCRIPTION_TEXT  ${SelectAssociation} "Select one or both associations"
  !insertmacro MUI_DESCRIPTION_TEXT  ${Associate1} "Open AVS files directly with Notepad to edit"
  !insertmacro MUI_DESCRIPTION_TEXT  ${Associate2} "Play AVS files directly with Media Player 6.4 (right click - play)"
  !insertmacro MUI_FUNCTION_DESCRIPTION_END


Function un.onUninstSuccess
    MessageBox MB_OK "Uninstall has been successfully completed."
  FunctionEnd

Section "Uninstall"
  Delete "$SYSDIR\devil.dll"
  Delete "$SYSDIR\AviSynth.dll"
  DeleteRegKey HKLM "Software\Classes\avs"
  DeleteRegKey HKCR ".avs"
  DeleteRegKey HKCR "CLSID\{E6D6B700-124D-11D4-86F3-DB80AFD98778}"
  DeleteRegKey HKCR "Media Type\Extensions\.avs"
  DeleteRegKey HKCR "avifile\Extensions\avs"
  DeleteRegKey HKCR "avsfile\DefaultIcon"
  DeleteRegValue HKCR "avsfile\shell\open\command"
  DeleteRegValue HKCR "avsfile\shell\play\command"

  SetShellVarContext All
  Delete "$SMPROGRAMS\AviSynth 2.5\*.*"
  RMDir  "$SMPROGRAMS\AviSynth 2.5"

  SetShellVarContext Current
  Delete "$SMPROGRAMS\AviSynth 2.5\*.*"
  RMDir  "$SMPROGRAMS\AviSynth 2.5"

  Delete "$INSTDIR\GPL.txt"
  Delete "$INSTDIR\Examples\*.*"
  RMDir  "$INSTDIR\Examples"

  Delete "$INSTDIR\plugins\DirectShowSource.dll"
  Delete "$INSTDIR\plugins\TCPDeliver.dll"

  Delete "$INSTDIR\Docs\english\advancedtopics\*.*"
  RMDir  "$INSTDIR\Docs\english\advancedtopics"
  Delete "$INSTDIR\Docs\english\corefilters\*.*"
  RMDir  "$INSTDIR\Docs\english\corefilters"
  Delete "$INSTDIR\Docs\english\externalfilters\*.*"
  RMDir  "$INSTDIR\Docs\english\externalfilters"
  Delete "$INSTDIR\Docs\english\pictures\advancedtopics\*.*"
  RMDir  "$INSTDIR\Docs\english\pictures\advancedtopics"
  Delete "$INSTDIR\Docs\english\pictures\corefilters\*.*"
  RMDir  "$INSTDIR\Docs\english\pictures\corefilters"
  Delete "$INSTDIR\Docs\english\pictures\externalfilters\*.*"
  RMDir  "$INSTDIR\Docs\english\pictures\externalfilters"
  RMDir  "$INSTDIR\Docs\english\pictures"
  Delete "$INSTDIR\Docs\english\*.*"
  RMDir  "$INSTDIR\Docs\english"

  Delete "$INSTDIR\Docs\german\corefilters\*.*"
  RMDir  "$INSTDIR\Docs\german\corefilters"
  Delete "$INSTDIR\Docs\german\externalfilters\*.*"
  RMDir  "$INSTDIR\Docs\german\externalfilters"
  Delete "$INSTDIR\Docs\german\*.*"
  RMDir  "$INSTDIR\Docs\german"

  Delete "$INSTDIR\Docs\french\corefilters\*.*"
  RMDir  "$INSTDIR\Docs\french\corefilters"
  Delete "$INSTDIR\Docs\french\externalfilters\*.*"
  RMDir  "$INSTDIR\Docs\french\externalfilters"
  Delete "$INSTDIR\Docs\french\*.*"
  RMDir  "$INSTDIR\Docs\french"

  Delete "$INSTDIR\Docs\italian\corefilters\*.*"
  RMDir  "$INSTDIR\Docs\italian\corefilters"
  Delete "$INSTDIR\Docs\italian\externalfilters\*.*"
  RMDir  "$INSTDIR\Docs\italian\externalfilters"
  Delete "$INSTDIR\Docs\italian\pictures\corefilters\*.*"
  RMDir  "$INSTDIR\Docs\italian\pictures\corefilters"
  RMDir  "$INSTDIR\Docs\italian\pictures"
  Delete "$INSTDIR\Docs\italian\*.*"
  RMDir  "$INSTDIR\Docs\italian"

  Delete "$INSTDIR\Docs\russian\advancedtopics\*.*"
  RMDir  "$INSTDIR\Docs\russian\advancedtopics"
  Delete "$INSTDIR\Docs\russian\corefilters\*.*"
  RMDir  "$INSTDIR\Docs\russian\corefilters"
  Delete "$INSTDIR\Docs\russian\externalfilters\*.*"
  RMDir  "$INSTDIR\Docs\russian\externalfilters"
  Delete "$INSTDIR\Docs\russian\pictures\advancedtopics\*.*"
  RMDir  "$INSTDIR\Docs\russian\pictures\advancedtopics"
  Delete "$INSTDIR\Docs\russian\pictures\corefilters\*.*"
  RMDir  "$INSTDIR\Docs\russian\pictures\corefilters"
  Delete "$INSTDIR\Docs\russian\pictures\externalfilters\*.*"
  RMDir  "$INSTDIR\Docs\russian\pictures\externalfilters"
  RMDir  "$INSTDIR\Docs\russian\pictures"
  Delete "$INSTDIR\Docs\russian\*.*"
  RMDir  "$INSTDIR\Docs\russian"

  Delete "$INSTDIR\Docs\*.*"
  RMDir  "$INSTDIR\Docs"
  Delete "$INSTDIR\Uninstall.exe"

  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\AviSynth"

IfFileExists $INSTDIR 0 Removed
    MessageBox MB_YESNO|MB_ICONQUESTION \
      "Do you want to remove the registry pointer to plugin directory (no files will be removed)?" IDNO Removed
    DeleteRegKey HKLM "Software\AviSynth"
  Removed:

SectionEnd

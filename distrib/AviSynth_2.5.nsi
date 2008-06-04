!packhdr tempfile.exe "upx --best --q tempfile.exe"

!DEFINE VERSION 2.5.8
!DEFINE /date DATE "%y%m%d"

SetCompressor /solid lzma


!include "MUI.nsh"
!include WinMessages.nsh
!include Sections.nsh
!define MUI_ICON   "avisynth.ico"
!define MUI_UNICON "un_avisynth.ico"
!define MUI_ABORTWARNING
;!define MUI_WELCOMEFINISHPAGE_BITMAP "${NSISDIR}\Welcome.bmp"
!define MUI_COMPONENTSPAGE_SMALLDESC
!define MUI_UI_COMPONENTSPAGE_SMALLDESC "AVS_UI.exe"
!define MUI_FINISHPAGE_LINK $(FINISHPAGE_TEXT)
!define MUI_FINISHPAGE_LINK_LOCATION "http://www.avisynth.org/"
!define MUI_INSTFILESPAGE_COLORS "C5DEFB 000000"
!define MUI_INSTFILESPAGE_PROGRESSBAR "colored"
!define MUI_FINISHPAGE_NOAUTOCLOSE
!define MUI_UNFINISHPAGE_NOAUTOCLOSE
!define MUI_LANGDLL_ALLLANGUAGES

;----------------------------------

; Tweak the finish page paremters -- Max out the MUI_FINISHPAGE_LINK field width & make 2 high.

  Function AVS_Finish_Pre

    !insertmacro MUI_INSTALLOPTIONS_WRITE "ioSpecial.ini" "Field 3" "Bottom" "165" ; 175
    !insertmacro MUI_INSTALLOPTIONS_WRITE "ioSpecial.ini" "Field 4" "Top"    "165" ; 175
;   !insertmacro MUI_INSTALLOPTIONS_WRITE "ioSpecial.ini" "Field 4" "Left"   "120" ; 120
    !insertmacro MUI_INSTALLOPTIONS_WRITE "ioSpecial.ini" "Field 4" "Right"  "330" ; 315
;   !insertmacro MUI_INSTALLOPTIONS_WRITE "ioSpecial.ini" "Field 3" "Bottom" "185" ; 185

  FunctionEnd

;Pages------------------------------

;  !insertmacro MUI_PAGE_WELCOME
  !insertmacro MUI_PAGE_LICENSE "GPL.txt"
  !insertmacro MUI_PAGE_COMPONENTS
  !insertmacro MUI_PAGE_DIRECTORY
  !insertmacro MUI_PAGE_INSTFILES

  !define      MUI_PAGE_CUSTOMFUNCTION_PRE AVS_Finish_Pre
  !insertmacro MUI_PAGE_FINISH

  !insertmacro MUI_UNPAGE_CONFIRM
  !insertmacro MUI_UNPAGE_INSTFILES


;----------------------------------
;Languages-------------------------
;----------------------------------
;
!macro AVS_LANGUAGE LANGUAGE
  !insertmacro MUI_LANGUAGE ${LANGUAGE}
;
  !define AvsLang ${LANG_${LANGUAGE}}
  !include "Languages\AVS_${LANGUAGE}.nsh"
  !undef AvsLang
!macroend
;
;----------------------------------

!insertmacro AVS_LANGUAGE "English"    ; 1033

!insertmacro AVS_LANGUAGE "Czech"      ; 1029
!insertmacro AVS_LANGUAGE "German"     ; 1031
!insertmacro AVS_LANGUAGE "Greek"      ; 1253
!insertmacro AVS_LANGUAGE "French"     ; 1036
; !insertmacro AVS_LANGUAGE "Italian"    ; 1040
!insertmacro AVS_LANGUAGE "Japanese"   ; 1041
; !insertmacro AVS_LANGUAGE "Portuguese" ; 2070
!insertmacro AVS_LANGUAGE "Russian"    ; 1049

!insertmacro MUI_RESERVEFILE_LANGDLL


;----------------------------------


NAME "AviSynth"
BRANDINGTEXT "AviSynth ${VERSION} -- [${DATE}]"
OutFile "AviSynth_${DATE}.exe"
SetOverwrite ON
Caption "AviSynth ${VERSION}"
ShowInstDetails show
CRCCheck ON

ComponentText $(COMPONENT_TEXT)

InstallDir "$PROGRAMFILES\AviSynth 2.5"
InstallDirRegKey HKLM SOFTWARE\AviSynth ""

InstType $(AVS_Standard)
InstType $(AVS_Minimal)
InstType $(AVS_Standalone)
InstType $(AVS_Full)

Var AdminInstall

Subsection "!$(Frameserving_Text)" Frameserving

Section $(SystemInstall_Text) SystemInstall
  SectionIn 1 2 4 RO

  StrCpy $AdminInstall "Yes"

  ClearErrors
  SetOutPath $SYSDIR
  File "..\src\release\AviSynth.dll"
  File "bin\devil.dll"

IfFileExists "$SYSDIR\msvcp60.dll" msvc60_exists
  File "bin\msvcp60.dll"
msvc60_exists:

IfErrors 0 dll_ok
  MessageBox MB_OK $(InUseMsg_Text)
  Abort

dll_ok:
  SetOutPath $INSTDIR
  File "GPL.txt"
  File "lgpl_for_used_libs.txt"

  ReadRegStr $0 HKLM "SOFTWARE\AviSynth" "plugindir2_5"
StrCmp "$0" "" 0 Plugin_exists
  CreateDirectory "$INSTDIR\plugins"
  StrCpy $0 "$INSTDIR\plugins"

Plugin_exists:
  ClearErrors
  SetOutPath $0
  File "..\src\plugins\DirectShowSource\Release\DirectShowSource.dll"
  File "..\src\plugins\TCPDeliver\Release\TCPDeliver.dll"
  File "color_presets\colors_rgb.avsi"

IfErrors 0 plug_ok
  MessageBox MB_OK $(PlugDir_Text)
  Abort

plug_ok:
  ClearErrors
  WriteRegStr HKLM "SOFTWARE\AviSynth" "" "$INSTDIR"
  WriteRegStr HKLM "SOFTWARE\AviSynth" "plugindir2_5" "$0"

  WriteRegStr HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\AviSynth" "DisplayName" "AviSynth 2.5"
  WriteRegStr HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\AviSynth" "UninstallString" '"$INSTDIR\Uninstall.exe"'

  WriteRegStr HKLM "SOFTWARE\Classes\.avs" "" "avsfile"

IfErrors 0 mreg_ok
  MessageBox MB_OK $(AdminRightsHKLM_Text)
  Abort

mreg_ok:
  ClearErrors
  WriteRegStr HKCR "CLSID\{E6D6B700-124D-11D4-86F3-DB80AFD98778}" "" "AviSynth"
  WriteRegStr HKCR "CLSID\{E6D6B700-124D-11D4-86F3-DB80AFD98778}\InProcServer32" "" "AviSynth.dll"
  WriteRegStr HKCR "CLSID\{E6D6B700-124D-11D4-86F3-DB80AFD98778}\InProcServer32" "ThreadingModel" "Apartment"

  WriteRegStr HKCR "Media Type\Extensions\.avs" "" ""
  WriteRegStr HKCR "Media Type\Extensions\.avs" "Source Filter" "{D3588AB0-0781-11CE-B03A-0020AF0BA770}"

  WriteRegStr HKCR ".avs" "" "avsfile"
  WriteRegStr HKCR "avsfile" "" "AviSynth Script"
  WriteRegStr HKCR "avsfile\DefaultIcon" "" "$SYSDIR\AviSynth.dll,0"

  WriteRegStr HKCR ".avsi" "" "avs_auto_file"
  WriteRegStr HKCR "avs_auto_file" "" "AviSynth Autoload Script"
  WriteRegStr HKCR "avs_auto_file\DefaultIcon" "" "$SYSDIR\AviSynth.dll,0"

  WriteRegStr HKCR "avifile\Extensions\AVS" "" "{E6D6B700-124D-11D4-86F3-DB80AFD98778}"

IfErrors 0 creg_ok
  MessageBox MB_OK $(AdminRightsHKCR_Text)
  Abort

creg_ok:
; These bits are for the administrator only
  SetShellVarContext Current
  CreateDirectory  "$SMPROGRAMS\AviSynth 2.5"
  CreateShortCut "$SMPROGRAMS\AviSynth 2.5\Uninstall AviSynth.lnk" "$INSTDIR\Uninstall.exe"

; These bits are for everybody
  SetShellVarContext All
  CreateDirectory  "$SMPROGRAMS\AviSynth 2.5"
  CreateShortCut "$SMPROGRAMS\AviSynth 2.5\License.lnk" "$INSTDIR\GPL.txt"
  CreateShortCut "$SMPROGRAMS\AviSynth 2.5\Plugin Directory.lnk" "$INSTDIR\Plugins"
  WriteINIStr    "$SMPROGRAMS\AviSynth 2.5\AviSynth Online.url" "InternetShortcut" "URL" "http://www.avisynth.org"
  WriteINIStr    "$SMPROGRAMS\AviSynth 2.5\Download Plugins.url" "InternetShortcut" "URL" "http://www.avisynth.org/warpenterprises/"

  SetOutPath $INSTDIR\Examples
  File "Examples\*.*"
  CreateShortCut "$SMPROGRAMS\AviSynth 2.5\Example Scripts.lnk" "$INSTDIR\Examples"

  Delete $INSTDIR\Uninstall.exe
  WriteUninstaller $INSTDIR\Uninstall.exe

SectionEnd

Section /o  $(StandAlone_Text) StandAlone
SectionIn 3 RO

  StrCpy $AdminInstall "No"

  ClearErrors
  SetOutPath $INSTDIR
  File "..\src\release\AviSynth.dll"
  File "bin\devil.dll"
  File "bin\msvcp60.dll"

  File "Avisynth_Template.reg"

  File "GPL.txt"
  File "lgpl_for_used_libs.txt"

  SetOutPath "$INSTDIR\plugins"
  File "..\src\plugins\DirectShowSource\Release\DirectShowSource.dll"
  File "..\src\plugins\TCPDeliver\Release\TCPDeliver.dll"
  File "color_presets\colors_rgb.avsi"

SectionEnd

Subsectionend


Subsection  $(Documentation_Text) Documentation

Section /o  $(English_Text) English
SectionIn 1 3 4

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
  StrCmp $AdminInstall "No" +2
  CreateShortCut "$SMPROGRAMS\AviSynth 2.5\AviSynth Documentation.lnk" "$INSTDIR\Docs\english\index.htm"

SectionEnd


Section /o  $(German_Text) German
  SectionIn 4
  SetOutPath $INSTDIR\Docs
  File "..\..\Docs\*.css"
  SetOutPath $INSTDIR\Docs\german
  File "..\..\Docs\german\*.*"
  SetOutPath $INSTDIR\Docs\german\corefilters
  File "..\..\Docs\german\corefilters\*.*"
  SetOutPath $INSTDIR\Docs\german\externalfilters
  File "..\..\Docs\german\externalfilters\*.*"

  SetShellVarContext All
  StrCmp $AdminInstall "No" +2
  CreateShortCut "$SMPROGRAMS\AviSynth 2.5\Deutsche AviSynth Dokumentation.lnk" "$INSTDIR\Docs\german\index.htm"

SectionEnd

Section /o  $(French_Text) French
  SectionIn 4
  SetOutPath $INSTDIR\Docs
  File "..\..\Docs\*.css"
  SetOutPath $INSTDIR\Docs\french
  File "..\..\Docs\french\*.*"
  SetOutPath $INSTDIR\Docs\french\corefilters
  File "..\..\Docs\french\corefilters\*.*"

  SetShellVarContext All
  StrCmp $AdminInstall "No" +2
  CreateShortCut "$SMPROGRAMS\AviSynth 2.5\French AviSynth Documentation.lnk" "$INSTDIR\Docs\french\index.htm"

SectionEnd

Section /o  $(Italian_Text) Italian
  SectionIn 4
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
  StrCmp $AdminInstall "No" +2
  CreateShortCut "$SMPROGRAMS\AviSynth 2.5\Italian AviSynth Documentation.lnk" "$INSTDIR\Docs\italian\index.htm"

SectionEnd

Section /o  $(Japanese_Text) Japanese

; Place holder

SectionEnd

Section /o  $(Portugese_Text) Portugese
  SectionIn 4
  SetOutPath $INSTDIR\Docs
  File "..\..\Docs\*.css"
  SetOutPath $INSTDIR\Docs\portugese
  File "..\..\Docs\portugese\*.*"
  SetOutPath $INSTDIR\Docs\portugese\advancedtopics
  File "..\..\Docs\portugese\advancedtopics\*.*"
  SetOutPath $INSTDIR\Docs\portugese\corefilters
  File "..\..\Docs\portugese\corefilters\*.*"
  SetOutPath $INSTDIR\Docs\portugese\externalfilters
  File "..\..\Docs\portugese\externalfilters\*.*"
  SetOutPath $INSTDIR\Docs\portugese\pictures\advancedtopics
  File "..\..\Docs\portugese\pictures\advancedtopics\*.*"
  SetOutPath $INSTDIR\Docs\portugese\pictures\corefilters
  File "..\..\Docs\portugese\pictures\corefilters\*.*"
  SetOutPath $INSTDIR\Docs\portugese\pictures\externalfilters
  File "..\..\Docs\portugese\pictures\externalfilters\*.*"

  SetShellVarContext All
  StrCmp $AdminInstall "No" +2
  CreateShortCut "$SMPROGRAMS\AviSynth 2.5\Portugese AviSynth Documentation.lnk" "$INSTDIR\Docs\portugese\index.htm"

SectionEnd

Section /o  $(Russian_Text) Russian
  SectionIn 4
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
  StrCmp $AdminInstall "No" +2
  CreateShortCut "$SMPROGRAMS\AviSynth 2.5\Russian AviSynth Documentation.lnk" "$INSTDIR\Docs\russian\index.htm"

SectionEnd

Subsectionend


SubSection  $(SelectAssociation_Text) SelectAssociation

Section /o  $(Associate1_Text) Associate1
  SectionIn 1 4
  StrCmp $AdminInstall "No" +3
  WriteRegStr HKCR "avsfile\shell\open\command" "" 'notepad.exe "%1"'
  WriteRegStr HKCR "avs_auto_file\shell\open\command" "" 'notepad.exe "%1"'

SectionEnd

Section /o  $(Associate2_Text) Associate2
  SectionIn 1 4
  StrCmp $AdminInstall "No" +2
  WriteRegStr HKCR "avsfile\shell\play\command" "" '"$PROGRAMFILES\Windows Media Player\mplayer2.exe" /Play "%L"'

SectionEnd

Section /o  $(Associate3_Text) Associate3
  SectionIn 1 4
; Blank new file
  StrCmp $AdminInstall "No" +3
  WriteRegStr HKCR ".avs\ShellNew" "NullFile" ""
  WriteRegStr HKCR "avsfile\ShellNew" "NullFile" ""

; Template file
;  SetOutPath $WINDIR\ShellNew or $TEMPLATES
;  File "Examples\Template.avs"
;  WriteRegStr HKCR ".avs\ShellNew" "FileName" "Template.avs"
SectionEnd

SubSectionEnd


SubSection  $(SelectExtraFiles_Text) SelectExtraFiles

Section /o  $(ExtraFiles3_Text) ExtraFiles3
  SectionIn 4
  SetOutPath $INSTDIR\FilterSDK
  File "..\filtersdk\*.*"
  SetOutPath $INSTDIR\FilterSDK\include
  File "..\src\core\avisynth.h"
  File "..\src\core\avisynth_c.h"
  SetShellVarContext All
  StrCmp $AdminInstall "No" +2
  CreateShortCut "$SMPROGRAMS\AviSynth 2.5\FilterSDK Information.lnk" "$INSTDIR\FilterSDK\FilterSDK.htm"

SectionEnd

Section /o  $(ExtraFiles1_Text) ExtraFiles1
  SectionIn 4
  SetOutPath $INSTDIR\Extras
  File "..\src\release\AviSynth.lib"
  File "..\src\release\AviSynth.exp"
SectionEnd

Section /o  $(ExtraFiles2_Text) ExtraFiles2
  SectionIn 4
  SetOutPath $INSTDIR\Extras
  File "..\src\Release\AviSynth.map"
  File "..\src\plugins\TCPDeliver\Release\TCPDeliver.map"
  File "..\src\plugins\DirectShowSource\Release\DirectShowSource.map"
SectionEnd

SubSectionEnd


Function .onInit
  !insertmacro MUI_LANGDLL_DISPLAY

; Match Language with Online Documentation Set

  Push $0
    SectionGetInstTypes ${English} $0

    StrCmp $LANGUAGE ${LANG_German} 0 +2
    SectionSetInstTypes ${German} $0

    StrCmp $LANGUAGE ${LANG_French} 0 +2
    SectionSetInstTypes ${French} $0

;    StrCmp $LANGUAGE ${LANG_Italian} 0 +2
;    SectionSetInstTypes ${Italian} $0

    StrCmp $LANGUAGE ${LANG_Japanese} 0 +2
    SectionSetInstTypes ${Japanese} $0

;    StrCmp $LANGUAGE ${LANG_Portuguese} 0 +2
;    SectionSetInstTypes ${Portuguese} $0

    StrCmp $LANGUAGE ${LANG_Russian} 0 +2
    SectionSetInstTypes ${Russian} $0
  Pop $0

  SetCurInstType 0

FunctionEnd


  !insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN

  !insertmacro MUI_DESCRIPTION_TEXT ${Frameserving}       $(Frameserving_Bubble)
  !insertmacro MUI_DESCRIPTION_TEXT ${SystemInstall}      $(SystemInstall_Bubble)
  !insertmacro MUI_DESCRIPTION_TEXT ${StandAlone}         $(StandAlone_Bubble)
  !insertmacro MUI_DESCRIPTION_TEXT ${Documentation}      $(Documentation_Bubble)
  !insertmacro MUI_DESCRIPTION_TEXT ${English}            $(English_Bubble)
  !insertmacro MUI_DESCRIPTION_TEXT ${German}             $(German_Bubble)
  !insertmacro MUI_DESCRIPTION_TEXT ${French}             $(French_Bubble)
  !insertmacro MUI_DESCRIPTION_TEXT ${Italian}            $(Italian_Bubble)
  !insertmacro MUI_DESCRIPTION_TEXT ${Japanese}           $(Japanese_Bubble)
  !insertmacro MUI_DESCRIPTION_TEXT ${Portugese}          $(Portugese_Bubble)
  !insertmacro MUI_DESCRIPTION_TEXT ${Russian}            $(Russian_Bubble)
  !insertmacro MUI_DESCRIPTION_TEXT ${SelectAssociation}  $(SelectAssociation_Bubble)
  !insertmacro MUI_DESCRIPTION_TEXT ${Associate1}         $(Associate1_Bubble)
  !insertmacro MUI_DESCRIPTION_TEXT ${Associate2}         $(Associate2_Bubble)
  !insertmacro MUI_DESCRIPTION_TEXT ${Associate3}         $(Associate3_Bubble)
  !insertmacro MUI_DESCRIPTION_TEXT ${SelectExtraFiles}   $(SelectExtraFiles_Bubble)
  !insertmacro MUI_DESCRIPTION_TEXT ${ExtraFiles1}        $(ExtraFiles1_Bubble)
  !insertmacro MUI_DESCRIPTION_TEXT ${ExtraFiles2}        $(ExtraFiles2_Bubble)
  !insertmacro MUI_DESCRIPTION_TEXT ${ExtraFiles3}        $(ExtraFiles3_Bubble)

  !insertmacro MUI_FUNCTION_DESCRIPTION_END


Function un.onInit
  !insertmacro MUI_UNGETLANGUAGE
FunctionEnd

Function un.onUninstSuccess
  MessageBox MB_OK $(Uninstall_Text)
FunctionEnd

Section "Uninstall"
  Delete "$SYSDIR\devil.dll"
  Delete "$SYSDIR\AviSynth.dll"
  DeleteRegKey HKLM "Software\Classes\.avs"

  DeleteRegKey HKCR "CLSID\{E6D6B700-124D-11D4-86F3-DB80AFD98778}"
  DeleteRegKey HKCR "Media Type\Extensions\.avs"
  DeleteRegKey HKCR ".avs"
  DeleteRegKey HKCR ".avsi"
  DeleteRegKey HKCR "avs_auto_file"
  DeleteRegKey HKCR "avsfile"
  DeleteRegKey HKCR "avifile\Extensions\AVS"

  SetShellVarContext All
  Delete "$SMPROGRAMS\AviSynth 2.5\*.*"
  RMDir  "$SMPROGRAMS\AviSynth 2.5"

  SetShellVarContext Current
  Delete "$SMPROGRAMS\AviSynth 2.5\*.*"
  RMDir  "$SMPROGRAMS\AviSynth 2.5"

  Delete "$INSTDIR\GPL.txt"
  Delete "$INSTDIR\lgpl_for_used_libs.txt"
  Delete "$INSTDIR\Examples\*.*"
  RMDir  "$INSTDIR\Examples"

  Delete "$INSTDIR\plugins\DirectShowSource.dll"
  Delete "$INSTDIR\plugins\TCPDeliver.dll"
  Delete "$INSTDIR\plugins\colors_rgb.avsi"

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

  Delete "$INSTDIR\Docs\portugese\advancedtopics\*.*"
  RMDir  "$INSTDIR\Docs\portugese\advancedtopics"
  Delete "$INSTDIR\Docs\portugese\corefilters\*.*"
  RMDir  "$INSTDIR\Docs\portugese\corefilters"
  Delete "$INSTDIR\Docs\portugese\externalfilters\*.*"
  RMDir  "$INSTDIR\Docs\portugese\externalfilters"
  Delete "$INSTDIR\Docs\portugese\pictures\advancedtopics\*.*"
  RMDir  "$INSTDIR\Docs\portugese\pictures\advancedtopics"
  Delete "$INSTDIR\Docs\portugese\pictures\corefilters\*.*"
  RMDir  "$INSTDIR\Docs\portugese\pictures\corefilters"
  Delete "$INSTDIR\Docs\portugese\pictures\externalfilters\*.*"
  RMDir  "$INSTDIR\Docs\portugese\pictures\externalfilters"
  RMDir  "$INSTDIR\Docs\portugese\pictures"
  Delete "$INSTDIR\Docs\portugese\*.*"
  RMDir  "$INSTDIR\Docs\portugese"

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

  Delete "$INSTDIR\Docs\*.css"
  RMDir  "$INSTDIR\Docs"

  Delete "$INSTDIR\Extras\Avisynth.exp"
  Delete "$INSTDIR\Extras\Avisynth.lib"
  Delete "$INSTDIR\Extras\Avisynth.map"
  Delete "$INSTDIR\Extras\DirectShowSource.map"
  Delete "$INSTDIR\Extras\TCPDeliver.map"
  RMDir  "$INSTDIR\Extras"

  Delete "$INSTDIR\FilterSDK\include\avisynth.h"
  Delete "$INSTDIR\FilterSDK\include\avisynth_c.h"
  RMDir  "$INSTDIR\FilterSDK\include"
  Delete "$INSTDIR\FilterSDK\*.*"
  RMDir  "$INSTDIR\FilterSDK"

  Delete "$INSTDIR\Uninstall.exe"

  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\AviSynth"

IfFileExists $INSTDIR 0 Removed
    MessageBox MB_YESNO|MB_ICONQUESTION $(RemoveReg_Text) IDNO Removed
    DeleteRegKey HKLM "Software\AviSynth"
Removed:

SectionEnd

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
  !insertmacro MUI_PAGE_LICENSE $(AVS_GPL_Text)
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

!verbose push
!verbose 2
!insertmacro AVS_LANGUAGE "English"      ; 1033
                                        
!insertmacro AVS_LANGUAGE "Czech"        ; 1029
!insertmacro AVS_LANGUAGE "German"       ; 1031
!insertmacro AVS_LANGUAGE "Greek"        ; 1253
!insertmacro AVS_LANGUAGE "French"       ; 1036
!insertmacro AVS_LANGUAGE "Italian"      ; 1040
!insertmacro AVS_LANGUAGE "Japanese"     ; 1041
!insertmacro AVS_LANGUAGE "Polish"       ; 1045
!insertmacro AVS_LANGUAGE "PortugueseBR" ; 1046
!insertmacro AVS_LANGUAGE "Portuguese"   ; 2070
!insertmacro AVS_LANGUAGE "Russian"      ; 1049
!verbose pop

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
  File "gpl*.txt"
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
  CreateShortCut "$SMPROGRAMS\AviSynth 2.5\$(Start_Uninstall).lnk" "$INSTDIR\Uninstall.exe"

; These bits are for everybody
  SetShellVarContext All
  CreateDirectory  "$SMPROGRAMS\AviSynth 2.5"
  CreateShortCut "$SMPROGRAMS\AviSynth 2.5\$(Start_License).lnk" "$INSTDIR\$(AVS_GPL_File)"
  CreateShortCut "$SMPROGRAMS\AviSynth 2.5\$(Start_Plugin).lnk" "$INSTDIR\Plugins"
  WriteINIStr    "$SMPROGRAMS\AviSynth 2.5\$(Start_Online).url" "InternetShortcut" "URL" "http://www.avisynth.org"
  WriteINIStr    "$SMPROGRAMS\AviSynth 2.5\$(Start_Download).url" "InternetShortcut" "URL" "http://www.avisynth.org/warpenterprises/"

  SetOutPath $INSTDIR\Examples
!verbose push
!verbose 2
  File "Examples\*.*"
!verbose pop
  CreateShortCut "$SMPROGRAMS\AviSynth 2.5\$(Start_Example).lnk" "$INSTDIR\Examples"

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

  File "gpl*.txt"
  File "lgpl_for_used_libs.txt"

  SetOutPath "$INSTDIR\Plugins"
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
  SetOutPath $INSTDIR\Docs\English
!verbose push
!verbose 2
  File "..\..\Docs\english\*.htm"
  SetOutPath $INSTDIR\Docs\English\advancedtopics
  File "..\..\Docs\english\advancedtopics\*.htm"
  SetOutPath $INSTDIR\Docs\English\corefilters
  File "..\..\Docs\english\corefilters\*.htm"
  SetOutPath $INSTDIR\Docs\English\externalfilters
  File "..\..\Docs\english\externalfilters\*.htm"
  SetOutPath $INSTDIR\Docs\English\pictures\advancedtopics
  File "..\..\Docs\english\pictures\advancedtopics\*.*"
  SetOutPath $INSTDIR\Docs\English\pictures\corefilters
  File "..\..\Docs\english\pictures\corefilters\*.*"
  SetOutPath $INSTDIR\Docs\English\pictures\externalfilters
  File "..\..\Docs\english\pictures\externalfilters\*.*"
!verbose pop

  SetOutPath $INSTDIR\Examples
  File "Examples\*.*"

  SetShellVarContext All
  StrCmp $AdminInstall "No" +2
  CreateShortCut "$SMPROGRAMS\AviSynth 2.5\$(Start_Doc_English).lnk" "$INSTDIR\Docs\English\index.htm"

SectionEnd

Section /o  $(German_Text) German
  SectionIn 4
  SetOutPath $INSTDIR\Docs
  File "..\..\Docs\*.css"
  SetOutPath $INSTDIR\Docs\German
!verbose push
!verbose 2
  File "..\..\Docs\german\*.htm"
  SetOutPath $INSTDIR\Docs\German\corefilters
  File "..\..\Docs\german\corefilters\*.htm"
  SetOutPath $INSTDIR\Docs\German\externalfilters
  File "..\..\Docs\german\externalfilters\*.htm"
!verbose pop

  SetShellVarContext All
  StrCmp $AdminInstall "No" +2
  CreateShortCut "$SMPROGRAMS\AviSynth 2.5\$(Start_Doc_German).lnk" "$INSTDIR\Docs\German\index.htm"

SectionEnd

Section /o  $(French_Text) French
  SectionIn 4
  SetOutPath $INSTDIR\Docs
  File "..\..\Docs\*.css"
  SetOutPath $INSTDIR\Docs\French
!verbose push
!verbose 2
  File "..\..\Docs\french\*.htm"
  SetOutPath $INSTDIR\Docs\French\corefilters
  File "..\..\Docs\french\corefilters\*.htm"
!verbose pop

  SetShellVarContext All
  StrCmp $AdminInstall "No" +2
  CreateShortCut "$SMPROGRAMS\AviSynth 2.5\$(Start_Doc_French).lnk" "$INSTDIR\Docs\French\index.htm"

SectionEnd

Section /o  $(Italian_Text) Italian
  SectionIn 4
  SetOutPath $INSTDIR\Docs
  File "..\..\Docs\*.css"
  SetOutPath $INSTDIR\Docs\Italian
!verbose push
!verbose 2
  File "..\..\Docs\italian\*.htm"
  SetOutPath $INSTDIR\Docs\Italian\corefilters
  File "..\..\Docs\italian\corefilters\*.htm"
  SetOutPath $INSTDIR\Docs\Italian\externalfilters
  File "..\..\Docs\italian\externalfilters\*.htm"
  SetOutPath $INSTDIR\Docs\Italian\pictures\corefilters
  File "..\..\Docs\italian\pictures\corefilters\*.*"
!verbose pop

  SetShellVarContext All
  StrCmp $AdminInstall "No" +2
  CreateShortCut "$SMPROGRAMS\AviSynth 2.5\$(Start_Doc_Italian).lnk" "$INSTDIR\Docs\Italian\index.htm"

SectionEnd

Section /o  $(Japanese_Text) Japanese
  SectionIn 4

  SetOverwrite ON
  
  SetOutPath $INSTDIR\Docs
  File "..\..\Docs\*.css"

  SetOutPath $INSTDIR\Docs\Japanese
!verbose push
!verbose 2
  File "..\..\Docs\english\*.htm"
  File "..\..\Docs\japanese\*.htm"  ; Overwrite with the translated versions
  File "..\..\Docs\japanese\ja.css"
  File "..\..\Docs\japanese\filelist.txt"
  File "..\..\Docs\japanese\readme_en.txt"
  File "..\..\Docs\japanese\readme_ja.txt"

  SetOutPath $INSTDIR\Docs\Japanese\advancedtopics
  File "..\..\Docs\english\advancedtopics\*.htm"

  SetOutPath $INSTDIR\Docs\Japanese\corefilters
  File "..\..\Docs\english\corefilters\*.htm"
  File "..\..\Docs\japanese\corefilters\*.htm"  ; Overwrite with the translated versions

  SetOutPath $INSTDIR\Docs\Japanese\externalfilters
  File "..\..\Docs\english\externalfilters\*.htm"

  SetOutPath $INSTDIR\Docs\Japanese\pictures\advancedtopics
  File "..\..\Docs\english\pictures\advancedtopics\*.*"

  SetOutPath $INSTDIR\Docs\Japanese\pictures\corefilters
  File "..\..\Docs\english\pictures\corefilters\*.*"

  SetOutPath $INSTDIR\Docs\Japanese\pictures\externalfilters
  File "..\..\Docs\english\pictures\externalfilters\*.*"
!verbose pop

  SetShellVarContext All
  StrCmp $AdminInstall "No" +2
  CreateShortCut "$SMPROGRAMS\AviSynth 2.5\$(Start_Doc_Japanese).lnk" "$INSTDIR\Docs\Japanese\index.htm"

SectionEnd

Section /o  $(Polish_Text) Polish
  SectionIn 4
  SetOutPath $INSTDIR\Docs\Polish
  File "..\..\Docs\Polish\*.*"
  SetOutPath $INSTDIR\Docs\Polish\corefilters
!verbose push
!verbose 2
  File "..\..\Docs\Polish\corefilters\*.*"
  SetOutPath $INSTDIR\Docs\Polish\externalfilters
  File "..\..\Docs\Polish\externalfilters\*.*"
!verbose pop

  SetShellVarContext All
  StrCmp $AdminInstall "No" +2
  CreateShortCut "$SMPROGRAMS\AviSynth 2.5\$(Start_Doc_Polish).lnk" "$INSTDIR\Docs\Polish\index.htm"

SectionEnd

Section /o  $(Portugese_Text) Portuguese
  SectionIn 4
  SetOutPath $INSTDIR\Docs
  File "..\..\Docs\*.css"
  SetOutPath $INSTDIR\Docs\Portuguese
!verbose push
!verbose 2
  File "..\..\Docs\portugese\*.htm"
  SetOutPath $INSTDIR\Docs\Portuguese\advancedtopics
  File "..\..\Docs\portugese\advancedtopics\*.htm"
  SetOutPath $INSTDIR\Docs\Portuguese\corefilters
  File "..\..\Docs\portugese\corefilters\*.htm"
  SetOutPath $INSTDIR\Docs\Portuguese\externalfilters
  File "..\..\Docs\portugese\externalfilters\*.htm"
  SetOutPath $INSTDIR\Docs\Portuguese\pictures\advancedtopics
  File "..\..\Docs\portugese\pictures\advancedtopics\*.*"
  SetOutPath $INSTDIR\Docs\Portuguese\pictures\corefilters
  File "..\..\Docs\portugese\pictures\corefilters\*.*"
  SetOutPath $INSTDIR\Docs\Portuguese\pictures\externalfilters
  File "..\..\Docs\portugese\pictures\externalfilters\*.*"
!verbose pop

  SetShellVarContext All
  StrCmp $AdminInstall "No" +2
  CreateShortCut "$SMPROGRAMS\AviSynth 2.5\$(Start_Doc_Portuguese).lnk" "$INSTDIR\Docs\Portuguese\index.htm"

SectionEnd

Section /o  $(Russian_Text) Russian
  SectionIn 4
  SetOutPath $INSTDIR\Docs
  File "..\..\Docs\*.css"
  SetOutPath $INSTDIR\Docs\Russian
!verbose push
!verbose 2
  File "..\..\Docs\russian\*.htm"
  File "..\..\Docs\russian\gpl-rus.txt"
  SetOutPath $INSTDIR\Docs\Russian\advancedtopics
  File "..\..\Docs\russian\advancedtopics\*.htm"
  SetOutPath $INSTDIR\Docs\Russian\corefilters
  File "..\..\Docs\russian\corefilters\*.htm"
  SetOutPath $INSTDIR\Docs\Russian\externalfilters
  File "..\..\Docs\russian\externalfilters\*.htm"
  SetOutPath $INSTDIR\Docs\Russian\pictures\advancedtopics
  File "..\..\Docs\russian\pictures\advancedtopics\*.*"
  SetOutPath $INSTDIR\Docs\Russian\pictures\corefilters
  File "..\..\Docs\russian\pictures\corefilters\*.*"
  SetOutPath $INSTDIR\Docs\Russian\pictures\externalfilters
  File "..\..\Docs\russian\pictures\externalfilters\*.*"
!verbose pop

  SetShellVarContext All
  StrCmp $AdminInstall "No" +2
  CreateShortCut "$SMPROGRAMS\AviSynth 2.5\$(Start_Doc_Russian).lnk" "$INSTDIR\Docs\Russian\index.htm"

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
!verbose push
!verbose 2
  File "..\filtersdk\*.*"
!verbose pop
  SetOutPath $INSTDIR\FilterSDK\include
  File "..\src\core\avisynth.h"
  File "..\src\core\avisynth_c.h"
  SetShellVarContext All
  StrCmp $AdminInstall "No" +2
  CreateShortCut "$SMPROGRAMS\AviSynth 2.5\$(Start_FilterSDK).lnk" "$INSTDIR\FilterSDK\FilterSDK.htm"

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

;----------------------------------

Function .onInit
  !insertmacro MUI_LANGDLL_DISPLAY

; Match Language with Online Documentation Set

  Push $0
    SectionGetInstTypes ${English} $0

    StrCmp $LANGUAGE ${LANG_German} 0 +2
    SectionSetInstTypes ${German} $0

    StrCmp $LANGUAGE ${LANG_French} 0 +2
    SectionSetInstTypes ${French} $0

    StrCmp $LANGUAGE ${LANG_Italian} 0 +2
    SectionSetInstTypes ${Italian} $0

    StrCmp $LANGUAGE ${LANG_Japanese} 0 +2
    SectionSetInstTypes ${Japanese} $0

    StrCmp $LANGUAGE ${LANG_Polish} 0 +2
    SectionSetInstTypes ${Polish} $0

    StrCmp $LANGUAGE ${LANG_PortugueseBR} 0 +2
    SectionSetInstTypes ${Portuguese} $0

    StrCmp $LANGUAGE ${LANG_Portuguese} 0 +2
    SectionSetInstTypes ${Portuguese} $0

    StrCmp $LANGUAGE ${LANG_Russian} 0 +2
    SectionSetInstTypes ${Russian} $0
  Pop $0

  SetCurInstType 0

FunctionEnd

;----------------------------------

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
  !insertmacro MUI_DESCRIPTION_TEXT ${Polish}             $(Polish_Bubble)
  !insertmacro MUI_DESCRIPTION_TEXT ${Portuguese}         $(Portugese_Bubble)
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
Retry:
  ClearErrors
  Delete "$SYSDIR\AviSynth.dll"
  Delete "$SYSDIR\devil.dll"

  IfErrors 0 Ignore
  MessageBox MB_ABORTRETRYIGNORE|MB_DEFBUTTON2|MB_ICONEXCLAMATION \
             "$(InUseMsg_Text)" /SD IDIGNORE IDIGNORE Ignore IDRETRY Retry
  DetailPrint "AviSynth.dll or DevIL.dll was inuse or protected."
  Abort
Ignore:
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

  Delete "$INSTDIR\gpl*.txt"
  Delete "$INSTDIR\lgpl_for_used_libs.txt"
  Delete "$INSTDIR\Examples\*.*"
  RMDir  "$INSTDIR\Examples"

  Delete "$INSTDIR\plugins\DirectShowSource.dll"
  Delete "$INSTDIR\plugins\TCPDeliver.dll"
  Delete "$INSTDIR\plugins\colors_rgb.avsi"

  Delete "$INSTDIR\Docs\English\advancedtopics\*.*"
  RMDir  "$INSTDIR\Docs\English\advancedtopics"
  Delete "$INSTDIR\Docs\English\corefilters\*.*"
  RMDir  "$INSTDIR\Docs\English\corefilters"
  Delete "$INSTDIR\Docs\English\externalfilters\*.*"
  RMDir  "$INSTDIR\Docs\English\externalfilters"
  Delete "$INSTDIR\Docs\English\pictures\advancedtopics\*.*"
  RMDir  "$INSTDIR\Docs\English\pictures\advancedtopics"
  Delete "$INSTDIR\Docs\English\pictures\corefilters\*.*"
  RMDir  "$INSTDIR\Docs\English\pictures\corefilters"
  Delete "$INSTDIR\Docs\English\pictures\externalfilters\*.*"
  RMDir  "$INSTDIR\Docs\English\pictures\externalfilters"
  RMDir  "$INSTDIR\Docs\English\pictures"
  Delete "$INSTDIR\Docs\English\*.*"
  RMDir  "$INSTDIR\Docs\English"

  Delete "$INSTDIR\Docs\German\corefilters\*.*"
  RMDir  "$INSTDIR\Docs\German\corefilters"
  Delete "$INSTDIR\Docs\German\externalfilters\*.*"
  RMDir  "$INSTDIR\Docs\German\externalfilters"
  Delete "$INSTDIR\Docs\German\*.*"
  RMDir  "$INSTDIR\Docs\German"

  Delete "$INSTDIR\Docs\French\corefilters\*.*"
  RMDir  "$INSTDIR\Docs\French\corefilters"
  Delete "$INSTDIR\Docs\French\*.*"
  RMDir  "$INSTDIR\Docs\French"

  Delete "$INSTDIR\Docs\Italian\corefilters\*.*"
  RMDir  "$INSTDIR\Docs\Italian\corefilters"
  Delete "$INSTDIR\Docs\Italian\externalfilters\*.*"
  RMDir  "$INSTDIR\Docs\Italian\externalfilters"
  Delete "$INSTDIR\Docs\Italian\pictures\corefilters\*.*"
  RMDir  "$INSTDIR\Docs\Italian\pictures\corefilters"
  RMDir  "$INSTDIR\Docs\Italian\pictures"
  Delete "$INSTDIR\Docs\Italian\*.*"
  RMDir  "$INSTDIR\Docs\Italian"

  Delete "$INSTDIR\Docs\Japanese\advancedtopics\*.*"
  RMDir  "$INSTDIR\Docs\Japanese\advancedtopics"
  Delete "$INSTDIR\Docs\Japanese\corefilters\*.*"
  RMDir  "$INSTDIR\Docs\Japanese\corefilters"
  Delete "$INSTDIR\Docs\Japanese\externalfilters\*.*"
  RMDir  "$INSTDIR\Docs\Japanese\externalfilters"
  Delete "$INSTDIR\Docs\Japanese\pictures\advancedtopics\*.*"
  RMDir  "$INSTDIR\Docs\Japanese\pictures\advancedtopics"
  Delete "$INSTDIR\Docs\Japanese\pictures\corefilters\*.*"
  RMDir  "$INSTDIR\Docs\Japanese\pictures\corefilters"
  Delete "$INSTDIR\Docs\Japanese\pictures\externalfilters\*.*"
  RMDir  "$INSTDIR\Docs\Japanese\pictures\externalfilters"
  RMDir  "$INSTDIR\Docs\Japanese\pictures"
  Delete "$INSTDIR\Docs\Japanese\*.*"
  RMDir  "$INSTDIR\Docs\Japanese"

  Delete "$INSTDIR\Docs\Polish\corefilters\*.*"
  RMDir  "$INSTDIR\Docs\Polish\corefilters"
  Delete "$INSTDIR\Docs\Polish\externalfilters\*.*"
  RMDir  "$INSTDIR\Docs\Polish\externalfilters"
  Delete "$INSTDIR\Docs\Polish\*.*"
  RMDir  "$INSTDIR\Docs\Polish"

  Delete "$INSTDIR\Docs\Portuguese\advancedtopics\*.*"
  RMDir  "$INSTDIR\Docs\Portuguese\advancedtopics"
  Delete "$INSTDIR\Docs\Portuguese\corefilters\*.*"
  RMDir  "$INSTDIR\Docs\Portuguese\corefilters"
  Delete "$INSTDIR\Docs\Portuguese\externalfilters\*.*"
  RMDir  "$INSTDIR\Docs\Portuguese\externalfilters"
  Delete "$INSTDIR\Docs\Portuguese\pictures\advancedtopics\*.*"
  RMDir  "$INSTDIR\Docs\Portuguese\pictures\advancedtopics"
  Delete "$INSTDIR\Docs\Portuguese\pictures\corefilters\*.*"
  RMDir  "$INSTDIR\Docs\Portuguese\pictures\corefilters"
  Delete "$INSTDIR\Docs\Portuguese\pictures\externalfilters\*.*"
  RMDir  "$INSTDIR\Docs\Portuguese\pictures\externalfilters"
  RMDir  "$INSTDIR\Docs\Portuguese\pictures"
  Delete "$INSTDIR\Docs\Portuguese\*.*"
  RMDir  "$INSTDIR\Docs\Portuguese"

  Delete "$INSTDIR\Docs\Russian\advancedtopics\*.*"
  RMDir  "$INSTDIR\Docs\Russian\advancedtopics"
  Delete "$INSTDIR\Docs\Russian\corefilters\*.*"
  RMDir  "$INSTDIR\Docs\Russian\corefilters"
  Delete "$INSTDIR\Docs\Russian\externalfilters\*.*"
  RMDir  "$INSTDIR\Docs\Russian\externalfilters"
  Delete "$INSTDIR\Docs\Russian\pictures\advancedtopics\*.*"
  RMDir  "$INSTDIR\Docs\Russian\pictures\advancedtopics"
  Delete "$INSTDIR\Docs\Russian\pictures\corefilters\*.*"
  RMDir  "$INSTDIR\Docs\Russian\pictures\corefilters"
  Delete "$INSTDIR\Docs\Russian\pictures\externalfilters\*.*"
  RMDir  "$INSTDIR\Docs\Russian\pictures\externalfilters"
  RMDir  "$INSTDIR\Docs\Russian\pictures"
  Delete "$INSTDIR\Docs\Russian\*.*"
  RMDir  "$INSTDIR\Docs\Russian"

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
    MessageBox MB_YESNO|MB_ICONQUESTION|MB_DEFBUTTON2 $(RemoveReg_Text) IDNO Removed
    DeleteRegKey HKLM "Software\AviSynth"
Removed:

SectionEnd

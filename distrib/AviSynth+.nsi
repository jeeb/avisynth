!include "x64.nsh"
!include "LogicLib.nsh"
!packhdr tempfile.exe "upx --best --q tempfile.exe"

; Set these paths to your CMake's build directory 
!DEFINE BUILD32_DIR "..\..\build-vs2012-x86"
!DEFINE BUILD64_DIR "..\..\build-vs2012-x64"


!DEFINE ISSUE 5
!DEFINE VERSION 2.6.0

!DEFINE /date DATE "%y%m%d"

!DEFINE AVS_DefaultLicenceFile "gpl.txt"

;----------------------------------

; Macro to ease conditionally accessing the file system and 
; registry on 64-bit systems
!macro If_X64
${If} ${RunningX64}
  ${DisableX64FSRedirection}
  SetRegView 64
!macroend
!macro End_X64
  SetRegView 32
  ${EnableX64FSRedirection}
${EndIf}
!macroend
!define If_X64 "!insertmacro If_X64"
!define End_X64 "!insertmacro End_X64"

;----------------------------------

; Call with "MakeNSISw /DFILE ..." to activate

!ifndef FILE
  !define FILE "File"
!else
  !undef FILE
  !define FILE ";" ; Set to ";" to make fileless demo
!endif

;----------------------------------

; Call with "MakeNSISw /DDOCFILE ..." to activate

!ifndef DOCFILE
  !define DOCFILE "File"
!else
  !undef DOCFILE
  !define DOCFILE ";" ; Set to ";" to make docless demo
!endif

;----------------------------------

VIProductVersion "${VERSION}.${ISSUE}"

VIAddVersionKey "ProductName"      "AviSynth+ 2.6"
VIAddVersionKey "Comments"         "Homepage: http://www.avisynth.org"
VIAddVersionKey "CompanyName"      "The Public"
VIAddVersionKey "LegalCopyright"   "© 2000-2013 Ben Rudiak-Gould and others"
VIAddVersionKey "FileDescription"  "AviSynth+ Installer"
VIAddVersionKey "FileVersion"      "${VERSION}.${ISSUE}"
VIAddVersionKey "ProductVersion"   "${VERSION}"
VIAddVersionKey "OriginalFilename" "AviSynth+_${DATE}.exe"

;VIAddVersionKey "InternalName"     ""
;VIAddVersionKey "LegalTrademarks"  ""
;VIAddVersionKey "PrivateBuild"     ""
;VIAddVersionKey "SpecialBuild"     ""

;----------------------------------

SetCompressor /solid lzma

;----------------------------------

!include "MUI.nsh"
!include WinMessages.nsh
!include Sections.nsh
!define MUI_ICON   "avisynth.ico"
!define MUI_UNICON "un_avisynth.ico"
!define MUI_ABORTWARNING
;!define MUI_WELCOMEFINISHPAGE_BITMAP "${NSISDIR}\Welcome.bmp"
!define MUI_COMPONENTSPAGE_SMALLDESC
;!define MUI_UI_COMPONENTSPAGE_SMALLDESC "AVS_UI.exe"
!define MUI_FINISHPAGE_LINK $(FINISHPAGE_TEXT)
!define MUI_FINISHPAGE_LINK_LOCATION "http://www.avisynth.org/"
!define MUI_INSTFILESPAGE_COLORS "C5DEFB 000000"
!define MUI_INSTFILESPAGE_PROGRESSBAR "colored"
!define MUI_FINISHPAGE_NOAUTOCLOSE
!define MUI_UNFINISHPAGE_NOAUTOCLOSE
!define MUI_LANGDLL_ALLLANGUAGES

;Pages------------------------------

;  !insertmacro MUI_PAGE_WELCOME

  !define      MUI_PAGE_CUSTOMFUNCTION_PRE AVS_License1_Pre
  !define      MUI_LICENSEPAGE_BUTTON $(AVS_LicenceBtn)
  !insertmacro MUI_PAGE_LICENSE $(AVS_GPL_Lang_Text)

  !define      MUI_PAGE_CUSTOMFUNCTION_PRE  AVS_License2_Pre
  !define      MUI_PAGE_CUSTOMFUNCTION_SHOW AVS_License2_Show
  !insertmacro MUI_PAGE_LICENSE ${AVS_DefaultLicenceFile}

  !insertmacro MUI_PAGE_COMPONENTS

  !insertmacro MUI_PAGE_DIRECTORY

!if "File" == "${FILE}"
  !insertmacro MUI_PAGE_INSTFILES
!endif

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
  !echo "Process Languages\AVS_${LANGUAGE}.nsh"
  !verbose push
  !verbose 2
  !define AvsLang ${LANG_${LANGUAGE}}
  !include "Languages\AVS_${LANGUAGE}.nsh"

  !ifndef AvsLicenceFile
    !define AvsLicenceFile ${AVS_DefaultLicenceFile}
  !endif

  !if ${AvsLicenceFile} == ${AVS_DefaultLicenceFile}
    LangString AVS_LicenceBtn ${AvsLang} $(^AgreeBtn)
  !else
    LangString AVS_LicenceBtn ${AvsLang} $(AVS_ReturnBtn)
  !endif

  LangString        AVS_GPL_Lang_File ${AvsLang} ${AvsLicenceFile}
  LicenseLangString AVS_GPL_Lang_Text ${AvsLang} ${AvsLicenceFile}

  !undef AvsLicenceFile
  !undef AvsLang
  !verbose pop
!macroend
;
;----------------------------------

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

!insertmacro MUI_RESERVEFILE_LANGDLL

;----------------------------------
; Page Callback Functions
;----------------------------------

; Skip page at startup if we have a translated GPL.txt

  Var StartUp
  Function AVS_License1_Pre

    StrCmp $(AVS_GPL_Lang_File) ${AVS_DefaultLicenceFile} +4
      StrCmp $StartUp "No" +3
        StrCpy $StartUp "No"
        Abort

  FunctionEnd

;----------------------------------

; Skip page if we do not have a translated GPL.txt

  Function AVS_License2_Pre

    StrCmp $(AVS_GPL_Lang_File) ${AVS_DefaultLicenceFile} 0 +2
      Abort

  FunctionEnd

;----------------------------------

; Change Back button text to "Translate"

  Function AVS_License2_Show
    Push $0

    GetDlgItem $0 $HWNDPARENT 3 ; Back button
    SendMessage $0 ${WM_SETTEXT} 0 "STR:$(AVS_TranslateBtn)"

    Pop $0
  FunctionEnd

;----------------------------------

; Tweak the finish page paremters -- Max out the MUI_FINISHPAGE_LINK field width & make 2 high.

  Function AVS_Finish_Pre

    !insertmacro MUI_INSTALLOPTIONS_WRITE "ioSpecial.ini" "Field 3" "Bottom" "165" ; 175
    !insertmacro MUI_INSTALLOPTIONS_WRITE "ioSpecial.ini" "Field 4" "Top"    "165" ; 175
;   !insertmacro MUI_INSTALLOPTIONS_WRITE "ioSpecial.ini" "Field 4" "Left"   "120" ; 120
    !insertmacro MUI_INSTALLOPTIONS_WRITE "ioSpecial.ini" "Field 4" "Right"  "330" ; 315
;   !insertmacro MUI_INSTALLOPTIONS_WRITE "ioSpecial.ini" "Field 3" "Bottom" "185" ; 185

  FunctionEnd

;----------------------------------


NAME "AviSynth+"
BRANDINGTEXT "AviSynth+ ${VERSION} -- [${DATE}]"
OutFile "AviSynth+_${DATE}.exe"
SetOverwrite ON
Caption "AviSynth+ ${VERSION}"
ShowInstDetails show
CRCCheck ON

ComponentText $(COMPONENT_TEXT)

InstallDir "$PROGRAMFILES\AviSynth+"
InstallDirRegKey HKLM SOFTWARE\AviSynth ""

InstType $(AVS_Standard)
InstType $(AVS_Minimal)
InstType $(AVS_Standalone)
InstType $(AVS_Full)

Var AdminInstall
Var PlugDir25_x86
Var PlugDirPlus_x86
Var PlugDir25_x64
Var PlugDirPlus_x64
Section "!$(Frameserving_Text)" Frameserving
  SectionIn 1 2 4 RO

  StrCpy $AdminInstall "Yes"

  ClearErrors
 
  ; -------------------------------------------------------
  ; Install system files
  ; -------------------------------------------------------
  SetOutPath $SYSDIR
  ${File} "${BUILD32_DIR}\Output\AviSynth.dll"
  ${File} "${BUILD32_DIR}\Output\system\DevIL.dll"
  ${If_X64}
    ${File} "${BUILD64_DIR}\Output\AviSynth.dll"
    ${File} "${BUILD64_DIR}\Output\system\DevIL.dll"
  ${End_X64}
   
IfErrors 0 dll_ok
  MessageBox MB_OK $(InUseMsg_Text)
  Abort

dll_ok:

  ; -------------------------------------------------------
  ; Install licence
  ; -------------------------------------------------------
  SetOutPath $INSTDIR
  ${File} ${AVS_DefaultLicenceFile}
  ${File} "lgpl_for_used_libs.txt"

  SetOutPath "$INSTDIR\License Translations"
  ${File} "gpl-*.txt"

  ; -------------------------------------------------------
  ; Create plugin directories
  ; -------------------------------------------------------
  ReadRegStr $PlugDir25_x86 HKLM "SOFTWARE\AviSynth" "PluginDir2_5"
  ReadRegStr $PlugDirPlus_x86 HKLM "SOFTWARE\AviSynth" "PluginDir+"
  ${If_X64}
    ReadRegStr $PlugDir25_x64 HKLM "SOFTWARE\AviSynth" "PluginDir2_5"
    ReadRegStr $PlugDirPlus_x64 HKLM "SOFTWARE\AviSynth" "PluginDir+"
  ${End_X64}
  
  ${If} $PlugDir25_x86 == ""
    CreateDirectory "$INSTDIR\plugins"
    StrCpy $PlugDir25_x86 "$INSTDIR\plugins"
  ${EndIf}
  ${If} $PlugDirPlus_x86 == ""
    CreateDirectory "$INSTDIR\plugins+"
    StrCpy $PlugDirPlus_x86 "$INSTDIR\plugins+"
  ${EndIf}
  ${If} $PlugDir25_x64 == ""
    StrCpy $PlugDir25_x64 "$INSTDIR\plugins64"
    CreateDirectory "$PlugDir25_x64"
  ${EndIf}
  ${If} $PlugDirPlus_x64 == ""
    StrCpy $PlugDirPlus_x64 "$INSTDIR\plugins64+"
    CreateDirectory "$PlugDirPlus_x64"
  ${EndIf}
  ClearErrors
  
  ; -------------------------------------------------------
  ; Install plugins
  ; -------------------------------------------------------
  SetOutPath $PlugDir25_x86
  ${File} "${BUILD32_DIR}\Output\plugins\DirectShowSource.dll"
  ${File} "${BUILD32_DIR}\Output\plugins\ImageSeq.dll"
  ${File} "${BUILD32_DIR}\Output\plugins\Shibatch.dll"
  ${File} "${BUILD32_DIR}\Output\plugins\TimeStretch.dll"
  ${File} "${BUILD32_DIR}\Output\plugins\VDubFilter.dll"
; ${File} "${BUILD32_DIR}\Output\plugins\TCPDeliver.dll"
; ${File} "${BUILD32_DIR}\Output\plugins\VFAPIFilter.dll"
  ${File} "ColorPresets\colors_rgb.avsi"
  SetOutPath $PlugDir25_x64
  ${File} "${BUILD64_DIR}\Output\plugins\DirectShowSource.dll"
  ${File} "${BUILD64_DIR}\Output\plugins\ImageSeq.dll"
  ${File} "${BUILD64_DIR}\Output\plugins\Shibatch.dll"
  ${File} "${BUILD64_DIR}\Output\plugins\TimeStretch.dll"
  ${File} "${BUILD64_DIR}\Output\plugins\VDubFilter.dll"
; ${File} "${BUILD64_DIR}\Output\plugins\TCPDeliver.dll"
; ${File} "${BUILD64_DIR}\Output\plugins\VFAPIFilter.dll"
  ${File} "ColorPresets\colors_rgb.avsi"
  
IfErrors 0 plug_ok
  MessageBox MB_OK $(PlugDir_Text)
  Abort

plug_ok:
  ClearErrors
  WriteRegStr HKLM "SOFTWARE\AviSynth" "" "$INSTDIR"
  WriteRegStr HKLM "SOFTWARE\AviSynth" "PluginDir2_5" "$PlugDir25_x86"
  WriteRegStr HKLM "SOFTWARE\AviSynth" "PluginDir+" "$PlugDirPlus_x86"
  ${If_X64}
    WriteRegStr HKLM "SOFTWARE\AviSynth" "PluginDir2_5" "$PlugDir25_x64"
    WriteRegStr HKLM "SOFTWARE\AviSynth" "PluginDir+" "$PlugDirPlus_x64"
  ${End_X64}

  ; -------------------------------------------------------
  ; Write uninstall information
  ; -------------------------------------------------------
  WriteRegStr HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\AviSynth" "DisplayName" "AviSynth+ 2.6"
  WriteRegStr HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\AviSynth" "UninstallString" '"$INSTDIR\Uninstall.exe"'
;  WriteRegStr HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\AviSynth" "Publisher" "GPL Public release."
  WriteRegStr HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\AviSynth" "DisplayIcon" "$SYSDIR\AviSynth.dll,0"
  WriteRegStr HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\AviSynth" "DisplayVersion" "${VERSION}.${ISSUE}"
  WriteRegStr HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\AviSynth" "URLInfoAbout" "http://avisynth.org/"

; Other Add/Remove Software registry keys

; "Comments"		(string) - A comment describing the installer package
; "EstimatedSize"	(DWORD)  - The size of the installed files (in KB)
; "HelpLink"		(string) - Link to the support website
; "HelpTelephone"	(string) - Telephone number for support
; "InstallLocation"	(string) - Installation directory ($INSTDIR)
; "InstallSource"	(string) - Location where the application was installed from
; "ModifyPath"		(string) - Path and filename of the application modify program
; "NoModify"		(DWORD)  - 1 if uninstaller has no option to modify the installed application
; "NoRepair"		(DWORD)  - 1 if the uninstaller has no option to repair the installation
; "ProductID"		(string) - Product ID of the application
; "RegCompany"		(string) - Registered company of the application
; "RegOwner"		(string) - Registered owner of the application
; "URLUpdateInfo"	(string) - Link to the website for application updates
; "VersionMajor"	(DWORD)  - Major version number of the application
; "VersionMinor"	(DWORD)  - Minor version number of the application


  WriteRegStr HKLM "SOFTWARE\Classes\.avs" "" "avsfile"

IfErrors 0 mreg_ok
  MessageBox MB_OK $(AdminRightsHKLM_Text)
  Abort

mreg_ok:
  ClearErrors

  ; -------------------------------------------------------
  ; Register frameserver
  ; -------------------------------------------------------
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
  ; -------------------------------------------------------
  ; Create Start Menu shortcuts
  ; -------------------------------------------------------
  SetShellVarContext All
  CreateDirectory  "$SMPROGRAMS\AviSynth+"

  CreateShortCut "$SMPROGRAMS\AviSynth+\$(Start_License).lnk" "$INSTDIR\${AVS_DefaultLicenceFile}"
  StrCmp $(AVS_GPL_Lang_File) ${AVS_DefaultLicenceFile} +2
    CreateShortCut "$SMPROGRAMS\AviSynth+\$(Start_License_Lang).lnk" "$INSTDIR\License Translations\$(AVS_GPL_Lang_File)"

  CreateShortCut "$SMPROGRAMS\AviSynth+\$(Start_Plugin).lnk" "$INSTDIR\Plugins"
  WriteINIStr    "$SMPROGRAMS\AviSynth+\$(Start_Online).url" "InternetShortcut" "URL" "http://www.avisynth.org"
  WriteINIStr    "$SMPROGRAMS\AviSynth+\$(Start_Download).url" "InternetShortcut" "URL" "http://www.avisynth.org/warpenterprises/"

  SetOutPath $INSTDIR\Examples
!echo " -- Supressed"
!verbose push
!verbose 2
  ${File} "Examples\*.*"
!verbose pop
  CreateShortCut "$SMPROGRAMS\AviSynth+\$(Start_Example).lnk" "$INSTDIR\Examples"

  Delete $INSTDIR\Uninstall.exe
  WriteUninstaller $INSTDIR\Uninstall.exe

; These bits are for the administrator only
  SetShellVarContext Current
  CreateDirectory  "$SMPROGRAMS\AviSynth+"
  CreateShortCut "$SMPROGRAMS\AviSynth+\$(Start_Uninstall).lnk" "$INSTDIR\Uninstall.exe"

SectionEnd

Subsection  $(Documentation_Text) Documentation

Section /o  $(English_Text) English
  SectionIn 1 3 4

  SetOutPath $INSTDIR\docs
  ${DocFile} "docs\*.css"
  SetOutPath $INSTDIR\docs\English
!echo " -- Supressed"
!verbose push
!verbose 2
  ${DocFile} "docs\english\*.htm"
  SetOutPath $INSTDIR\docs\English\advancedtopics
  ${DocFile} "docs\english\advancedtopics\*.htm"
  SetOutPath $INSTDIR\docs\English\corefilters
  ${DocFile} "docs\english\corefilters\*.htm"
  SetOutPath $INSTDIR\docs\English\externalfilters
  ${DocFile} "docs\english\externalfilters\*.htm"
  SetOutPath $INSTDIR\docs\English\pictures\advancedtopics
  ${DocFile} "docs\english\pictures\advancedtopics\*.*"
  SetOutPath $INSTDIR\docs\English\pictures\corefilters
  ${DocFile} "docs\english\pictures\corefilters\*.*"
  SetOutPath $INSTDIR\docs\English\pictures\externalfilters
  ${DocFile} "docs\english\pictures\externalfilters\*.*"
!verbose pop

  SetOutPath $INSTDIR\Examples
  ${File} "Examples\*.*"

  SetShellVarContext All
  StrCmp $AdminInstall "No" +2
  CreateShortCut "$SMPROGRAMS\AviSynth+\$(Start_Doc_English).lnk" "$INSTDIR\docs\English\index.htm"

SectionEnd

Section /o  $(Czech_Text) Czech
  SectionIn 4
  SetOutPath $INSTDIR\docs
  ${DocFile} "docs\*.css"
  SetOutPath $INSTDIR\docs\Czech
!echo " -- Supressed"
!verbose push
!verbose 2
  ${DocFile} "docs\czech\*.htm"
  ${DocFile} "docs\czech\gpl-cs.txt"
  SetOutPath $INSTDIR\docs\Czech\advancedtopics
  ${DocFile} "docs\czech\advancedtopics\*.htm"
  SetOutPath $INSTDIR\docs\Czech\corefilters
  ${DocFile} "docs\czech\corefilters\*.htm"
  SetOutPath $INSTDIR\docs\Czech\externalfilters
  ${DocFile} "docs\czech\externalfilters\*.htm"
  SetOutPath $INSTDIR\docs\Czech\pictures\advancedtopics
  ${DocFile} "docs\czech\pictures\advancedtopics\*.*"
  SetOutPath $INSTDIR\docs\Czech\pictures\corefilters
  ${DocFile} "docs\czech\pictures\corefilters\*.*"
!verbose pop

  SetShellVarContext All
  StrCmp $AdminInstall "No" +2
  CreateShortCut "$SMPROGRAMS\AviSynth+\$(Start_Doc_Czech).lnk" "$INSTDIR\docs\Czech\index.htm"

SectionEnd

Section /o  $(German_Text) German
  SectionIn 4
  SetOutPath $INSTDIR\docs
  ${DocFile} "docs\*.css"
  SetOutPath $INSTDIR\docs\German
!echo " -- Supressed"
!verbose push
!verbose 2
  ${DocFile} "docs\german\*.htm"
  SetOutPath $INSTDIR\docs\German\corefilters
  ${DocFile} "docs\german\corefilters\*.htm"
  SetOutPath $INSTDIR\docs\German\externalfilters
  ${DocFile} "docs\german\externalfilters\*.htm"
!verbose pop

  SetShellVarContext All
  StrCmp $AdminInstall "No" +2
  CreateShortCut "$SMPROGRAMS\AviSynth+\$(Start_Doc_German).lnk" "$INSTDIR\docs\German\index.htm"

SectionEnd

Section /o  $(French_Text) French
  SectionIn 4
  SetOutPath $INSTDIR\docs
  ${DocFile} "docs\*.css"
  SetOutPath $INSTDIR\docs\French
!echo " -- Supressed"
!verbose push
!verbose 2
  ${DocFile} "docs\french\*.htm"
  SetOutPath $INSTDIR\docs\French\corefilters
  ${DocFile} "docs\french\corefilters\*.htm"
!verbose pop

  SetShellVarContext All
  StrCmp $AdminInstall "No" +2
  CreateShortCut "$SMPROGRAMS\AviSynth+\$(Start_Doc_French).lnk" "$INSTDIR\docs\French\index.htm"

SectionEnd

Section /o  $(Italian_Text) Italian
  SectionIn 4
  SetOutPath $INSTDIR\docs
  ${DocFile} "docs\*.css"
  SetOutPath $INSTDIR\docs\Italian
!echo " -- Supressed"
!verbose push
!verbose 2
  ${DocFile} "docs\italian\*.htm"
  SetOutPath $INSTDIR\docs\Italian\corefilters
  ${DocFile} "docs\italian\corefilters\*.htm"
  SetOutPath $INSTDIR\docs\Italian\externalfilters
  ${DocFile} "docs\italian\externalfilters\*.htm"
  SetOutPath $INSTDIR\docs\Italian\pictures\corefilters
  ${DocFile} "docs\italian\pictures\corefilters\*.*"
!verbose pop

  SetShellVarContext All
  StrCmp $AdminInstall "No" +2
  CreateShortCut "$SMPROGRAMS\AviSynth+\$(Start_Doc_Italian).lnk" "$INSTDIR\docs\Italian\index.htm"

SectionEnd

Section /o  $(Japanese_Text) Japanese
  SectionIn 4

  SetOverwrite ON

  SetOutPath $INSTDIR\docs
  ${DocFile} "docs\*.css"

  SetOutPath $INSTDIR\docs\Japanese
!echo " -- Supressed"
!verbose push
!verbose 2
  ${DocFile} "docs\english\*.htm"
  ${DocFile} "docs\japanese\*.htm"  ; Overwrite with the translated versions
  ${DocFile} "docs\japanese\ja.css"
  ${DocFile} "docs\japanese\filelist.txt"
  ${DocFile} "docs\japanese\readme_en.txt"
  ${DocFile} "docs\japanese\readme_ja.txt"

  SetOutPath $INSTDIR\docs\Japanese\advancedtopics
  ${DocFile} "docs\english\advancedtopics\*.htm"

  SetOutPath $INSTDIR\docs\Japanese\corefilters
  ${DocFile} "docs\japanese\corefilters\*.htm"

  SetOutPath $INSTDIR\docs\Japanese\externalfilters
  ${DocFile} "docs\english\externalfilters\*.htm"

  SetOutPath $INSTDIR\docs\Japanese\pictures\advancedtopics
  ${DocFile} "docs\english\pictures\advancedtopics\*.*"

  SetOutPath $INSTDIR\docs\Japanese\pictures\corefilters
  ${DocFile} "docs\english\pictures\corefilters\*.*"

  SetOutPath $INSTDIR\docs\Japanese\pictures\externalfilters
  ${DocFile} "docs\english\pictures\externalfilters\*.*"
!verbose pop

  SetShellVarContext All
  StrCmp $AdminInstall "No" +2
  CreateShortCut "$SMPROGRAMS\AviSynth+\$(Start_Doc_Japanese).lnk" "$INSTDIR\docs\Japanese\index.htm"

SectionEnd

Section /o  $(Polish_Text) Polish
  SectionIn 4
  SetOutPath $INSTDIR\docs\Polish
  ${DocFile} "docs\polish\*.*"
  SetOutPath $INSTDIR\docs\Polish\corefilters
!echo " -- Supressed"
!verbose push
!verbose 2
  ${DocFile} "docs\polish\corefilters\*.*"
  SetOutPath $INSTDIR\docs\Polish\externalfilters
  ${DocFile} "docs\polish\externalfilters\*.*"
!verbose pop

  SetShellVarContext All
  StrCmp $AdminInstall "No" +2
  CreateShortCut "$SMPROGRAMS\AviSynth+\$(Start_Doc_Polish).lnk" "$INSTDIR\docs\Polish\index.htm"

SectionEnd

Section /o  $(Portugese_Text) Portuguese
  SectionIn 4
  SetOutPath $INSTDIR\docs
  ${DocFile} "docs\*.css"
  SetOutPath $INSTDIR\docs\Portuguese
!echo " -- Supressed"
!verbose push
!verbose 2
  ${DocFile} "docs\portugese\*.htm"
  SetOutPath $INSTDIR\docs\Portuguese\advancedtopics
  ${DocFile} "docs\portugese\advancedtopics\*.htm"
  SetOutPath $INSTDIR\docs\Portuguese\corefilters
  ${DocFile} "docs\portugese\corefilters\*.htm"
  SetOutPath $INSTDIR\docs\Portuguese\externalfilters
  ${DocFile} "docs\portugese\externalfilters\*.htm"
  SetOutPath $INSTDIR\docs\Portuguese\pictures\advancedtopics
  ${DocFile} "docs\portugese\pictures\advancedtopics\*.*"
  SetOutPath $INSTDIR\docs\Portuguese\pictures\corefilters
  ${DocFile} "docs\portugese\pictures\corefilters\*.*"
  SetOutPath $INSTDIR\docs\Portuguese\pictures\externalfilters
  ${DocFile} "docs\portugese\pictures\externalfilters\*.*"
!verbose pop

  SetShellVarContext All
  StrCmp $AdminInstall "No" +2
  CreateShortCut "$SMPROGRAMS\AviSynth+\$(Start_Doc_Portuguese).lnk" "$INSTDIR\docs\Portuguese\index.htm"

SectionEnd

Section /o  $(Russian_Text) Russian
  SectionIn 4
  SetOutPath $INSTDIR\docs
  ${DocFile} "docs\*.css"
  SetOutPath $INSTDIR\docs\Russian
!echo " -- Supressed"
!verbose push
!verbose 2
  ${DocFile} "docs\russian\*.htm"
  ${DocFile} "docs\russian\gpl-rus.txt"
  SetOutPath $INSTDIR\docs\Russian\advancedtopics
  ${DocFile} "docs\russian\advancedtopics\*.htm"
  SetOutPath $INSTDIR\docs\Russian\corefilters
  ${DocFile} "docs\russian\corefilters\*.htm"
  SetOutPath $INSTDIR\docs\Russian\externalfilters
  ${DocFile} "docs\russian\externalfilters\*.htm"
  SetOutPath $INSTDIR\docs\Russian\pictures\advancedtopics
  ${DocFile} "docs\russian\pictures\advancedtopics\*.*"
  SetOutPath $INSTDIR\docs\Russian\pictures\corefilters
  ${DocFile} "docs\russian\pictures\corefilters\*.*"
  SetOutPath $INSTDIR\docs\Russian\pictures\externalfilters
  ${DocFile} "docs\russian\pictures\externalfilters\*.*"
!verbose pop

  SetShellVarContext All
  StrCmp $AdminInstall "No" +2
  CreateShortCut "$SMPROGRAMS\AviSynth+\$(Start_Doc_Russian).lnk" "$INSTDIR\docs\Russian\index.htm"

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
;  ${File} "Examples\Template.avs"
;  WriteRegStr HKCR ".avs\ShellNew" "FileName" "Template.avs"
SectionEnd

SubSectionEnd


SubSection  $(SelectExtraFiles_Text) SelectExtraFiles

Section /o  $(ExtraFiles3_Text) ExtraFiles3
  SectionIn 4
  SetOutPath $INSTDIR\FilterSDK
!echo " -- Supressed"
!verbose push
!verbose 2
  ${DocFile} "FilterSDK\*.*"
  SetOutPath $INSTDIR\FilterSDK\Pictures
  ${DocFile} "FilterSDK\pictures\*.*"
!verbose pop
  SetOutPath $INSTDIR\FilterSDK\include
  ${File} "..\avs_core\include\avisynth.h"
  ${File} "..\avs_core\include\avisynth_c.h"
  SetOutPath $INSTDIR\FilterSDK\include\avs
  ${File} "..\avs_core\include\avs\*.h"
  SetShellVarContext All
  StrCmp $AdminInstall "No" +2
  CreateShortCut "$SMPROGRAMS\AviSynth+\$(Start_FilterSDK).lnk" "$INSTDIR\FilterSDK\FilterSDK.htm"

SectionEnd

;Don't install import libraries. Avisynth should be used with LoadLibrary.
;Section /o  $(ExtraFiles1_Text) ExtraFiles1
;  SectionIn 4
;  SetOutPath $INSTDIR\Extras
;  ${File} "..\avs_core\AviSynth.lib"
;  ${File} "..\avs_core\AviSynth.exp"
;SectionEnd

; .map files get generated by /MAP option, according to MS docs, but AviSynth+ doesn't use the /MAP option,
; so just comment out this whole section in case /MAP does get added later.

;Section /o  $(ExtraFiles2_Text) ExtraFiles2
;  SectionIn 4
;  SetOutPath $INSTDIR\Extras
;  ${File} "..\avs_core\AviSynth.map"
; Comment out TCPDeliver.map if/until such time that AviSynth+ provides it too
;  ${File} "..\plugins\TCPDeliver\TCPDeliver.map"
;  ${File} "..\plugins\DirectShowSource\DirectShowSource.map"
;  ${File} "..\plugins\ImageSeq\ImageSeq.map"
;  ${File} "..\plugins\Shibatch\Shibatch.map"
;  ${File} "..\plugins\TimeStretch\TimeStretch.map"
;  ${File} "..\plugins\VDubFilter\VDubFilter.map"
; Comment out VFAPIFilter.dll until it can be built with AviSynth+
;  ${File} "..\plugins\VFAPIFilter\VFAPIFilter.map"
;
;SectionEnd

SubSectionEnd

;----------------------------------

Function .onInit

  StrCpy $StartUp "Yes"

  !insertmacro MUI_LANGDLL_DISPLAY

; Match Language with Online Documentation Set

  Push $0
    SectionGetInstTypes ${English} $0

    StrCmp $LANGUAGE ${LANG_Czech} 0 +2
    SectionSetInstTypes ${Czech} $0

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
;  !insertmacro MUI_DESCRIPTION_TEXT ${SystemInstall}      $(SystemInstall_Bubble)
  !insertmacro MUI_DESCRIPTION_TEXT ${Documentation}      $(Documentation_Bubble)
  !insertmacro MUI_DESCRIPTION_TEXT ${English}            $(English_Bubble)
  !insertmacro MUI_DESCRIPTION_TEXT ${Czech}              $(Czech_Bubble)
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
  ;!insertmacro MUI_DESCRIPTION_TEXT ${ExtraFiles1}        $(ExtraFiles1_Bubble)
  ;!insertmacro MUI_DESCRIPTION_TEXT ${ExtraFiles2}        $(ExtraFiles2_Bubble)
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
  Delete "$SYSDIR\DevIL.dll"
  ${If_X64}
    Delete "$SYSDIR\AviSynth.dll"
    Delete "$SYSDIR\DevIL.dll"
  ${End_X64}

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
  Delete "$SMPROGRAMS\AviSynth+\*.*"
  RMDir  "$SMPROGRAMS\AviSynth+"

  SetShellVarContext Current
  Delete "$SMPROGRAMS\AviSynth+\*.*"
  RMDir  "$SMPROGRAMS\AviSynth+"
!echo " -- Supressed"
!verbose push
!verbose 2
  Delete "$INSTDIR\${AVS_DefaultLicenceFile}"
  Delete "$INSTDIR\lgpl_for_used_libs.txt"

  Delete "$INSTDIR\License Translations\gpl-*.txt"
  RmDir  "$INSTDIR\License Translations"

  Delete "$INSTDIR\Examples\*.*"
  RMDir  "$INSTDIR\Examples"

  Delete "$INSTDIR\plugins\DirectShowSource.dll"
  Delete "$INSTDIR\plugins\ImageSeq.dll"
  Delete "$INSTDIR\plugins\Shibatch.dll"
  Delete "$INSTDIR\plugins\TimeStretch.dll"
  Delete "$INSTDIR\plugins\VDubFilter.dll"
; Delete "$INSTDIR\plugins\TCPDeliver.dll"
; Delete "$INSTDIR\plugins\VFAPIFilter.dll"
  Delete "$INSTDIR\plugins\colors_rgb.avsi"
  Delete "$INSTDIR\plugins64\DirectShowSource.dll"
  Delete "$INSTDIR\plugins64\ImageSeq.dll"
  Delete "$INSTDIR\plugins64\Shibatch.dll"
  Delete "$INSTDIR\plugins64\TimeStretch.dll"
  Delete "$INSTDIR\plugins64\VDubFilter.dll"
; Delete "$INSTDIR\plugins64\TCPDeliver.dll"
; Delete "$INSTDIR\plugins64\VFAPIFilter.dll"
  Delete "$INSTDIR\plugins64\colors_rgb.avsi"

  Delete "$INSTDIR\docs\English\advancedtopics\*.*"
  RMDir  "$INSTDIR\docs\English\advancedtopics"
  Delete "$INSTDIR\docs\English\corefilters\*.*"
  RMDir  "$INSTDIR\docs\English\corefilters"
  Delete "$INSTDIR\docs\English\externalfilters\*.*"
  RMDir  "$INSTDIR\docs\English\externalfilters"
  Delete "$INSTDIR\docs\English\pictures\advancedtopics\*.*"
  RMDir  "$INSTDIR\docs\English\pictures\advancedtopics"
  Delete "$INSTDIR\docs\English\pictures\corefilters\*.*"
  RMDir  "$INSTDIR\docs\English\pictures\corefilters"
  Delete "$INSTDIR\docs\English\pictures\externalfilters\*.*"
  RMDir  "$INSTDIR\docs\English\pictures\externalfilters"
  RMDir  "$INSTDIR\docs\English\pictures"
  Delete "$INSTDIR\docs\English\*.*"
  RMDir  "$INSTDIR\docs\English"

  Delete "$INSTDIR\docs\Czech\advancedtopics\*.*"
  RMDir  "$INSTDIR\docs\Czech\advancedtopics"
  Delete "$INSTDIR\docs\Czech\corefilters\*.*"
  RMDir  "$INSTDIR\docs\Czech\corefilters"
  Delete "$INSTDIR\docs\Czech\externalfilters\*.*"
  RMDir  "$INSTDIR\docs\Czech\externalfilters"
  Delete "$INSTDIR\docs\Czech\pictures\advancedtopics\*.*"
  RMDir  "$INSTDIR\docs\Czech\pictures\advancedtopics"
  Delete "$INSTDIR\docs\Czech\pictures\corefilters\*.*"
  RMDir  "$INSTDIR\docs\Czech\pictures\corefilters"
  RMDir  "$INSTDIR\docs\Czech\pictures"
  Delete "$INSTDIR\docs\Czech\*.*"
  RMDir  "$INSTDIR\docs\Czech"

  Delete "$INSTDIR\docs\German\corefilters\*.*"
  RMDir  "$INSTDIR\docs\German\corefilters"
  Delete "$INSTDIR\docs\German\externalfilters\*.*"
  RMDir  "$INSTDIR\docs\German\externalfilters"
  Delete "$INSTDIR\docs\German\*.*"
  RMDir  "$INSTDIR\docs\German"

  Delete "$INSTDIR\docs\French\corefilters\*.*"
  RMDir  "$INSTDIR\docs\French\corefilters"
  Delete "$INSTDIR\docs\French\*.*"
  RMDir  "$INSTDIR\docs\French"

  Delete "$INSTDIR\docs\Italian\corefilters\*.*"
  RMDir  "$INSTDIR\docs\Italian\corefilters"
  Delete "$INSTDIR\docs\Italian\externalfilters\*.*"
  RMDir  "$INSTDIR\docs\Italian\externalfilters"
  Delete "$INSTDIR\docs\Italian\pictures\corefilters\*.*"
  RMDir  "$INSTDIR\docs\Italian\pictures\corefilters"
  RMDir  "$INSTDIR\docs\Italian\pictures"
  Delete "$INSTDIR\docs\Italian\*.*"
  RMDir  "$INSTDIR\docs\Italian"

  Delete "$INSTDIR\docs\Japanese\advancedtopics\*.*"
  RMDir  "$INSTDIR\docs\Japanese\advancedtopics"
  Delete "$INSTDIR\docs\Japanese\corefilters\*.*"
  RMDir  "$INSTDIR\docs\Japanese\corefilters"
  Delete "$INSTDIR\docs\Japanese\externalfilters\*.*"
  RMDir  "$INSTDIR\docs\Japanese\externalfilters"
  Delete "$INSTDIR\docs\Japanese\pictures\advancedtopics\*.*"
  RMDir  "$INSTDIR\docs\Japanese\pictures\advancedtopics"
  Delete "$INSTDIR\docs\Japanese\pictures\corefilters\*.*"
  RMDir  "$INSTDIR\docs\Japanese\pictures\corefilters"
  Delete "$INSTDIR\docs\Japanese\pictures\externalfilters\*.*"
  RMDir  "$INSTDIR\docs\Japanese\pictures\externalfilters"
  RMDir  "$INSTDIR\docs\Japanese\pictures"
  Delete "$INSTDIR\docs\Japanese\*.*"
  RMDir  "$INSTDIR\docs\Japanese"

  Delete "$INSTDIR\docs\Polish\corefilters\*.*"
  RMDir  "$INSTDIR\docs\Polish\corefilters"
  Delete "$INSTDIR\docs\Polish\externalfilters\*.*"
  RMDir  "$INSTDIR\docs\Polish\externalfilters"
  Delete "$INSTDIR\docs\Polish\*.*"
  RMDir  "$INSTDIR\docs\Polish"

  Delete "$INSTDIR\docs\Portuguese\advancedtopics\*.*"
  RMDir  "$INSTDIR\docs\Portuguese\advancedtopics"
  Delete "$INSTDIR\docs\Portuguese\corefilters\*.*"
  RMDir  "$INSTDIR\docs\Portuguese\corefilters"
  Delete "$INSTDIR\docs\Portuguese\externalfilters\*.*"
  RMDir  "$INSTDIR\docs\Portuguese\externalfilters"
  Delete "$INSTDIR\docs\Portuguese\pictures\advancedtopics\*.*"
  RMDir  "$INSTDIR\docs\Portuguese\pictures\advancedtopics"
  Delete "$INSTDIR\docs\Portuguese\pictures\corefilters\*.*"
  RMDir  "$INSTDIR\docs\Portuguese\pictures\corefilters"
  Delete "$INSTDIR\docs\Portuguese\pictures\externalfilters\*.*"
  RMDir  "$INSTDIR\docs\Portuguese\pictures\externalfilters"
  RMDir  "$INSTDIR\docs\Portuguese\pictures"
  Delete "$INSTDIR\docs\Portuguese\*.*"
  RMDir  "$INSTDIR\docs\Portuguese"

  Delete "$INSTDIR\docs\Russian\advancedtopics\*.*"
  RMDir  "$INSTDIR\docs\Russian\advancedtopics"
  Delete "$INSTDIR\docs\Russian\corefilters\*.*"
  RMDir  "$INSTDIR\docs\Russian\corefilters"
  Delete "$INSTDIR\docs\Russian\externalfilters\*.*"
  RMDir  "$INSTDIR\docs\Russian\externalfilters"
  Delete "$INSTDIR\docs\Russian\pictures\advancedtopics\*.*"
  RMDir  "$INSTDIR\docs\Russian\pictures\advancedtopics"
  Delete "$INSTDIR\docs\Russian\pictures\corefilters\*.*"
  RMDir  "$INSTDIR\docs\Russian\pictures\corefilters"
  Delete "$INSTDIR\docs\Russian\pictures\externalfilters\*.*"
  RMDir  "$INSTDIR\docs\Russian\pictures\externalfilters"
  RMDir  "$INSTDIR\docs\Russian\pictures"
  Delete "$INSTDIR\docs\Russian\*.*"
  RMDir  "$INSTDIR\docs\Russian"

  Delete "$INSTDIR\docs\*.*"
  RMDir  "$INSTDIR\docs"

  ;Delete "$INSTDIR\Extras\Avisynth.exp"
  ;Delete "$INSTDIR\Extras\Avisynth.lib"
  ;Delete "$INSTDIR\Extras\Avisynth.map"
  ;Delete "$INSTDIR\Extras\DirectShowSource.map"
  ;Delete "$INSTDIR\Extras\TCPDeliver.map"
  ;Delete "$INSTDIR\Extras\ImageSeq.map"
  ;Delete "$INSTDIR\Extras\Shibatch.map"
  ;Delete "$INSTDIR\Extras\TimeStretch.map"
  ;Delete "$INSTDIR\Extras\VDubFilter.map"
  ;Delete "$INSTDIR\Extras\VFAPIFilter.map"
  RMDir  "$INSTDIR\Extras"

  Delete "$INSTDIR\FilterSDK\include\avs\*.*"
  RMDir  "$INSTDIR\FilterSDK\include\avs"
  Delete "$INSTDIR\FilterSDK\include\*.*"
  RMDir  "$INSTDIR\FilterSDK\include"
  Delete "$INSTDIR\FilterSDK\Pictures\*.*"
  RMDir  "$INSTDIR\FilterSDK\Pictures"
  Delete "$INSTDIR\FilterSDK\*.*"
  RMDir  "$INSTDIR\FilterSDK"
!verbose pop
  Delete "$INSTDIR\Uninstall.exe"

  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\AviSynth"

IfFileExists $INSTDIR 0 Removed
    MessageBox MB_YESNO|MB_ICONQUESTION|MB_DEFBUTTON2 $(RemoveReg_Text) IDNO Removed
    DeleteRegKey HKLM "Software\AviSynth"
    ${If_X64}
      DeleteRegKey HKLM "Software\AviSynth"
    ${End_X64}
Removed:

SectionEnd

;For use with Nullsoft's NSIS v1.98
;Compile the script inside the NSIS directory otherwise specify the exact file directories
;The installer automatically sets Notepad as the default viewer for .avs files
;If you add the icon to the AviSynth dll with Resource Hacker (unpack with UPX first) it will be ;visible for .avs files 
;To further compress the installer remove ";" from the below line (you must have UPX) 
;!packhdr tempfil.exe "C:\upx\upx --best --q tempfil.exe"
;If you choose the alternate icons see the below "or alternate lines"

Name "AviSynth"
Caption "AviSynth 2.5.0 beta"
Icon "AviSynth.ico"              ;or alternate16_Avs.ico
UninstallIcon "Un_AviSynth.ico"  ;or alternate16_UnAvs.ico
EnabledBitmap "on.bmp"
DisabledBitmap "off.bmp"
OutFile "AviSynth_250.exe"
BrandingText "NSIS"
InstallColors "C5DEFB" "000000"  ;or alternate "A9A8A9" "444644"
InstProgressFlags "Colored"
SetOverwrite Ifnewer
CRCCheck on
UninstallText "This will uninstall AviSynth. Click Uninstall to continue."

LicenseText "AviSynth is distributed under the following license agreement.$\nYou must accept the agreement to install AviSynth."

LicenseData GPL.txt


Function .onInit
	
MessageBox MB_YESNO|MB_ICONQUESTION   "This will install AviSynth 2.5.0 beta. Do you wish to continue ?" IDYES NoAbort
		Abort
	NoAbort:
FunctionEnd

InstallDir "$PROGRAMFILES\AviSynth 2.5"

InstallDirRegKey HKLM SOFTWARE\AviSynth "Install_Dir_2_5"

ComponentText "AviSynth - the premiere, frameserving tool available today.$\nCopyright © 2000-2003 Ben Rudiak-Gould, et al."

DirText   "AviSynth - use it to create, edit or enhance video.$\nCopyright © 2000-2003 Ben Rudiak-Gould, et al."
                            
InstType Normal
   
Section "AviSynth Base (required)"
SectionIn 1 2

  SetOutPath $SYSDIR
  File "..\release\AviSynth.dll"
IfErrors dll_not_ok

  SetOutPath $INSTDIR
  File "GPL.txt"

;  WriteRegStr HKLM "SOFTWARE\AviSynth" "" "$INSTDIR"

  ReadRegStr $0 HKEY_LOCAL_MACHINE "SOFTWARE\AviSynth" "plugindir2_5"
	StrCmp "$0" "" No_Plugin_exists Plugin_exists
No_Plugin_exists:
	CreateDirectory "$INSTDIR\plugins"
	StrCpy $0 "$INSTDIR\plugins"
Plugin_exists:

ClearErrors

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
  WriteRegStr HKCR "avsfile\shell\open\command" "" 'notepad.exe "%1"'
  WriteRegStr HKCR "avsfile\DefaultIcon" "" $SYSDIR\AviSynth.dll,0

CreateDirectory  "$SMPROGRAMS\AviSynth 2.5"
  
  CreateShortCut "$SMPROGRAMS\AviSynth 2.5\Uninstall AviSynth.lnk" "$INSTDIR\Uninstall.exe"
  CreateShortCut "$SMPROGRAMS\AviSynth 2.5\License.lnk" "$INSTDIR\GPL.txt"
  CreateShortCut "$SMPROGRAMS\AviSynth 2.5\Plugin Directory.lnk" "$INSTDIR\Plugins"

Delete $INSTDIR\Uninstall.exe 
WriteUninstaller $INSTDIR\Uninstall.exe

goto dll_ok
dll_not_ok:
MessageBox MB_OK "Could not copy avisynth.dll to system directory - Close down all applications that use Avisynth, and sure to have write permission to the system directory, and try again."
Abort
dll_ok:

SectionEnd

SectionDivider

Section "Documentation (recommended)"
  SectionIn 1 2
  SetOutPath $INSTDIR\Docs
  File "..\Docs\*.*"
  SetOutPath $INSTDIR\Docs\Filters
  File "..\Docs\Filters\*.*"
  SetOutPath $INSTDIR\Examples
  File "Examples\*.*"

  CreateShortCut "$SMPROGRAMS\AviSynth 2.5\AviSynth Documentation.lnk" "$INSTDIR\Docs\index.html"
  CreateShortCut "$SMPROGRAMS\AviSynth 2.5\Example Scripts.lnk" "$INSTDIR\Examples"
  WriteINIStr    "$SMPROGRAMS\AviSynth 2.5\AviSynth Online.url" "InternetShortcut" "URL" "http://www.avisynth.org"

Delete $INSTDIR\Uninstall.exe 
WriteUninstaller $INSTDIR\Uninstall.exe

SectionEnd

;Section "German Documentation"
;  SectionIn 2 
;  SetOutPath $INSTDIR\Docs_ger
;  File "..\Docs_ger\*.*"
;  SetOutPath $INSTDIR\Docs_ger\Filters
;  File "..\Docs_ger\Filters\*.*"
;  SetOutPath $INSTDIR\Examples
;  File "..\Examples\*.*"
  
;CreateShortCut "$SMPROGRAMS\AviSynth 2\Deutsche AviSynth Dokumentation.lnk" "$INSTDIR\Docs_ger\index.html"

;CreateShortCut "$SMPROGRAMS\AviSynth 2\Skript Beispiele.lnk" "$INSTDIR\Examples"

;Delete $INSTDIR\Uninstall.exe 
;WriteUninstaller $INSTDIR\Uninstall.exe

;SectionEnd

Section "Uninstall"

MessageBox MB_YESNO "Do you want to remove pointer to plugin directory (no files will be removed)?" IDYES removeplug IDNO dontremoveplug
removeplug:
DeleteRegKey HKEY_LOCAL_MACHINE "SOFTWARE\AviSynth" 
dontremoveplug:

  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\AviSynth"
  Delete "$SYSDIR\AviSynth.dll"
  DeleteRegKey HKLM "Software\Classes\avs"
  DeleteRegKey HKLM "Software\AviSynth"
  DeleteRegKey HKCR ".avs"
  DeleteRegKey HKCR "CLSID\{E6D6B700-124D-11D4-86F3-DB80AFD98778}"
  DeleteRegKey HKCR "Media Type\Extensions\.avs"
  DeleteRegKey HKCR "avifile\Extensions\avs"
  Delete "$SMPROGRAMS\AviSynth 2.5\*.*"
  RMDir "$SMPROGRAMS\AviSynth 2.5"
  Delete "$INSTDIR\GPL.txt"
  Delete "$INSTDIR\Docs\Filters\*.*"
  RMDir  "$INSTDIR\Docs\Filters"
  Delete "$INSTDIR\Docs\*.*"
  RMDir  "$INSTDIR\Docs"
  Delete "$INSTDIR\Docs_ger\Filters\*.*"
  RMDir  "$INSTDIR\Docs_ger\Filters"
  Delete "$INSTDIR\Docs_ger\*.*"
  RMDir  "$INSTDIR\Docs_ger"
  Delete "$INSTDIR\Uninstall.exe"


SectionEnd

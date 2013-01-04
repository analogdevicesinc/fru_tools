; Mostly borrowed from http://nsis.sourceforge.net/Examples/Modern%20UI/Basic.nsi
; NSIS Modern User Interface
; Basic Example Script
; Written by Joost Verburg
; Changes/Bugs added by Robin Getz <robin.getz@analog.com>

;--------------------------------
;Include Modern UI

  !include "MUI2.nsh"

;--------------------------------
;General

  ;Name and file
  Name "FMC FRU Tools, by Analog Devices Inc."
  OutFile "fru-tools_installer.exe"

  ;Default installation folder
  InstallDir "$PROGRAMFILES32\Analog Devices\FRU Tools"
  
  ;Get installation folder from registry if available
  InstallDirRegKey HKCU "Software\Analog Devices\fru-tools" ""

  ;Request application privileges for Windows Vista
  RequestExecutionLevel admin

  LicenseForceSelection checkbox "i accept"
  Caption "FMC FRU Tools by Analog Devices, Inc"
  VIProductVersion ${VERSION}
  VIAddVersionKey ProductName "fru dump"
  VIAddVersionKey Comments "For more info, visit http://wiki.analog.com/resources/tools-software/linux-software/fru_dump"
  VIAddVersionKey CompanyName "Analog Devices, Inc."
  VIAddVersionKey LegalCopyright "Analog Devices, Inc."
  VIAddVersionKey FileDescription "FMC FRU Dump"
  VIAddVersionKey FileVersion ${VERSION}
  VIAddVersionKey ProductVersion ${VERSION}
  VIAddVersionKey InternalName "FMC FRU Dumper"
  VIAddVersionKey OriginalFilename "${AppID}.exe"

;--------------------------------
;Interface Settings

  !define MUI_ABORTWARNING

;--------------------------------
;Pages

  !insertmacro MUI_PAGE_LICENSE "license.txt"
  !insertmacro MUI_PAGE_COMPONENTS
  !insertmacro MUI_PAGE_DIRECTORY
  !insertmacro MUI_PAGE_INSTFILES
  
  !insertmacro MUI_UNPAGE_CONFIRM
  !insertmacro MUI_UNPAGE_INSTFILES
  
;--------------------------------
;Languages
 
  !insertmacro MUI_LANGUAGE "English"

;--------------------------------
;Installer Sections

Section "FMC FRU dump" SecFRU_Dump

  SetOutPath "$INSTDIR"
  File fru-dump.exe
  
  ;Store installation folder
  WriteRegStr HKCU "Software\Analog Devices\fru-tools" "" $INSTDIR
  
  ;Create uninstaller
  WriteUninstaller "$INSTDIR\Uninstall.exe"

SectionEnd

;--------------------------------
;Descriptions

  ;Language strings
  LangString DESC_SecFRU_Dump ${LANG_ENGLISH} "A command line utility to dump/manipulate FMC FRU files. There is NO GUI."

  ;Assign language strings to sections
  !insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
    !insertmacro MUI_DESCRIPTION_TEXT ${SecFRU_Dump} $(DESC_SecFRU_Dump)
  !insertmacro MUI_FUNCTION_DESCRIPTION_END

;--------------------------------
;Uninstaller Section

Section "Uninstall"

  ;ADD YOUR OWN FILES HERE...

  Delete "$INSTDIR\Uninstall.exe"

  RMDir "$INSTDIR"

  DeleteRegKey /ifempty HKCU "Software\Analog Devices\fru-tools"

SectionEnd

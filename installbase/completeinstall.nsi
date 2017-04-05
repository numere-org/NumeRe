# NumeRe-Installer
# NSIS-Script
# 2016-05-08
# -------------------------------

# Modern UI laden
!include "MUI2.nsh"

# Variablen definieren
!define PRODUCT "NumeRe - Framework für Numerische Rechnungen"
!define DIRNAME "NumeRe"
!define MUI_FILE "numere"
!define VERSION "1.1.0"# $\"Bloch$\""
!define VERSIONEXT " $\"rc1$\""
!define MUI_BRANDINGTEXT "NumeRe: Framework für Numerische Rechnungen v${VERSION}${VERSIONEXT}"
BrandingText "${MUI_BRANDINGTEXT}"

!define SHCNE_ASSOCCHANGED 0x08000000
!define SHCNF_IDLIST 0

!define StrStr "!insertmacro StrStr"

!macro StrStr ResultVar String SubString
	Push '${String}'
	Push '${SubString}'
	Call StrStr
	Pop '${ResultVar}'
!macroend

# Installationsname definieren
Name "NumeRe v${VERSION}${VERSIONEXT}"

# CRCCheck aktiveren
CRCCheck On

# Verwendung noch unklar. Ggf. auch unnötig
#!include "${NSISDIR}\Contrib\Modern UI\System.nsh"
#---------------------------------

# Ausgabedateiname
OutFile "numereinstaller.exe"

# LZMA-Compressor auswählen
SetCompressor "lzma"

# Modern UI-Graphiken umdefinieren
!define MUI_ICON "icons\icon.ico"
!define MUI_UNICON "icons\icon.ico"
!define MUI_WELCOMEFINISHPAGE_BITMAP "wizard.bmp"
!define MUI_UNWELCOMEFINISHPAGE_BITMAP "wizard.bmp"
#--------------------------------

VIProductVersion "${VERSION}.35"
VIAddVersionKey ProductName "${PRODUCT}"
VIAddVersionKey CompanyName "Erik Hänel et al."
VIAddVersionKey OriginalFilename "numereinstaller.exe"
VIAddVersionKey FileDescription "Installer für NumeRe: Framework für Numerische Rechnungen"
VIAddVersionKey FileVersion "${VERSION}.35"
VIAddVersionKey InternalName "NumeRe-Installer"
VIAddVersionKey LegalCopyright "(c) 2016, Erik Hänel et al. Installer und enthaltene Dateien unterstehen der GNU GPL v3"


# Standard-Dateiverzeichnis
InstallDir "C:\Software\${DIRNAME}"
InstallDirRegKey HKCU "Software\Classes\Applications\${PRODUCT}" ""
RequestExecutionLevel admin

# Abbruchwarnung aktivieren
!define MUI_ABORTWARNING
#--------------------------------



# Reihenfolge der Modern UI-Pages für die Installation
!insertmacro MUI_PAGE_WELCOME  
!insertmacro MUI_PAGE_LICENSE "licence.txt"
!define MUI_PAGE_CUSTOMFUNCTION_LEAVE "VerifyInstDir"
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_INSTFILES

	!define MUI_FINISHPAGE_RUN "$INSTDIR\numere.exe"
	!define MUI_FINISHPAGE_RUN_NOTCHECKED
	!define MUI_FINISHPAGE_RUN_TEXT "NumeRe v${VERSION}${VERSIONEXT} starten"
	!define MUI_FINISHPAGE_SHOWREADME
	!define	MUI_FINISHPAGE_SHOWREADME_NOTCHECKED
	!define MUI_FINISHPAGE_SHOWREADME_TEXT "Dokumentation öffnen"
	!define MUI_FINISHPAGE_SHOWREADME_FUNCTION LaunchPDF
!insertmacro MUI_PAGE_FINISH

# Reihenfolge der Modern UI-Pages für die Deinstallation
!insertmacro MUI_UNPAGE_WELCOME
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_COMPONENTS
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_UNPAGE_FINISH
#--------------------------------

# Sprache
!insertmacro MUI_LANGUAGE "German"
!insertmacro MUI_LANGUAGE "English"
#-------------------------------- 

# Zusätzliche Lang-Strings für die Fileassociations
LangString LS_UNINST ${LANG_GERMAN} "NumeRe Deinstallieren"
LangString LS_DOCUM ${LANG_GERMAN} "${PRODUCT}-Dokumentation"
LangString LS_REMOV ${LANG_GERMAN} "${PRODUCT} (Entfernen)"
LangString LS_STRTSCR ${LANG_GERMAN} "Script starten ..."
LangString LS_OPENWITH ${LANG_GERMAN} "Mit ${DIRNAME} öffnen ..."
LangString LS_EXEC ${LANG_GERMAN} "In ${DIRNAME} ausführen ..."

LangString LS_FT_NSCR ${LANG_GERMAN} "NumeRe-Script"
LangString LS_FT_NPRC ${LANG_GERMAN} "NumeRe-Prozedur"
LangString LS_FT_NDAT ${LANG_GERMAN} "NumeRe-Datenfile"
LangString LS_FT_CASSY ${LANG_GERMAN} "CASSYLab-Datei"
LangString LS_FT_JDX ${LANG_GERMAN} "JCAMP-DX-Spektrum"
LangString LS_FT_IBW ${LANG_GERMAN} "IGOR-Binärwelle"

LangString LS_UNINST ${LANG_ENGLISH} "Uninstall NumeRe"
LangString LS_DOCUM ${LANG_ENGLISH} "${PRODUCT} documentation"
LangString LS_REMOV ${LANG_ENGLISH} "${PRODUCT} (Remove)"
LangString LS_STRTSCR ${LANG_ENGLISH} "Start script ..."
LangString LS_OPENWITH ${LANG_ENGLISH} "Open with ${DIRNAME} ..."
LangString LS_EXEC ${LANG_ENGLISH} "Execute in ${DIRNAME} ..."

LangString LS_FT_NSCR ${LANG_ENGLISH} "NumeRe script"
LangString LS_FT_NPRC ${LANG_ENGLISH} "NumeRe procedure"
LangString LS_FT_NDAT ${LANG_ENGLISH} "NumeRe datafile"
LangString LS_FT_CASSY ${LANG_ENGLISH} "CASSYLab file"
LangString LS_FT_JDX ${LANG_ENGLISH} "JCAMP-DX spectrum"
LangString LS_FT_IBW ${LANG_ENGLISH} "IGOR binary wave"

# TODO: "Update", "KeepSaves"
# Installer Sections
Section "!Core" Core
	SectionIn RO
	# Hauptverzeichnis
	SetOutPath "$INSTDIR"
	File "${MUI_FILE}.exe"
	File "ChangesLog.txt"
	File "win7\hdf5dll.dll"
	File "win7\libgcc_s_dw2-1.dll"
	File "win7\libgomp-1.dll"
	File "win7\libmgl.dll"
	File "win7\libgsl.dll"
	File "win7\libgslcblas.dll"
	File "win7\libmgl-qt5.dll"
	File "win7\libmgl-wnd.dll"
	File "win7\libstdc++-6.dll"
	File "win7\libwinpthread-1.dll"
	File "win7\libzlib.dll"
	File "win7\pthreadGC2.dll"
	#File "pthreadGC2.dll"
	File "readme.txt"

	# Schriftarten
	SetOutPath "$INSTDIR\fonts"
	File "fonts\*.vfm"
	# Icons
	SetOutPath "$INSTDIR\icons"
	File "icons\*.ico"
	File "icons\*.png"
	# Hauptverzeichnis
	SetOutPath "$INSTDIR"

	# Desktop-Icon
	Delete "$DESKTOP\${PRODUCT}.lnk"
	CreateShortCut "$DESKTOP\${DIRNAME}.lnk" "$INSTDIR\${MUI_FILE}.exe" ""

SectionEnd
#-------------------------------- 

Section /o "!Win 8.1/10" Compatibility
	SetOutPath "$INSTDIR"
	File "win10\libmgl.dll"
	File "win10\libmgl-qt5.dll"
	File "win10\libmgl-wnd.dll"
SectionEnd

Section "Sample files" Samples
	# Beispieldateien
	SetOutPath "$INSTDIR\samples"
	File "samples\*.dat"
	File "samples\*.ndat"
	File "samples\*.nscr"	
SectionEnd

Section "Plugins" Plugins
	# Plugins
	SetOutPath "$INSTDIR\scripts"
	File "scripts\*.nscr"
SectionEnd

Section "File associations" Fileallocs
	# Startmenü-Einträge
	CreateDirectory "$SMPROGRAMS\${PRODUCT}"
	CreateShortCut "$SMPROGRAMS\${PRODUCT}\$(LS_UNINST).lnk" "$INSTDIR\Uninstall.exe" "" "$INSTDIR\Uninstall.exe" 0
	CreateShortCut "$SMPROGRAMS\${PRODUCT}\${PRODUCT}.lnk" "$INSTDIR\${MUI_FILE}.exe" "" "$INSTDIR\${MUI_FILE}.exe" 0
	CreateShortCut "$SMPROGRAMS\${PRODUCT}\$(LS_DOCUM).lnk" "$INSTDIR\docs\NumeRe-Dokumentation.pdf" "" "$INSTDIR\docs\NumeRe-Dokumentation.pdf" 0

	# Deinstallationseinträge in die Registry
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT}" "DisplayIcon" "$INSTDIR\icons\icon.ico"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT}" "DisplayName" "$(LS_REMOV)"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT}" "DisplayVersion" "${VERSION}"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT}" "InstallLocation" "$INSTDIR"
	WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT}" "EstimatedSize" 40346
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT}" "Publisher" "Erik Hänel et al."
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT}" "UninstallString" "$INSTDIR\Uninstall.exe"
	
	# Installationsverzeichnis protokollieren
	WriteRegStr HKCU "Software\Classes\Applications\${PRODUCT}" "" "$INSTDIR"
	
	# Öffnen-Mit-Eintrag
	WriteRegStr HKCU "Software\Classes\Applications\${MUI_FILE}.exe\shell\open\command" "" "$INSTDIR\${MUI_FILE}.exe $\"%1$\""

	# Dateiverknüpfungen
	# *.nscr
	WriteRegStr HKLM "Software\Classes\.nscr" "" "NumeRe.NSCR"
	WriteRegStr HKCU "Software\Classes\.nscr" "" "NumeRe.NSCR"
	WriteRegStr HKLM "Software\Classes\.nscr" "Content Type" "text/plain"
	WriteRegStr HKLM "Software\Classes\.nscr" "PercievedType" "text"
	WriteRegStr HKLM "Software\Classes\.nscr\ShellNew" "NullFile" ""
	WriteRegStr HKLM "Software\Classes\NumeRe.NSCR" "" "$(LS_FT_NSCR)"
	WriteRegStr HKLM "Software\Classes\NumeRe.NSCR\DefaultIcon" "" "$INSTDIR\icons\nscr.ico"
	WriteRegStr HKLM "Software\Classes\NumeRe.NSCR\Shell" "" "Open"
	WriteRegStr HKLM "Software\Classes\NumeRe.NSCR\Shell\Open" "" "$(LS_STRTSCR)"
	WriteRegStr HKLM "Software\Classes\NumeRe.NSCR\Shell\Open\command" "" "$INSTDIR\${MUI_FILE}.exe $\"%1$\""

	# *.nprc
	WriteRegStr HKLM "Software\Classes\.nprc" "" "NumeRe.NPRC"
	WriteRegStr HKCU "Software\Classes\.nprc" "" "NumeRe.NPRC"
	WriteRegStr HKLM "Software\Classes\.nprc" "Content Type" "text/plain"
	WriteRegStr HKLM "Software\Classes\.nprc" "PercievedType" "text"
	WriteRegStr HKLM "Software\Classes\.nprc\ShellNew" "NullFile" ""
	WriteRegStr HKLM "Software\Classes\NumeRe.NPRC" "" "$(LS_FT_NPRC)"
	WriteRegStr HKLM "Software\Classes\NumeRe.NPRC\DefaultIcon" "" "$INSTDIR\icons\nprc.ico"
	WriteRegStr HKLM "Software\Classes\NumeRe.NPRC\Shell" "" "Open"
	WriteRegStr HKLM "Software\Classes\NumeRe.NPRC\Shell\Open" "" "$(LS_EXEC)"
	WriteRegStr HKLM "Software\Classes\NumeRe.NPRC\Shell\Open\command" "" "$INSTDIR\${MUI_FILE}.exe $\"%1$\""

	# *.ndat
	WriteRegStr HKLM "Software\Classes\.ndat" "" "NumeRe.NDAT"
	WriteRegStr HKCU "Software\Classes\.ndat" "" "NumeRe.NDAT"
	WriteRegStr HKLM "Software\Classes\.ndat" "Content Type" "application/octet-stream"
	WriteRegStr HKLM "Software\Classes\.ndat" "PercievedType" "application"
	WriteRegStr HKLM "Software\Classes\NumeRe.NDAT" "" "$(LS_FT_NDAT)"
	WriteRegStr HKLM "Software\Classes\NumeRe.NDAT\DefaultIcon" "" "$INSTDIR\icons\ndat.ico"
	WriteRegStr HKLM "Software\Classes\NumeRe.NDAT\Shell" "" "Open"
	WriteRegStr HKLM "Software\Classes\NumeRe.NDAT\Shell\Open" "" "$(LS_OPENWITH)"
	WriteRegStr HKLM "Software\Classes\NumeRe.NDAT\Shell\Open\command" "" "$INSTDIR\${MUI_FILE}.exe $\"%1$\""

	# *.labx
	WriteRegStr HKLM "Software\Classes\.labx" "" "NumeRe.LABX"
	WriteRegStr HKCU "Software\Classes\.labx" "" "NumeRe.LABX"
	WriteRegStr HKLM "Software\Classes\.labx" "Content Type" "text/plain"
	WriteRegStr HKLM "Software\Classes\.labx" "PercievedType" "text"
	WriteRegStr HKLM "Software\Classes\NumeRe.LABX" "" "$(LS_FT_CASSY)"
	WriteRegStr HKLM "Software\Classes\NumeRe.LABX\DefaultIcon" "" "$INSTDIR\icons\labx.ico"
	WriteRegStr HKLM "Software\Classes\NumeRe.LABX\Shell" "" "Open"
	WriteRegStr HKLM "Software\Classes\NumeRe.LABX\Shell\Open" "" "$(LS_OPENWITH)"
	WriteRegStr HKLM "Software\Classes\NumeRe.LABX\Shell\Open\command" "" "$INSTDIR\${MUI_FILE}.exe $\"%1$\""

	# *.jdx, *.dx, *.jcm
	WriteRegStr HKLM "Software\Classes\.jdx" "" "NumeRe.JDX"
	WriteRegStr HKCU "Software\Classes\.jdx" "" "NumeRe.JDX"
	WriteRegStr HKLM "Software\Classes\.jdx" "Content Type" "text/plain"
	WriteRegStr HKLM "Software\Classes\.jdx" "PercievedType" "text"
	WriteRegStr HKLM "Software\Classes\.dx" "" "NumeRe.JDX"
	WriteRegStr HKCU "Software\Classes\.dx" "" "NumeRe.JDX"
	WriteRegStr HKLM "Software\Classes\.dx" "Content Type" "text/plain"
	WriteRegStr HKLM "Software\Classes\.dx" "PercievedType" "text"
	WriteRegStr HKLM "Software\Classes\.jcm" "" "NumeRe.JDX"
	WriteRegStr HKCU "Software\Classes\.jcm" "" "NumeRe.JDX"
	WriteRegStr HKLM "Software\Classes\.jcm" "Content Type" "text/plain"
	WriteRegStr HKLM "Software\Classes\.jcm" "PercievedType" "text"
	WriteRegStr HKLM "Software\Classes\NumeRe.JDX" "" "$(LS_FT_JDX)"
	WriteRegStr HKLM "Software\Classes\NumeRe.JDX\DefaultIcon" "" "$INSTDIR\icons\jdx.ico"
	WriteRegStr HKLM "Software\Classes\NumeRe.JDX\Shell" "" "Open"
	WriteRegStr HKLM "Software\Classes\NumeRe.JDX\Shell\Open" "" "$(LS_OPENWITH)"
	WriteRegStr HKLM "Software\Classes\NumeRe.JDX\Shell\Open\command" "" "$INSTDIR\${MUI_FILE}.exe $\"%1$\""

	# *.ibw
	WriteRegStr HKLM "Software\Classes\.ibw" "" "NumeRe.IBW"
	WriteRegStr HKLM "Software\Classes\.ibw" "Content Type" "application/octet-stream"
	WriteRegStr HKLM "Software\Classes\.ibw" "PercievedType" "application"
	WriteRegStr HKLM "Software\Classes\NumeRe.IBW" "" "$(LS_FT_IBW)"
	WriteRegStr HKLM "Software\Classes\NumeRe.IBW\DefaultIcon" "" "$INSTDIR\icons\ibw.ico"
	WriteRegStr HKLM "Software\Classes\NumeRe.IBW\Shell" "" "Open"
	WriteRegStr HKLM "Software\Classes\NumeRe.IBW\Shell\Open" "" "$(LS_OPENWITH)"
	WriteRegStr HKLM "Software\Classes\NumeRe.IBW\Shell\Open\command" "" "$INSTDIR\${MUI_FILE}.exe $\"%1$\""

	# Deinstallationsroutine generieren
	WriteUninstaller "$INSTDIR\Uninstall.exe"
	
	Call RefreshShellIcons
SectionEnd

Section /o "Deutsch" German
	SectionIn RO
	SetOutPath "$INSTDIR"
	File "de-DE\update.hlpidx"
	# Sprachdateien
	SetOutPath "$INSTDIR\lang"
	File "de-DE\lang\*.nlng"
	# Dokumentationsverzeichnis
	SetOutPath "$INSTDIR\docs"
	File "de-DE\docs\*.nhlp"
	File "de-DE\docs\*.ndb"
	File "de-DE\docs\*.pdf"	
	File "de-DE\docs\*.png"	
SectionEnd

Section /o "English" English
	SectionIn RO
	SetOutPath "$INSTDIR"
	File "en-GB\update.hlpidx"
	# Sprachdateien
	SetOutPath "$INSTDIR\lang"
	File "en-GB\lang\*.nlng"
	# Dokumentationsverzeichnis
	SetOutPath "$INSTDIR\docs"
	File "en-GB\docs\*.nhlp"
	File "en-GB\docs\*.ndb"
	File "en-GB\docs\*.pdf"	
	File "en-GB\docs\*.png"	
SectionEnd

#-----------------------

#Uninstaller Section  
Section "!un.Core" UnCore
	SectionIn RO
	Delete "$INSTDIR\Uninstall.exe"
	# Rekursiv Hauptverzeichnis und alle untergeordneten Verzeichnisse löschen
	RMDir /r "$INSTDIR\docs"
	RMDir /r "$INSTDIR\lang"
	RMDir /r "$INSTDIR\fonts"    
	RMDir /r "$INSTDIR\samples"    
	RMDir /r "$INSTDIR\icons"
	Delete "$INSTDIR\${MUI_FILE}.*"
	Delete "$INSTDIR\ChangesLog.txt"
	Delete "$INSTDIR\hdf5dll.dll"
	Delete "$INSTDIR\libgcc_s_dw2-1.dll"
	Delete "$INSTDIR\libgomp-1.dll"
	Delete "$INSTDIR\libmgl.dll"
	Delete "$INSTDIR\libgsl.dll"
	Delete "$INSTDIR\libgslcblas.dll"
	Delete "$INSTDIR\libmgl-fltk.dll"
	Delete "$INSTDIR\libmgl-wnd.dll"
	Delete "$INSTDIR\libstdc++-6.dll"
	Delete "$INSTDIR\libwinpthread-1.dll"
	Delete "$INSTDIR\libzlib.dll"
	Delete "$INSTDIR\numere_nprc_highlighting.xml"
	Delete "$INSTDIR\numere_nscr_highlighting.xml"
	Delete "$INSTDIR\pthreadGC2.dll"
	Delete "$INSTDIR\readme.txt"

	# Hauptverzeichnis löschen
	#RMDir "$INSTDIR"

	# Startmenü-Einträge löschen
	Delete "$DESKTOP\${PRODUCT}.lnk"
	#Delete "$SMPROGRAMS\${PRODUCT}\*.lnk"
	RMDir /r "$SMPROGRAMS\${PRODUCT}"

	# Deinstallationseinträge in der Registry löschen
	DeleteRegKey HKEY_CURRENT_USER "SOFTWARE\Classes\Applications\${PRODUCT}"
	DeleteRegKey HKEY_LOCAL_MACHINE "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT}"
	
	#Öffnen-Mit-Eintrag löschen
	DeleteRegKey HKCU "Software\Classes\Applications\${MUI_FILE}.exe"
	
	# Dateiverknüpfungen löschen
	DeleteRegKey HKLM "Software\Classes\.nscr"
	DeleteRegKey HKLM "Software\Classes\.nprc"
	DeleteRegKey HKLM "Software\Classes\.ndat"
	DeleteRegKey HKLM "Software\Classes\.labx"
	DeleteRegKey HKLM "Software\Classes\.jdx"
	DeleteRegKey HKLM "Software\Classes\.dx"
	DeleteRegKey HKLM "Software\Classes\.jcm"
	DeleteRegKey HKLM "Software\Classes\.ibw"
	DeleteRegKey HKCU "Software\Classes\.nscr"
	DeleteRegKey HKCU "Software\Classes\.nprc"
	DeleteRegKey HKCU "Software\Classes\.ndat"
	DeleteRegKey HKCU "Software\Classes\.labx"
	DeleteRegKey HKCU "Software\Classes\.jdx"
	DeleteRegKey HKCU "Software\Classes\.dx"
	DeleteRegKey HKCU "Software\Classes\.jcm"
	DeleteRegKey HKCU "Software\Classes\.ibw"
	DeleteRegKey HKLM "Software\Classes\NumeRe.NSCR"
	DeleteRegKey HKLM "Software\Classes\NumeRe.NPRC"
	DeleteRegKey HKLM "Software\Classes\NumeRe.NDAT"
	DeleteRegKey HKLM "Software\Classes\NumeRe.LABX"
	DeleteRegKey HKLM "Software\Classes\NumeRe.JDX"
	DeleteRegKey HKLM "Software\Classes\NumeRe.IBW"
	
	Call Un.RefreshShellIcons
SectionEnd
#--------------------------------    

Section /o "un.Files" UnFileallocs
	StrCmp $INSTDIR $PROGRAMFILES +3 0
		RMDir /r "$INSTDIR"
		Goto +6
	RMDir /r "$INSTDIR\data"    
	RMDir /r "$INSTDIR\save"    
	RMDir /r "$INSTDIR\scripts"    
	RMDir /r "$INSTDIR\plots"    
	RMDir /r "$INSTDIR\procedures"    
SectionEnd

#---ENGLISH---------------
LangString DESC_Core ${LANG_ENGLISH} "Installs the needed core files with the components for Windows 7."
LangString DESC_Compatibility ${LANG_ENGLISH} "Updates the components for compatibility for Windows 8.1 and 10. Not needed and not recommended for Windows 7."
LangString DESC_Samples ${LANG_ENGLISH} "Installs sample files (NumeRe scripts and data files)."
LangString DESC_Plugins ${LANG_ENGLISH} "Adds some installable extensions in the form of scripts to the script subdirectory."
LangString DESC_Fileallocs ${LANG_ENGLISH} "[NOT PORTABLE] Updates the Windows registry with file associations to the known file types. Creates an uninstaller, too."
LangString DESC_German ${LANG_ENGLISH} "Set German as main language."
LangString DESC_English ${LANG_ENGLISH} "Set English as main language."
LangString DESC_UnCore ${LANG_ENGLISH} "Removes all NumeRe core files and all related entries in the Windows registry."
LangString DESC_UnFileallocs ${LANG_ENGLISH} "Removes all saved files and all subdirectories in the NumeRe root directory."
#---GERMAN---------------
LangString DESC_Core ${LANG_GERMAN} "Installiert die nötigen NumeRe-Kerndateien mit den Komponenten für Windows 7."
LangString DESC_Compatibility ${LANG_GERMAN} "Aktualisiert die Komponenten für die Kompatibilität zu Windows 8.1 und 10. Für Windows 7 nicht nötig und nicht empfohlen."
LangString DESC_Samples ${LANG_GERMAN} "Installiert Beispieldateien (NumeRe-Scripte und Datenfiles)."
LangString DESC_Plugins ${LANG_GERMAN} "Fügt einige installierbare Erweiterungen in Form von Scripten zum Scripte-Unterverzeichnis hinzu."
LangString DESC_Fileallocs ${LANG_GERMAN} "[NICHT PORTABEL] Aktualisiert die Windowsregistrierung mit Dateiverknüpfungen zu bekannten Dateiformaten. Generiert außerdem eine Deinstallationsroutine."
LangString DESC_German ${LANG_GERMAN} "Deutsch als Programmsprache."
LangString DESC_English ${LANG_GERMAN} "Englisch als Programmsprache."
LangString DESC_UnCore ${LANG_GERMAN} "Entfernt alle NumeRe-Kerndateien und verbundene Einträge in der Windowsregistrierung."
LangString DESC_UnFileallocs ${LANG_GERMAN} "Entfernt alle gespeicherten Dateien und Unterverzeichnisse im NumeRe-Stammverzeichnis."

!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
	!insertmacro MUI_DESCRIPTION_TEXT ${Core} $(DESC_Core)
	!insertmacro MUI_DESCRIPTION_TEXT ${Compatibility} $(DESC_Compatibility)
	!insertmacro MUI_DESCRIPTION_TEXT ${Samples} $(DESC_Samples)
	!insertmacro MUI_DESCRIPTION_TEXT ${Plugins} $(DESC_Plugins)
	!insertmacro MUI_DESCRIPTION_TEXT ${Fileallocs} $(DESC_Fileallocs)
	!insertmacro MUI_DESCRIPTION_TEXT ${German} $(DESC_German)
	!insertmacro MUI_DESCRIPTION_TEXT ${English} $(DESC_English)
!insertmacro MUI_FUNCTION_DESCRIPTION_END

!insertmacro MUI_UNFUNCTION_DESCRIPTION_BEGIN
	!insertmacro MUI_DESCRIPTION_TEXT ${UnCore} $(DESC_UnCore)
	!insertmacro MUI_DESCRIPTION_TEXT ${UnFileallocs} $(DESC_UnFileallocs)
!insertmacro MUI_UNFUNCTION_DESCRIPTION_END


Function LaunchPDF
	ExecShell "" "$INSTDIR\docs\NumeRe-Dokumentation.pdf"
FunctionEnd

Function .onInit
	!insertmacro MUI_LANGDLL_DISPLAY
	${if} $LANGUAGE == ${LANG_ENGLISH}
		!insertmacro SelectSection ${English}
	${else}
		!insertmacro SelectSection ${German}
	${endif}
FunctionEnd

Function Un.onInit
  !insertmacro MUI_LANGDLL_DISPLAY
FunctionEnd

Function  VerifyInstDir # .onVerifyInstDir
	${StrStr} $0 $INSTDIR $PROGRAMFILES
	StrCmp $0 $PROGRAMFILES 0 NoAbort
		MessageBox MB_YESNO|MB_ICONEXCLAMATION "In $\"$PROGRAMFILES$\" kann NumeRe möglicherweise nicht korrekt ausgeführt werden, falls die Benutzerkontensteuerung aktiv ist. Trotzdem installieren?" IDYES Continue
			Abort
	NoAbort:
	${StrStr} $0 $INSTDIR $PROGRAMFILES64
	StrCmp $0 $PROGRAMFILES64 0 Continue
		MessageBox MB_YESNO|MB_ICONEXCLAMATION "In $\"$PROGRAMFILES64$\" kann NumeRe möglicherweise nicht korrekt ausgeführt werden, falls die Benutzerkontensteuerung aktiv ist. Trotzdem installieren?" IDYES Continue
			Abort
	Continue:
FunctionEnd

Function RefreshShellIcons
	System::Call 'shell32.dll::SHChangeNotify(i,i,i,i) v \
	(${SHCNE_ASSOCCHANGED}, ${SHCNF_IDLIST}, 0,0)'
FunctionEnd

Function Un.RefreshShellIcons
	System::Call 'shell32.dll::SHChangeNotify(i,i,i,i) v \
	(${SHCNE_ASSOCCHANGED}, ${SHCNF_IDLIST}, 0,0)'
FunctionEnd

Function StrStr
	Exch $R0
	Exch
	Exch $R1
	Push $R2
	Push $R3
	Push $R4
	Push $R5
	
	StrLen $R2 $R0
	StrLen $R3 $R1
	
	StrCpy $R4 0
	
	loop:
		StrCpy $R5 $R1 $R2 $R4
		
		StrCmp $R5 $R0 done
		IntCmp $R4 $R3 done 0 done
		IntOp $R4 $R4 + 1
		Goto loop
	done:
	StrCpy $R0 $R1 $R2 $R4
	
	Pop $R5
	Pop $R4
	Pop $R3
	Pop $R2
	Pop $R1
	Exch $R0
FunctionEnd

#eof
# NumeRe-Installer
# NSIS-Script
# 2016-05-08
# -------------------------------

# Modern UI laden
!include "MUI2.nsh"

# Variablen definieren
!define PRODUCT "NumeRe - Framework für Numerische Rechnungen (Portable)"
!define DIRNAME "NumeRe"
!define MUI_FILE "numere"
!define VERSION "1.1.0"# $\"Bloch$\""
!define VERSIONEXT " $\"rc2$\""
!define MUI_BRANDINGTEXT "NumeRe: Framework für Numerische Rechnungen v${VERSION}${VERSIONEXT} (Portable)"
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
Name "NumeRe v${VERSION}${VERSIONEXT} (Portable)"

# CRCCheck aktiveren
CRCCheck On

# Verwendung noch unklar. Ggf. auch unnötig
#!include "${NSISDIR}\Contrib\Modern UI\System.nsh"
#---------------------------------

# Ausgabedateiname
OutFile "numereportable.exe"

# LZMA-Compressor auswählen
SetCompressor "lzma"

# Modern UI-Graphiken umdefinieren
!define MUI_ICON "icons\icon.ico"
!define MUI_UNICON "icons\icon.ico"
!define MUI_WELCOMEFINISHPAGE_BITMAP "wizard.bmp"
!define MUI_UNWELCOMEFINISHPAGE_BITMAP "wizard.bmp"
#--------------------------------

VIProductVersion "${VERSION}.47"
VIAddVersionKey ProductName "${PRODUCT}"
VIAddVersionKey CompanyName "Erik Hänel et al."
VIAddVersionKey OriginalFilename "numereportable.exe"
VIAddVersionKey FileDescription "Installer für NumeRe: Framework für Numerische Rechnungen (Portable Version)"
VIAddVersionKey FileVersion "${VERSION}.47"
VIAddVersionKey InternalName "NumeRe-Installer (Portable)"
VIAddVersionKey LegalCopyright "(c) 2017, Erik Hänel et al. Installer und enthaltene Dateien unterstehen der GNU GPL v3"


# Standard-Dateiverzeichnis
InstallDir "C:\Software\${DIRNAME}"
InstallDirRegKey HKCU "Software\Classes\Applications\${PRODUCT}" ""
RequestExecutionLevel user

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


# Sprache
!insertmacro MUI_LANGUAGE "German"
!insertmacro MUI_LANGUAGE "English"
#-------------------------------- 

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

#---ENGLISH---------------
LangString DESC_Core ${LANG_ENGLISH} "Installs the needed core files with the components for Windows 7."
LangString DESC_Compatibility ${LANG_ENGLISH} "Updates the components for compatibility for Windows 8.1 and 10. Not needed and not recommended for Windows 7."
LangString DESC_Samples ${LANG_ENGLISH} "Installs sample files (NumeRe scripts and data files)."
LangString DESC_Plugins ${LANG_ENGLISH} "Adds some installable extensions in the form of scripts to the script subdirectory."
LangString DESC_German ${LANG_ENGLISH} "Set German as main language."
LangString DESC_English ${LANG_ENGLISH} "Set English as main language."
#---GERMAN---------------
LangString DESC_Core ${LANG_GERMAN} "Installiert die nötigen NumeRe-Kerndateien mit den Komponenten für Windows 7."
LangString DESC_Compatibility ${LANG_GERMAN} "Aktualisiert die Komponenten für die Kompatibilität zu Windows 8.1 und 10. Für Windows 7 nicht nötig und nicht empfohlen."
LangString DESC_German ${LANG_GERMAN} "Deutsch als Programmsprache."
LangString DESC_English ${LANG_GERMAN} "Englisch als Programmsprache."
LangString DESC_Samples ${LANG_GERMAN} "Installiert Beispieldateien (NumeRe-Scripte und Datenfiles)."
LangString DESC_Plugins ${LANG_GERMAN} "Fügt einige installierbare Erweiterungen in Form von Scripten zum Scripte-Unterverzeichnis hinzu."


!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
	!insertmacro MUI_DESCRIPTION_TEXT ${Core} $(DESC_Core)
	!insertmacro MUI_DESCRIPTION_TEXT ${Compatibility} $(DESC_Compatibility)
	!insertmacro MUI_DESCRIPTION_TEXT ${Samples} $(DESC_Samples)
	!insertmacro MUI_DESCRIPTION_TEXT ${Plugins} $(DESC_Plugins)
	!insertmacro MUI_DESCRIPTION_TEXT ${German} $(DESC_German)
	!insertmacro MUI_DESCRIPTION_TEXT ${English} $(DESC_English)	
!insertmacro MUI_FUNCTION_DESCRIPTION_END


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
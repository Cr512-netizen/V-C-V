; Inno Setup script for the Windows installer of V-C-V.
;
; It is not meant to be compiled by hand from Explorer. The Windows workflow
; stages a portable directory first, then calls ISCC with three defines:
;
;   ISCC.exe /DStageDir=<abs path to package-windows>
;             /DAppVersion=0.1.0.<run number>
;             /DOutDir=<abs path to dist>
;             installer\vcv.iss
;
; AppVersion must be four numeric parts, because it is copied into
; VersionInfoVersion of the produced exe: 0.1.0.17 is valid, 0.1.0-local is not.
;
; Building it locally works the same way once Inno Setup 6 is installed:
;
;   "C:\Program Files (x86)\Inno Setup 6\ISCC.exe" `
;     "/DStageDir=$PWD\package-windows" "/DAppVersion=0.1.0.0" `
;     "/DOutDir=$PWD\dist" installer\vcv.iss

#ifndef StageDir
  #define StageDir "..\package-windows"
#endif

#ifndef AppVersion
  #define AppVersion "0.1.0.0"
#endif

#ifndef OutDir
  #define OutDir "dist"
#endif

#define AppName "V-C-V"
#define AppPublisher "Nuc-lVuc-l"
#define AppExe "nuc_app.exe"
#define AppId "{{8F3C1B7A-2E4D-4C55-9A31-7B6D5E0C4A18}"

[Setup]
AppId={#AppId}
AppName={#AppName}
AppVersion={#AppVersion}
AppVerName={#AppName} {#AppVersion}
AppPublisher={#AppPublisher}
VersionInfoVersion={#AppVersion}
VersionInfoDescription={#AppName} installer
DefaultDirName={autopf}\{#AppName}
DefaultGroupName={#AppName}
DisableProgramGroupPage=yes
OutputDir={#OutDir}
OutputBaseFilename=V-C-V-Setup-x64
Compression=lzma2/max
SolidCompression=yes
WizardStyle=modern
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
UninstallDisplayIcon={app}\{#AppExe}
UninstallDisplayName={#AppName}
PrivilegesRequired=lowest
SetupLogging=yes
AllowNoIcons=yes

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"
Name: "russian"; MessagesFile: "compiler:Languages\Russian.isl"

[Tasks]
Name: "desktopicon"; Description: "Create a desktop shortcut"; GroupDescription: "Shortcuts:"; Flags: unchecked

[Files]
Source: "{#StageDir}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{group}\{#AppName}"; Filename: "{app}\{#AppExe}"
Name: "{group}\Uninstall {#AppName}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\{#AppName}"; Filename: "{app}\{#AppExe}"; Tasks: desktopicon

[Run]
Filename: "{app}\{#AppExe}"; Description: "Launch {#AppName}"; Flags: nowait postinstall skipifsilent

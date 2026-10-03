#ifndef AppVersion
  #define AppVersion "1.0.0"
#endif
#ifndef BundleDir
  #define BundleDir "..\dist\LogicLab"
#endif
#ifndef OutputDir
  #define OutputDir "..\dist\installers"
#endif

[Setup]
AppId={{9AF58938-EE6F-4D09-9C78-CC98971B2F65}
AppName=LogicLab
AppVersion={#AppVersion}
AppPublisher=LogicLab Project
AppPublisherURL=https://github.com/Zongyuan-Cai/Digital-logic-circuit
DefaultDirName={localappdata}\Programs\LogicLab
DefaultGroupName=LogicLab
DisableProgramGroupPage=yes
PrivilegesRequired=lowest
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
MinVersion=10.0
OutputDir={#OutputDir}
OutputBaseFilename=LogicLab-{#AppVersion}-Windows-x64-Setup
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
UninstallDisplayIcon={app}\LogicLab.exe
CloseApplications=yes
RestartApplications=no
CreateUninstallRegKey=not IsTestInstall

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "Create a desktop shortcut"; Flags: unchecked

[Files]
Source: "{#BundleDir}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\docs\安装包说明.md"; DestDir: "{app}"; DestName: "安装包说明.md"; Flags: ignoreversion

[Icons]
Name: "{group}\LogicLab"; Filename: "{app}\LogicLab.exe"; WorkingDir: "{app}"
Name: "{group}\Installation guide"; Filename: "{sys}\notepad.exe"; Parameters: """{app}\安装包说明.md"""
Name: "{group}\Uninstall LogicLab"; Filename: "{uninstallexe}"
Name: "{autodesktop}\LogicLab"; Filename: "{app}\LogicLab.exe"; WorkingDir: "{app}"; Tasks: desktopicon

[Run]
Filename: "{app}\LogicLab.exe"; Description: "Open LogicLab"; Flags: nowait postinstall skipifsilent

[Code]
function IsTestInstall: Boolean;
begin
  Result := ExpandConstant('{param:TESTINSTALL|0}') = '1';
end;

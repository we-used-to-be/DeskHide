#define AppVersion "1.0.0"
[Setup]
AppId={{7D56E20F-7281-4FB2-82A4-5D9C0712B036}
AppName=桌隐 DeskHide
AppVersion={#AppVersion}
DefaultDirName={localappdata}\Programs\DeskHide
DefaultGroupName=桌隐 DeskHide
PrivilegesRequired=lowest
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
MinVersion=10.0
AppMutex=DeskHide.7D56E20F-7281-4FB2-82A4-5D9C0712B036
CloseApplications=yes
RestartApplications=no
DisableProgramGroupPage=yes
UninstallDisplayIcon={app}\DeskHide.exe
SetupIconFile=..\assets\deskh.ico
OutputDir=..\artifacts
OutputBaseFilename=DeskHide-1.0.0-x64-Setup
Compression=lzma2/max
SolidCompression=yes
WizardStyle=modern
LicenseFile=..\LICENSE

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "autostart"; Description: "Start DeskHide when I sign in"; Flags: unchecked

[Files]
Source: "..\artifacts\DeskHide.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\LICENSE"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\THIRD_PARTY_NOTICES.md"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\README.md"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\README.zh-CN.md"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\docs\*.md"; DestDir: "{app}\docs"; Flags: ignoreversion

[Icons]
Name: "{group}\桌隐 DeskHide"; Filename: "{app}\DeskHide.exe"

[Registry]
Root: HKCU; Subkey: "Software\Microsoft\Windows\CurrentVersion\Run"; ValueType: string; ValueName: "DeskHide"; ValueData: """{app}\DeskHide.exe"""; Tasks: autostart; Flags: uninsdeletevalue
; Also remove a startup value enabled later from the tray, even if this task was unchecked.
Root: HKCU; Subkey: "Software\Microsoft\Windows\CurrentVersion\Run"; ValueName: "DeskHide"; Flags: dontcreatekey uninsdeletevalue

[Run]
Filename: "{app}\DeskHide.exe"; Description: "Launch DeskHide"; Flags: nowait postinstall skipifsilent

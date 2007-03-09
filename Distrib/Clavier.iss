#define RootDir "."
#define BinDir "."

#define AppName "Clavier+"
#define AppExeName "Clavier.exe"
#define InstallerFileName Copy(AppExeName, 1, RPos(".", AppExeName) - 1) + "Setup"
#define VersionFile AddBackslash(RootDir) + AddBackslash(BinDir) + AppExeName

#define AppVersion GetFileProductVersion(VersionFile)
#define AppVerName AppName + " " + AppVersion
#define AppPublisher "Guillaume Ryder"
#define AppURL "http://utilfr42.free.fr"

#define IniFile "Clavier.ini"

[Setup]
OutputDir=.
SourceDir={#RootDir}
ShowLanguageDialog=yes
OutputBaseFilename={#InstallerFileName}
AppName={#AppName}
AppVerName={#AppVerName}
AppVersion={#AppVersion}
AppPublisher={#AppPublisher}
AppPublisherURL={#AppURL}
AppSupportURL={#AppURL}
AppUpdatesURL={#AppURL}
DefaultDirName={pf}\{#AppName}
DefaultGroupName={#AppName}
VersionInfoVersion={#AppVersion}
VersionInfoCompany={#GetFileCompany(VersionFile)}
VersionInfoCopyright={#GetFileCopyright(VersionFile)}
Compression=lzma/ultra
SolidCompression=true
InternalCompressLevel=ultra

[Languages]
Name: english; MessagesFile: compiler:Default.isl
Name: french; MessagesFile: compiler:Languages\French.isl
Name: german; MessagesFile: compiler:Languages\German.isl

[Files]
Source: {#BinDir}\Clavier.exe; DestDir: {app}; Flags: ignoreversion
Source: {#BinDir}\ClavierEnglish.ini; DestDir: {app}; Flags: ignoreversion onlyifdoesntexist; DestName: {#IniFile}; Languages: english
Source: {#BinDir}\ClavierFrench.ini; DestDir: {app}; Flags: ignoreversion onlyifdoesntexist; DestName: {#IniFile}; Languages: french
Source: {#BinDir}\ClavierGerman.ini; DestDir: {app}; Flags: ignoreversion onlyifdoesntexist; DestName: {#IniFile}; Languages: german
Source: {#BinDir}\Help.htm; DestDir: {app}; Flags: ignoreversion; Languages: english, german
Source: {#BinDir}\Aide.htm; DestDir: {app}; Flags: ignoreversion; Languages: french
Source: {#BinDir}\Hilfe.htm; DestDir: {app}; Flags: ignoreversion; Languages: german

[Icons]
Name: {group}\{#AppName}; Filename: {app}\{#AppExeName}
Name: {group}\Help; Filename: {app}\Help.htm; Languages: english
Name: {group}\Aide; Filename: {app}\Aide.htm; Languages: french
Name: {group}\Hilfe; Filename: {app}\Hilfe.htm; Languages: german
Name: {group}\{cm:ProgramOnTheWeb,{#AppName}}; Filename: {#AppURL}
Name: {group}\{cm:UninstallProgram,{#AppName}}; Filename: {uninstallexe}

[Registry]
Root: HKCU; Subkey: Software\Microsoft\Windows\CurrentVersion\Run; ValueType: string; ValueName: {#AppName}; ValueData: {app}\{#AppExeName}; Flags: uninsdeletevalue

[Run]
Filename: {app}\{#AppExeName}; Description: {cm:LaunchProgram,{#AppName}}; Flags: postinstall skipifsilent nowait

[_ISTool]
UseAbsolutePaths=false

[Code]
// Close Clavier+ just before install
// Display Clavier+ settings just after install
procedure CurStepChanged(CurStep: TSetupStep);
begin
	if CurStep = ssInstall then
		SendBroadcastMessage(RegisterWindowMessage('RyderClavierOptions'), 1, 0)
	else if CurStep = ssDone then
	begin
		Sleep(200);
		PostBroadcastMessage(RegisterWindowMessage('RyderClavierOptions'), 0, $201)
	end
end;

// Close Clavier+ just before uninstall
procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
begin
	if CurUninstallStep = usUninstall then
		SendBroadcastMessage(RegisterWindowMessage('RyderClavierOptions'), 1, 0)
end;


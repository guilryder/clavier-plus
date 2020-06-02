#ifdef IS_X64
#define Platform "x64"
#define PlatformName "64"
#else
#define Platform "Win32"
#define PlatformName "32"
#endif

#define RootDir "."
#define BinDir "."
#define CompilOutputDir "..\output\" + Platform + "_Release\"

#define AppName "Clavier+"
#define AppExeName "Clavier.exe"
#define InstallerFileName Copy(AppExeName, 1, RPos(".", AppExeName) - 1) + "Setup" + PlatformName
#define VersionFile AddBackslash(RootDir) + AddBackslash(BinDir) + CompilOutputDir + AppExeName

#define AppVersion GetFileProductVersion(VersionFile)
#define AppVerName AppName + " " + AppVersion
#define AppPublisher "Guillaume Ryder"
#define AppURL "http://utilfr42.free.fr"
#define HelpFileBaseUrl "http://utilfr42.free.fr/util/ClavierDoc_"

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
DefaultDirName={localappdata}\{#AppName}
DefaultGroupName={#AppName}
VersionInfoVersion={#AppVersion}
VersionInfoCompany={#GetFileCompany(VersionFile)}
VersionInfoCopyright={#GetFileCopyright(VersionFile)}
Compression=lzma/ultra
SolidCompression=true
InternalCompressLevel=ultra
PrivilegesRequired=none
#ifdef IS_X64
ArchitecturesInstallIn64BitMode=x64
ArchitecturesAllowed=x64
#endif

[Languages]
Name: chinesesimpl; MessagesFile: compiler:Languages\ChineseSimplified.isl
Name: german; MessagesFile: compiler:Languages\German.isl
Name: english; MessagesFile: compiler:Default.isl
Name: spanish; MessagesFile: compiler:Languages\Spanish.isl
Name: french; MessagesFile: compiler:Languages\French.isl
Name: italian; MessagesFile: compiler:Languages\Italian.isl
Name: dutch; MessagesFile: compiler:Languages\Dutch.isl
Name: polish; MessagesFile: compiler:Languages\Polish.isl
Name: portuguese; MessagesFile: compiler:Languages\BrazilianPortuguese.isl
Name: slovak; MessagesFile: compiler:Languages\Slovak.isl
Name: greek; MessagesFile: compiler:Languages\Greek.isl
Name: russian; MessagesFile: compiler:Languages\Russian.isl

[Files]
Source: {#BinDir}\{#CompilOutputDir}{#AppExeName}; DestDir: {app}; Flags: ignoreversion
Source: {#BinDir}\Clavier_zh-CN.ini; DestDir: {app}; Flags: ignoreversion onlyifdoesntexist; DestName: {#IniFile}; Languages: chinesesimpl
Source: {#BinDir}\Clavier_de.ini; DestDir: {app}; Flags: ignoreversion onlyifdoesntexist; DestName: {#IniFile}; Languages: german
Source: {#BinDir}\Clavier_en.ini; DestDir: {app}; Flags: ignoreversion onlyifdoesntexist; DestName: {#IniFile}; Languages: english
Source: {#BinDir}\Clavier_es.ini; DestDir: {app}; Flags: ignoreversion onlyifdoesntexist; DestName: {#IniFile}; Languages: spanish
Source: {#BinDir}\Clavier_fr.ini; DestDir: {app}; Flags: ignoreversion onlyifdoesntexist; DestName: {#IniFile}; Languages: french
Source: {#BinDir}\Clavier_it.ini; DestDir: {app}; Flags: ignoreversion onlyifdoesntexist; DestName: {#IniFile}; Languages: italian
Source: {#BinDir}\Clavier_nl.ini; DestDir: {app}; Flags: ignoreversion onlyifdoesntexist; DestName: {#IniFile}; Languages: dutch
Source: {#BinDir}\Clavier_pl.ini; DestDir: {app}; Flags: ignoreversion onlyifdoesntexist; DestName: {#IniFile}; Languages: polish
Source: {#BinDir}\Clavier_pt-BR.ini; DestDir: {app}; Flags: ignoreversion onlyifdoesntexist; DestName: {#IniFile}; Languages: portuguese
Source: {#BinDir}\Clavier_sk.ini; DestDir: {app}; Flags: ignoreversion onlyifdoesntexist; DestName: {#IniFile}; Languages: slovak
Source: {#BinDir}\Clavier_el.ini; DestDir: {app}; Flags: ignoreversion onlyifdoesntexist; DestName: {#IniFile}; Languages: greek
Source: {#BinDir}\Clavier_ru.ini; DestDir: {app}; Flags: ignoreversion onlyifdoesntexist; DestName: {#IniFile}; Languages: russian

[Icons]
Name: {group}\{#AppName}; Filename: {app}\{#AppExeName}
Name: {group}\帮助; Filename: {#HelpFileBaseUrl}zh-CN.html; Languages: chinesesimpl
Name: {group}\Hilfe; Filename: {#HelpFileBaseUrl}de.html; Languages: german
Name: {group}\Help; Filename: {#HelpFileBaseUrl}en.html; Languages: english
Name: {group}\Ayuda; Filename: {#HelpFileBaseUrl}en.html; Languages: spanish
Name: {group}\Aide; Filename: {#HelpFileBaseUrl}fr.html; Languages: french
Name: {group}\Aiuto; Filename: {#HelpFileBaseUrl}it.html; Languages: italian
Name: {group}\Help; Filename: {#HelpFileBaseUrl}nl.html; Languages: dutch
Name: {group}\Pomoc; Filename: {#HelpFileBaseUrl}pl.html; Languages: polish
Name: {group}\Ajuda; Filename: {#HelpFileBaseUrl}pt-BR.html; Languages: portuguese
Name: {group}\Pomoc; Filename: {#HelpFileBaseUrl}sk.html; Languages: slovak
Name: {group}\Βοήθεια; Filename: {#HelpFileBaseUrl}el.html; Languages: greek
Name: {group}\Помощь; Filename: {#HelpFileBaseUrl}ru.html; Languages: russian
Name: {group}\{cm:ProgramOnTheWeb,{#AppName}}; Filename: {#AppURL}
Name: {group}\{cm:UninstallProgram,{#AppName}}; Filename: {uninstallexe}

[Registry]
Root: HKCU; Subkey: Software\Microsoft\Windows\CurrentVersion\Run; ValueType: string; ValueName: {#AppName}; ValueData: {app}\{#AppExeName}; Flags: uninsdeletevalue

[Run]
Filename: {app}\{#AppExeName}; Description: {cm:LaunchProgram,{#AppName}}; Flags: postinstall skipifsilent nowait

[_ISTool]
UseAbsolutePaths=false

[Code]
const
	MSG_CLAVIER_NOTIFY_ICON = 'RyderClavierOptions';
	WM_COMMAND = $111;
	WM_LBUTTONUP = $202;
	IDCCMD_QUIT = 1018;
	QUIT_POST_DELAY_MS = 1000;
	DISPLAY_CONFIG_PRE_DELAY_MS = 200;

// Close Clavier+ just before install
// Display Clavier+ settings just after install
procedure CurStepChanged(CurStep: TSetupStep);
begin
	if CurStep = ssInstall then
	begin
		PostBroadcastMessage(RegisterWindowMessage(MSG_CLAVIER_NOTIFY_ICON), IDCCMD_QUIT, WM_COMMAND);
		Sleep(QUIT_POST_DELAY_MS)
	end
	else if CurStep = ssDone then
	begin
		Sleep(DISPLAY_CONFIG_PRE_DELAY_MS);
		PostBroadcastMessage(RegisterWindowMessage(MSG_CLAVIER_NOTIFY_ICON), 0, WM_LBUTTONUP)
	end
end;

// Close Clavier+ just before uninstall
procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
begin
	if CurUninstallStep = usUninstall then
	begin
		PostBroadcastMessage(RegisterWindowMessage(MSG_CLAVIER_NOTIFY_ICON), IDCCMD_QUIT, WM_COMMAND)
		Sleep(QUIT_POST_DELAY_MS);
	end
end;

[code]
#define QtDir "C:/msys64/mingw64"
#define MingwDir "C:/msys64/mingw64"
#define ApplicationDir "../.."

#define AppName "DG-LAN"
#define ExePath ApplicationDir + "/Core/output/release/DG-LAN.Core.exe"
#define Version GetStringFileInfo(ExePath, 'ProductVersion')
#define VersionTag GetStringFileInfo(ExePath, 'VersionTag')
#define BuildTime GetStringFileInfo(ExePath, 'BuildTime')

[Setup]
AppName={#AppName}
AppVersion={#Version} {#VersionTag} - {#BuildTime}
SetupIconFile={#ApplicationDir}/Common/ressources/icon.ico
ArchitecturesInstallIn64BitMode=x64
DefaultDirName={pf}/{#AppName}
DefaultGroupName={#AppName}
UninstallDisplayIcon={app}/DG-LAN.Core.exe
Compression=lzma2
SolidCompression=yes
OutputDir=Installations
OutputBaseFilename={#AppName}-{#Version}{#VersionTag}-{#BuildTime}-Setup

[Files]
Source: "{#ApplicationDir}/Core/output/release/DG-LAN.Core.exe"; DestDir: "{app}"; Flags: comparetimestamp
Source: "{#ApplicationDir}/GUI/output/release/DG-LAN.GUI.exe"; DestDir: "{app}"; Flags: comparetimestamp
Source: "{#ApplicationDir}/translations/*.qm"; DestDir: "{app}/languages"; Flags: comparetimestamp skipifsourcedoesntexist
Source: "{#ApplicationDir}/styles/*"; DestDir: "{app}/styles"; Flags: comparetimestamp recursesubdirs createallsubdirs
Source: "{#ApplicationDir}/GUI/ressources/emoticons/*"; DestDir: "{app}/Emoticons"; Flags: comparetimestamp recursesubdirs createallsubdirs
Source: "{#QtDir}/bin/Qt5Core.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#QtDir}/bin/Qt5Gui.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#QtDir}/bin/Qt5Network.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#QtDir}/bin/Qt5Widgets.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#QtDir}/bin/Qt5Xml.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#QtDir}/bin/libicuin78.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#QtDir}/bin/libicuuc78.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#QtDir}/bin/libicudt78.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#QtDir}/bin/libgcc_s_seh-1.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#QtDir}/bin/libwinpthread-1.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#QtDir}/bin/libstdc++-6.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#QtDir}/bin/libdouble-conversion.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#QtDir}/bin/libpcre2-16-0.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#QtDir}/bin/zlib1.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#QtDir}/bin/libzstd.dll"; DestDir: "{app}"; Flags: ignoreversion
; OpenSSL (for HTTPS update checker)
Source: "{#QtDir}/bin/libssl-3-x64.dll"; DestDir: "{app}"; Flags: ignoreversion skipifsourcedoesntexist
Source: "{#QtDir}/bin/libcrypto-3-x64.dll"; DestDir: "{app}"; Flags: ignoreversion skipifsourcedoesntexist
Source: "{#QtDir}/bin/libssl-1_1-x64.dll"; DestDir: "{app}"; Flags: ignoreversion skipifsourcedoesntexist
Source: "{#QtDir}/bin/libcrypto-1_1-x64.dll"; DestDir: "{app}"; Flags: ignoreversion skipifsourcedoesntexist
; Qt5 GUI/font/image dependencies
Source: "{#QtDir}/bin/libmd4c.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#QtDir}/bin/libpng16-16.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#QtDir}/bin/libharfbuzz-0.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#QtDir}/bin/libfreetype-6.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#QtDir}/bin/libbrotlidec.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#QtDir}/bin/libbrotlicommon.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#QtDir}/bin/libbz2-1.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#QtDir}/bin/libglib-2.0-0.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#QtDir}/bin/libgraphite2.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#QtDir}/bin/libiconv-2.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#QtDir}/bin/libintl-8.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#QtDir}/bin/libpcre2-8-0.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#QtDir}/bin/libjpeg-8.dll"; DestDir: "{app}"; Flags: ignoreversion skipifsourcedoesntexist
Source: "{#QtDir}/share/qt5/plugins/platforms/qwindows.dll"; DestDir: "{app}/platforms"; Flags: ignoreversion
; Qt5 image format plugins
Source: "{#QtDir}/share/qt5/plugins/imageformats/qjpeg.dll"; DestDir: "{app}/imageformats"; Flags: ignoreversion skipifsourcedoesntexist
Source: "{#QtDir}/share/qt5/plugins/imageformats/qpng.dll"; DestDir: "{app}/imageformats"; Flags: ignoreversion skipifsourcedoesntexist
Source: "{#QtDir}/share/qt5/plugins/imageformats/qsvg.dll"; DestDir: "{app}/imageformats"; Flags: ignoreversion skipifsourcedoesntexist

[Icons]
Name: "{group}\DG-LAN"; Filename: "{app}/DG-LAN.GUI.exe"; WorkingDir: "{app}"

[Languages]
; Name has to be coded as ISO-639 (two letters).
Name: "en"; MessagesFile: "compiler:Default.isl,{#ApplicationDir}/translations/d_lan.en.isl"
Name: "fr"; MessagesFile: "compiler:Languages/French.isl,{#ApplicationDir}/translations/d_lan.fr.isl"

[Tasks]
Name: "Firewall"; Description: {cm:firewallException}; MinVersion: 0,5.01.2600sp2;
Name: "ResetSettings"; Description: {cm:resetSettings}

[Run]
Filename: "{sys}/netsh.exe"; Parameters: "firewall add allowedprogram ""{app}/DG-LAN.Core.exe"" ""DG-LAN.Core"" ENABLE ALL"; Flags: runhidden; MinVersion: 0,5.01.2600sp2; Tasks: Firewall
Filename: "{app}/DG-LAN.Core.exe"; Parameters: "--reset-settings"; Flags: RunHidden; Description: "Reset settings"; Tasks: ResetSettings
Filename: "{app}/DG-LAN.Core.exe"; Parameters: "-i --lang {language}"; Flags: RunHidden; Description: "Install the DG-LAN service and define the language"
Filename: "{app}/DG-LAN.GUI.exe"; Parameters: "--lang {language}"; Flags: RunHidden; Description: "Define the language for the GUI"
Filename: "{app}/DG-LAN.GUI.exe"; Flags: nowait postinstall runasoriginaluser; Description: "{cm:launchDLAN}"

[UninstallRun]
Filename: {app}/DG-LAN.Core.exe; Parameters: -u;
Filename: {sys}/netsh.exe; Parameters: "firewall delete allowedprogram program=""{app}/DG-LAN.Core.exe"""; Flags: runhidden; MinVersion: 0,5.01.2600sp2; Tasks: Firewall;

[code]
// Will stop the Core service.
function PrepareToInstall(var NeedsRestart: Boolean): String;
var
  ResultCode: integer;
begin
  Exec(ExpandConstant('{sys}/sc.exe'), 'stop "DG-LAN Core"', '', SW_HIDE, ewWaitUntilTerminated, ResultCode);
  Exec(ExpandConstant('{sys}/sc.exe'), 'stop "D-LAN Core"', '', SW_HIDE, ewWaitUntilTerminated, ResultCode);
  Exec(ExpandConstant('{sys}/taskkill.exe'), '/F /IM DG-LAN.Core.exe', '', SW_HIDE, ewWaitUntilTerminated, ResultCode);
  Exec(ExpandConstant('{sys}/taskkill.exe'), '/F /IM DG-LAN.GUI.exe', '', SW_HIDE, ewWaitUntilTerminated, ResultCode);
  Sleep(1000);
  NeedsRestart := False;
  Result := '';
end;

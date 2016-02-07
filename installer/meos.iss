; Script generated by the Inno Setup Script Wizard.
; SEE THE DOCUMENTATION FOR DETAILS ON CREATING INNO SETUP SCRIPT FILES!

[Setup]
; NOTE: The value of AppId uniquely identifies this application.
; Do not use the same AppId value in installers for other applications.
; (To generate a new GUID, click Tools | Generate GUID inside the IDE.)
AppId={{5A8ACFDC-9E0A-4B59-9D8F-4013251EB156}
AppName=MEOS-OZ
AppVersion=3.2.451.2
AppPublisher=undy
AppPublisherURL=http://sourceforge.net/projects/meosoz/
AppSupportURL=http://sourceforge.net/projects/meosoz/
AppUpdatesURL=http://sourceforge.net/projects/meosoz/
DefaultDirName={pf}\MEOS-OZ
DefaultGroupName=MEOS-OZ
LicenseFile=license.txt
OutputDir=.
OutputBaseFilename=setup
Compression=lzma
SolidCompression=yes

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
Source: "meos.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "libHaru.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "libmySQL.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "license.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "meos_doc_eng.html"; DestDir: "{app}"; Flags: ignoreversion
Source: "meos_doc_swe.html"; DestDir: "{app}"; Flags: ignoreversion
Source: "mysqlpp.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "thirdpartylicense.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "baseclass.xml"; DestDir: "{app}"; Flags: ignoreversion
Source: "database.clubs"; DestDir: "{app}"; Flags: ignoreversion
Source: "database.persons"; DestDir: "{app}"; Flags: ignoreversion
Source: "ind_finalresult.lxml"; DestDir: "{app}"; Flags: ignoreversion
Source: "ind_totalresult.lxml"; DestDir: "{app}"; Flags: ignoreversion
Source: "ind_courseresult.lxml"; DestDir: "{app}"; Flags: ignoreversion
Source: "sss201230.xml"; DestDir: "{app}"; Flags: ignoreversion
Source: "classcourse.lxml"; DestDir: "{app}"; Flags: ignoreversion
Source: "msvcm90.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "SSS Receipt Results.xml"; DestDir: "{app}"; Flags: ignoreversion
Source: "SSS Results.xml"; DestDir: "{app}"; Flags: ignoreversion
; NOTE: Don't use "Flags: ignoreversion" on any shared system files

[Icons]
Name: "{group}\MEOS-OZ"; Filename: "{app}\meos.exe"
Name: "{commondesktop}\MEOS-OZ"; Filename: "{app}\meos.exe"; Tasks: desktopicon

[Run]
Filename: "{app}\meos.exe"; Parameters: "-s"
Filename: "{app}\meos.exe"; Description: "{cm:LaunchProgram,MEOS-OZ}"; Flags: nowait postinstall skipifsilent


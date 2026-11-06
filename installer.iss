[Setup]
AppId={{D4A7E912-5B83-4F29-8C64-1E3A9B7D2F50}
AppName=AQUAMARINE PLUG - EQP - 1 Ultra VST3
AppVersion=2.1.0
AppPublisher=AQUAMARINE PLUG
DefaultDirName={commoncf64}\VST3
DirExistsWarning=no
AppendDefaultDirName=no
DisableProgramGroupPage=yes
PrivilegesRequired=admin
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
Compression=lzma2/max
SolidCompression=yes
WizardStyle=modern
OutputDir=Output
OutputBaseFilename=AQUAMARINE_PLUG_EQP1_Ultra_Setup

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"
Name: "russian"; MessagesFile: "compiler:Languages\Russian.isl"

[Files]
Source: "build\PultecEQP1A_artefacts\Release\VST3\*\Contents\*"; DestDir: "{app}\EQP - 1 Ultra.vst3\Contents"; Flags: ignoreversion recursesubdirs createallsubdirs

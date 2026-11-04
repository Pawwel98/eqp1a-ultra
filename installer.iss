[Setup]
AppId={{D4A7E912-5B83-4F29-8C64-1E3A9B7D2F50}
AppName=PULTEC EQP-1A ULTRA VST3
AppVersion=1.0.0
AppPublisher=CervinizatorAudio
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
OutputBaseFilename=PULTEC_EQP1A_ULTRA_Setup

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"
Name: "russian"; MessagesFile: "compiler:Languages\Russian.isl"

[Files]
Source: "build\PultecEQP1A_artefacts\Release\VST3\PULTEC EQP-1A ULTRA.vst3\*"; DestDir: "{app}\PULTEC EQP-1A ULTRA.vst3"; Flags: ignoreversion recursesubdirs createallsubdirs

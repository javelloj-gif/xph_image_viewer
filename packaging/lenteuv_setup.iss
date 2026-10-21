; ==============================================================================
; lenteuv_setup.iss — Inno Setup Script para LenteUV Image Viewer
; ==============================================================================
; Autor: xPhang (fork de Sergkruma's image viewer por Sergei Krumas)
; Licencia: ver LICENSE (GNU GPL v3)
;
; Compilar mediante scripts/build_release.ps1 -SkipIcons -Format Installer
; Resultado: dist/lenteuv_setup_<version>.exe
; ==============================================================================

#define MyAppName        "LenteUV Image Viewer"
#ifndef MyAppId
  #define MyAppId "{{D4A9F3B2-8C17-4E5F-A3D6-2B7E9C1F0A84}"
#endif
#ifndef MyAppVersion
  #error MyAppVersion must be passed by the packaging script
#endif
#define MyAppPublisher   "xPhang"
#define MyAppURL         "https://github.com/javelloj-gif/xph_image_viewer"
#define MyAppExeName     "lenteuv.exe"
#ifndef StageDir
  #error StageDir must be passed by the packaging script
#endif
#define MyAppIcon        StageDir + "\lenteuv.ico"
#ifndef MyOutputDir
  #error MyOutputDir must be passed by the packaging script
#endif

; ─── Extensions soportadas por LenteUV ───────────────────────────────────────
; Se registran en SystemFileAssociations para no romper aplicaciones predeterminadas.
#define SupportedExts "jpg,jpeg,jfif,png,apng,gif,webp,bmp,ico,tif,tiff,avif,heif,heic,jxr,wdp,hdp,svg,svgz"

[Setup]
; Información de la aplicación
AppId                    = {#MyAppId}
AppName                  = {#MyAppName}
AppVersion               = {#MyAppVersion}
AppVerName               = {#MyAppName} v{#MyAppVersion}
AppPublisher             = {#MyAppPublisher}
AppPublisherURL          = {#MyAppURL}
AppSupportURL            = {#MyAppURL}
AppUpdatesURL            = {#MyAppURL}

; Directorios
DefaultDirName           = {autopf}\LenteUV
DefaultGroupName         = {#MyAppName}
DisableDirPage           = no
DisableProgramGroupPage  = yes

; Icono del instalador
SetupIconFile            = {#MyAppIcon}

; Archivos de salida
OutputDir                = {#MyOutputDir}
OutputBaseFilename       = lenteuv_setup_{#MyAppVersion}

; Compresión
Compression              = lzma2/ultra64
SolidCompression         = yes
LZMAUseSeparateProcess   = yes

; Requisitos mínimos del sistema
MinVersion               = 10.0.18362

; Privilegios: pedir elevación para escribir en Program Files y HKLM
PrivilegesRequired       = admin
PrivilegesRequiredOverridesAllowed = dialog

; Pantalla de bienvenida
WizardStyle              = modern
WizardSizePercent        = 110

; Información adicional en el panel de control de Windows
UninstallDisplayIcon     = {app}\{#MyAppExeName}
UninstallDisplayName     = {#MyAppName}
VersionInfoVersion       = {#MyAppVersion}
VersionInfoCompany       = {#MyAppPublisher}
VersionInfoDescription   = {#MyAppName} Installer
VersionInfoCopyright     = Copyright (C) 2022-2026 xPhang. LenteUV is a fork of Sergkruma's image viewer by Sergei Krumas.
VersionInfoProductName   = {#MyAppName}
VersionInfoProductVersion= {#MyAppVersion}

; Permitir reiniciar si hay archivos bloqueados
CloseApplications        = yes
RestartApplications      = no

[Languages]
; Idioma del instalador (Inno Setup detecta automáticamente el idioma del sistema)
Name: "english";   MessagesFile: "compiler:Default.isl"
Name: "spanish";   MessagesFile: "compiler:Languages\Spanish.isl"
Name: "german";    MessagesFile: "compiler:Languages\German.isl"
Name: "french";    MessagesFile: "compiler:Languages\French.isl"
Name: "italian";   MessagesFile: "compiler:Languages\Italian.isl"
Name: "russian";   MessagesFile: "compiler:Languages\Russian.isl"

[Tasks]
; Opciones que el usuario puede elegir durante la instalación
Name: "desktopicon";    Description: "Crear acceso directo en el &Escritorio";     GroupDescription: "Accesos directos:"; Flags: unchecked
Name: "contextmenu";    Description: "Integrar en el &menú contextual del Explorador de archivos"; GroupDescription: "Integración del sistema:"; Flags: checkedonce
Name: "contextfolder";  Description: "Opción ""Abrir carpeta en LenteUV"" en carpetas";  GroupDescription: "Integración del sistema:"; Flags: checkedonce; Check: WizardIsTaskSelected('contextmenu')

[Files]
; Fresh staging includes EXE, VC runtime, translations and portable defaults.
Source: "{#StageDir}\*"; DestDir: "{app}"; Excludes: "user_data\*"; Flags: ignoreversion recursesubdirs createallsubdirs
; Existing preferences and sessions belong to the user, including on upgrade/uninstall.
Source: "{#StageDir}\user_data\*"; DestDir: "{app}\user_data"; Flags: onlyifdoesntexist uninsneveruninstall recursesubdirs createallsubdirs

[Dirs]
Name: "{app}\user_data"; Flags: uninsneveruninstall
Name: "{app}\user_data\boards"; Flags: uninsneveruninstall
Name: "{app}\user_data\cache"; Flags: uninsneveruninstall
Name: "{app}\user_data\tags"; Flags: uninsneveruninstall

[Icons]
; Acceso directo en el Menú de Inicio
Name: "{group}\{#MyAppName}";           Filename: "{app}\{#MyAppExeName}"; IconFilename: "{app}\lenteuv.ico"; Comment: "Visualizador de imágenes LenteUV"
Name: "{group}\Desinstalar {#MyAppName}"; Filename: "{uninstallexe}";

; Acceso directo en el Escritorio (opcional, según tarea seleccionada)
Name: "{autodesktop}\{#MyAppName}";     Filename: "{app}\{#MyAppExeName}"; IconFilename: "{app}\lenteuv.ico"; Tasks: desktopicon; Comment: "Visualizador de imágenes LenteUV"

[Registry]
; ==============================================================================
; MENÚ CONTEXTUAL — Archivos de imagen soportados
; Estrategia: SystemFileAssociations no roba la asociación predeterminada,
;             solo añade la opción al submenú "Abrir con".
; ==============================================================================

; .jpg / .jpeg
Root: HKCR; Subkey: "SystemFileAssociations\.jpg\shell\LenteUV";          ValueType: string; ValueName: ""; ValueData: "Abrir con LenteUV";                Tasks: contextmenu; Flags: uninsdeletekey
Root: HKCR; Subkey: "SystemFileAssociations\.jpg\shell\LenteUV";          ValueType: string; ValueName: "Icon"; ValueData: """{app}\lenteuv.ico"",0";     Tasks: contextmenu
Root: HKCR; Subkey: "SystemFileAssociations\.jpg\shell\LenteUV\command";  ValueType: string; ValueName: ""; ValueData: """{app}\{#MyAppExeName}"" ""%1"""; Tasks: contextmenu

Root: HKCR; Subkey: "SystemFileAssociations\.jpeg\shell\LenteUV";         ValueType: string; ValueName: ""; ValueData: "Abrir con LenteUV";                Tasks: contextmenu; Flags: uninsdeletekey
Root: HKCR; Subkey: "SystemFileAssociations\.jpeg\shell\LenteUV";         ValueType: string; ValueName: "Icon"; ValueData: """{app}\lenteuv.ico"",0";     Tasks: contextmenu
Root: HKCR; Subkey: "SystemFileAssociations\.jpeg\shell\LenteUV\command"; ValueType: string; ValueName: ""; ValueData: """{app}\{#MyAppExeName}"" ""%1"""; Tasks: contextmenu

Root: HKCR; Subkey: "SystemFileAssociations\.jfif\shell\LenteUV";         ValueType: string; ValueName: ""; ValueData: "Abrir con LenteUV";                Tasks: contextmenu; Flags: uninsdeletekey
Root: HKCR; Subkey: "SystemFileAssociations\.jfif\shell\LenteUV";         ValueType: string; ValueName: "Icon"; ValueData: """{app}\lenteuv.ico"",0";     Tasks: contextmenu
Root: HKCR; Subkey: "SystemFileAssociations\.jfif\shell\LenteUV\command"; ValueType: string; ValueName: ""; ValueData: """{app}\{#MyAppExeName}"" ""%1"""; Tasks: contextmenu

; .png / .apng
Root: HKCR; Subkey: "SystemFileAssociations\.png\shell\LenteUV";          ValueType: string; ValueName: ""; ValueData: "Abrir con LenteUV";                Tasks: contextmenu; Flags: uninsdeletekey
Root: HKCR; Subkey: "SystemFileAssociations\.png\shell\LenteUV";          ValueType: string; ValueName: "Icon"; ValueData: """{app}\lenteuv.ico"",0";     Tasks: contextmenu
Root: HKCR; Subkey: "SystemFileAssociations\.png\shell\LenteUV\command";  ValueType: string; ValueName: ""; ValueData: """{app}\{#MyAppExeName}"" ""%1"""; Tasks: contextmenu

Root: HKCR; Subkey: "SystemFileAssociations\.apng\shell\LenteUV";         ValueType: string; ValueName: ""; ValueData: "Abrir con LenteUV";                Tasks: contextmenu; Flags: uninsdeletekey
Root: HKCR; Subkey: "SystemFileAssociations\.apng\shell\LenteUV";         ValueType: string; ValueName: "Icon"; ValueData: """{app}\lenteuv.ico"",0";     Tasks: contextmenu
Root: HKCR; Subkey: "SystemFileAssociations\.apng\shell\LenteUV\command"; ValueType: string; ValueName: ""; ValueData: """{app}\{#MyAppExeName}"" ""%1"""; Tasks: contextmenu

; .gif
Root: HKCR; Subkey: "SystemFileAssociations\.gif\shell\LenteUV";          ValueType: string; ValueName: ""; ValueData: "Abrir con LenteUV";                Tasks: contextmenu; Flags: uninsdeletekey
Root: HKCR; Subkey: "SystemFileAssociations\.gif\shell\LenteUV";          ValueType: string; ValueName: "Icon"; ValueData: """{app}\lenteuv.ico"",0";     Tasks: contextmenu
Root: HKCR; Subkey: "SystemFileAssociations\.gif\shell\LenteUV\command";  ValueType: string; ValueName: ""; ValueData: """{app}\{#MyAppExeName}"" ""%1"""; Tasks: contextmenu

; .webp
Root: HKCR; Subkey: "SystemFileAssociations\.webp\shell\LenteUV";         ValueType: string; ValueName: ""; ValueData: "Abrir con LenteUV";                Tasks: contextmenu; Flags: uninsdeletekey
Root: HKCR; Subkey: "SystemFileAssociations\.webp\shell\LenteUV";         ValueType: string; ValueName: "Icon"; ValueData: """{app}\lenteuv.ico"",0";     Tasks: contextmenu
Root: HKCR; Subkey: "SystemFileAssociations\.webp\shell\LenteUV\command"; ValueType: string; ValueName: ""; ValueData: """{app}\{#MyAppExeName}"" ""%1"""; Tasks: contextmenu

; .bmp
Root: HKCR; Subkey: "SystemFileAssociations\.bmp\shell\LenteUV";          ValueType: string; ValueName: ""; ValueData: "Abrir con LenteUV";                Tasks: contextmenu; Flags: uninsdeletekey
Root: HKCR; Subkey: "SystemFileAssociations\.bmp\shell\LenteUV";          ValueType: string; ValueName: "Icon"; ValueData: """{app}\lenteuv.ico"",0";     Tasks: contextmenu
Root: HKCR; Subkey: "SystemFileAssociations\.bmp\shell\LenteUV\command";  ValueType: string; ValueName: ""; ValueData: """{app}\{#MyAppExeName}"" ""%1"""; Tasks: contextmenu

; .ico
Root: HKCR; Subkey: "SystemFileAssociations\.ico\shell\LenteUV";          ValueType: string; ValueName: ""; ValueData: "Abrir con LenteUV";                Tasks: contextmenu; Flags: uninsdeletekey
Root: HKCR; Subkey: "SystemFileAssociations\.ico\shell\LenteUV";          ValueType: string; ValueName: "Icon"; ValueData: """{app}\lenteuv.ico"",0";     Tasks: contextmenu
Root: HKCR; Subkey: "SystemFileAssociations\.ico\shell\LenteUV\command";  ValueType: string; ValueName: ""; ValueData: """{app}\{#MyAppExeName}"" ""%1"""; Tasks: contextmenu

; .tif / .tiff
Root: HKCR; Subkey: "SystemFileAssociations\.tif\shell\LenteUV";          ValueType: string; ValueName: ""; ValueData: "Abrir con LenteUV";                Tasks: contextmenu; Flags: uninsdeletekey
Root: HKCR; Subkey: "SystemFileAssociations\.tif\shell\LenteUV";          ValueType: string; ValueName: "Icon"; ValueData: """{app}\lenteuv.ico"",0";     Tasks: contextmenu
Root: HKCR; Subkey: "SystemFileAssociations\.tif\shell\LenteUV\command";  ValueType: string; ValueName: ""; ValueData: """{app}\{#MyAppExeName}"" ""%1"""; Tasks: contextmenu

Root: HKCR; Subkey: "SystemFileAssociations\.tiff\shell\LenteUV";         ValueType: string; ValueName: ""; ValueData: "Abrir con LenteUV";                Tasks: contextmenu; Flags: uninsdeletekey
Root: HKCR; Subkey: "SystemFileAssociations\.tiff\shell\LenteUV";         ValueType: string; ValueName: "Icon"; ValueData: """{app}\lenteuv.ico"",0";     Tasks: contextmenu
Root: HKCR; Subkey: "SystemFileAssociations\.tiff\shell\LenteUV\command"; ValueType: string; ValueName: ""; ValueData: """{app}\{#MyAppExeName}"" ""%1"""; Tasks: contextmenu

; .avif / .heif / .heic
Root: HKCR; Subkey: "SystemFileAssociations\.avif\shell\LenteUV";         ValueType: string; ValueName: ""; ValueData: "Abrir con LenteUV";                Tasks: contextmenu; Flags: uninsdeletekey
Root: HKCR; Subkey: "SystemFileAssociations\.avif\shell\LenteUV";         ValueType: string; ValueName: "Icon"; ValueData: """{app}\lenteuv.ico"",0";     Tasks: contextmenu
Root: HKCR; Subkey: "SystemFileAssociations\.avif\shell\LenteUV\command"; ValueType: string; ValueName: ""; ValueData: """{app}\{#MyAppExeName}"" ""%1"""; Tasks: contextmenu

Root: HKCR; Subkey: "SystemFileAssociations\.heif\shell\LenteUV";         ValueType: string; ValueName: ""; ValueData: "Abrir con LenteUV";                Tasks: contextmenu; Flags: uninsdeletekey
Root: HKCR; Subkey: "SystemFileAssociations\.heif\shell\LenteUV";         ValueType: string; ValueName: "Icon"; ValueData: """{app}\lenteuv.ico"",0";     Tasks: contextmenu
Root: HKCR; Subkey: "SystemFileAssociations\.heif\shell\LenteUV\command"; ValueType: string; ValueName: ""; ValueData: """{app}\{#MyAppExeName}"" ""%1"""; Tasks: contextmenu

Root: HKCR; Subkey: "SystemFileAssociations\.heic\shell\LenteUV";         ValueType: string; ValueName: ""; ValueData: "Abrir con LenteUV";                Tasks: contextmenu; Flags: uninsdeletekey
Root: HKCR; Subkey: "SystemFileAssociations\.heic\shell\LenteUV";         ValueType: string; ValueName: "Icon"; ValueData: """{app}\lenteuv.ico"",0";     Tasks: contextmenu
Root: HKCR; Subkey: "SystemFileAssociations\.heic\shell\LenteUV\command"; ValueType: string; ValueName: ""; ValueData: """{app}\{#MyAppExeName}"" ""%1"""; Tasks: contextmenu

; .jxr / .wdp / .hdp (JPEG XR)
Root: HKCR; Subkey: "SystemFileAssociations\.jxr\shell\LenteUV";          ValueType: string; ValueName: ""; ValueData: "Abrir con LenteUV";                Tasks: contextmenu; Flags: uninsdeletekey
Root: HKCR; Subkey: "SystemFileAssociations\.jxr\shell\LenteUV";          ValueType: string; ValueName: "Icon"; ValueData: """{app}\lenteuv.ico"",0";     Tasks: contextmenu
Root: HKCR; Subkey: "SystemFileAssociations\.jxr\shell\LenteUV\command";  ValueType: string; ValueName: ""; ValueData: """{app}\{#MyAppExeName}"" ""%1"""; Tasks: contextmenu

Root: HKCR; Subkey: "SystemFileAssociations\.wdp\shell\LenteUV";          ValueType: string; ValueName: ""; ValueData: "Abrir con LenteUV";                Tasks: contextmenu; Flags: uninsdeletekey
Root: HKCR; Subkey: "SystemFileAssociations\.wdp\shell\LenteUV";          ValueType: string; ValueName: "Icon"; ValueData: """{app}\lenteuv.ico"",0";     Tasks: contextmenu
Root: HKCR; Subkey: "SystemFileAssociations\.wdp\shell\LenteUV\command";  ValueType: string; ValueName: ""; ValueData: """{app}\{#MyAppExeName}"" ""%1"""; Tasks: contextmenu

Root: HKCR; Subkey: "SystemFileAssociations\.hdp\shell\LenteUV";          ValueType: string; ValueName: ""; ValueData: "Abrir con LenteUV";                Tasks: contextmenu; Flags: uninsdeletekey
Root: HKCR; Subkey: "SystemFileAssociations\.hdp\shell\LenteUV";          ValueType: string; ValueName: "Icon"; ValueData: """{app}\lenteuv.ico"",0";     Tasks: contextmenu
Root: HKCR; Subkey: "SystemFileAssociations\.hdp\shell\LenteUV\command";  ValueType: string; ValueName: ""; ValueData: """{app}\{#MyAppExeName}"" ""%1"""; Tasks: contextmenu

; .svg / .svgz
Root: HKCR; Subkey: "SystemFileAssociations\.svg\shell\LenteUV";          ValueType: string; ValueName: ""; ValueData: "Abrir con LenteUV";                Tasks: contextmenu; Flags: uninsdeletekey
Root: HKCR; Subkey: "SystemFileAssociations\.svg\shell\LenteUV";          ValueType: string; ValueName: "Icon"; ValueData: """{app}\lenteuv.ico"",0";     Tasks: contextmenu
Root: HKCR; Subkey: "SystemFileAssociations\.svg\shell\LenteUV\command";  ValueType: string; ValueName: ""; ValueData: """{app}\{#MyAppExeName}"" ""%1"""; Tasks: contextmenu

Root: HKCR; Subkey: "SystemFileAssociations\.svgz\shell\LenteUV";         ValueType: string; ValueName: ""; ValueData: "Abrir con LenteUV";                Tasks: contextmenu; Flags: uninsdeletekey
Root: HKCR; Subkey: "SystemFileAssociations\.svgz\shell\LenteUV";         ValueType: string; ValueName: "Icon"; ValueData: """{app}\lenteuv.ico"",0";     Tasks: contextmenu
Root: HKCR; Subkey: "SystemFileAssociations\.svgz\shell\LenteUV\command"; ValueType: string; ValueName: ""; ValueData: """{app}\{#MyAppExeName}"" ""%1"""; Tasks: contextmenu

; ==============================================================================
; MENÚ CONTEXTUAL — Directorios (clic derecho sobre una carpeta)
; ==============================================================================
Root: HKCR; Subkey: "Directory\shell\LenteUV";          ValueType: string; ValueName: ""; ValueData: "Abrir carpeta en LenteUV";              Tasks: contextfolder; Flags: uninsdeletekey
Root: HKCR; Subkey: "Directory\shell\LenteUV";          ValueType: string; ValueName: "Icon"; ValueData: """{app}\lenteuv.ico"",0";           Tasks: contextfolder
Root: HKCR; Subkey: "Directory\shell\LenteUV\command";  ValueType: string; ValueName: ""; ValueData: """{app}\{#MyAppExeName}"" ""%1""";      Tasks: contextfolder

; Fondo de carpeta (clic derecho sobre el fondo de una ventana del Explorador)
Root: HKCR; Subkey: "Directory\Background\shell\LenteUV";         ValueType: string; ValueName: ""; ValueData: "Abrir aquí con LenteUV";              Tasks: contextfolder; Flags: uninsdeletekey
Root: HKCR; Subkey: "Directory\Background\shell\LenteUV";         ValueType: string; ValueName: "Icon"; ValueData: """{app}\lenteuv.ico"",0";         Tasks: contextfolder
Root: HKCR; Subkey: "Directory\Background\shell\LenteUV\command"; ValueType: string; ValueName: ""; ValueData: """{app}\{#MyAppExeName}"" ""%V""";     Tasks: contextfolder

[Run]
; Opción de ejecutar la aplicación al finalizar el instalador
Filename: "{app}\{#MyAppExeName}"; Description: "Iniciar {#MyAppName}"; Flags: nowait postinstall skipifsilent

[UninstallDelete]
; Eliminar datos del usuario creados en runtime (sesión, favoritos, caché)
; Opcional: comentar si se quiere preservar los datos del usuario al desinstalar
; Type: filesandordirs; Name: "{userappdata}\LenteUV"
; (LenteUV guarda user_data/ junto al exe — se elimina automáticamente con los archivos)

; Eliminar el directorio de instalación si quedó vacío
Type: dirifempty; Name: "{app}"

[Code]
// ─── Refrescar el caché de iconos del Explorador al finalizar ─────────────────
// Nota: DirExists() es una función nativa de Inno Setup Pascal — no es necesario redefinirla.
procedure CurStepChanged(CurStep: TSetupStep);
var
  ResultCode: Integer;
begin
  if CurStep = ssDone then
  begin
    // Notificar al sistema que las asociaciones de archivo cambiaron
    Exec('cmd.exe', '/c ie4uinit.exe -show', '', SW_HIDE, ewWaitUntilTerminated, ResultCode);
  end;
end;

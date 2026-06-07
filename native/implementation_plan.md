# Fix: Crash 0xC0000005 al re-abrir la versión portable via asociación de archivos

## Descripción del Problema

Al realizar una asociación de archivos Windows (Abrir con → lenteuv.exe), cerrar la aplicación y volver a abrirla, la app crashea con `ExceptionCode: 0xC0000005` (Access Violation). La única solución temporal es eliminar y recrear la carpeta portable.

## Análisis de Causa Raíz

### Causa Primaria: `SingleInstanceIpc` nunca integrado en `App`

La clase `SingleInstanceIpc` existe en `single_instance_ipc.h/.cpp` pero:
1. **No está en `CMakeLists.txt`** (SOURCES list) → no se compila al binario
2. **No hay `m_ipc` member** en `App` → nunca se instancia
3. **`WM_IPC_OPEN_PATH` no se maneja** en `window.cpp` → el PostMessage del hilo IPC cae al `DefWindowProcW`

### Causa Secundaria: `hide_to_tray_on_close = true` por defecto

Con el valor por defecto `hide_to_tray_on_close = true` en `settings.h`:
- Al cerrar con el botón X → la ventana se **oculta** (no se destruye)
- El proceso sigue corriendo en el systray
- El usuario intenta re-abrir via asociación → **se lanza un segundo proceso**
- Ambos procesos: crean directamente DirectComposition, registran tray icons con el mismo `uID`, etc.
- Resultado: Access Violation (corrupción de estado COM/D3D)

### Causa Terciaria: Falta de Mutex de instancia única

Sin un named mutex o pipe guardián, no hay forma de detectar que ya hay una instancia activa antes de inicializar recursos pesados.

---

## Solución Propuesta (3 cambios coordinados)

### Cambio 1: Integrar `SingleInstanceIpc` completamente

**Archivos modificados:**
- `CMakeLists.txt` → agregar `single_instance_ipc.cpp` a SOURCES y header a HEADERS  
- `app.h` → agregar `#include "core/single_instance_ipc.h"` y `SingleInstanceIpc m_ipc;`
- `app.cpp` → llamar `m_ipc.Initialize(hwnd, config.initialFilePath)` after window creation; si retorna `false` → retornar `InitResult::IpcHandedOff` (nueva variante) para que el proceso secundario salga limpiamente
- `window.cpp` → agregar `case SingleInstanceIpc::WM_IPC_OPEN_PATH:` en `HandleMessage`
- `app.cpp` → agregar callback `events.onIpcOpenPath` que llame a `LoadImage` con el path recibido

### Cambio 2: Corregir el valor por defecto de `hide_to_tray_on_close`

En `settings.h`, cambiar el default de `hide_to_tray_on_close` de `true` a `false`.

> **Razón**: Para la versión portable sin IPC activo (antes del fix), el tray ocultaba silenciosamente el proceso. Con IPC activo, si se quiere mantener `hide_to_tray_on_close = true` es posible, pero el segundo intento de apertura via asociación enviará el path al primer proceso via IPC y el segundo saldrá.

### Cambio 3: Agregar variante `InitResult::IpcHandedOff` 

Para que `main.cpp` pueda distinguir entre un fallo real y un "handoff exitoso" al primer proceso, evitando mostrar un mensaje de error.

---

## Proposed Changes

### CMake Build System

#### [MODIFY] [CMakeLists.txt](file:///e:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/CMakeLists.txt)
- Agregar `src/core/single_instance_ipc.cpp` a la lista `SOURCES`
- Agregar `src/core/single_instance_ipc.h` a la lista `HEADERS`

---

### Core Application

#### [MODIFY] [settings.h](file:///e:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/core/settings.h)
- Cambiar `bool hide_to_tray_on_close = true;` → `bool hide_to_tray_on_close = false;`

#### [MODIFY] [app.h](file:///e:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/core/app.h)
- Agregar `#include "core/single_instance_ipc.h"` en los includes
- Agregar `SingleInstanceIpc m_ipc;` como member de la clase `App`

#### [MODIFY] [app.cpp](file:///e:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/core/app.cpp)
- En `Initialize()`: después de crear la ventana y antes de inicializar el renderer, llamar a `m_ipc.Initialize(m_window.GetHwnd(), config.initialFilePath)`
- Si retorna `false`: retornar `InitResult::IpcHandedOff`
- Agregar el callback `events.onIpcOpenPath` que reciba `std::wstring path` y llame a `LoadImage(path)` + `m_window.Show()` + `SetForegroundWindow`

#### [MODIFY] [window.h](file:///e:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/core/window.h)
- Agregar `std::function<void(const std::wstring&)> onIpcOpenPath;` en la estructura de eventos

#### [MODIFY] [window.cpp](file:///e:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/core/window.cpp)
- Agregar `#include "core/single_instance_ipc.h"` (para la constante `WM_IPC_OPEN_PATH`)
- En `HandleMessage()`, agregar `case SingleInstanceIpc::WM_IPC_OPEN_PATH:` para manejar el mensaje: recuperar el `std::wstring*` de `wParam`, llamar `events.onIpcOpenPath(*pPath)`, y `delete pPath`

---

### Entry Point

#### [MODIFY] [main.cpp](file:///e:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/main.cpp)
- Agregar `enum class InitResult { Success, Failed, IpcHandedOff };` (ya existe en `app.h`, solo actualizar el enum)
- En `wWinMainImpl()`, si `initResult == InitResult::IpcHandedOff`: retornar 0 silenciosamente (sin MessageBox de error)

---

## Flujo Corregido

```
[Primer launch]
  wWinMain → App::Initialize()
    → m_ipc.Initialize(hwnd, filePath)
      → Intenta conectar al pipe → falla (no hay servidor)
      → Crea servidor pipe, retorna true
    → Continúa inicialización normal
    → LoadImage(filePath) si filePath != ""

[App oculta en tray o visible]

[Segundo launch via asociación de archivo]
  wWinMain → App::Initialize()
    → m_ipc.Initialize(hwnd, newFilePath)  
      → Detecta pipe existente
      → Envía newFilePath al primer proceso via pipe
      → Retorna false
    → App::Initialize() retorna IpcHandedOff
  → wWinMainImpl retorna 0 (sin error, silenciosamente)

[Primer proceso recibe WM_IPC_OPEN_PATH]
  → App::events.onIpcOpenPath(newFilePath)
    → LoadImage(newFilePath)
    → m_window.Show()
    → SetForegroundWindow(hwnd)
```

---

## Verification Plan

### Build Verification
```powershell
$cmake = "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
& $cmake --build native/build --config Release --target lenteuv 2>&1 | Select-Object -Last 30
```

### Manual Test Sequence
1. Abrir `lenteuv.exe` directamente → verificar que abre normalmente
2. Abrir una imagen con `lenteuv.exe` via doble clic o "Abrir con" → debe abrir y mostrar la imagen
3. Cerrar la app (X o ESC)
4. Intentar re-abrir directamente → debe abrir normalmente
5. Abrir una imagen vía "Abrir con" mientras la app ya está abierta → la app existente debe traerse al frente y mostrar la imagen
6. Verificar que NO se crea `debug_ipc.txt` en directorios inesperados


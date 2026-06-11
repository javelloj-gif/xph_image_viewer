# Plan de Implementación — Corrección del Error de Arranque de la Sesión al Asociar Archivos

Este plan detalla la solución para el bug de arranque donde la aplicación falla al abrirse desde una asociación de archivos si existe un archivo `session.txt` poblado.

## Razón del error
Durante `App::Initialize()`, el método `m_libraryData.LoadSession()` es llamado de forma prematura (antes de que la ventana y el renderizador Direct2D estén creados y configurados). Si `session.txt` contiene directorios con imágenes, `LoadSession()` intenta cargar automáticamente la primera imagen llamando a `m_app->LoadImage()`, que a su vez invoca `ImageLoader::UploadToGpu()` e intenta acceder al contexto Direct2D/Direct3D nulo de `m_renderer`, resultando en un fallo de violación de acceso de puntero (crash de segmentación).

## Proposed Changes

### 1. Modificación de la Firma de LoadSession

#### [MODIFY] [folder_data_manager.h](file:///e:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/library/folder_data_manager.h)
* Cambiar la firma de `bool LoadSession();` a `bool LoadSession(bool loadImage = true);`.

#### [MODIFY] [folder_data_manager.cpp](file:///e:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/library/folder_data_manager.cpp)
* Modificar la definición de `FolderDataManager::LoadSession(bool loadImage)` para pasar `loadImage` a `SetCurrentFolderIndex(0, loadImage)`.

### 2. Sincronización del Arranque en App

#### [MODIFY] [app.cpp](file:///e:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/core/app.cpp)
* Eliminar la llamada síncrona prematura `m_libraryData.LoadSession();` de las fases iniciales de `App::Initialize` (donde el renderer es nulo).
* Insertar la llamada a `m_libraryData.LoadSession(loadSessionImage)` justo antes del bloque `// ── Load Initial Content ──────────────────────────────────────────────`.
* Determinar si se debe cargar la imagen de la sesión basándose en si se ha suministrado una ruta inicial por línea de comandos (evitando doble lectura).

---

## Verification Plan

### Automated Tests
* Compilar la aplicación en modo Release:
  ```powershell
  & "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build native/build --config Release
  ```

### Manual Verification
1. Crear un archivo `session.txt` con un directorio que contenga imágenes.
2. Iniciar la aplicación pasando un archivo de imagen como parámetro de línea de comandos (simulando la asociación de archivos):
   ```powershell
   & native/build/bin/Release/lenteuv.exe test/linux.png
   ```
3. Verificar que la aplicación inicia correctamente, muestra la imagen especificada, y no se congela ni crashea.
4. Cerrar y abrir la aplicación sin argumentos. Confirmar que restaura correctamente la sesión anterior y carga la primera imagen de la sesión.

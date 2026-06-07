# Plan: Bug de Crash en Versión Portable de LenteUV

## Problema

La versión portable de LenteUV crashea al ejecutarse mediante asociación de archivos de Windows ("Abrir siempre con"). El crash ocurre con SEH exception `0xC0000005` (Access Violation) después de `FolderDataManager initialized` pero antes de completar la inicialización.

**Pasos para reproducir:**
1. Abrir una imagen con "Abrir siempre con" → LenteUV portable
2. Cerrar LenteUV manualmente
3. Intentar reabrir cualquier imagen con LenteUV → CRASH

**Eliminando `user_data\session.txt` manualmente, la app vuelve a funcionar.**

---

## Análisis de Causa Raíz

### Hallazgos clave:

1. **`session.txt` contiene rutas de desarrollo inválidas:**
   ```
   E:\TEMPORAL\Proyectos\Sergkrumas\image_viewer\native\build\bin\Release
   E:\xphang\Imágenes
   E:\xphang\Descargas\Trash\Real
   ./test
   E:\TEMPORAL\Proyectos\Sergkrumas\image_viewer\test
   C:\Users\xxx_h\OneDrive\Desktop\TestFiles
   C:\Users\xxx_h\OneDrive\Desktop\TestFiles - Copy
   ```

2. **El ZIP portable NO incluye `user_data/`**, pero el archivo `session.txt` del repositorio de desarrollo (que contiene rutas inválidas) está disponible en el workspace.

3. **`GetSessionFilePath()` busca hacia arriba 6 niveles** para encontrar `user_data/`. Cuando no lo encuentra, usa el directorio del ejecutable. El `session.txt` del repositorio termina siendo usado.

4. **Ruta relativa `./test`** en `session.txt` se interpreta respecto al directorio de trabajo actual, no respecto al ejecutable. Esto puede causar comportamiento inesperado.

5. **El formato de `session.txt` tiene líneas en blanco extras**, lo que sugiere que el parser podría desincronizarse al leer 4 líneas por entrada.

6. **Cuando `LoadSession()` procesa las rutas:**
   - `AddFolder()` se llama con `libraryLoading=true`
   - Dentro de `AddFolder()`, se ejecuta `SortFolder(static_cast<int>(m_folders.size() - 1), ...)`
   - `SortFolder` accede a `m_app->m_settings.sort_mode` (línea 91 en folder_data_manager.cpp)

   Si `m_app` es `nullptr` o `m_settings` está corrupto/inaccesible, esto causaría el crash.

### Hipótesis principal:

`session.txt` en el repositorio contiene rutas de desarrollo que al procesarse con la lógica de `LoadSession()` → `AddFolder()` → `SortFolder()` causan un acceso a memoria inválida. Es posible que:

- La combinación de rutas inválidas + línea vacías extras cause un buffer overrun
- O que alguna operación de filesystem falle de forma no manejada

### Hipótesis alternativa:

El archivo `session.txt` del repositorio tiene un formato inconsistente que causa que `LoadSession` procese entradas malformadas. La ruta `./test` podría no ser procesada correctamente por `fs::path` o `lexically_normal()`.

---

## Plan de Investigación y Corrección

### Fase 1: Confirmar la Causa Raíz

1. **Agregar trazas de debug en `LoadSession` y `AddFolder`** para identificar exactamente dónde ocurre el crash:
   - En `LoadSession`: traza antes y después de cada `AddFolder`
   - En `AddFolder`: traza de la ruta procesada
   - En `SortFolder`: traza de acceso a `m_settings.sort_mode`

2. **Verificar el estado de `session.txt` en el ZIP portable**

3. **Crear un scenario de prueba** con una imagen asociada a LenteUV portable

### Fase 2: Corrección Inmediata (Workaround)

1. **Excluir `user_data/` del script `create_portable_zip.ps1`** para que la versión portable no incluya `session.txt` con rutas de desarrollo.

2. **Crear un `session.txt` vacío o minimal** para incluir en el portable si es necesario.

### Fase 3: Corrección de Largo Plazo

1. **Mejorar `LoadSession`** para manejar gracefully:
   - Rutas que no existen (`fs::exists` returning false)
   - Rutas relativas inválidas
   - Entradas malformadas

2. **Validar el formato de `session.txt`** antes de procesarlo

3. **Posiblemente**: No guardar rutas absolutas en `session.txt`, usar rutas relativas al executable o normalizadas.

### Fase 4: Verificación

1. **Probar la versión portable** con asociación de archivos
2. **Probar que `session.txt` se crea correctamente** cuando se cierra la app
3. **Verificar que la segunda ejecución funciona** sin necesidad de borrar `session.txt`

---

## Archivos Relevantes a Modificar

| Archivo | Cambio propuesto |
|---------|-----------------|
| `create_portable_zip.ps1` | Excluir `user_data/` del ZIP |
| `native/src/library/folder_data_manager.cpp` | Agregar validación robusta en `LoadSession` |
| `native/src/library/folder_data_manager.h` | Posiblemente agregar trazas de debug |

---

## Dependencias y Precondiciones

- Necesario Visual Studio 2022 para recompilar después de cambios
- La traza de debug (`debug_ipc.txt`) ya está siendo escrita, revisar su contenido
- El archivo `xmp_tag_cache.bin` también está en `user_data/` y debería excluded

---

## Estimación de Tiempo

- **Fase 1 (Investigación)**: 30 minutos
- **Fase 2 (Workaround)**: 15 minutos
- **Fase 3 (Corrección robusta)**: 1-2 horas
- **Fase 4 (Verificación)**: 30 minutos
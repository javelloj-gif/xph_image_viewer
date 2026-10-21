# Plan de Implementación — Botón por defecto al eliminar, Apertura en Instancia Única y Arrastre de Ventana con Clic Derecho

Este documento contiene el plan de acción detallado para implementar tres cambios específicos en la aplicación nativa **LenteUV Image Viewer** en C++20. El diseño se estructuró de tal manera que un agente ejecutor pueda seguir y aplicar las modificaciones de forma limpia, directa e inequívoca.

---

## 📋 Resumen de los Cambios Solicitados

1. **Botón por defecto al presionar [Supr / Delete]**:
   - Al presionar la tecla "Suprimir", el cuadro de diálogo modal de confirmación `MessageBoxW` debe tener preseleccionada por defecto la opción **"Sí"** (`MB_DEFBUTTON1`).
2. **Abrir nueva imagen en Instancia Única activa**:
   - Cuando la opción `single_instance` esté habilitada en la configuración y el usuario haga doble clic sobre una imagen desde el Explorador de Windows, la instancia en ejecución debe cargar dicha imagen, cambiar a la vista de visor (`Page::Viewer`), restaurar/traer al frente la ventana y rediseñar el canvas.
3. **Arrastre de Ventana con Clic Derecho Sostenido en Modo Ventana**:
   - En modo ventana (`!m_window.IsFullscreen()`), al mantener presionado el clic derecho y arrastrar sobre la imagen, se debe arrastrar la ventana completa (`WS_POPUP`) por la pantalla. Si solo se hace un clic derecho simple sin arrastrar, debe abrirse el menú contextual. Además, al pasar a modo ventana o abrir una imagen en este modo, la ventana debe ajustarse de forma precisa a las proporciones y bordes de la imagen (`AdjustWindowToImage()`).

---

## ⚠️ User Review Required

> [!IMPORTANT]
> **Comportamiento del Clic Derecho en Modo Ventana**:
> Se diferencia entre un clic derecho rápido (muestra el menú contextual) y un clic derecho sostenido con movimiento (arrastra la ventana mediante `WM_NCLBUTTONDOWN` con `HTCAPTION`). En modo pantalla completa (`F11`), el clic derecho mantendrá su comportamiento nativo de mostrar directamente el menú contextual.

---

## ❓ Open Questions

*Ninguna. Todos los requisitos y comportamientos esperados fueron analizados y validados contra la arquitectura del código fuente nativo en C++20.*

---

## 🛠️ Cambios Propuestos

---

### Módulo Core & Orquestador de Aplicación (`app.cpp` / `app.h`)

#### [MODIFY] [app.cpp](file:///e:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/core/app.cpp)

1. **Cambio 1: Opción Afirmativa por Defecto al Eliminar (`case VK_DELETE`)**
   - **Línea ~1106**: En la llamada a `MessageBoxW`, reemplazar `MB_DEFBUTTON2` por `MB_DEFBUTTON1`.
   ```cpp
   // ANTES:
   int result = MessageBoxW(m_window.GetHwnd(),
       L"¿Estás seguro de que deseas enviar esta imagen a la Papelera de reciclaje?",
       L"Eliminar Imagen",
       MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2);

   // DESPUÉS:
   int result = MessageBoxW(m_window.GetHwnd(),
       L"¿Estás seguro de que deseas enviar esta imagen a la Papelera de reciclaje?",
       L"Eliminar Imagen",
       MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON1);
   ```

2. **Cambio 2: Manejador de IPC para Instancia Única (`events.onIpcOpenPath`)**
   - **Líneas ~150-158**: Actualizar el callback `onIpcOpenPath` para normalizar la ruta recibida, verificar si es un archivo o directorio, forzar la navegación a `Page::Viewer` (o `Page::Library`), restaurar la ventana si está minimizada/oculta, ajustar la ventana a los bordes de la imagen si está en modo ventana, y traerla al primer plano.
   ```cpp
   m_window.events.onIpcOpenPath = [this](const std::wstring& rawPath) {
       if (rawPath.empty()) return;

       namespace fs = std::filesystem;
       std::wstring path = fs::absolute(rawPath).wstring();

       if (fs::is_directory(path)) {
           LoadFolder(path);
           SetPage(Page::Library);
       } else {
           LoadImage(path);
           SetPage(Page::Viewer);
       }

       // Restaurar si está minimizada u oculta
       if (IsIconic(m_window.GetHwnd())) {
           ShowWindow(m_window.GetHwnd(), SW_RESTORE);
       } else {
           m_window.Show();
       }

       if (m_window.IsFullscreen()) {
           m_window.SetFullscreen(true);
       } else {
           AdjustWindowToImage();
       }

       SetForegroundWindow(m_window.GetHwnd());
       m_window.RequestRedraw();
   };
   ```

3. **Cambio 3: Ajuste de Ventana al Alternar a Modo Ventana (`ToggleFullscreen` / `SetFullscreen`)**
   - En `OnKeyDown` al procesar `VK_F11` / `'F'`, asegurar que si se sale de pantalla completa, se invoque `AdjustWindowToImage()` para que la ventana se reduzca y ajuste perfectamente al contorno de la imagen.

---

### Módulo Visor de Imagen (`viewer_page.h` / `viewer_page.cpp`)

#### [MODIFY] [viewer_page.h](file:///e:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/viewer/viewer_page.h)

1. Agregar miembros de estado para rastrear el arrastre con clic derecho en modo ventana:
   ```cpp
   bool   m_rightMouseDown = false;
   bool   m_isRightDragging = false;
   PointF m_rightDragStart;
   POINT  m_rightDragStartScreen;
   ```

#### [MODIFY] [viewer_page.cpp](file:///e:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/viewer/viewer_page.cpp)

1. **Manejo de `OnMouseDown` para Clic Derecho**:
   - En lugar de abrir `TrackPopupMenu` inmediatamente al pulsar `btn == MouseButton::Right`:
     - Si la ventana no está en pantalla completa (`!m_app->m_window.IsFullscreen()`), registrar `m_rightMouseDown = true`, `m_isRightDragging = false` y capturar la posición del cursor en coordenadas de pantalla (`GetCursorPos(&m_rightDragStartScreen)`).
     - Si la ventana está en pantalla completa (`m_app->m_window.IsFullscreen()`), abrir `TrackPopupMenu` directamente.

2. **Manejo de `OnMouseMove` para Arrastre de Ventana**:
   - Si `m_rightMouseDown` es `true` en modo ventana:
     - Obtener la posición actual del cursor en pantalla (`GetCursorPos(&pt)`).
     - Calcular la distancia recorrida (`dx = pt.x - m_rightDragStartScreen.x`, `dy = pt.y - m_rightDragStartScreen.y`).
     - Si `!m_isRightDragging` y la distancia es mayor a un umbral (ej. 4 píxeles), marcar `m_isRightDragging = true`.
     - Si `m_isRightDragging` es `true`:
       - Invocar la liberación del mouse de la ventana y enviar el comando nativo de arrastre de título de Win32:
         ```cpp
         ReleaseCapture();
         SendMessageW(m_app->m_window.GetHwnd(), WM_NCLBUTTONDOWN, HTCAPTION, 0);
         m_rightMouseDown = false;
         m_isRightDragging = false;
         ```
       - Esto delega el movimiento suave de la ventana al gestor de ventanas nativo de Windows.

3. **Manejo de `OnMouseUp` para Clic Derecho**:
   - Si `btn == MouseButton::Right` y `m_rightMouseDown` es `true`:
     - Si `!m_isRightDragging` (el usuario hizo un clic derecho rápido sin arrastrar), mostrar el menú contextual flotante (`TrackPopupMenu`).
     - Restablecer `m_rightMouseDown = false` y `m_isRightDragging = false`.

---

## 🧪 Plan de Verificación

### Pruebas de Compilación
1. Recompilar el proyecto usando la herramienta CMake en modo Release:
   ```powershell
   $cmake = "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
   & $cmake --build native/build --config Release
   ```
2. Verificar que no se generen errores de compilación ni advertencias severas.

### Pruebas Manuales
1. **Verificación de Tecla Suprimir**:
   - Abrir `native/build/bin/Release/lenteuv.exe` con una imagen de prueba.
   - Presionar la tecla `[Supr / Delete]`.
   - Verificar visualmente que el botón "Sí" esté enfocado por defecto. Presionar `[Enter]` y verificar que la imagen se envíe a la Papelera de reciclaje.

2. **Verificación de Instancia Única**:
   - Asegurarse de que `single_instance` esté activo en `user_data/settings.json`.
   - Ejecutar la aplicación (Instancia 1).
   - Desde el Explorador de archivos de Windows, hacer doble clic en una imagen diferente (ej. `test/linux.png` o `test/kimono.avif`).
   - Verificar que no se cree un nuevo proceso ni ventana, y que la ventana existente cargue y muestre la nueva imagen en pantalla.

3. **Verificación de Arrastre de Ventana con Clic Derecho**:
   - Ejecutar la aplicación en modo ventana (`!IsFullscreen()`).
   - Hacer un clic derecho simple sobre la imagen: debe aparecer el menú contextual.
   - Mantener presionado el clic derecho sobre la imagen y mover el cursor: la ventana completa debe desplazarse suavemente siguiendo el mouse por la pantalla, manteniendo su ajuste al borde de la imagen.
   - Alternar a pantalla completa (`F11`) y regresar a modo ventana: verificar que la ventana se reajuste exactamente al aspecto de la imagen.

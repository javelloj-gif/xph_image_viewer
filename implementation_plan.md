# Plan de Implementación — Opciones de Búsqueda en Board y Limpieza de HUD en Visor

Este plan detalla los cambios necesarios para cumplir con los requerimientos previos a la distribución del visor de imágenes **LenteUV**:
1. **Eliminar el mensaje flotante** de depuración en la esquina superior izquierda del visor (FPS y Opacidad).
2. **Implementar el comportamiento de Agrupación y Enfoque** por defecto al buscar etiquetas (`CTRL+F`) en el modo Tablero (`BoardPage`), con la posibilidad de alternar al comportamiento original de filtrado mediante un botón toggle en la esquina inferior derecha.

---

## User Review Required

> [!IMPORTANT]
> - **Comportamiento por Defecto:** La búsqueda por etiquetas (`CTRL+F`) en Board ahora agrupará y enfocará las imágenes coincidentes alrededor del centro de la pantalla actual en coordenadas del mundo (`worldCenter`), sin eliminar del lienzo las imágenes que no coincidan.
> - **Botón Toggle Flotante:** Se añadirá un botón con estilo Cyberpunk en la esquina inferior derecha de la pantalla para alternar de manera síncrona entre "Búsqueda: Agrupar" (predeterminado) y "Búsqueda: Filtrar" (comportamiento original de eliminación).
> - **Persistencia:** De acuerdo con las preferencias del usuario, este estado se mantendrá únicamente en memoria durante la sesión activa del tablero (inicializándose siempre en "Agrupar" al arrancar).
> - **Limpieza de HUD:** Se eliminará por completo la cadena flotante de FPS y Opacidad en la parte superior izquierda del visor de imagen única para asegurar una experiencia limpia y libre de ruidos visuales.

---

## Proposed Changes

### 1. Segmento: Visor de Imagen Única (`viewer/`)

#### [MODIFY] [viewer_page.cpp](file:///e:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/viewer/viewer_page.cpp)
* **Objetivo:** Eliminar el renderizado del texto flotante en la esquina superior izquierda.
* **Cambios:**
  * En el método `ViewerPage::DrawOverlayInfo(Renderer& renderer)` (líneas ~381-382):
    * Comentar o eliminar la inicialización de `fpsText` y la llamada a `renderer.DrawText()` que renderiza las métricas de FPS y Opacidad.
    * Conservar intacta la lógica posterior de visualización de metadatos de la imagen (la isla inferior).

---

### 2. Segmento: Tablero Virtual Infinito (`board/`)

#### [MODIFY] [board_page.h](file:///e:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/board/board_page.h)
* **Objetivo:** Declarar las variables de estado y helpers necesarios para el control del toggle de búsqueda.
* **Cambios:**
  * Añadir las siguientes variables miembro privadas:
    ```cpp
    bool m_searchModeGroup = true;   // true = Agrupar y enfocar (default), false = Filtrar (original)
    bool m_hoveredToggleBtn = false; // Indica si el puntero está sobre el botón toggle
    ```
  * Declarar un método auxiliar privado:
    ```cpp
    RectF GetSearchToggleRect(const SizeF& screenSize) const;
    ```

#### [MODIFY] [board_page.cpp](file:///e:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/board/board_page.cpp)
* **Objetivo:** Implementar la UI del botón flotante y el nuevo algoritmo de búsqueda.
* **Cambios:**
  * **Inicialización:**
    * En el constructor o en `BoardPage::Initialize`, inicializar `m_searchModeGroup = true;` y `m_hoveredToggleBtn = false;`.
  * **Posicionamiento del Botón:**
    * Implementar `GetSearchToggleRect` para calcular los límites del botón en la esquina inferior derecha:
      ```cpp
      RectF BoardPage::GetSearchToggleRect(const SizeF& screenSize) const {
          float btnW = 200.0f;
          float btnH = 35.0f;
          return RectF::FromXYWH(
              screenSize.width - btnW - 20.0f,
              screenSize.height - btnH - 20.0f,
              btnW,
              btnH
          );
      }
      ```
  * **Dibujo (Draw):**
    * En `BoardPage::Draw(Renderer& renderer)` (al final, antes de dibujar modales o toasts):
      * Obtener el rectángulo usando `GetSearchToggleRect(size)`.
      * Renderizar el botón flotante con diseño Cyberpunk:
        * Fondo: `Color::FromHex(0x1E1E1E, m_hoveredToggleBtn ? 0.95f : 0.85f)`.
        * Borde: `Color::FromHex(0x00E5FF, m_hoveredToggleBtn ? 0.65f : 0.35f)` de espesor `1.0f` (o `1.5f` si está hovered).
        * Texto: `m_searchModeGroup ? L"Búsqueda: Agrupar" : L"Búsqueda: Filtrar"` dibujado de forma centrada en color blanco/cian, tamaño `11.0f`.
  * **Interacción del Mouse (OnMouseMove):**
    * En `BoardPage::OnMouseMove`:
      * Si el buscador modal o el escaneo están activos, omitir.
      * Evaluar si `GetSearchToggleRect` contiene las coordenadas del mouse `{x, y}`.
      * Actualizar `m_hoveredToggleBtn` y llamar a `m_app->m_window.RequestRedraw()` si el estado cambia.
  * **Interacción del Mouse (OnMouseDown):**
    * Al principio de `BoardPage::OnMouseDown`:
      * Si se hace click izquierdo dentro de `GetSearchToggleRect(screenSize)`:
        * Alternar `m_searchModeGroup = !m_searchModeGroup;`.
        * Solicitar redibujado y retornar `true` de inmediato para interceptar el evento.
  * **Algoritmo de Búsqueda (PerformTagQueryAndInsert):**
    * Reestructurar `PerformTagQueryAndInsert` para condicionar el flujo según el modo:
      ```cpp
      if (m_searchModeGroup) {
          // --- NUEVO COMPORTAMIENTO B (AGRUPAR Y ENFOCAR) ---
          
          // 1. Obtener la lista de archivos coincidentes (matchingFiles) según la búsqueda actual.
          // 2. Identificar cuáles imágenes en m_items ya existen y coinciden. Almacenar sus índices.
          // 3. Establecer temporalmente la posición de estas imágenes a coordenadas muy lejanas (ej. {-999999.0f, -999999.0f})
          //    para evitar que colisionen en la búsqueda espacial de la espiral.
          // 4. Calcular el centro actual de la pantalla en coordenadas de mundo (worldCenter).
          // 5. Inicializar un vector temporal de colisión local 'addedItems'.
          // 6. Para cada imagen coincidente existente (usando sus índices):
          //    - Calcular su nueva posición en espiral: FindSpiralPosition(worldCenter, item.width, item.height, 40.0f, addedItems).
          //    - Asignar la nueva posición en m_items.
          //    - Agregar una copia de la imagen posicionada a 'addedItems'.
          // 7. Para cada archivo coincidente que no estuviera ya en el tablero:
          //    - Instanciar un nuevo BoardItem (Image) de 400x300.
          //    - Calcular su posición en espiral: FindSpiralPosition(worldCenter, 400.0f, 300.0f, 40.0f, addedItems).
          //    - Asignar posición y agregarlo a m_items y a 'addedItems'.
          //    - Solicitar la carga de su textura.
          // 8. Desplazar la cámara del tablero (m_canvas.m_pan) para centrarla en 'worldCenter':
          //    - m_canvas.m_pan.x = (targetSize.width / 2.0f) - worldCenter.x * m_canvas.m_zoom;
          //    - m_canvas.m_pan.y = (targetSize.height / 2.0f) - worldCenter.y * m_canvas.m_zoom;
      } else {
          // --- COMPORTAMIENTO ORIGINAL A (FILTRAR / ELIMINAR) ---
          // Ejecutar la lógica actual: eliminar de m_items las imágenes no coincidentes
          // e insertar las nuevas imágenes coincidentes en espiral partiendo del worldCenter.
      }
      ```

---

## Verification Plan

### Automated Tests
* Compilar el proyecto en modo Release usando el comando CMake absoluto para asegurar que no existan errores sintácticos ni de enlazado:
  ```powershell
  & "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" -S native -B native/build
  & "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build native/build --config Release
  ```
* Ejecutar las pruebas unitarias existentes (`tagging_tests.exe`) para garantizar que la caché de metadatos mantenga su consistencia al 100%.

### Manual Verification
1. **Comprobación del HUD del Visor:**
   * Entrar en el visor de imagen única (Viewer Page).
   * Confirmar visualmente que la esquina superior izquierda está completamente vacía (no se dibuja el texto de FPS ni de Opacidad).
   * Verificar que la isla inferior de metadatos siga apareciendo en hover/inactividad normal de acuerdo al temporizador de 2s.
2. **Comprobación del Toggle de Búsqueda:**
   * Entrar en el modo Tablero (Board Page).
   * Confirmar que el botón flotante "Búsqueda: Agrupar" se dibuja en la esquina inferior derecha.
   * Probar el hover (debe iluminarse de color cian brillante) y hacer click izquierdo. Confirmar que alterna cíclicamente a "Búsqueda: Filtrar".
3. **Comprobación del Comportamiento de Agrupación (Por defecto):**
   * Configurar el toggle en "Búsqueda: Agrupar".
   * Agregar varias imágenes al tablero.
   * Realizar una búsqueda con `CTRL+F` por una etiqueta existente.
   * Verificar que **ninguna** imagen no coincidente se borre del lienzo.
   * Confirmar que todas las imágenes coincidentes (las que ya estaban en el lienzo + las nuevas descubiertas en el directorio) se posicionan de manera concéntrica (patrón radial/espiral) y que la cámara del tablero centra la vista automáticamente en este grupo recién formado.
   * Confirmar que las conexiones o flechas (links) que tenían estas imágenes se actualicen dinámicamente sin romperse.
4. **Comprobación del Comportamiento de Filtrado:**
   * Configurar el toggle en "Búsqueda: Filtrar".
   * Realizar otra búsqueda por tags.
   * Confirmar que las imágenes que no coinciden se eliminan del tablero y sus enlaces se purgan (comportamiento original).

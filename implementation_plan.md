# Plan de Implementación — Mejoras del Modal de Tagging y Optimización del Infinity Board

Este documento detalla las soluciones técnicas y de diseño para implementar mejoras de usabilidad en el modal de tagging, la categorización de etiquetas desde archivos JSON, la optimización y escalado para directorios masivos (+10,000 elementos) en el modo Board, la redefinición de atajos de selección/enlace, y el soporte de etiquetado masivo.

---

## User Review Required

> [!NOTE]
> Todos los cambios se realizarán utilizando C++20 nativo de Windows (Win32/DirectX) sin introducir dependencias externas. 
> La persistencia de cambios en metadatos multi-imagen llamará individualmente a `XmpTagManager::WriteTags` para mantener la sincronización con el explorador y la caché en disco.

> [!WARNING]
> La tecla para enlazar nodos en el tablero cambiará de **CTRL + Click** a **ALT + Click** para liberar la combinación estándar de selección múltiple (**CTRL + Click**).

---

## Proposed Changes

### Componente: Tagging Modal (`ui/tagging_modal`)

#### [MODIFY] [tagging_modal.h](file:///e:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/ui/tagging_modal.h)
* Añadir miembros privados para soportar categorías y selección múltiple:
  ```cpp
  struct AvailableTag {
      std::wstring name;
      std::wstring category; // Vacío si no tiene categoría
  };
  std::vector<std::wstring> m_categories; // Orden de aparición en el JSON
  std::unordered_map<std::wstring, std::wstring> m_tagToCategory;
  
  // Soporte multi-imagen
  std::vector<std::wstring> m_filePaths;
  std::vector<std::vector<std::wstring>> m_originalFileTags;
  std::vector<std::wstring> m_initialCommonTags;
  bool m_multiImageMode = false;
  ```
* Declarar sobrecarga de `Show`:
  ```cpp
  void Show(const std::vector<std::wstring>& filePaths);
  ```

#### [MODIFY] [tagging_modal.cpp](file:///e:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/ui/tagging_modal.cpp)
* **Requisito 1 & 2 (Auto-añadir y Foco de Búsqueda)**:
  * En `OnMouseDown`, al hacer click sobre un tag de disponibles:
    * Si `mods.ctrl` es falso: Añadir el tag directamente a `m_imageTags` (si no existía). Luego, vaciar `m_searchText`, actualizar la búsqueda (`m_filteredTags = m_availableTags`) y fijar `m_activeInputType = 1` (foco en buscar etiquetas).
    * Si `mods.ctrl` es verdadero: Alternar selección en la lista de disponibles (multi-selección para copiar por lote).
  * Al hacer click en el botón copiar `[ > ]`, tras transferir los elementos, vaciar `m_searchText`, filtrar disponibles y fijar `m_activeInputType = 1`.
* **Requisito 3 (Foco al crear etiqueta)**:
  * Al presionar `ENTER` en el input de nueva etiqueta (`m_activeInputType == 6`) o al hacer click en el botón `+`:
    * Añadir el tag a `m_imageTags`, vaciar `m_manualTagText` y **mantener** `m_activeInputType = 6` para que el cursor siga activo para la siguiente escritura.
* **Requisito 4 (Tags categorizados)**:
  * Refactorizar `LoadAvailableTags()` para detectar la estructura del archivo `xmpTags.json`:
    * Leer el primer carácter sin espacios de `wcontent`.
    * Si es `{`: Es un objeto clasificado (ej. `{"personajes": [...], "lugares": [...]}`).
      * Analizar las claves (categorías) y añadir cada una a `m_categories`.
      * Extraer el array de strings asociado a cada categoría, guardándolos en `m_availableTags` y mapeando `m_tagToCategory[tag] = categoria`.
    * Si es `[`: Es un listado plano tradicional. Limpiar `m_categories`.
  * Adaptar `GetScrollLimits()` and `DrawTagsTab()` para calcular y renderizar el scroll con secciones:
    * Si `m_categories` no está vacío, iterar por categorías. Si una categoría tiene tags coincidentes con la búsqueda:
      * Renderizar una cabecera con el nombre de la categoría en cian cyberpunk (`Color::FromRGBA(0, 240, 255, 180)`).
      * Envolver y dibujar los tags correspondientes como píldoras tabuladas.
      * Ajustar la altura acumulada del listado en consecuencia.
* **Requisito 7 (Etiquetado masivo multi-imagen)**:
  * Implementar `Show(const std::vector<std::wstring>& filePaths)`:
    * Establecer `m_multiImageMode = true`, `m_filePaths = filePaths`.
    * Leer metadatos originales de cada archivo y guardarlos en `m_originalFileTags`.
    * Computar la intersección de todas las listas de etiquetas para rellenar `m_imageTags` (etiquetas en común). Guardar copia en `m_initialCommonTags`.
  * En `Draw()` y `DrawTagsTab()`:
    * Si `m_multiImageMode` es activo, ocultar o inhabilitar la pestaña "Metadata" (mostrando un aviso en su lugar) y el control de clasificación de estrellas. Cambiar el título a "Etiquetado Múltiple (X imágenes)".
  * En `SaveChanges()`:
    * Si `m_multiImageMode` es verdadero, calcular diferencias:
      * Nuevos tags añadidos: presentes en `m_imageTags` pero no en `m_initialCommonTags`.
      * Tags eliminados: presentes en `m_initialCommonTags` pero no en `m_imageTags`.
    * Para cada archivo en `m_filePaths`, mezclar sus tags originales aplicando las diferencias (añadir nuevos y borrar eliminados).
    * Guardar en disco llamando a `m_app->m_tagManager.WriteTags(path, nuevosTags)`.

---

### Componente: Board Page (`board`)

#### [MODIFY] [board_page.h](file:///e:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/board/board_page.h)
* Declarar constantes y métodos auxiliares para optimización y adición masiva:
  ```cpp
  static constexpr size_t MAX_BOARD_TEXTURES = 150; // Umbral de texturas en VRAM
  void ListCurrentDirectoryImages();
  void EvictFarBoardTextures();
  ```

#### [MODIFY] [board_page.cpp](file:///e:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/board/board_page.cpp)
* **Requisito 5 (Listar directorio masivo + Optimización VRAM)**:
  * Agregar el atajo `SHIFT + CTRL + L` en `OnKeyDown` y la opción `"Listar imágenes (SHIFT+CTRL+L)"` al menú contextual del lienzo vacío en `OnMouseUp`.
  * Implementar `ListCurrentDirectoryImages()`:
    * Obtener el directorio activo. Leer de forma no bloqueante todas las rutas de imágenes soportadas.
    * Calcular un grid centrado en la vista de cámara (ej. cuadrícula de 12 columnas).
    * Crear ítems de tablero `BoardItemType::Image` para cada archivo e insertarlos en `m_items`.
    * **CRÍTICO**: No invocar `LoadItemTexture()` para estos elementos al crearlos. Quedarán en cola con `texture = nullptr`.
  * Implementar `EvictFarBoardTextures()` para descargar memoria:
    * Si el recuento de ítems con `texture != nullptr` supera `MAX_BOARD_TEXTURES`:
      * Calcular el centro actual de la pantalla en coordenadas de mundo.
      * Obtener todos los ítems cargados y ordenarlos por distancia euclidiana descendente respecto al centro de la cámara.
      * Para los elementos más lejanos, poner `item.texture = nullptr` (liberando automáticamente el puntero inteligente y su recurso `ID2D1Bitmap1` de la GPU) hasta que el número de texturas cargadas se reduzca a `MAX_BOARD_TEXTURES`.
      * Llamar a esta función al final de traslaciones/zoom importantes o periódicamente en el ciclo de dibujado.
* **Requisito 6 (Cambio de Atajos de Enlace / Selección)**:
  * En `OnMouseDown`, cambiar la detección de conectores de `mods.ctrl` a `mods.alt`.
  * En `OnMouseDown`, si se hace click sobre un nodo o imagen:
    * Si `mods.ctrl` o `mods.shift` es verdadero: Alternar estado de selección (`clickedItem.selected = !clickedItem.selected`) permitiendo seleccionar múltiples elementos manualmente uno a uno.
    * Si no hay modificadores: Si el ítem no estaba seleccionado, deseleccionar todos los demás y seleccionar únicamente el actual.
* **Requisito 8 (Rectángulo envolvente de selección múltiple)**:
  * En `Draw()`, al renderizar el rectángulo envolvente de la caja delimitadora `selBox`:
    * Contar el número de elementos seleccionados.
    * Si el número de elementos seleccionados es mayor que 1:
      * Renderizar el rectángulo delimitador externo `selScreenBox` y sus tiradores en color **Cian/Azul Cyberpunk** (`Color::FromHex(0x00E5FF)`) en lugar del naranja estándar.
    * Mantener el dibujo individual de los bordes amarillos/naranjas de cada imagen seleccionada dentro de `BoardItemRenderer::DrawItems`.

#### [MODIFY] [app.cpp](file:///e:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/core/app.cpp)
* **Integración del atajo de etiquetado múltiple**:
  * En el handler de la tecla `'T'`, si la página actual es `Page::Board`:
    * Recopilar todos los archivos de imágenes actualmente seleccionados en el tablero.
    * Si se selecciona solo 1, llamar a `m_taggingModal.Show(selectedPath)`.
    * Si se seleccionan 2 o más, llamar a `m_taggingModal.Show(selectedPaths)`.

---

## Verification Plan

### Automated Tests
1. Ejecutar las pruebas unitarias existentes para asegurar que no hay regresiones colaterales en la caché ni en la codificación XML/XMP:
   ```powershell
   & "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build native/build --config Release --target tagging_tests
   native/build/bin/Release/tagging_tests.exe
   ```

### Manual Verification
1. **Verificar Usabilidad de Foco y Clics**:
   * Abrir el modal de tagging, hacer clic en un tag de la izquierda y confirmar que se agrega instantáneamente a la derecha y el cursor se enfoca en el input de búsqueda (vacío).
   * Escribir un tag manual, presionar ENTER, confirmar su inserción a la derecha y verificar que el cursor sigue en el input de manual listo para escribir el siguiente.
2. **Verificar Tags Categorizados**:
   * Crear un archivo `xmpTags.json` estructurado con objetos (ej. `{"Categoría A": ["tag1", "tag2"], "Categoría B": ["tag3"]}`).
   * Abrir el modal y confirmar que los tags se listan agrupados bajo las cabeceras cian correspondientes.
3. **Verificar Selección y Enlaces en Board**:
   * Confirmar que **ALT + Click** dibuja y crea el enlace direccional entre nodos.
   * Confirmar que **CTRL + Click** añade/remueve nodos a la selección grupal.
4. **Verificar Rectángulos de Selección**:
   * Seleccionar una única imagen y verificar que se dibuja el marco naranja con tiradores naranjas.
   * Seleccionar múltiples imágenes (mediante lasso o CTRL+click) y verificar que el marco exterior que las une a todas cambia a cian brillante, mientras que cada imagen individual mantiene su borde naranja.
5. **Verificar Etiquetado Masivo**:
   * Seleccionar 3 imágenes en el Board, presionar `T`.
   * Confirmar que solo se muestran los tags comunes. Añadir un tag nuevo y presionar Guardar.
   * Abrir el modal individualmente para cada una de las 3 imágenes y verificar que el nuevo tag fue guardado en todas ellas.
6. **Verificar Carga Masiva y Optimización VRAM**:
   * Copiar temporalmente 5,000-10,000 imágenes en una carpeta de pruebas.
   * En el Board, presionar `SHIFT + CTRL + L` o usar el menú contextual.
   * Confirmar que se añaden los ítems en un grid masivo y que la app sigue fluida sin disparar el consumo de VRAM (las imágenes se renderizan y decodifican bajo demanda a medida que se desplaza la cámara por el tablero, y las lejanas liberan su textura de la memoria GPU).

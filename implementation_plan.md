# Plan de Implementación — Interactividad de Scroll, Soporte de Espacios y Sincronización con Windows Keywords

Este plan de implementación detalla los pasos para solucionar los bugs reportados en el modal de tagging (barra de scroll no interactiva y problemas de visualización/registro al ingresar espacios) e implementar la sincronización bidireccional entre las etiquetas XMP y la propiedad de metadatos `System.Keywords` de Windows.

---

## Nuevos Requerimientos y Correcciones

### 1. Interactividad de la Barra de Scroll en `TaggingModal`
* **Causa del Bug**: El modal `TaggingModal` dibuja las barras de scroll visualmente (tanto para la lista izquierda como para el grid de la derecha), pero no registra las pulsaciones de mouse ni los arrastres sobre las barras. Solo el scroll con la rueda de mouse es funcional.
* **Solución Propuesta**:
  * Agregar variables de control en `TaggingModal` ([tagging_modal.h](file:///e:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/ui/tagging_modal.h)):
    ```cpp
    bool m_leftScrollCaptured = false;
    bool m_rightScrollCaptured = false;
    float m_dragStartY = 0.0f;
    float m_dragStartScrollOffset = 0.0f;
    ```
  * En `TaggingModal::OnMouseDown` ([tagging_modal.cpp](file:///e:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/ui/tagging_modal.cpp)), verificar si las coordenadas del cursor coinciden con los tracks de las barras de scroll (definidos en los bordes derechos de `listRect` y `gridRect` con un ancho de ~8px).
  * Si coinciden, capturar el scroll (`m_leftScrollCaptured` o `m_rightScrollCaptured` = true), registrar el punto de inicio `m_dragStartY = y` y la posición de scroll inicial `m_dragStartScrollOffset`.
  * En `TaggingModal::OnMouseMove`, si se está capturando un arrastre, calcular la diferencia en el eje `Y`, multiplicarla por la tasa de proporción de altura del contenido respecto a la barra, y ajustar `m_leftScroll` o `m_rightScroll` incrementándolos/decrementándolos de forma proporcional.
  * En `TaggingModal::OnMouseUp`, resetear las banderas de captura.

---

### 2. Soporte para Espacios en las Etiquetas (Caret Position bug)
* **Causa del Bug**: El modal de tagging ya permite la inserción física del carácter de espacio (`ch >= 32` en `OnChar`), pero al escribir un espacio al final del texto en los campos de entrada (`m_searchText`, `m_manualTagText`, etc.), la función DirectWrite `MeasureText` de la clase `Renderer` no calcula el ancho del carácter de espacio final (comportamiento por defecto de DirectWrite que omite espacios finales). Esto provoca que el cursor de texto (caret) no avance visualmente de posición, haciendo que el usuario crea que no se permiten espacios.
* **Solución Propuesta**:
  * En [tagging_modal.cpp](file:///e:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/ui/tagging_modal.cpp), antes de calcular el ancho con `MeasureText` para dibujar el caret, reemplazar temporalmente todos los espacios con espacios de no-rompimiento (`L'\u00A0'`).
  * Ejemplo de reemplazo en la medición:
    ```cpp
    std::wstring measureText = text;
    for (auto& c : measureText) {
        if (c == L' ') c = L'\u00A0';
    }
    float textW = renderer.MeasureText(measureText, 12.0f).x;
    ```
  * Aplicar esta corrección a todas las mediciones de caret en el modal (cajas de búsqueda, entrada manual de tags, edición inline de tags, y campos de metadatos Title/Creator/Description).

---

### 3. Sincronización con `System.Keywords` de Windows
* **Objetivo**: Que las etiquetas XMP Dublin Core (`dc:subject`) guardadas por el visualizador se guarden y muestren también en las propiedades del archivo en Windows Explorer (columna "Etiquetas"), y viceversa.
* **Solución Propuesta**:
  * Utilizar la API de COM de Windows `IPropertyStore` para escribir la propiedad nativa de metadatos `System.Keywords` (`PKEY_Keywords`).
  * En [CMakeLists.txt](file:///e:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/CMakeLists.txt), enlazar la librería de sistema `propsys.lib` a ambos ejecutables (`KrumassanViewer` y `tagging_tests`).
  * En `XmpTagManager` ([xmp_tag_manager.cpp](file:///e:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/metadata/xmp_tag_manager.cpp)):
    * Incluir `<propsys.h>`, `<propkey.h>` y `<propvarutil.h>`.
    * En `WriteTags` y `WriteMetadataExtended`, llamar a un método auxiliar de sincronización:
      ```cpp
      void XmpTagManager::SyncToWindowsKeywords(const std::wstring& filePath, const std::vector<std::wstring>& tags) {
          ComPtr<IPropertyStore> pStore;
          HRESULT hr = SHGetPropertyStoreFromParsingName(filePath.c_str(), nullptr, GPS_READWRITE, IID_PPV_ARGS(&pStore));
          if (SUCCEEDED(hr)) {
              std::vector<PCWSTR> ptrs;
              ptrs.reserve(tags.size());
              for (const auto& tag : tags) ptrs.push_back(tag.c_str());
              PROPVARIANT var;
              PropVariantInit(&var);
              if (SUCCEEDED(InitPropVariantFromStringVector(ptrs.data(), static_cast<ULONG>(ptrs.size()), &var))) {
                  pStore->SetValue(PKEY_Keywords, var);
                  pStore->Commit();
                  PropVariantClear(&var);
              }
          }
      }
      ```
    * En `ReadTags` de [xmp_tag_manager.cpp](file:///e:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/metadata/xmp_tag_manager.cpp), si la lectura de XMP no retorna etiquetas, intentar leer `System.Keywords` vía `IPropertyStore` como fallback para lograr la sincronización inversa:
      ```cpp
      // Fallback a IPropertyStore si XMP no tiene tags
      ComPtr<IPropertyStore> pStore;
      if (SUCCEEDED(SHGetPropertyStoreFromParsingName(normalized.c_str(), nullptr, GPS_READ, IID_PPV_ARGS(&pStore)))) {
          PROPVARIANT var;
          PropVariantInit(&var);
          if (SUCCEEDED(pStore->GetValue(PKEY_Keywords, &var))) {
              if (var.vt == (VT_VECTOR | VT_LPWSTR)) {
                  for (ULONG i = 0; i < var.calpwstr.cElems; ++i) {
                      if (var.calpwstr.pElems[i]) outTags.push_back(var.calpwstr.pElems[i]);
                  }
              }
              PropVariantClear(&var);
          }
      }
      ```

---

## Proposed Changes

### 1. Sistema de Compilación
#### [MODIFY] [CMakeLists.txt](file:///e:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/CMakeLists.txt)
* Agregar `propsys.lib` a las dependencias de enlace para `KrumassanViewer` y `tagging_tests`.

### 2. Definición del TaggingModal
#### [MODIFY] [tagging_modal.h](file:///e:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/ui/tagging_modal.h)
* Declarar las variables de arrastre del scroll: `m_leftScrollCaptured`, `m_rightScrollCaptured`, `m_dragStartY` y `m_dragStartScrollOffset`.

### 3. Lógica del UI de Tagging
#### [MODIFY] [tagging_modal.cpp](file:///e:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/ui/tagging_modal.cpp)
* Implementar el arrastre de scrollbars en `OnMouseDown`, `OnMouseMove` y `OnMouseUp`.
* Reemplazar caracteres de espacio con `L'\u00A0'` antes de llamar a `renderer.MeasureText` en `DrawTagsTab` y `DrawMetadataTab`.

### 4. Sincronización en XmpTagManager
#### [MODIFY] [xmp_tag_manager.cpp](file:///e:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/metadata/xmp_tag_manager.cpp)
* Incluir las cabeceras de propiedades COM de Windows.
* Implementar `SyncToWindowsKeywords` e invocarlo en escrituras.
* Añadir el fallback de lectura desde `IPropertyStore` en `ReadTags`.

---

## Verification Plan

### Automated Tests
* Compilar la suite de pruebas unitarias (`tagging_tests`).
* Agregar un test de verificación en `tagging_tests.cpp` para comprobar que al escribir metadatos en un archivo temporal, las palabras clave escritas persisten y coinciden al consultarlas tanto por XMP como por el almacén de propiedades COM.

### Manual Verification
1. **Arrastre de Scrollbars**: Abrir el modal de tagging en un directorio con múltiples etiquetas disponibles. Hacer clic izquierdo en la barra de scroll y arrastrar. Confirmar que la lista de tags se desplaza fluidamente en consonancia.
2. **Registro de Espacios**: Escribir una etiqueta como `"tag con espacios"` en la caja manual. Confirmar que el cursor avanza correctamente al pulsar la barra espaciadora. Presionar Enter y confirmar que se agrega correctamente a las etiquetas asociadas.
3. **Sincronización en Explorador**: Guardar etiquetas en una imagen, abrir las propiedades de la imagen en el Explorador de Windows y confirmar que se lista en el campo "Etiquetas". Modificar las etiquetas desde Windows y confirmar que se cargan al abrir la imagen en el visualizador.

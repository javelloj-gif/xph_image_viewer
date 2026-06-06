# Plan de Implementación — Fase 4: Empaquetamiento e Instalador para Windows

Este plan detalla el proceso para estructurar el empaquetamiento y la distribución de **LenteUV Image Viewer** en dos formatos para sistemas Windows 10/11:
1. **Instalador Interactivo (`.exe`):** Creado con **Inno Setup**, que permite la selección de directorio de destino, accesos directos y la integración nativa en el menú contextual de Windows.
2. **Versión Portable Standalone (`.zip`):** Un archivo comprimido que incluye el ejecutable compilado junto con todos sus assets, traducciones y dependencias, operable directamente sin instalación previa.

Adicionalmente, se automatiza la generación de los recursos gráficos oficiales de la aplicación a partir de su logo vectorial base.

---

## User Review Required

> [!IMPORTANT]
> - **Claves de Registro del Menú Contextual:** La integración clásica se realizará a través de claves en `HKEY_CLASSES_ROOT` (vía `SystemFileAssociations` para no romper otras aplicaciones de visualización predeterminadas) y claves de `Directory` / `Directory\Background` para poder abrir carpetas enteras con un clic derecho en el Explorador de Windows.
> - **Generación de Icono Multibanda:** Se requiere un script de Python con soporte para `cairosvg` and `Pillow`. Si `cairosvg` requiere librerías nativas de C que no estén presentes, se proveerá un fallback automático que renderizará los PNGs necesarios si se provee una imagen PNG base de alta resolución, o bien utilizará comandos de CLI disponibles en el entorno.

---

## Proposed Changes

### 1. Recursos Gráficos e Iconografía

#### [NEW] [generate_icons.py](file:///e:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/generate_icons.py)
* **Objetivo:** Convertir el archivo vectorial [lente_uv.svg](file:///e:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/lente_uv.svg) en un archivo de ícono estándar de Windows `.ico` con soporte multibanda (`16x16`, `32x32`, `48x48`, `256x256`).
* **Implementación:**
  * Script en Python que utiliza `cairosvg` para exportar a PNG de alta resolución.
  * Utiliza `Pillow` (`PIL.Image`) para reescalar a las resoluciones estándar y empaquetar todo como un único archivo `lenteuv.ico` en `native/src/resources/lenteuv.ico`.

#### [MODIFY] [app.rc](file:///e:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/resources/app.rc)
* **Objetivo:** Asociar el nuevo archivo `.ico` generado al identificador del ícono de aplicación principal del ejecutable.
* **Cambios:**
  * Reemplazar la referencia del ícono antiguo `image_viewer.ico` (eliminado) para que apunte al nuevo `lenteuv.ico`.

---

### 2. Automatización del Empaquetamiento Portable

#### [NEW] [create_portable_zip.ps1](file:///e:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/create_portable_zip.ps1)
* **Objetivo:** Recopilar el binario compilado y sus dependencias para la versión portable.
* **Flujo del script:**
  * Crear un directorio temporal `lenteuv_portable/`.
  * Copiar el binario compilado `native/build/bin/Release/lenteuv.exe` a `lenteuv_portable/`.
  * Copiar el directorio `locales/` (contiene traducciones de localización `.mo`).
  * Copiar el directorio `resources/` (cursores y otros assets).
  * Comprimir todo el lote en `lenteuv_portable.zip`.

---

### 3. Instalador Interactivo (Inno Setup)

#### [NEW] [lenteuv_setup.iss](file:///e:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/lenteuv_setup.iss)
* **Objetivo:** Definir el instalador oficial de Windows.
* **Secciones del Script de Inno Setup:**
  * `[Setup]`: Define nombres, versión, editor, e ícono del instalador.
    * Nombre: `LenteUV Image Viewer`
    * Carpeta por defecto: `{autopf}\LenteUV`
    * Permite la personalización de la ruta mediante `DisableDirPage=no`.
  * `[Files]`: Copia `lenteuv.exe`, `locales/*` y `resources/*` a los directorios de destino correspondientes.
  * `[Icons]`: Crea accesos directos en el menú de Inicio y en el Escritorio.
  * `[Registry]`: Integración nativa del menú contextual:
    * **Para archivos de imágenes soportadas (ej. `.png`, `.jpg`, `.webp`, `.gif`):**
      * Crear la clave `HKCR\SystemFileAssociations\<ext>\shell\LenteUV` con el valor de texto `Abrir con LenteUV`.
      * Crear la subclave `command` apuntando a `"{app}\lenteuv.exe" "%1"`.
    * **Para directorios y fondos de carpeta:**
      * Crear la clave `HKCR\Directory\shell\LenteUV` con el valor `Abrir carpeta en LenteUV`.
      * Crear la subclave `command` con `"{app}\lenteuv.exe" "%1"`.
      * Crear la clave `HKCR\Directory\Background\shell\LenteUV` con el valor `Abrir aquí con LenteUV`.
      * Crear la subclave `command` con `"{app}\lenteuv.exe" "%V"`.
  * `[Run]`: Opción de iniciar la aplicación al finalizar la instalación.

---

## Verification Plan

### Automated Verification
1. **Generación del ícono:**
   * Ejecutar el script `python native/generate_icons.py`.
   * Verificar que se crea el archivo `native/src/resources/lenteuv.ico`.
2. **Compilación del ejecutable con el nuevo recurso:**
   * Ejecutar la compilación en configuración **Release**.
   * Validar que el compilador de recursos (`rc.exe`) integre de manera correcta el nuevo ícono y enlace sin errores.
3. **Compilación del Instalador ISS:**
   * Si el compilador de línea de comandos de Inno Setup (`ISCC.exe`) está disponible en el PATH:
     ```powershell
     ISCC.exe lenteuv_setup.iss
     ```
   * Comprobar que genere el binario `Output/lenteuv_setup.exe`.

### Manual Verification
1. **Prueba de la versión Portable:**
   * Descomprimir `lenteuv_portable.zip` en una ruta limpia.
   * Ejecutar `lenteuv.exe` y verificar que la interfaz cargue correctamente en el idioma detectado y localice sus recursos.
2. **Prueba del Instalador:**
   * Ejecutar el instalador `lenteuv_setup.exe`.
   * Verificar la pantalla de selección de ruta y la creación de accesos directos.
   * Hacer clic derecho sobre un archivo de imagen en el explorador y seleccionar la opción "Abrir con LenteUV".
   * Hacer clic derecho sobre un directorio y verificar la opción "Abrir carpeta en LenteUV".
   * Desinstalar la aplicación y corroborar que se eliminen todos los accesos directos, archivos instalados y claves del registro de manera limpia.

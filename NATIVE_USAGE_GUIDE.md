# LenteUV — Native C++ Usage and Testing Guide

Esta guía describe cómo compilar, ejecutar y realizar pruebas funcionales de la implementación nativa en C++ de **LenteUV** (Win32 + Direct2D/Direct3D 11/DirectComposition), un visor de imágenes de alta velocidad y lienzo infinito basado en el visualizador original de Sergei Krumas (Sergkruma's image viewer) y Google Picasa.

---

## 1. Compilación y Ejecución

### 📁 Directorio de Trabajo y Preparación
Antes de ejecutar los comandos de compilación, abre tu consola (PowerShell o Command Prompt) en el directorio raíz del proyecto:
```powershell
# Verifica que te encuentras en el directorio raíz:
# e:\TEMPORAL\Proyectos\Sergkrumas\image_viewer
```
> [!IMPORTANT]
> Todos los comandos de CMake y construcción se deben ejecutar desde el directorio raíz (`image_viewer`). No los ejecutes dentro de la carpeta `native/` o `native/build/`.

---

### Prerrequisitos
* **Sistema Operativo**: Windows 10 (Build 18362 o superior) o Windows 11.
* **Compilador**: Visual Studio 2022 con la carga de trabajo **"Desarrollo para el escritorio con C++"** instalada.
* **Sistema de Construcción**: CMake 3.20+ (incluido opcionalmente en el instalador de Visual Studio o de forma independiente).

---

### 🚀 Guía de Compilación Paso a Paso

#### Opción A: Usando CMake en el PATH del Sistema (Recomendado)
1. **Configurar y Generar Archivos de Construcción**:
   ```powershell
   cmake -S native -B native/build
   ```
2. **Compilar el Ejecutable en Release**:
   ```powershell
   cmake --build native/build --config Release
   ```

---

#### Opción B: Usando Rutas Absolutas (Visual Studio Build Tools)
Si `cmake` no está en las variables de entorno, puedes utilizar la ruta absoluta de las herramientas de Visual Studio 2022:

##### Paso 1: Configurar Proyecto
```powershell
& "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" -S native -B native/build
```

##### Paso 2: Compilar Proyecto
```powershell
& "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\MSBuild\Current\Bin\MSBuild.exe" native/build/lenteuv.sln /p:Configuration=Release /p:Platform=x64
```

---

### 📂 Ubicación del Ejecutable Compilado
Una vez completada la compilación, el ejecutable binario de LenteUV se generará en:
```text
e:\TEMPORAL\Proyectos\Sergkrumas\image_viewer\native\build\bin\Release\lenteuv.exe
```

### 🏃 Ejecutar la Aplicación
Desde el directorio raíz del proyecto, ejecuta:
```powershell
./native/build/bin/Release/lenteuv.exe "./test/linux.png"
```
Puedes pasar cualquier ruta absoluta o relativa de un archivo de imagen o directorio como argumento de inicio.

---

## 2. Atajos de Teclado Globales y Navegación

Estos controles funcionan en toda la aplicación de forma universal:
*   **`2`**: Cambiar a **Viewer Page** (visor de imagen única con zoom, paneo y rotación acelerados por GPU).
*   **`3`**: Cambiar a **Board Page** (lienzo virtual infinito estilo PureRef).
*   **`4`**: Cambiar a **Library Page** (navegador de carpetas y grilla masonry de miniaturas).
*   **`5`**: Cambiar a **Waterfall Page** (grilla masonry fluida con scroll virtual y visor modal integrado).
*   **`F11` / `F`**: Alternar el modo **Pantalla Completa** (Fullscreen).
*   **`Escape`**: Salir de pantalla completa, o cerrar el modal actual. Si no hay elementos activos, cierra la aplicación (o minimiza al área de notificación si la opción *Hide to tray on close* está activa).
*   **`Alt + F4`**: Cierre universal inmediato y limpio de la ventana.
*   **`Supr` (Delete)**: En el visor de imágenes (Viewer), muestra un diálogo nativo de Windows para mover el archivo actual a la Papelera de Reciclaje de forma segura sin salir de pantalla completa.
*   **`Page Up` / `Page Down`**: Aumentar o disminuir la opacidad/traslucencia de la ventana con el escritorio (efecto translúcido DirectComposition).
*   **`Flecha Izquierda` / `Flecha Arriba` / `Inicio`**: Cargar la imagen anterior de la lista.
*   **`Flecha Derecha` / `Flecha Abajo` / `Fin`**: Cargar la siguiente imagen de la lista.
*   **`0` / `Numpad 0`**: Restablecer el zoom, paneo y rotación del visor a su estado por defecto (escala de pantalla completa o 1:1 original).
*   **`O` / `~` (Tilde)**: Alternar el menú HUD de Opciones de configuración.
*   **`T`**: Alternar el modal de edición de etiquetas XMP (`TaggingModal`).

---

## 3. Página 2: Controles del Visor (Viewer Page)

Esta página sirve como el visor principal de imágenes a pantalla completa:

### Interacciones de Ratón
*   **Arrastrar (Click Izquierdo)**: Panea la imagen por el visor.
*   **Rueda del Ratón (Defecto)**: Zoom suave centrado en la posición actual del cursor.
*   **Ctrl + Rueda del Ratón (Defecto)**: Navegar por la lista de imágenes (anterior/siguiente).
*   **Rueda sobre la tira de miniaturas**: Desplaza horizontalmente la fila de previsualizaciones inferior.
*   **Doble Click**: Alterna el zoom entre escala 1:1 real y modo fit-to-screen adaptativo (las imágenes pequeñas caben al 100% sin estirarse, y las grandes se adaptan proporcionalmente).
*   **Click Derecho**: Abre el menú de contexto (Settings, Fullscreen, Exit).
*   **Ctrl + Arrastre Izquierdo**: Herramienta de lupa de zoom rectangular; al soltar, la cámara encuadra y hace zoom en el recuadro seleccionado.
*   **Ctrl + Click Izquierdo en Miniatura**: Selección múltiple para operaciones masivas.
*   **Arrastrar y Soltar Miniaturas**: Permite reordenar el carrusel de miniaturas inferior de forma visual con un cursor rojo guía.

### Atajos de Teclado
*   **`1`**: Alternar escala de pixel 1:1 vs fit-to-screen adaptativa.
*   **`R`**: Rotar la imagen 90° en sentido de las agujas del reloj.
*   **`Shift + R`**: Rotar la imagen 90° en sentido antihorario.
*   **`W` / `A` / `S` / `D`**: Paneo rápido de la imagen (Arriba / Izquierda / Abajo / Derecha). Sostener `Shift` aumenta la velocidad de desplazamiento.
*   **`Flecha Arriba` / `+` / `Numpad +`**: Zoom in centrado.
*   **`Flecha Abajo` / `-` / `Numpad -`**: Zoom out centrado.
*   **`Espacio`**: Pausar o reanudar la reproducción de imágenes animadas (GIF, APNG, WebP animado).
*   **`C`**: Alternar esquinas de decoración Cyberpunk cian.
*   **`G`**: Alternar grilla de composición fotográfica (tercios).
*   **`M`**: Alternar HUD flotante de información básica y metadatos de la imagen (se oculta automáticamente tras 2 segundos de inactividad del mouse).
*   **`I`**: Invertir colores de la imagen.
*   **`P`**: Alternar interpolación Pixelada (Nearest Neighbor) y Suavizada (High-Quality Cubic).

---

## 4. Página 3: Lienzo Virtual Infinito (Board Page)

El Board funciona como un lienzo virtual infinito para organizar imágenes y notas en flujo libre:

### Navegación y Cámara
*   **Arrastrar (Click Central del Ratón)**: Panea la cámara por el lienzo virtual.
*   **Rueda del Ratón**: Zoom centrado en el puntero del ratón.
*   **Doble Click en Fondo Vacío**: Restablece el zoom de la cámara al 100% y la centra.

### Algoritmo de Colocación Radial en Espiral (Evasión de Colisiones)
Cuando importas múltiples imágenes del directorio actual (`CTRL + O` o arrastrar y soltar) o traes resultados de búsquedas de tags, el sistema **no las apila en la parte inferior**.
* LenteUV calcula el centro de la pantalla del usuario en el mundo del lienzo.
* Coloca cada imagen en una espiral de Arquímedes concéntrica hacia afuera en ángulos de salida aleatorios.
* Cada posición candidata es validada contra las imágenes ya existentes en el tablero y las nuevas imágenes añadidas en la transacción actual, aplicando una holgura de separación de 40px, garantizando un esparcimiento orgánico y libre de solapamientos molestos.

### Creación de Elementos
*   **Arrastrar y Soltar Archivos**: Arrastra imágenes o carpetas desde el Explorador de Windows y suéltalas en el lienzo para añadirlas en el punto del mouse.
*   **Click Derecho en Espacio Vacío**: Abre un menú flotante para:
    1. **Add Note**: Crear una tarjeta de notas de texto editable (amarilla).
    2. **Add Node**: Crear un círculo/nodo conceptual.
*   **Teclas `CTRL + F` (Búsqueda por Tags en Board)**:
    * Abre el modal de búsqueda de tags con diseño cyberpunk cian translúcido.
    * Muestra sugerencias autocompletables dinámicas desde `xmpTags.json`.
    * Soporta queries booleanos avanzados separando por comas (ej. `vacaciones, -mar` para incluir `vacaciones` y excluir `mar`).
    * Filtra y elimina las imágenes del tablero que no correspondan con la búsqueda y coloca los nuevos aciertos de manera concéntrica mediante el algoritmo espiral.
*   **Teclas `CTRL + L` (Barrido e Indexación de Tags)**:
    * Inicia un hilo de fondo asíncrono que barre todas las imágenes del directorio actual leyendo sus metadatos XMP.
    * Bloquea temporalmente el lienzo y dibuja una barra de progreso superior que indica el porcentaje analizado.
    * Escribe los tags únicos detectados en un archivo de configuración unificado `xmpTags.json` ordenado por orden alfabético. Si ya existe, muestra un toast de advertencia de 2.5s.
    * Actualiza y guarda la caché binaria del proyecto (`xmp_tag_cache.bin`).

### Modificación y Enlace de Elementos
*   **Click Izquierdo**: Seleccionar un ítem. Click en vacío para deseleccionar.
*   **Shift + Click Izquierdo**: Selección múltiple acumulativa de ítems.
*   **Arrastrar ítem seleccionado**: Mueve todos los elementos seleccionados juntos.
*   **Arrastrar manijas de bordes**: Escala los ítems seleccionados. Sostener `Shift` preserva el ratio de aspecto.
*   **`Delete` / `Supr`**: Elimina todos los elementos seleccionados del tablero.
*   **Ctrl + Arrastrar Click Izquierdo entre ítems**: Dibuja una flecha de enlace direccional (Link) que conecta visualmente ambos elementos.
*   **Click Derecho en Ítem**: Menú de contexto para enlazarlo, borrarlo, o borrar un enlace si se hace click sobre su línea conductora.
*   **Doble Click en Nota o Nodo**: Abre el modo de edición de texto directo con cursor parpadeante. Presiona `Escape` o `Enter` para guardar los cambios de texto.

---

## 5. Página 4: Explorador de Biblioteca (Library Page)

*   **Panel Izquierdo (Carpetas del Historial)**: Muestra las carpetas de imágenes registradas en tu sesión.
    *   **Click Izquierdo**: Cargar el directorio en la biblioteca.
    *   **Click Derecho en Carpeta**: Abre un menú contextual para remover permanentemente el directorio de tu lista.
*   **Panel Derecho (Grilla Masonry)**: Muestra todas las previsualizaciones de la carpeta activa en un diseño masonry dinámico.
    *   **Click Izquierdo**: Abre la imagen a pantalla completa en la Viewer Page.
    *   **Atajo Tecla `3` (Hover + 3)**: Al posicionar el mouse sobre una miniatura del panel derecho y presionar la tecla `3`, el visualizador la carga de forma inmediata y cambia de página de forma automática al Board ( lienzo infinito), insertándola en la posición del cursor para agilizar el flujo de referencias.
*   **Hovering Live Preview**: Posicionar el puntero sobre cualquier imagen de la grilla abre un panel flotante de previsualización a gran escala en el extremo izquierdo de la pantalla (reproduciendo animaciones en tiempo real si el formato lo soporta).

---

## 6. Página 5: Vista en Cascada (Waterfall Page)

Presenta una rejilla masonry de desplazamiento infinito de miniaturas optimizada para el rendimiento:
*   **Rueda del Ratón**: Desplazamiento vertical reactivo.
*   **Ctrl + Rueda del Ratón**: Incrementar o disminuir el número de columnas (ancho del layout).
*   **Ctrl + Shift + Rueda del Ratón**: Ajustar dinámicamente el espaciado interior de celdas (de 0px a 100px).
*   **Alt + Rueda del Ratón**: Modificar la curvatura/redondeado de esquinas en las tarjetas (de 0px a 50px).
*   **Click Izquierdo / Enter**: Abre la imagen en un modal visor superpuesto translúcido. Puedes usar todos los controles del visor en este modal. Presiona `Escape` para cerrarlo.

---

## 7. Menú de Opciones (HUD settings)

Accesible desde el atajo `O` o `~` (Tilde). Permite ajustar los parámetros globales de LenteUV:
*   **Pestaña General**: Habilitar ejecución al inicio de Windows, modo lite multiproceso, ocultar al área de notificación, forzar instancia única, etc.
*   **Pestaña Viewer Page**: Habilitar zoom animado, encuadrar paneles, activar rotación de mousewheel, y mostrar marcas del HUD de metadatos.
*   **Pestaña Advanced**: Deslizador de opacidad de la ventana de configuración y opacidad de la ventana principal con el escritorio (canal alpha de DirectComposition). Configuración de páginas por defecto de inicio.
*   **Pestaña Language / About**: Permite alternar dinámicamente el idioma entre Inglés y Ruso. Muestra los créditos y la versión nativa del software.

---

## 8. Arquitectura y Detalles del Sistema de Tagging XMP

El gestor de metadatos de LenteUV (`XmpTagManager`) abandona los archivos MD5 antiguos y graba los datos de forma estándar dentro de los contenedores de metadatos XMP del propio archivo.

### Almacenamiento
* **Metadatos del Archivo**: Los tags se inyectan en el espacio Dublin Core (`dc:subject` de tipo Bag).
* **Caché en Disco (`xmp_tag_cache.bin`)**: Para evitar relecturas costosas de I/O de disco en directorios masivos, el estado de etiquetas y marcas de tiempo (`last_write_time`) se serializa en un mapa binario rápido en la carpeta `user_data/` optimizando las búsquedas.
* **Inyección Binaria en Caliente**:
  * **PNG (Chunks iTXt)**: Modifica la estructura de bloques binarios del archivo recalculando el CRC32 manualmente e inyectando un chunk iTXt.
  * **JPEG (APP1 Segments)**: Inserta directamente segmentos APP1/XMP en los marcadores de cabeceras de archivo JPEG.
  * Ambas operaciones utilizan escritura de archivos temporales con reemplazo atómico para evitar corrupción en cortes eléctricos o caídas del hilo.
* **Sincronización Bidireccional con Windows**:
  * Mediante el uso del componente `propsys.lib` del SDK de Windows, LenteUV lee y escribe directamente en el almacén de propiedades COM de Windows (`PKEY_Keywords` en `System.Keywords`).
  * Si cargas una imagen sin bloque XMP, la aplicación recuperará automáticamente las etiquetas que le hayas asignado previamente desde el Explorador de archivos de Windows.

---

## 9. Ejecución de Pruebas Unitarias (Tagging Tests)

LenteUV incluye un ejecutable aislado para verificar el estado y consistencia del sistema de metadatos e inyectores.

### Compilar el Target de Pruebas
Ejecuta esto desde la raíz del proyecto para compilar únicamente los tests unitarios:
```powershell
& "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build native/build --config Release --target tagging_tests
```

### Ejecutar las Pruebas
```powershell
./native/build/Release/tagging_tests.exe
```

Las pruebas validan la inyección limpia de tags en formatos JPEG y PNG, la persistencia binaria de la caché, la cancelación segura de hilos asíncronos y la consistencia de sincronización bidireccional con el almacén de propiedades de Windows.

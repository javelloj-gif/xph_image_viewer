# Resultado 032 — Fotassa

Fotassa es el nombre definitivo. Se corrigen títulos de los tres modos, bandeja,
About y sus traducciones español/ruso, errores, descripción del manifiesto y
VERSIONINFO. CMake genera fotassa.exe; el empaquetador y el instalador usan Fotassa.
Se retiran también los placeholders de nombre en comentarios de fuentes.

AGENTS, README, contexto técnico, desarrollo, uso, roadmap y la presentación de
la habilidad local reflejan el nombre. El ID de la habilidad se conserva.
Recursos internos image_viewer.ico/image_viewer.svg/image_viewer_setup.iss y los
nombres de staging de diagnósticos conservan identidad técnica. No son la marca.
Se mantienen licencia y atribuciones a Sergei Krumas y xPhang, datos/formatos,
pipe, clase HWND, identidad del manifiesto, Run/shell, AppId y directorio instalado.

## Historial y publicación

Base publicada: 4661acc en ambas ramas de origin y etiqueta anotada v1.0.0.
Se aplica la alternativa autorizada de un commit de corrección y push normal a
codex/015-roadmap-design y codex/repository-foundation. No se modifica upstream.
No se reescribe historia ni se mueve v1.0.0: esa etiqueta y el resultado 031
conservan la entrega con placeholder. La versión numérica permanece en 1.0.0.
Los paquetes Fotassa nuevos son locales, ignorados por Git; no se sustituyen
assets de releases remotas ni los paquetes Image Viewer originales.

## Evidencia (2026-10-26)

- AC-01/02: cmake --preset windows-x64 y cmake --build --preset release correctos,
  MSVC 19.43.34809 y SDK 10.0.22621.0. VERSIONINFO leído del PE: ProductName y
  FileDescription Fotassa; OriginalFilename fotassa.exe; FileVersion 1.0.0.0.
- AC-03: diff revisado; guías vigentes distinguen fuentes corregidas de la etiqueta
  histórica. Evidencia anterior en docs/changes se conserva sin renombrarla.
- AC-04: ctest --preset release, 10/10 en 20,74 s. El primer pase detectó la clave
  antigua de About en tests/ui_message_keys.h; se actualizó el inventario y se
  recompiló antes de repetir la suite completa. No se relajó ninguna comprobación.
- Empaquetado: scripts/build_release.ps1 -SkipIcons -SkipBuild, ambos formatos,
  staging nuevo y comprobación dumpbin de imports correctos. Sintaxis de scripts y
  lectura automática de versión comprobadas. Inno Setup compiló el instalador.
- ZIP extraído bajo build, hash del EXE igual al compilado. Harness copiado junto
  al paquete, fixture copiada y user_data propios: --ipc-package con fotassa.exe
  pasó --multiple y --lite, tres modos y cierre normal (exit 0).
- Publicación AC-05: push normal y atómico de HEAD a las dos ramas citadas;
  verificar identidad remota con git ls-remote --heads --tags origin.

| Artefacto local | SHA256 |
| --- | --- |
| fotassa.exe | 7DA9BFA52118F1F2A4ABF5733CDF03EA1A8EAE539F641894B1272A366D2A2AD1 |
| fotassa_portable_1.0.0.zip | 5F988B28953DE762849CE6C83B5FFE23833E631FA3403C890D849034A9E37FE0 |
| fotassa_setup_1.0.0.exe | 6E86DAF1D3E6A70177D2637D8AF968E95DC2CABA82D2DC1A4E7AE021E1EEFDCF |

Logs y extracción QA permanecen bajo build/fotassa-*, fuera de Git. No se ejecutó
instalación/desinstalación real ni se certifica UX visual, rendimiento o migración
de instalaciones antiguas. Datos locales y fixtures originales no se modificaron.
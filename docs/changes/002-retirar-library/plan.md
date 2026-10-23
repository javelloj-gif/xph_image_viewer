# Plan: 002 Retirada de Library

Especificación: [spec.md](spec.md). Actualización: 2026-10-23.
Estado: inventario y diseño inicial realizados; dos decisiones pendientes.
Entrada: rama `codex/repository-foundation`, HEAD `8625d95`, árbol limpio.

## Componentes y diseño

[Inventario y revisión de diseño](review.md), aplicando la habilidad local
`lenteuv-architecture-review`. Alcance exclusivo de 002, sin campaña general.
`FolderDataManager`, `FolderInfo` y `PreviewItem` ya están en core. No hay extracción
pendiente. AR-002 está resuelto en 004/011 y AR-005 implementado en 014; conservar
tokens/rutas, predicados de upload y leases débiles/cancelación en los consumidores
restantes. No modificar loader, renderer ni escáneres por retirar una página.

## Tareas y porciones verificables

- [x] T-01: Inventario de referencias exclusivas, compartidas e históricas; revisión
  estática de puntos de entrada y persistencia. AC-03/AC-04. Véase review.
- [ ] T-02: Resolver conjuntamente valor antiguo 4 y textos; registrar decisión y
  semántica de cancelación antes del código dependiente. AC-02/AC-06.
- [x] T-03: Diseño mínimo de servicios: conservar ubicación/propiedad actuales,
  sin extracción. Renombrar `libraryLoading` a un nombre de restauración de sesión
  conservando efectos y argumentos, y corregir logs/comentarios afectados.
- [ ] T-04a: Retirar LibraryPage, miembro/friend/include, enum 4, inicialización,
  animación/update/draw, ramas de eventos/sincronización/hover y fuentes CMake.
  Retirar solo SetPage(Library) en la rama IPC de carpetas. Adaptar harness para
  compilar preservando los casos vigentes de Waterfall/tira/Board. AC-03/AC-04.
- [ ] T-04b: Eliminar selección por NUM2/2 preservando OnChar y editores;
  placeholder gris sin activación ni propagación del clic. Conservar tooltip al
  hover sin estilo activo. Waterfall mantiene posición y NUM3/3. AC-01/AC-02.
- [ ] T-04c: Retirar opciones Library de Save/defaults/GetPageName y sustituir
  incremento continuo de opciones por secuencia de IDs existentes sin 4.
  Implementar decisión explícita sobre preferencias antiguas; no migrar al cargar
  ni usar cast 4 como página en blanco. No rediseñar Start. AC-03/AC-06.
- [ ] T-05: Release/CTest y comprobaciones nativas aisladas; matriz de abajo.
- [ ] T-06: Actualizar guías vigentes/AGENTS/roadmap, responsabilidades de cada
  método modificado y revisión del diff con la habilidad. AC-07.
- [ ] T-07: Completar result con evidencia real por AC, guardar cambios y dejar
  continuidad. No declarar cierre con aceptación necesaria pendiente.

## Validación prevista

| Área | Procedimiento y límite |
| --- | --- |
| Compatibilidad | Copias de settings con 4 en cada una de las tres claves, por separado y juntas; decisión/cancelación, guardado y recarga. Comparar claves ajenas y IDs 2/3/5; no datos personales |
| Teclado | NUM2/2 en Viewer, Waterfall (grid/modal), Board y overlays; escribir 2 en notas/búsqueda/tags mediante OnChar y comprobar contenido |
| Radial | Gris en reposo/hover; tooltip acordado; clic sin cambio de modo, cierre accidental ni acción de fondo; Waterfall, Viewer y Board activables |
| Arranque/opciones/drop | Inicio limpio, archivo/carpeta, sesión restaurada y configuraciones 2/3/5; opciones nunca ofrecen 4; drop archivo/carpeta y drop propio de Board |
| Servicios | Sesión/favoritos en copias, navegación/orden/layout, previews vigentes, cancelación al cambiar carpeta/modo, modal Waterfall e incorporación a Board |
| Build/tests | `cmake --preset windows-x64`, `cmake --build --preset release`, `ctest --preset release`; no debilitar regresiones compartidas al retirar casos exclusivos |
| Nativo | Adaptar matrices de arranque/paquete a tres modos; ejecución aislada con --multiple/--lite y HWND propio. No reproducir IPC-014-01 |

CTest no certifica UX/presentación nativa ni aceptación completa de IPC.
Registrar aparte observación visual, comprobación por mensajes/estado y reporte
del propietario. Si una comprobación necesaria no puede ejecutarse, queda pendiente.

## Riesgos y recuperación

No compactar Page; no tocar formatos/rutas, KTAG/KBRD ni datos originales. No borrar
catálogos heredados ni informes históricos por coincidencia textual. Artefactos solo
en build/dist ignorados, staging nuevo. Revisar Git antes de guardar; conservar cambios
posteriores del propietario. No ejecutar binarios con datos personales del checkout.

## Continuidad

La consulta conjunta está enviada; propuestas concretas en spec. No se ha recibido
decisión ni se han modificado fuentes. Siguiente paso dependiente: T-02. Después,
T-04a/b/c con configuración/build y revisión por porción. IPC-014-01 sigue diferido;
no investigar su transporte, entrega o presentación dentro de 002.

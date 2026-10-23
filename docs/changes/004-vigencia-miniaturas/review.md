# Revisión de 004 / AR-002

Fecha: 2026-10-22. Base: `ac4a5da` más diff R1/R2 preservado; revisión del cambio
local con lenteuv-architecture-review y sus referencias, en la misma sesión.

## Cobertura

- Lectura de ThumbnailInfo/ThumbnailStrip, SetFileList, LoadThumbnail y nuevo
  CompleteThumbnail; seguimiento de Rearrange y llamadores desde App/gestor.
- PreviewItem/FolderInfo y nuevo CompletePreview (`core/folder_data_manager.cpp:307`):
  token y ruta actuales, consumo único, flags y textura. Seguimiento de SortFolder
  (copia el PreviewItem completo), erase/clear en App::DeleteImage, RemoveFolder,
  Initialize y construcción de carpetas en gestor y Library. No se añade persistencia.
- LoadPreviewTexture de Waterfall y Library: captura token, conserva 200/180px,
  llama al gestor y recalcula/redibuja solo cuando aplicó el resultado.
- Loader: LoadAsync y ProcessPendingTextures inspeccionados para el hilo/propiedad
  de entrega; solo se añade friend del harness, sin tocar lógica de workers/colas.
- App/Window: únicamente friend de test. Constructor, CalculateLayout y RequestRedraw
  inspeccionados para ejecutar sin App::Initialize; HWND solo de mensajes evita
  InvalidateRect(NULL). El harness enlaza implementaciones completas y no usa stubs.
- Lectura completa de image_request.h, test nuevo y cambios CMake; comentarios de
  modelos/métodos nuevos o modificados comprobados contra implementación.
- Fuera de alcance: hover estático/animado por ruta, Board async, scanners, IPC,
  cancelación/cierre, render general y todos los hallazgos distintos de AR-002.

## Diseño y dictamen

Identidad mínima por asignación compartida inmutable de ruta. No se usa igualdad de
texto como identidad: dos solicitudes para la misma ruta tienen tokens distintos.
Mientras existe callback antiguo, su asignación sigue viva y no puede reciclarse su
dirección. Mover/reordenar elementos conserva su token; borrarlos lo elimina del modelo.
Esto sustituye la generación global por identidad de cada solicitud y no exige
invalidar trabajo válido de toda una carpeta al ordenar o borrar otro elemento.

La tira traslada cada entrada previa una vez. Las ocurrencias repetidas comparten
solo caché terminado, evitando isLoading sin callback. Ambos consumidores de previews
comparten el mismo token/flag y no emiten una segunda carga del elemento en vuelo.
Las búsquedas actuales son lineales; no se afirma ganancia de rendimiento ni se añade
un índice/caché paralelo a mantener. La responsabilidad de resolver el modelo queda
en el gestor; cada página conserva layout/redraw.

Sin hallazgos nuevos accionables ni bloqueantes en este diff. La prueba inicial
reprodujo asociación incorrecta A/B sin crash. Build Release, 98 checks async y
CTest 3/3 pasaron; resultados y comandos en [result.md](result.md).
AR-002 corregido en ese alcance, no como garantía general de seguridad del loader.

Límites: pruebas deterministas de colas con cero workers y WARP; no UX completa,
rendimiento, carga masiva ni carreras temporales reales. Los callbacks siguen
capturando consumidores vivos según el ciclo de vida de App; no hay nueva cancelación
ni control de vida de objetos externos. El procesamiento GPU anterior al descarte
se mantiene. Siguiente paso propuesto: 003/AR-003, diagnóstico antes de corrección IPC.

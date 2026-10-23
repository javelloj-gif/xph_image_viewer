# Revisión 011

Fecha: 2026-10-22. lenteuv-architecture-review, sesión actual, sin delegación.
Rama codex/repository-foundation, HEAD ac4a5da25b3cdaf395569b7b5a9c3bf07ecd4452.
Diff acotado contra build/optimization-011/before, no contra HEAD que carece de
las entregas previas sin commit. Sin auditoría global del repositorio.

## Cobertura

ImageLoader::LoadAsync (:441), ProcessPendingTextures (:469), colas, workers y
Shutdown; declaraciones de ImageUploadPredicate y PendingTexture. Tira:
LoadThumbnail (:147), IsThumbnailRequestCurrent (:162) y CompleteThumbnail (:169).
FolderDataManager::IsPreviewRequestCurrent (:310), CompletePreview (:320), contrato
de revisión 007; LoadPreviewTexture de Library (:335) y Waterfall (:370).
Llamadores restantes de LoadAsync/LoadAnimatedAsync y orden de miembros de App.
Harness TakeUploadRequest (:1036), UploadValidityRegressions (:1062), spec/plan y
documentos 004/007/010. Sin revisión nueva de decoder, scanner, compositor o IPC.

## Riesgos tratados y coste

- UP-001 — Riesgo de compatibilidad, prioridad alta, confianza confirmada por código:
  ProcessPendingTextures subía antes de entregar callbacks; suprimir completions o
  sustituir resultados por nullptr cambiaría efectos de consumidores. Se omite solo
  UploadToGpu, se entrega el resultado original. Pruebas WARP y fallos conservan
  identidad/hilo/conteo y reencolado; necesario para AC-01. Coste pequeño.
- UP-002 — Riesgo de vigencia, prioridad alta, evidencia de código: capturar índice,
  dirección de item o solo ruta permitiría aceptar otra generación. Ambos predicados
  comparan token y ruta actual, igual que 004. No consumen tokens ni modifican revisión
  007. AC-02 prueba movimiento, recreación con solicitud nueva y duplicados. Coste
  pequeño de implementación; búsqueda lineal adicional por completion, sin ahorro
  temporal garantizado. Comprobación final deliberadamente conservada.
- UP-003 — Restricción de propiedad/hilo, prioridad alta, evidencia de código:
  lambdas poseen token y capturan consumidor prestado, como los callbacks previos.
  Solo UI puede consultarlas, fuera del mutex; sin bombeo ni excepciones. Workers
  transportan/destruyen funciones pero no leen modelos. Shutdown no entrega callbacks
  al destruir consumidores. No se certifica cierre acotado ni se corrige AR-005/006.
  Prueba con tarea en hilo COM y entrega UI; coste pequeño, necesario para AC-01.

No hallazgos accionables pendientes dentro del diff 011. Cada método nuevo/modificado
tiene comentario de responsabilidad; los contratos públicos documentan propiedad y
hilo. Las comparaciones de token/ruta permanecen en el guard y la completion para
mantener la defensa final sin guardar punteros reubicables ni introducir otro índice.

## Evidencia y límites

Release correcto, CTest 5/5, 162022 checks async totales (321 nuevos) y 294 decoder.
WARP crea bitmaps en controles vigentes y ninguno en obsoletos; código confirma que
el guard rodea la única llamada UploadToGpu. Sin contador global de GPU, medición de
VRAM, latencia, nueva comparación de frames ni recorrido nativo en esta sesión.
Los diez escenarios nativos de 010 son evidencia anterior, no repetida para 011.
Sin cambios a ACK, elección, cierre, Library, gestos, persistencia o scopes pendientes.

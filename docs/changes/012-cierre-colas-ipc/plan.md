> Cierre funcional 2026-10-23: aceptado el lote base según reporte del propietario. 013 añade join de Board/búsqueda antes de sus dependencias. AR-005 global queda diferido explícitamente; no iniciar nueva porción automáticamente. [Continuidad vigente](../013-cierre-base/resume.md).

# Plan 012

[Spec](spec.md), [resultado](result.md), [revisión](review.md).

- [x] Revisar Git/guías/habilidad y revisiones 004–011; snapshot de fuentes/binario en
  build/shutdown-012/before. No usar HEAD como baseline de producto.
- [x] Actualizar inventario acumulado; especificar primera porción AR-005 y límites.
- [x] Añadir harness de cierre aislado por procesos y reproducir baseline.
- [x] Loader: detener admisión, cancelar cola, unir activos y liberar pendientes en UI.
- [x] IPC: evento stop y operaciones overlapped; esperar cancelación antes de liberar.
- [x] App: cierre previo a destrucción de consumidores/fallo parcial; payloads pendientes.
- [x] Release/CTest, smoke nativo, revisión acotada y evidencia con tiempos de cierre.
- [x] Portable aislado, hashes, protocolo, roadmap y continuidad.

Estado 2026-10-23: completado en alcance. Riesgos CL-001..004 tratados; Release/CTest
6/6 y diez escenarios nativos. Portable 012 listo; aceptación funcional del usuario
pendiente. AR-005 sigue abierto para cancelación por consumidor y límites de WIC,
scanners y envío síncrono. Siguiente porción requiere especificación propia, según
[continuidad](resume.md); no pasar a OPT-007 por inercia del prompt anterior.

Sin delegación. Recuperación solo contra snapshot del diff 012, nunca reset global.
No escribir fixtures ni usar endpoint global en tests. Preservar portable 010/011.

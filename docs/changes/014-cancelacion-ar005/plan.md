# Plan 014

[Spec](spec.md). Estado: completado; evidencia por criterio en [resultado](result.md).

Seguimiento manual: [IPC-014-01](user-validation.md), fallo del punto 6 reportado
el 2026-10-23. El propietario lo considera legado y pospone expresamente su
tratamiento. No añadir investigación/corrección a este plan ni reabrir AR-005 por
inercia. No hay aceptación funcional completa de IPC.

- [x] Revisar Git, contratos 012/013, guías, habilidad y consumidores actuales.
- [x] Especificar alcance completo y límites físicos de WIC antes de implementar.
- [x] Loader: lifetime débil por solicitud, poda real de cola, parada cooperativa
  global/por consumidor y entrega UI sin callbacks cancelados.
- [x] Integrar todos los consumidores y sus transiciones; conservar identidades 004.
- [x] Board/escáner de búsqueda: cancelación, publicación, progreso y join/reinicio.
- [x] Cliente IPC: presupuesto total y escritura overlapped sin flush bloqueante.
- [x] Regresiones de consumidor/WIC/escáner/IPC; caracterización real aislada.
- [x] Release/CTest y smoke nativo; revisión de diff, contratos y comentarios.
- [x] Portable limpio extraído, hashes, arranque y protocolo funcional.
- [x] Roadmap/cierre/evidencia y commit, con auditoría de todos los AC.

La siguiente acción al retomar se determina por esta lista y Git; no reemplazar
los faltantes por disposición documental ni cerrar con criterios necesarios pendientes.

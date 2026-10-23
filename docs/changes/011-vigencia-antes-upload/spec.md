# 011: Vigencia antes del upload (OPT-006)

Fecha: 2026-10-22. Autorizado por el usuario; base ac4a5da y cambios locales 001..010.

## Contrato

Añadir a LoadAsync un predicado opcional de vigencia, transportado con el resultado
CPU y evaluado únicamente en ProcessPendingTextures, en UI y fuera de mutex,
inmediatamente antes del upload. Debe ser de solo lectura, no lanzar ni bombear
mensajes; el consumidor y su modelo deben vivir durante la entrega, como antes.
Ausencia de predicado conserva el comportamiento previo. False omite solo upload:
el callback se entrega una vez con el resultado CPU original, incluso nullptr en
fallo. No convertir caducidad en fallo ni suprimir callbacks. Las comprobaciones
finales de 004 permanecen, incluidas identidad, ruta y consumo único.

Aplicar solo a miniaturas de tira y previews compartidos Library/Waterfall, usando
el token y ruta actuales, sin índices capturados. Reordenar/retener permite upload;
borrar/recrear la misma ruta no revive la solicitud. Predicado no modifica loading,
tokens ni revisión 007; únicamente la completion vigente conserva sus efectos.
Board, hover y LoadAnimatedAsync quedan intactos. No cancelar decode/tareas ni
rediseñar cierre, ACK, elección IPC, Library, scheduler, datos o gestos.

## Aceptación

- AC-01: con D2D WARP real, resultados CPU válidos caducados no crean bitmap;
  vigentes/sin predicado sí lo crean. Conservar callback, resultado y hilo UI;
  predicado no corre al decodificar y callbacks pueden encolar trabajo posterior.
- AC-02: tira y ambas páginas: reemplazo, eliminación, recreación de la misma ruta,
  ruta mutada, retención/reordenación, duplicados y fallos conservan 004. Verificar
  bitmap omitido/creado por la cola productiva y revisión 007 solo cuando corresponda.
- AC-03: Release/CTest completo y regresiones 004/007. Contrastar fuentes con snapshot
  local anterior, revisar contratos/comentarios y actualizar continuidad.

Se prueba trabajo GPU evitado, no mejora de latencia, memoria global ni UX nativa.
No exige un benchmark temporal para este guard; añade una búsqueda lineal por
completion optada, también para fallos. Aceptación visual 010 sigue pendiente.

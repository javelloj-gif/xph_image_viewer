# 003: implementación autorizada — 2026-10-23

Base: 9b1d7e8. Rama: codex/003-arranque-cache. El propietario aprobó ampliar 003
a instrumentación, arranque asíncrono completo, planificación y caché persistente
de miniaturas, conservando calidad, efectos, datos, IDs e IPC. Esta decisión
sustituye la exclusión histórica de optimizaciones de spec.md.

## Contratos y secuencia

- [x] P1: diagnóstico QPC opcional, eventos por generación/frame, ejecutor CSV/JSON;
  medir base instrumentada antes de cambiar comportamiento. Candidato 002 intacto.
- [x] P2: decodificación principal async con política estática/animada/fallback
  conservada; GPU/UI en hilo principal, latest-wins y cancelación cooperativa.
- [x] P3: enumeración/ordenación en trabajador dedicado; prioridad a ruta explícita,
  snapshots vigentes y sesión pendiente preservada al guardar/cerrar.
- [x] P4: cuatro workers, como máximo tres miniaturas concurrentes, prioridad al
  destino y cancelación fuera del viewport; mismas animaciones.
- [x] P5: caché compartida de variantes 100/200, BGRA sin pérdidas, Compression API
  y SQLite de Windows. Raíz user_data/cache propia versionada; sin escribir originales.
  Firma ruta/identidad/tamaño/mtime/pipeline antes/después; errores son misses.
- [x] P6: tests de píxeles, invalidación, corrupción, presupuesto, concurrencia,
  generaciones, sesión pendiente y cierre; Release/CTest y revisión de arquitectura.
- [ ] P7: comparación nativa, 30 aperturas por posición/base/candidato, caché vacía,
  preparada y reutilización; candidato portable desde staging nuevo y evidencia.

## Presupuestos y medición

Caché: min(1 GiB, 1% del espacio libre), expulsión LRU, protección bajo 1 GiB libre,
colas/transacciones acotadas y escrituras prescindibles al cerrar. Conservar tamaños
100x100 y 200x200 sin derivarlos entre sí. No precalcular toda la carpeta.

Caso autorizado: F:\Pictures\FotosTEMP, solo lectura: 7749 elementos, 6694 JPEG.
500 ms de apertura y 1–5 s de miniaturas son estimaciones del propietario.
Objetivo: original utilizable en 100 ms, sin vista reducida ni proceso residente.
Separar invocación, inicio, UI, original, navegación y miniaturas. QPC/Present enviado
no certifican pantalla: correlacionar estadísticas DXGI o traza gráfica; si falta
esa evidencia, no certificar p95 <=100 ms. Registrar CPU/RAM/GPU/E/S/disco y límites.
Primera sesión real requiere coordinación; no reiniciar ni borrar cachés del SO.

## Continuidad

Implementación y Release/CTest 8/8 correctos; smoke nativo de diez escenarios
pasado en la porción anterior, se repite sobre el candidato final. La serie inicial
fue exploratoria; no se utiliza para afirmar una mejora. La base instrumentada se
guarda en commits reproducibles anteriores a la implementación. P7 en curso.

La selección real encontró JPEG cercanos a 5 MB de unos 10–12,6 MP; el ejecutor
elige tres de resolución comparable y registra dimensiones, sin atribuirles 24 MP.
Conservar el reporte inicial del propietario como estimación, separado de esta evidencia.

Registrar implementación, comandos y resultados en result.md;
no cerrar criterios pendientes ni atribuir mediciones de otra revisión.

# 003: implementación autorizada — 2026-10-23

**P1..P7 y último lote interno cerrados por aceptación del propietario, 2026-10-24.**
Último lote: enumeración solapada con renderer, prioridad explícita sin escaneo
duplicado, instrumentación y regresiones. Release/CTest 8/8, diez escenarios nativos
y smoke final correctos. Medición fría y límites en [cierre](closure.md).

## Registro anterior al cierre (histórico)

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
- [x] P7: comparación nativa, 30 aperturas por posición/base/candidato, caché vacía,
  preparada y reutilización; candidato portable desde staging nuevo y evidencia.

## Presupuestos y medición

Caché: min(1 GiB, 1% del espacio libre), expulsión LRU, protección bajo 1 GiB libre,
colas/transacciones acotadas y escrituras prescindibles al cerrar. Conservar tamaños
100x100 y 200x200 sin derivarlos entre sí. No precalcular toda la carpeta.

Colección real autorizada, solo lectura: 7749 elementos, 6694 JPEG. Ruta y nombres
privados en manifests locales, no en Git.
500 ms de apertura y 1–5 s de miniaturas son estimaciones del propietario.
Objetivo: original utilizable en 100 ms, sin vista reducida ni proceso residente.
Separar invocación, inicio, UI, original, navegación y miniaturas. QPC/Present enviado
no certifican pantalla: correlacionar estadísticas DXGI o traza gráfica; si falta
esa evidencia, no certificar p95 <=100 ms. Registrar CPU/RAM/GPU/E/S/disco y límites.
Primera sesión real requiere coordinación; no reiniciar ni borrar cachés del SO.

## Continuidad

P1..P7 implementados y comprobados en sus porciones automatizadas. Fuentes b0e98f8;
Release/CTest 8/8 (94 checks startup/cache), diez escenarios nativos y smoke del
portable sin diagnóstico correctos. Las series anteriores son exploratorias cuando
su hash no coincide con el final. Base 5d1f67f preservada en referencia Git propia.

Matriz posterior: n=30 por posición/versión/escenario, 003 preparada y vacía;
120 ciclos comparables de reutilización. [Resultado de rendimiento](performance-result.md)
explica primera generación, recursos y censuras. EXE/ZIP limpios y hashes en result.md.
La selección efectiva fue de unos 12,5 MP, sin atribuirle 24 MP.

- [x] Reporte del propietario: zoom/paneo, tira, navegación rápida, Waterfall, Board e IPC OK.
- [x] Reporte manual de primera apertura tras iniciar el SO: unos 3 s, 2026-10-24.
- [x] Reporte posterior: múltiples pruebas con SO recién iniciado; unos 3000 ms
  iniciales NO aceptables. La aceptación previa de ese tiempo queda sustituida.
- [x] Una sola primera apertura instrumentada autorizada: 1932 ms hasta envío
  del original; confirmación DXGI observada a 1956 ms. Sin warmup ni repetición.
- [x] Análisis acotado de causas y opciones con compromisos, sin implementar producto.
- [x] Opción 1 seleccionada para preparación: WPT instalado, ETW validado sin producto,
  directorio de entrada fijo y multimedia solo lectura; scripts listos según
  [etw-capture-protocol.md](etw-capture-protocol.md).
- [x] Captura ETW única de otro boot y análisis causal: [etw-result.md](etw-result.md).
  Original enviado 2216 ms, DXGI observado 2237 ms, espera síncrona del creador
  1426 ms en ruta Defender; FileScan del EXE 1408 ms. Sin cambiar producto/seguridad.
- [x] Mejoras internas implementadas y medidas; el propietario acepta el cierre con el rendimiento restante como seguimiento separado.
- [x] Disposición expresa: seguimiento independiente aceptado de IPC-003-CONCURRENT-01, sin declararlo corregido.

Evidencia y alcance del reporte en [user-validation.md](user-validation.md).
La primera captura y propuestas están en [first-session-analysis.md](first-session-analysis.md).
El rendimiento inicial está rechazado; no se certifica 100 ms ni matriz funcional universal.

No se declara cierre global ni se reabre IPC-014-01. Continuar con resume.md.

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

- [ ] Aceptación funcional del propietario sobre el candidato 003.
- [ ] Primera apertura real de una nueva sesión Windows, coordinada y sin reinicio automático.
- [ ] Disposición explícita de límites: 100 ms no acreditados, Waterfall vacío más lento,
  intermitencia concurrente del harness IPC sin causa confirmada.

No se declara cierre global ni se reabre IPC-014-01. Continuar con resume.md.

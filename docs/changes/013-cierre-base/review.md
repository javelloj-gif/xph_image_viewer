# Revisión de cierre 013

2026-10-23. Habilidad lenteuv-architecture-review, misma sesión, sin delegación.
Base Git ac4a5da, rama codex/repository-foundation, más lote local 001–012 conservado.
El snapshot funcional de entrada es el árbol local, no HEAD. Revisión de alcance
acotado; las revisiones históricas mantienen su cobertura y ubicaciones.

## Cobertura y hallazgos

Lectura de MetadataCache .h/.cpp completos y su uso Initialize/ReadTags en
XmpTagManager, algoritmo de AsyncDirectoryScanner, BoardTagScanner .h/.cpp,
orden de miembros/cierre de App y BoardPage, cierre del loader y cliente/servidor IPC.
Contraste de CMake, harness de metadatos, aislamiento PowerShell, pruebas de cierre
y aceptación de 001/010/012. Se leyeron habilidad, criterios y formato de hallazgos.

- **AR-006:** LoadCache publicaba después de lecturas no comprobadas y borraba la
  caché viva prematuramente. Regresión nueva falló antes del fix. El candidato
  comprueba bytes restantes antes de resize, valida mínimos de registros/tags,
  conserva el mutex contra escritores y publica con swap solo al completar.
  Excepciones de lectura/asignación retornan false conservando entradas/dirty.
  Sin nueva versión, ruta o límite arbitrario. El escritor no cambia. La memoria
  de una caché válida grande sigue siendo proporcional al archivo y hay mapa temporal.
- **CL-013-01 (defecto, prioridad alta, código confirmado):** App declaraba
  XmpTagManager después de BoardPage; por destrucción inversa el gestor desaparecía
  antes del join de BoardTagScanner, que lo usa en ReadTags/SaveCache. Además el
  loader se apagaba antes de unir búsquedas que pueden usar su WIC factory prestada.
  No se afirma crash reproducido. Corrección mínima: unir ambos productores en App
  antes de Shutdown del loader, tanto en WM_QUIT como destructor/fallo parcial.
  Test retiene un trabajador de Board y callback de búsqueda durante destrucción:
  ambos deben observar el loader aún activo. Sin cambio de orden de miembros o
  de política de escaneo. El test añade solo amistad privada a la clase existente.
- **AR-007:** comentario exacto de LoadCache y sus lambdas, nueva prueba y contratos
  de App modificados. Se corrige comentario engañoso de CancelScan: espera, no cancela.

Sin otros hallazgos bloqueantes en este diff. No se auditó íntegramente Board,
escritores, entradas, shell, compositor ni todos los headers. Riesgos de sincronización
del escáner y cancelación durante navegación permanecen candidatos separados;
no afirmar que el join arregla toda su concurrencia ni que WIC tenga plazo máximo.

## Comprobaciones

Configure y Release completos; CTest 6/6. Caché: 164 prefijos truncados en ambos
estados dirty, firma/versión y cuatro cantidades imposibles, roundtrip, duplicados,
Unicode, vacío y bytes finales. Async: 162025 checks incluyendo tres nuevos de
preparación/orden de productores. El control previo KTAG falló con excepción de
regresión; el fix pasa. No se ejecutó una campaña visual nueva ni profiling.
Portable desde staging nuevo, imports validados por script; no instalado ni asociado.

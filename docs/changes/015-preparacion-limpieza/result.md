# Resultado: 015

Especificación: [spec.md](spec.md). Plan: [plan.md](plan.md).
Estado: completado en el alcance de regenerables identificados, 2026-10-25.

## Cambios y evidencia

Revisión inicial `9a334ad2b775bd5fe7244e805384ea4bed2038cd`, sin cambios locales.
Trabajo aislado en rama codex/015-roadmap-design. Sin cambios de código funcional.

| Criterio | Procedimiento / resultado |
| --- | --- |
| AC-01 | Pasó: Git registrado; SHA256 del portable final coincide con cierre 003: `874F5783B219E24FE1AD74B330BBF9A344EF2DAB0E3664738CED579CF5FF50D6` |
| AC-02 | Pasó: manifest absoluto previo; 361 archivos, 1 645 472 736 bytes (1,53 GiB) retirados y directorio dist/legacy-output vacío eliminado; rutas verificadas dentro del workspace y sin reparse points |
| AC-03 | Pasó: 48 hashes de archivos de ambas entregas, ZIP y user_data raíz idénticos antes/después; dist contiene exactamente los dos directorios y dos ZIP acordados |
| AC-04 | Pasó: roadmap, índice y contexto actualizados; diff y destinos relativos comprobados |

Se retiraron objetos, PCH, bases intermedias y logs de tracking de compilación de
build/windows-x64 y build/legacy-native, el ZIP de la entrega 002 sobrante y el
debug_ipc.txt versionado en raíz (residuo de diagnóstico ya cubierto por .gitignore).
No se retiraron binarios de referencia ni runtime. Será necesario regenerar los
intermediarios al compilar; no se hizo una compilación solo para recrearlos.

Evidencia local ignorada: build/session-015/removed-files.csv, preserved-before.csv,
preserved-after.csv y summary.json. No publicar estos manifests, datos ni artefactos.

## Preservación y excepciones

- Conservadas dist/lenteuv_003_final_20261024 y dist/lenteuv_qa_003_20261024_async_cache,
  con todos sus archivos y ZIP originales. No renombrados ni ejecutados.
- Conservados user_data, fixtures, fuentes, licencia, assets y estado de herramientas.
- Conservados build/backups, checkouts anidados clean-checkout/publish-checkout y el
  worktree bajo .kilo: incluyen historial, respaldos o estado que no es un regenerable.
- Conservados diagnósticos de 003, trazas ETW/QPC, binarios/símbolos de referencia,
  manifests, scripts locales y árboles de datos de pruebas. No se pueden regenerar
  observaciones frías consumidas; su presencia en build no autoriza destruirlas.
- No se eliminó build completo ni se recorrieron repositorios anidados para limpiar.

## Límites y continuidad

La identidad conservada permite preparar la próxima comparación; no equivale a una
medición actual ni certifica rendimiento futuro. No se ejecutaron CTest, benchmarks,
metadatos ni recorridos nativos: este cambio solo altera documentación y artefactos.
Base y candidato deben medirse al iniciar el primer cambio funcional. 016 presenta
propuestas; 017–022 siguen pendientes de sus planes.

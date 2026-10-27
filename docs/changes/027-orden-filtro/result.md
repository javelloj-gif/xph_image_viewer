# Resultado: Orden y filtro

Implementado en el árbol de trabajo sobre `36e4248c15db1ea6cceedaf9c7f1ce19e6acd9ed`.
Criterios funcionales **AC-03 / AC-04** comprobados con el harness productivo aislado y revisión de fuentes.

Release compila. CTest final 10/10 (16,79 s); ui.product_batch añade 509 comprobaciones.
Las capturas WARP ejercitan controles nativos a 96/144/192 DPI, anchos 320/801, en inglés/ruso/español: 72 frames de docks/modal/idiomas y 72 adicionales con metadatos.

Evidencia local ignorada: build/batch-026-visual y build/windows-x64/Testing.
Comparación final de arranque/reposo completada: [medidas, hashes y límites](../027-orden-filtro/performance.md).
Mediana/p95 dentro del presupuesto; cero Draw/Present en cinco intervalos estables de reposo.

Portable limpio: dist/image_viewer_026_030_20261026.zip, creado con el script productivo
desde staging nuevo, runtime app-local, LICENSE y catálogos. Sin datos del checkout.
Imports verificados con dumpbin; el EXE coincide con el medido. No se probó instalación en otra máquina.

No equivale a aceptación manual del propietario ni a una certificación de todos los códecs o carpetas fotográficas grandes. Un fallo intermitente de IPC concurrente apareció en una ejecución intermedia; la repetición aislada y la suite completa posterior pasaron. Se conserva el seguimiento histórico IPC-003-CONCURRENT-01, sin declararlo corregido.

## Evidencia por criterio

| Criterio | Evidencia y límite |
| --- | --- |
| AC-03 | Ejecución: seis órdenes, familias combinadas, vacío, draft/Cancel, Apply y round trip de Settings; snapshots WARP del modal en tres idiomas/DPI. Navegación desde índice -1 revisada en el handler real. |
| AC-04 | Ejecución: solicitudes canceladas, revisión de mutación, manager real con cambios rápidos, lista vacía, original excluido e identidad de preview recuperado. Suites de cierre/cancelación existentes pasan. |

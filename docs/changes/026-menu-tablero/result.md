# Resultado: Menú y envío al tablero

## Ajuste tras validación del propietario — 2026-10-26

El propietario confirma pruebas funcionales satisfactorias del lote, salvo el orden
del menú. Corregido a Delete, Rule of Thirds, Tags, Send to Board, Settings y Exit.
La disposición se comparte entre Viewer y su modal de Waterfall. Se conservan los
IDs de comandos, sus handlers, estados y traducciones.

Comprobación proporcional: orden de las seis entradas revisado en fuentes,
`git diff --check` y compilación Release de image_viewer satisfactorios.
Nuevo portable limpio: `dist/image_viewer_026_030_20261026_menu_fix.zip`;
imports comprobados por el empaquetador. EXE SHA256:
`81A5F784EB1DCC0858E16A2AF523FCE657A4EA7DFD2EAC55CDF8B5C09CAEE672`.
No se repiten mediciones de rendimiento por esta reordenación de entradas;
las métricas inferiores identifican el candidato anterior. La validación funcional
del propietario sustituye la ausencia de aceptación manual indicada en el registro inicial.

## Registro inicial del lote

Implementado en el árbol de trabajo sobre `36e4248c15db1ea6cceedaf9c7f1ce19e6acd9ed`.
Criterios funcionales **AC-01 / AC-02** comprobados con el harness productivo aislado y revisión de fuentes.

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
| AC-01 | Código: seis AppendMenuW del visor/modal, flags sin original y check de tercios; catálogos comprobados por el loader real. No se automatizó TrackPopupMenu interactivo. |
| AC-02 | Ejecución: KBRD sintético, preparación, envío único, original/modo conservados y varias entradas/salidas con nota sin guardar. Acción contextual enlaza ese mismo método por revisión de código. |

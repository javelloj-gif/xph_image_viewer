# Resultado: 006 Optimizaciones locales

Seguimiento independiente: [007](../007-layouts-por-cambios/result.md) implementa
layouts por cambios Library/Waterfall y conserva los 12 frames de esta entrega.
La evidencia/cierre original de 006 de abajo se mantiene como registro histórico.

[Spec](spec.md), [plan](plan.md), [revisión](review.md). Fecha: 2026-10-22.
Estado: completado en alcance. Base ac4a5da más R1/R2/003/004/005 sin commit,
preservadas en codex/repository-foundation. Spec/plan/resultado esperado ante
discrepancia se escribieron antes de editar producción.

## Implementación

- SetFileList indexa rutas con cadena de índices de ocurrencias antiguas y primera
  ocurrencia nueva. Evita find_if/erase por entrada; coste promedio O(A+B), con A/B
  tamaños antiguo/nuevo. Conserva orden, duplicados, textura/error, tokens y selección.
- Update/Draw bisecan geometría float existente para recorrer candidatos visibles,
  conservando filtros inclusivos originales. Coste O(log N + V) en geometría normal;
  parámetros inválidos/no finitos/overflow de base conservan recorrido completo.
- DrawOverlayInfo comprueba opacidad antes de formatear/medir. Sin cambios de Update,
  fade, comandos de dibujo visibles, otros overlays o cadencia.
- Harness existente ampliado: oráculo previo, equivalencia diferencial, snapshot
  offscreen y microbenchmark optativos. Dos accesos friend para pruebas, sin API nueva
  de producto, nueva ventana visible, swapchain ni datos personales.

## Evidencia de aceptación

Entorno: Windows x64, Visual Studio 2022, Release, 16 procesadores lógicos disponibles;
WIC/DirectWrite y D3D11 WARP del sistema. Mismo checkout y proceso para cada par del
microbenchmark. No campaña de rendimiento global ni garantía entre equipos.

| Criterio | Procedimiento y resultado |
| --- | --- |
| AC-01 | Pasó: 250 reconstrucciones con semilla 6004, rutas Unicode, tamaños variables, duplicados y mezcla de vacío/textura/error/pendiente; todos los campos y tokens coinciden con el algoritmo previo. Se conservan los 98 checks históricos 004. |
| AC-02 | Pasó: 3402 geometrías, listas hasta 10000, anchos cero/negativos/fraccionarios, bordes exactos, scroll/extremos finitos/Inf/NaN; mismo conjunto y orden para ambos filtros Update/Draw. |
| AC-03 | Pasó en alcance offscreen: 12 frames de métodos reales antes/después; anchos 320/801, DPI 96/144, opacidad cero/intermedia/completa, selección/hover, drag/inserción, carga/error/textura, crop/reflejo, Unicode y modal Waterfall. Bytes idénticos. |
| AC-04 | Pasó: cmake --build --preset release; ctest --preset release, 4/4 en 10,50 s. ui.async_previews informa 19501 checks, incluidas las regresiones nuevas. Diff/check/comentarios revisados; medición acotada debajo. |

Baseline de píxeles generado antes de cambiar métodos de producción. Snapshot local
de las seis fuentes afectadas y sus hashes en build/optimization-006/before.
SHA256 conjunto before.bgra y after.bgra:
`64E006D5427E0F17A04BDA1CAED169C845F5AC985457C9E1D2D3EFC12207D88B`.
EXE Release de producto final:
`597B406F8C248E72F66E246610DFA5CD00D2086229740DECAAFB8B440FA75F1B`.
Capturas, datos sintéticos y CSV quedan ignorados bajo build; no se publican.

## Medición local

Modo optativo de async_preview_tests: `--optimization-benchmark`. Cada par recibe
entradas preconstruidas iguales fuera del intervalo. Se mide reconstrucción y reset
de selección, incluidos temporales internos; alterna orden, descarta una ronda y toma
mediana de cinco. No hay umbral de tiempo en CTest. Listas sintéticas, sin lectura de
archivos, decodificación, Present, carga completa de carpeta o medición de memoria.

| Entradas | Escenario | Referencia ms | Índice ms |
| --- | --- | ---: | ---: |
| 100 | Lista nueva | 0,0336 | 0,0208 |
| 100 | Orden inverso retenido | 0,0369 | 0,0254 |
| 100 | 32 rutas repetidas | 0,0267 | 0,0209 |
| 1000 | Lista nueva | 2,9593 | 0,2848 |
| 1000 | Orden inverso retenido | 3,1198 | 0,3816 |
| 1000 | 32 rutas repetidas | 2,4874 | 0,2285 |
| 10000 | Lista nueva | 341,993 | 3,8230 |
| 10000 | Orden inverso retenido | 338,133 | 3,9706 |
| 10000 | 32 rutas repetidas | 249,155 | 2,2239 |

Se observa reducción de tiempo de esta operación en las muestras, no del arranque
completo. El subconjunto de visibilidad con ancho normal enumera 133 candidatos
frente a 60720 visitas completas agregadas; excluye probes de bisección y no es
medición de tiempo/GPU. El índice añade claves, buckets y enlaces temporales O(A+B),
liberados al terminar; no demuestra menor RAM, working set o pico total.

## Revisión, límites y continuidad

Sin criterios necesarios fallidos o pendientes dentro del alcance definido. Equivalencia
probada en los casos descritos; no certifica compositor/Explorer, interacción nativa,
todos los DPI/formatos, duración de animaciones ni rendimiento global. No sesión real
de primera apertura o reposo nueva. No reducir FPS, calidad, efectos o resolución.

Actualizados índice de cambios, roadmap, desarrollo, contexto y seguimiento 003.
Resto OPT-002 grids y OPT-003 cachés, OPT-004/005/006/007 siguen sin implementar.
AR-005/006/007 previos y decisiones Library no se cierran por este trabajo.
Sin cambios a IPC, cierre, datos persistidos ni asociaciones. Sin commits.

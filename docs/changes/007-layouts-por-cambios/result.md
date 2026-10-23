# Resultado: 007 Layouts por cambios

Seguimiento independiente: [008](../008-formatos-texto/result.md) reutiliza formatos
DirectWrite privados y conserva los snapshots 006/007. El cierre de 007 debajo se
mantiene como evidencia histórica; su recomendación inicial se ejecutó en otro cambio.

[Spec](spec.md), [plan](plan.md), [revisión](review.md). Fecha: 2026-10-22.
Estado: completado en alcance. Base ac4a5da más trabajo R1/R2/003/004/005/006 preservado
sin commit en codex/repository-foundation. Se escribieron aceptación y resultado
esperado ante discrepancia antes de modificar producción.

## Cambios

Library y Waterfall conservan su cálculo greedy sin cambios en operaciones, fallback,
desempate, rects o dibujo. Cada CalculateLayout retorna temprano con clave válida.
La clave contiene revisión transitoria del modelo, índice de carpeta, ancho y cantidad;
Waterfall añade columnas y gap. Publicación solo tras cálculo correcto.

FolderDataManager incrementa revisión en Initialize/AddFolder/RemoveFolder/SortFolder
y al aplicar una textura en CompletePreview; Library::AddFolder y App::DeleteImage
notifican mutaciones directas. LoadSession/ApplySortMode ya usan esos mutadores.
Una entrega de cualquier página invalida ambas; fallos/obsoletos/repetidos conservan
revisión. La selección/ancho/parámetros se detectan en clave sin recorrer previews.
Scroll/hover/alto/corner radius siguen aplicándose fuera del cálculo base.

Se conservan los almacenamientos existentes PreviewItem::rect y m_previewRects.
Solo claves y epoch de tamaño fijo adicionales, sin nueva memoria proporcional a N,
texturas retenidas, mensajes o hilos. Revisiones no se guardan en archivos.

## Evidencia

Windows x64, Visual Studio 2022, Release; WIC/DirectWrite/WARP del sistema, 16
procesadores lógicos según entorno previo. Todos los datos sintéticos/artefactos bajo
build/optimization-007, ignorados. Sin ventana visible/swapchain ni cambios de asociaciones.

| Criterio | Resultado |
| --- | --- |
| AC-01 | Pasó: 140 modelos con semilla 7002, vacíos y mezclas de fallback/texturas/proporciones; cada etapa compara bits de todos los rects contra fórmulas previas. Cambios de ancho, columnas y gap por separado, orden, selección, scroll/alto, reemplazo en mismo slot, reubicación y reinicialización. Hits conservan una salida marcada sin reescribirla. |
| AC-01 callbacks/mutadores | Pasó: entregas reales por LoadPreviewTexture/ProcessPendingTextures desde ambas páginas, invalidación cruzada, fallo/repetición; SortFolder y App::DeleteImage cambian revisión. Puntos Initialize/AddFolder/RemoveFolder y Library::AddFolder revisados en fuente; altas/bajas de carpetas también simuladas por contrato mutable, sin ejecutar SaveSession. |
| AC-02 | Pasó: 24 frames de grids antes/después, anchos 801/1601, DPI 96/144, scroll fraccionario, hover, sombras/rounded/checkerboard, placeholders y textura progresiva. Bytes idénticos. Los 12 frames strip/overlay de 006 también conservan su hash. |
| AC-03 | Pasó: cmake --build --preset release; ctest --preset release, 4/4, 9,97 s. ui.async_previews: 160440 checks totales, incluyendo originales 004/006. Revisión de diff frente a snapshot previo y git diff --check correctos. |
| AC-04 | Pasó: microbenchmark con cálculo productivo forzado y mismos inputs, separado de hits estables, debajo. Sin umbral de tiempo en pruebas ni atribución a aplicación completa. |

SHA256 de before.bgra/after.bgra de grids:
`0890DFBA56E4AB5CF488D85B023E8BFA2AA4DBD5C409189F19A66463E8B86652`.
Snapshot strip/overlay conservado:
`64E006D5427E0F17A04BDA1CAED169C845F5AC985457C9E1D2D3EFC12207D88B`.
EXE Release final:
`323087419CE0EF5D1E040A7B85E6D3A7817044B29744625D63C7D9FDF1A4B2CE`.
Baseline de grids generado antes de tocar producción; snapshot de nueve fuentes y
hashes en build/optimization-007/before. No se versionan píxeles ni logs.

## Medición local

Modo `--layout-benchmark` del harness existente, ancho 1601, columnas auto/gap 15.
Cinco muestras tras warmup, cada una 100 llamadas por lado, orden alternado y mediana
por llamada. Datos y texturas sintéticas preconstruidos fuera del intervalo. La base
es CalculateLayout actual forzado a reconstruir (mismas fórmulas previas), no timings
de un EXE anterior. El oráculo independiente solo valida bits: su almacenamiento y
código unificado no serían una referencia justa de tiempo.

| Previews | Página | Recalcular µs | Hit estable µs | Invalidada por revisión µs |
| --- | --- | ---: | ---: | ---: |
| 100 | Library | 0,479 | 0,005 | 0,478 |
| 100 | Waterfall | 0,866 | 0,007 | 0,866 |
| 1000 | Library | 4,646 | 0,005 | 4,652 |
| 1000 | Waterfall | 8,765 | 0,007 | 8,720 |
| 10000 | Library | 57,875 | 0,006 | 57,637 |
| 10000 | Waterfall | 102,981 | 0,007 | 101,130 |

Los hits omiten el algoritmo O(N·columnas) y su asignación temporal colHeights; solo
comprueban clave O(1). Los tiempos pequeños cercanos a resolución del temporizador
no deben leerse como latencia garantizada de un frame. Recalcular después de invalidar
mantiene el trabajo previo; variaciones pequeñas no prueban regresión o mejora global.
Los modos forzados tienen su propia base en CSV: 10000 Library 57,356 y Waterfall
101,100 µs; no se usa una diferencia de muestras para prometer aceleración en mutaciones.

## Límites y continuidad

Sin criterios necesarios fallidos/pendientes en el alcance. Equivalencia en casos
ejecutados, no certificación exhaustiva de interacción nativa, compositor/driver físico,
primera ejecución o carpetas reales. Frames son de grids, no una campaña completa
de páginas/visor modal. No medición nueva de CPU/GPU/RAM en reposo o arranque global.
El dibujo sigue recorriendo previews para filtrar visibilidad; un índice masonry es
otra porción. Revisión global puede invalidar por carpeta inactiva, con una reconstrucción
extra respecto a cache por carpeta, conservando corrección y menor complejidad.

Toda futura mutación directa del modelo/dimensiones/rects debe notificar; contrato
documentado y consumidores actuales auditados. Sin ACK, elección atómica ni cierre.
Library espera valor antiguo 4/placeholder. AR-005/006 y cobertura AR-007, pendientes
de 003 y demás OPT se conservan. Actualizados roadmap, índice, desarrollo, contexto y
seguimiento 003/006. Siguiente recomendado: formatos DirectWrite internos acotados.

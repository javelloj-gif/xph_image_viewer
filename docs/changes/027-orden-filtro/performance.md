# Rendimiento final del lote 026–030

Fecha local: 2026-10-26. Windows 10.0.26300, AMD64 Family 25 Model 33,
RTX 4070 Ti SUPER / driver 616.56. Fuentes: base `36e4248`, más diff local del lote.
Los hashes identifican los ejecutables medidos; no atribuir al HEAD limpio el código nuevo.

- Base SHA256: `25A0FE8581830E54069DC2DE8C97435C0D37E34B7E249B9F7285A36A654F298D`.
- Candidato SHA256: `F286E98F839B516E930EB390A132E1B81352676472E01C452F9B5AB9A2AA94DF`.

## Arranque

Comando: `pwsh -NoProfile -File scripts/run_product_batch_benchmark.ps1 -ReferenceExecutable build/batch-026-baseline/image_viewer.exe -CandidateExecutable build/windows-x64/bin/Release/image_viewer.exe -RunRoot build/batch-026-performance-final -Repetitions 30 -FileCount 1000`.

Mil imágenes copiadas de fixtures: 900 JPEG de ~45 KB y 100 PNG de ~818 KB.
Dos calentamientos y 30 pares por escenario, alternando base/candidato, secuencialmente
sin builds ni otras pruebas durante la medición final. Datos y cachés aislados por EXE.
Defaults: nombre ascendente/Todos; saved: tamaño descendente/PNG. La base ignora la
nueva máscara y conserva su comportamiento anterior. Caché OS no controlada.

Tiempos en ms. Submitted es envío del frame original, no una confirmación del monitor.
Navigation mide disponibilidad de la lista, no el dibujo de todas las miniaturas.

| Escenario | Métrica | Mediana base | Mediana final | Cambio | p95 base | p95 final | Cambio p95 |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: |
| defaults | InvocationToSubmittedMs | 255.47 | 255.67 | +0.08% | 289.13 | 258.92 | -10.45% |
| defaults | EntryToSubmittedMs | 244.50 | 244.60 | +0.04% | 273.06 | 246.88 | -9.59% |
| defaults | NavigationMs | 245.59 | 245.66 | +0.03% | 279.45 | 248.83 | -10.96% |
| saved | InvocationToSubmittedMs | 255.42 | 253.32 | -0.82% | 263.99 | 266.56 | +0.97% |
| saved | EntryToSubmittedMs | 244.56 | 242.39 | -0.89% | 253.37 | 255.64 | +0.90% |
| saved | NavigationMs | 245.98 | 243.64 | -0.95% | 254.74 | 254.73 | -0.00% |


**Cumple** mediana <= +5% y p95 <= +10% en los seis pares de métricas.
Manifiesto, 128 trazas (incluye calentamientos), muestras y resumen locales:
`build/batch-026-performance-final/`. Hubo una comparación anterior satisfactoria,
supersedida al añadir retención de previews ocultos; solo el resultado final certifica
este candidato. No se hicieron capturas ETW ni mediciones tras boot.

## Cambio inicial y reutilización

Harness productivo: `async_preview_tests.exe --view-benchmark build/batch-026-performance/images`,
sin builds/otras pruebas. Se utiliza la misma colección sintética de mil archivos.
Una primera selección y 30 reutilizaciones/filtros por criterio. Los tiempos incluyen
sondeo/entrega del harness y Sleep de Windows: son observaciones de entrega, no CPU puro.
Una ejecución exploratoria concurrente con CTest se excluyó; se usa únicamente
`build/batch-026-visual/view-latency-isolated.csv`.

| SortMode | Primera entrega ms | Mediana reutilización ms | p95 envío UI ms |
| --- | ---: | ---: | ---: |
| 0 | 21.71 | 31.79 | 0.176 |
| 1 | 31.69 | 31.82 | 0.150 |
| 2 | 93.86 | 31.89 | 0.178 |
| 3 | 32.71 | 31.78 | 0.126 |
| 4 | 95.55 | 31.76 | 0.122 |
| 5 | 32.14 | 31.81 | 0.147 |


Las primeras lecturas de modificación/tamaño cuestan ~94–96 ms en el trabajador;
la lista anterior sigue utilizable. Reutilizar devuelve ~32 ms con el sondeo del
harness, con envío UI p95 <0,18 ms. El test modifica un archivo sintético y comprueba
que repetir el criterio conserva las claves, mientras cambiar la revisión las vuelve
a leer. Otro test comprueba la identidad del recurso preview tras ocultar/restaurar tipos.

## Reposo

Comando: `pwsh -NoProfile -File scripts/run_gpu_idle_diagnostics.ps1 -Executable build/windows-x64/bin/Release/image_viewer.exe -RunRoot build/batch-026-idle-final -Seconds 3 -Repetitions 1`.
Una instancia nativa aislada, imagen sintética 2560x1440 y ventana 1280x720.
Tres muestras por estado; la consulta de contadores agrega tiempo, por eso se
informa la duración QPC real. Misma identidad del candidato, cierre correcto y
trace.dropped=0. Artefactos locales en build/batch-026-idle-final.

| Estado estable | Duración s | Draw | Present |
| --- | ---: | ---: | ---: |
| visible | 8.72 | 0 | 0 |
| dock-focus | 7.72 | 0 | 0 |
| minimized | 7.68 | 0 | 0 |
| hidden | 7.67 | 0 | 0 |
| restored | 7.64 | 0 | 0 |


No hay presentaciones continuas durante esos intervalos. Se conserva el sondeo CPU
existente; los sensores del dispositivo no representan exclusivamente este proceso.
No se afirma consumo GPU cero universal ni un límite de potencia.

## Límites

Esto caracteriza el lote en esta colección y entorno. No certifica arranque frío de
Windows, una primera generación de miniaturas de fotografías grandes, todos los
códecs ni UX manual. No se reduce calidad, FPS objetivo ni efectos para pasar las pruebas.

# Resultado: 009 Decoder compartido

[Spec](spec.md), [plan](plan.md), [revisión](review.md). Fecha: 2026-10-22.
Estado: completado en alcance. Base ac4a5da, codex/repository-foundation, trabajo
previo R1/R2/003..008 preservado sin commit. Contrato y excepciones/fallback definidos
antes de editar producción; evidencia local ignorada bajo build/optimization-009 y
build/windows-x64/test-runs/decode-930823acb04f42cfbb5b84def98d4246.

## Implementación

LoadForViewerSync devuelve como máximo un candidato CPU estático o animado. Mantiene
selección por extensión. Comparte decoder tras consulta correcta: <=1 frame reutiliza
frame 0; >1 usa compositor existente. Apertura/consulta/animación inválida o decode
estático reutilizado fallido liberan el intento y vuelven a LoadSync con decoder nuevo.
Una apertura normal, dos intentos en fallback; nunca se retiene el primer decoder
durante la reapertura. No hay caché persistente ni nuevos workers/counters productivos.

LoadSync/LoadAnimatedSync y las APIs async conservan sus contratos mediante helpers
con decoder prestado. App::LoadImage solo sustituye la obtención del resultado: rama
animada y su retorno temprano, GPU, secuenciador, títulos, historial y sincronización
estática posterior se mantienen. Fallo conserva el comportamiento anterior de App.

## Evidencia de aceptación

Entorno Windows x64 10.0.26300, VS2022, Windows SDK 10.0.22621.0, Release y WIC real.
Suite aislada en copia del EXE/user_data, sin App::Initialize, workers, ventana visible
ni datos personales. Fixtures del repositorio solo se leen/copían. GIF/PNG propios y
WebP lossless 4x4 de 140 bytes embebido en fuente; no se requiere Python para tests.

| Criterio | Resultado |
| --- | --- |
| AC-01 | Pasó: comparación de resultados contra decisión previa App con APIs legacy. Ruta/dimensiones/alpha/bytes estáticos y cada frame/delay/loops/duración iguales. PNG con alpha, JPEG, GIF de un frame, GIF sintético de 5 frames, GIF real de 100 frames, WebP estático y pequeño animado, APNG como estático según codec. |
| AC-02 | Pasó: proxy IWICImagingFactory2/decoder real cuenta invocaciones CreateDecoderFromFilename y comprueba lifetimes. PNG/GIF de un frame/misnamed JPEG.png: 2→1; JPEG y GIF válido/parcial: 1→1. Primer open/count/GetFrame fallidos y animación vacía: 2→2 con fallback. Reporte de count cero: estático válido 2→1. Sin decoders solapados ni retenidos al terminar. |
| AC-03 | Pasó: 294 checks en loader.viewer_decode, incluida composición independiente GIF con offsets, transparencia, disposal 2/3 y delays (50/100/70/80/90 ms, total 390). Fallos de todos/últimos frames, corrupto/inexistente y API estática escalada. Suite IPC continúa exigiendo aplicación de bitmap en App. |
| AC-04 | Pasó: cmake --preset windows-x64; cmake --build --preset release; ctest --preset release, 5/5 en 12,82 s. Se conservan 161701 checks del harness previo. Revisión específica/comentarios y git diff --check correctos. Medición local debajo, sin umbrales temporales ni mejora global atribuida. |

Los cuerpos de frame 0 y composición de animación son texto idéntico tras extracción
respecto al snapshot (lectura UTF-8 y newlines normalizadas). Hashes SHA256 de cuerpos:
estático `4747E03FCD66734B15C271B51ADCCEE215C1049D1D4F394D2ADE6CC7B8B6076F`;
animación `020025C2750CB01C736705AB3272559C7762D6F3B9C159646F9E5F6894353347`.
Esto complementa equivalencia de ejecución; no sustituye las comparaciones de bytes.

EXE Release final: `E4452CA73195CB41A9E0DC3BBA85DD6883615CC26F606C3F7C031E93DA93CA26`.
Snapshot de seis fuentes previas y hashes bajo build/optimization-009/before; tests
nuevos/scripts/artefactos tienen responsabilidades separadas del producto.

## Codecs y límites observados

| Caso opcional | WIC frames | Estimación de frames almacenados | Resultado y llamadas viejo/nuevo |
| --- | ---: | ---: | --- |
| apng.apng / apng_png.png | 1 cada uno | 160000 bytes cada uno | Estático equivalente, 2/1; no prueba de APNG multiframe |
| static.webp | 1 | 3080192 bytes | Estático equivalente, 2/1 |
| animated.webp grande | 88 | 108134400 bytes | Omitido antes de decode completo por presupuesto |
| rubik.gif | 100 | 27000000 bytes | Animado equivalente, 1/1 |
| tiny.webp propio | 2 | 128 bytes | Animado equivalente, 1/1 |

Presupuesto 32 MiB de estimación por conjunto de frames, no cota de pico de proceso:
referencia/candidata, canvas, decoder y buffers añaden memoria. No instalar codecs.
Equivalencia no corrige ni certifica limitaciones preexistentes de composición WIC
GIF-oriented para todos los APNG/WebP. No se descarta trabajo del producto por este
presupuesto: el límite se aplica exclusivamente a la campaña opcional de tests.

## Medición local

En la suite aislada, PNG 64x32 con alpha generado por WIC, seis rondas alternadas,
una descartada, veinte cargas CPU por lado/muestra; mediana por llamada del resto:
referencia 294,600 µs, candidato 154,890 µs. Consumo de bytes fuera de optimización
vacía, sin umbral temporal en CTest. Mismo proceso/factory/archivo, cachés de sistema
calientes, proxy de conteo incluido. No mide App, escaneo, GPU, Present ni primera
ejecución. La prueba dura de reducción es 2→1 invocaciones WIC; no son contadores de
syscalls de archivos. No afirmar aceleración de arranque o menor RAM/reposo global.

## Límites y continuidad

Sin criterios necesarios pendientes en alcance. No recorrido visual nativo ni primera
ejecución real nueva; GIF/App completo/animación GPU no se presentan visualmente en
esta suite. Se preservan bytes CPU y código de upload/secuenciación; IPC sí recorre
Window/App con WARP para estáticos. Recuperación usa decoder fresco; ante reuse fallido
puede realizar un intento estático adicional, por lo que el beneficio no es universal
ni una garantía de latencia ante corrupción/codecs defectuosos.

Sin cambios a Renderer, calidad, FPS, asociaciones, IPC, elección/ACK/cierre ni Library.
Resto de pendientes 003/AR-005/006/007, AR-008 y límites 008 conservados. Actualizados
roadmap/índice/desarrollo/contexto/seguimiento 003/008. Siguiente: especificar arranque
secundario mínimo OPT-005 con prerrequisitos y errores de 005 intactos. Sin commits.

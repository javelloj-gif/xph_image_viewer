# Revisión: 009 Decoder compartido

Fecha: 2026-10-22. Habilidad lenteuv-architecture-review, sesión actual sin delegación.
Git ac4a5da y diff previo preservados; snapshot local como comparación específica.
Leídos AGENTS/skill/referencias/workflow/contexto/roadmap y documentos 008.

## Cobertura

ImageLoader CreateDecoder/DecodeImage/DecodeFrame/DecodeAnimatedImage/composición,
LoadSync/LoadAnimatedSync y consumidores async; App::LoadImage ambas ramas y efecto
de retorno temprano; CMake, dispatch del harness, script aislado y proxies WIC.
No auditoría de codecs, composición nueva, renderer, tags, cierre o arranque global.

## Hallazgos y resolución

- DC-001, riesgo confirmado de fallback con decoder parcialmente consumido: no asumir
  que todo codec queda utilizable tras error. LoadForViewerSync (image_loader.cpp:240)
  libera el intento completo antes de LoadSync fresco. Se permite esa segunda apertura
  en fallos; tests Count/FrameZero/Open/AllFrames prueban recuperación y cero solapamiento.
- DC-002, riesgo de cambiar animación parcial o efectos App. IsValid animado sigue
  siendo frames no vacíos, no obliga a conservar >1 tras frames fallidos. Test de
  LaterFrames mantiene un frame válido y ningún fallback; App conserva retorno temprano,
  secuenciador/historial/título y sincronización estática en su rama original.
- Extracción: cuerpos estático y compositor idénticos (hash normalizado en resultado).
  Callers originales conservan lectura de count y semántica; helpers toman decoder
  prestado válido, caller conserva ComPtr. Ningún objeto WIC escapa al resultado CPU.
- Tests: proxy COM usa factory/frames/converter reales, sin estado/hook nuevo de
  producto. GIF independiente prueba disposal/transparencia/delays; opcionales limitan
  estimación de frames y registran capacidades reales. El WebP grande omitido no se
  presenta como validado: se añade uno diminuto que el codec expone con dos frames.

Sin hallazgos accionables pendientes en alcance. Métodos/struct/clases nuevos y
modificados documentan responsabilidad/propiedad/hilo; familias COM delegadas comparten
comentario de contrato. Runtime extra se limita al resultado y coordinación de carga,
sin threads/cachés persistentes/counters. Error de reuse puede añadir intento de decode;
se declara y no se vende como mejora universal.

Release/CTest 5/5, 294 checks propios más suite previa y aplicación estática IPC con
bitmap; git diff --check pasó. Evidencia CPU no certifica todos los codecs/presentación
nativa ni primera ejecución/performance global. Sin nuevos comportamientos persistidos
ni extensión accidental a AR-005/cancelación o a IPC/cierre/Library.

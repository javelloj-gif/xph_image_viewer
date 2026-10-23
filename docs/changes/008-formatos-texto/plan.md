# Plan: 008 Formatos de texto

[Spec](spec.md). Estado: completado en alcance. Fecha: 2026-10-22.

- [x] Revisar Git/habilidad/AGENTS/guías/007, consumidores y ciclo de vida de factory.
- [x] Guardar snapshot local y baseline de texto WARP con renderer previo. AC-02.
- [x] Implementar helper privado, clave y FIFO acotados; limpiar por lifecycle/factory;
  mantener fábrica pública y layouts independientes. AC-01.
- [x] Comprobar aislamiento/propiedades/medidas/capacidad/fallos/factory; comparar
  píxeles nuevos y snapshots 006/007. AC-01/02.
- [x] Benchmark acotado, Release/CTest, revisión, resultado, guías y roadmap. AC-03/04.

Fuentes previstas: renderer.cpp/.h y harness async_preview_tests.cpp. Sin otro
componente productivo. Clave completa para propiedades efectivas actuales; API
pública conserva weight/style. Formatos internos solo para uso inmediato en UI,
retornados con ComPtr local para que expulsión no invalide una referencia en uso.

Baseline antes de modificar producción. Artefactos ignorados bajo build/optimization-008.
Oráculo usa CreateTextFormat público con configuración previa; compara propiedades
completas y medidas bit a bit. Benchmark mismo proceso Release, datos preconstruidos,
orden alternado y mediana tras warmup; sin umbral temporal ni aplicación/IO reales.
WARP offscreen con métodos reales. Recuperar solo diff 008 desde snapshot local si
falla equivalencia, sin reset/restauración global. Ningún dato personal o fixture escrito.

Resultado: [evidencia](result.md) y [revisión](review.md). Release/CTest 4/4, 161701
checks totales; 12 frames iguales al control fresco y 12/24 de 006/007 conservados.
Seguimiento: [009](../009-decoder-compartido/result.md) implementa decoder compartido
con composición y fallback preservados. Siguiente recomendado: especificar arranque
secundario mínimo OPT-005, conservando prerrequisitos de UI/IPC y errores.
Última medida del overlay e índice masonry permanecen como candidatos separados;
no extender 008 a ellos ni a inicialización secundaria, uploads o scheduler.

Control de validación: captura inicial de texto difirió en 48 bytes respecto a la
candidata. Se añadió oráculo con formato fresco en el mismo ejecutable y se recompiló
renderer.cpp original desde snapshot (confirmando compilación, no solo timestamps);
ambos produjeron el hash candidato. Fuente candidata restaurada por hash en finally.
No se confirma causa de la variación inicial; mantener evidencia y exigir igualdad
controlada, sin tolerancia de píxeles ni modificación visual de producto.

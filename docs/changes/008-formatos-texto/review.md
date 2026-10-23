# Revisión: 008 Formatos de texto

Fecha: 2026-10-22. Habilidad lenteuv-architecture-review, revisión en sesión actual.
Base ac4a5da más diff previo preservado, alcance específico frente a snapshot 007.
Leídos Git/AGENTS/workflow/contexto/roadmap/007 y referencias de la habilidad.

## Cobertura

renderer.cpp/.h: DrawText/MeasureText/CreateTextFormat, creación/descarte de recursos
y consumidor de factory; harness async_preview_tests.cpp. Búsqueda global de llamadas
CreateTextFormat y escrituras m_dwriteFactory: producción solo en Renderer.
No auditoría global de páginas, decoder, IPC, cierre, scheduler o rasterizador del SO.

## Hallazgos y resolución

- TF-001, riesgo confirmado de contaminación si se reutilizara fábrica mutable pública.
  Renderer::GetInternalTextFormat (renderer.cpp:503) queda privado, solo DrawText y
  MeasureText lo consumen. Public CreateTextFormat no cambia y no devuelve el cache.
  Propiedades variables en clave; constantes del contrato enumeradas. Setter de
  alineación solo al crear, formatos privados inmutables después. API pública y
  separación entre alineaciones probadas con objetos simultáneos.
- TF-002, riesgo de retención o identidad obsoleta. Capacidad 32/FIFO y familias hasta
  128, bypass sin truncar. Referencia de factory evita direcciones reutilizadas; clear
  en CreateDeviceResources:37 y DiscardDeviceResources:583, más guard por identidad.
  Reemplazo de factory aislada y descarte ejercitados. La retención CPU es un coste
  acotado declarado; no se presenta como reducción de RAM.
- TF-003, limitación confirmada de baseline visual: 48 bytes iniciales distintos.
  No exclusiva del camino cacheado: control renderer.cpp original recompilado y
  oráculo fresco producen los mismos bytes que la candidata. Cero tolerancia en
  contraste controlado. Causa inicial sin confirmar, sin ocultarla ni retocar visuales.
  [Resultado](result.md) conserva hashes, método, alcance del control y límites.

Sin hallazgos de producto accionables pendientes en este alcance. No se promueve la
variación inicial a causa/defecto de caché. Comentarios de responsabilidad/hilo/propiedad
en métodos y struct nuevos/modificados. ComPtr local mantiene uso durante una llamada;
no locks ni uso por workers. No cambian layouts/textos/medidas persistidas o formatos.

Release/CTest 4/4, 161701 checks, texto controlado idéntico y snapshots 006/007
conservados. git diff --check pasó. Mediciones de formato y medida separadas, con
retención y límites; no certifican toda UX/driver ni ahorro global. Si aparece una
diferencia nativa reproducible exclusiva del cambio, reabrir la equivalencia.

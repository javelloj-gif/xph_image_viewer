# 019 — Composición compartida

Autorizado por el propietario el 2026-10-26, después de 018 (44/57,2 DIP).

- AC-01: Waterfall modal usa la misma imagen, tira, metadatos, transformaciones y entradas de Viewer, sin segundo loader ni copia del estado.
- AC-02: overlays, controles y gestos consumen antes que el clic exterior. Escape/clic exterior vuelven a la misma carpeta y desplazamiento.
- AC-03: navegación, animación, multiselección y cancelación mantienen contratos UI/loader y caché 100/200; sin cambios persistidos.

Excluidos dock (020), gestos de compactación (021) y marca (022). Evidencia nueva solo del candidato; no recompilar/ejecutar la base histórica.

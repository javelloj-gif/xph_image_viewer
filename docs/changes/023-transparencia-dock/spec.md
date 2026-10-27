# 023: Transparencia de la leyenda superior

Alcance autorizado el 2026-10-26: corregir la leyenda emergente de comandos del
dock, similar a la barra de metadatos inferior. DrawDock usaba negro al 95%;
DrawOverlayInfo usa negro al 60% y borde blanco al 15%, modulados por su fade.

- AC-01: fondo negro al 60% y borde blanco al 15%, multiplicados por la opacidad
  del dock. Conservar texto, geometría, comandos y aparición.
- AC-02: Release, regresiones y revisión de snapshot productivo WARP.

Fuera del alcance: rediseñar el panel de iconos, cambiar el tooltip del nombre
de archivo o implementar optimizaciones GPU (análisis separado en 024).
La aceptación visual nativa del propietario se distingue de estas comprobaciones.

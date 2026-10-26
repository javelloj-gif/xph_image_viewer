# Revisión 020

Habilidad local lenteuv-architecture-review. HEAD 2d19d3d + árbol 018–022, revisión acotada del dock respecto al estado inicial guardado en build/session-019/before.

Cobertura: App routing/UpdateDock/DrawDock/geometría/foco/InvokeDock, retirada de radial, menús Waterfall/Board, Window WM_GETOBJECT, DockAccessibility, enlaces CMake y tests productivos. Glifos originales; sin dependencia gráfica externa. Recursos y callbacks conservan UI/STA; el proveedor accesible posee solo estado débil.

AR-020-001 (defecto medio, confirmado por recorrido): tooltip de foco anterior persistía tras elegir otro icono con ratón. Corregido al retirar foco al pulsar y preservar hover sobre tooltip; recorrido posterior observa Rotate 90° y Settings correctos. Ver result.md para hashes.

Sin hallazgos accionables restantes en cobertura. No auditoría completa de Board/renderer/lectores de pantalla; no rendimiento inferido de primitivas ni de build.


## Revisión de corrección visual

Habilidad local aplicada al diff guardado en build/session-020-visual-correction/before:
App::GetDockBounds/GetDockTooltipRect/DrawDock, DockIcons/GlyphWriter/WriteGlyph,
Viewer::DrawOverlayInfo, ThumbnailStrip::GetThumbnailRect, CMake y regresiones.

AR-020-002 (defecto visual medio, reporte del propietario y código confirmados):
la primera implementación reducía la referencia a símbolos genéricos y omitía
fotos, cuadro, etiqueta, embudo y dientes. Se sustituye por paths detallados y
estilo redondeado, con centro superior por instrucción nueva. Cache por factory,
propiedad RAII y transform restaurado comprobados en WARP; eventos/imagen no
son responsabilidad del helper. Sin nuevos contratos persistidos ni dependencias
externas. Métodos/clases nuevos y modificados tienen responsabilidad documentada.

Sin hallazgos accionables adicionales en este diff acotado. El límite de capturas
intermedias del modal queda en result.md; no se presenta como renderer corregido
ni se abre una campaña de diagnóstico por esta revisión visual.

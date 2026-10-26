# Revisión 021

Habilidad local lenteuv-architecture-review. Cobertura: Viewer::IsExteriorPress/IsPointOverImage/OnMouseDown/Up/DoubleClick/CancelPointerGesture; Window::HitTest/HandleMessage y callback App; Waterfall::EnlargeActiveItemRect y llamadores de dibujo/hits. Misma inversa y límites de bitmap; sin lectura de alfa/GPU ni reenvío a terceros.

AR-021-001 (riesgo medio, código/indicio de recorrido): un drag pendiente durante cambio de geometría podía prolongarse al alternar fullscreen. CancelPointerGesture antes de alternar y regresión de flag lo excluyen; no se afirma diagnóstico causal universal de cada desplazamiento visual.

AR-021-002 (defecto medio, ejecución): caption nativo podía interceptar la franja superior del visor compacto antes del dock/exterior. Se añadió onClientHitTest tras bordes resize y antes de HTCAPTION. Test con HWND nativo oculto conserva HTLEFT/HTTOP y permite HTCLIENT; recorrido final valida el fondo compacto. Grid/Board conservan caption.

Responsabilidades documentadas en declaraciones. Sin cambios a formatos/hilos/caché. Fuera: múltiples monitores físicos, toda matriz de gestos y redimensionado; result.md conserva límites.

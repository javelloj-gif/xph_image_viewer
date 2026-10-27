# Docks por modo

Criterios: **AC-05**. [Acuerdo completo](../product-batch-20261026.md).

Seis posiciones con comandos independientes del índice: Viewer muestra Waterfall/Board/Rotate/Tags/Sort & Filter/Settings; Waterfall muestra Viewer primero y Board segundo; Board muestra Viewer y Waterfall, con Rotate y Sort & Filter bloqueados. Tags en Board depende de imágenes seleccionadas; Rotate/Tags en la cuadrícula requieren el visor modal.

Dibujo, tooltips, invocación y MSAA consultan el mismo descriptor. El dock precede a los gestos del contenido; no se dibuja encima de modales, pero conserva su retención. Mantiene apariencia y temporizadores. El cálculo de columnas también respeta la altura disponible. Tres propuestas vectoriales en viewer-icons.svg; se integra foto individual con referencia 32, trazo 1,65 y geometría D2D cacheada.

Conservar atribuciones, formatos y rutas. UI/GPU permanecen en el hilo principal.

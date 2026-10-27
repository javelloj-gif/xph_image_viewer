# Desplazamiento de metadatos

Criterios: **AC-06**. [Acuerdo completo](../product-batch-20261026.md).

La burbuja interpola su elevación entre el borde inferior y la posición sobre la tira. Mantiene opacidad durante el movimiento y después conserva el fade por inactividad. Con effects desactivado aplica la posición final directamente. Dibujo y hit testing usan la misma geometría; no se invalida una vez estable. Tooltip, anclaje horizontal, fuente y composición Viewer/Waterfall se conservan.

Conservar atribuciones, formatos y rutas. UI/GPU permanecen en el hilo principal.

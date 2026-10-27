# Ruso y español

Criterios: **AC-07**. [Acuerdo completo](../product-batch-20261026.md).

Completar las opciones activas en ruso y español usando I18nEngine y PO existentes, incluyendo menús especializados, dock, modal de orden/filtro, Settings, Tags, búsqueda, bandeja, mensajes y accesibilidad. El catálogo español heredado ya existía: se completa y se hace accesible con Español (ES), sin eliminar sus entradas anteriores. Cambio inmediato y persistencia.

No traducir tags, nombres de archivo, categorías aportadas por el usuario ni contenido guardado. Pestañas/botones de idiomas se ajustan al ancho. El modal de metadatos conserva su composición en un viewport ajustado al cliente, con coordenadas de entrada equivalentes y restauración de transform; los campos conservan su contenido.

Conservar atribuciones, formatos y rutas. UI/GPU permanecen en el hilo principal.

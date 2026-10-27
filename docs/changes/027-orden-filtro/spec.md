# Orden y filtro

Criterios: **AC-03 / AC-04**. [Acuerdo completo](../product-batch-20261026.md).

Modal con borrador, Aplicar/Cancelar, teclado y layout compartido para dibujo/hit testing. Preferencias globales persistidas: seis valores originales de sort_mode y nueva máscara image_filter_mask (511 inicial). Familias: JPEG/JFIF, PNG/APNG, GIF, BMP, TIFF, WebP, AVIF, HEIF/HEIC y Otros. Cero familias es válido.

Nombre sin distinguir mayúsculas; fecha de modificación y bytes. Desempate por nombre/ruta. Inventario completo separado de la vista. Conservar original excluido con índice -1; siguiente/anterior seleccionan primer/último resultado. Carpetas inactivas se actualizan al seleccionarlas.

FolderScanService calcula claves/órdenes CPU y los reutiliza por inventario/revisión; fecha/tamaño solo se leen al necesitarse, fuera del comparador y de UI. No se vuelve a enumerar si cambian preferencias durante un escaneo completado. Generaciones y leases impiden aplicar trabajo antiguo. La UI reconcilia previews por ruta y conserva miniaturas preparadas de tipos ocultos, acotadas al inventario. Borrado y escrituras conocidas invalidan atributos. No se añade un watcher del sistema de archivos.

Conservar atribuciones, formatos y rutas. UI/GPU permanecen en el hilo principal.

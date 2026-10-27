# Revisión de arquitectura del lote 026–030

Revisión acotada del código modificado y consumidores activos: App/enrutamiento,
FolderDataManager/FolderScanService, Board/serialización de entrada, dock y MSAA,
miniaturas, Viewer/metadatos, overlays, motor PO y menús/bandeja. Se aplicaron los
criterios de la habilidad local lenteuv-architecture-review. No es una auditoría integral.

## Hallazgos abordados

- **AR-026-01, defecto de integración:** recargar !board.kbrd en cada entrada
  sobrescribiría envíos y cambios en memoria. Preparación única separada de navegación;
  regresión con nota persistida, envío duplicado y varias entradas/salidas.
- **AR-026-02, riesgo de respuesta:** el cambio manual anterior consultaba atributos
  en UI. Una proyección worker, claves cacheadas y comparador común para consumidores
  síncronos/async sustituyen ese camino. Cancelación y revisión de mutación impiden
  publicaciones antiguas. CPU/GPU siguen sus contratos existentes.
- **AR-026-03, defecto visual observado en WARP:** dock detrás del texto de modales
  y altura excesiva en clientes estrechos con DPI alto. Supresión de dibujo bajo
  overlays y columnas limitadas también por altura; pruebas de geometría y nuevas capturas.
- **AR-026-04, coste evitable de reutilización:** descartar previews de tipos ocultos
  obligaría a pedirlos otra vez. Conservar preparados por ruta, cancelar leases ocultos
  y borrar su estado al eliminar el archivo. Test de ida/vuelta conserva el mismo recurso.

## Contratos revisados

Sin cambios en enums Page/SortMode, rutas de datos ni KTAG/KBRD. App conserva la
coordinación; no hay nueva jerarquía de páginas ni otro loader. Modal solo posee
borrador; worker solo tiene datos CPU. Los resultados se publican en ProcessPending
y las texturas siguen entrando por ProcessPendingTextures en UI. Nombres MSAA,
acciones y estados usan el descriptor que dibuja el dock. Comentarios de métodos
modificados describen responsabilidad e hilos donde corresponde.

Se conserva contenido del usuario en tags/notas/metadatos. Las traducciones no
transforman documentos persistidos. Las pruebas escriben solamente en staging propio.

## Límites

No se revisaron escritores/códecs completos, recuperación del dispositivo, instalador
ni todos los flujos de Board. No se afirma mejora GPU ni ausencia universal de carreras
por inspección. IPC concurrente mantiene el seguimiento histórico: una repetición falló
y otras pasaron; no hay cambio de transporte en este lote.

# Revisión AR-005 / 014

2026-10-23. Base 6f84dc5, rama codex/repository-foundation, árbol inicialmente
limpio. Habilidad lenteuv-architecture-review y referencias, misma sesión sin
delegación. Revisión del diff productivo completo y seguimiento de consumidores.

## Cobertura

- common/image_request: lease fuerte solo en modelo UI y snapshot débil/stop_token
  en worker. Los callbacks conservan identidad de ruta, nunca el lease que cancelan.
- loader: admisión, cola, poda, workers, cierre y decodificación estática/animada;
  entrega UI y distinción entre predicado 011 (solo upload) y cancelación 014.
- Tira, previews Library/Waterfall y mutadores del gestor; hover, sort/delete,
  selección y salida de página. Board: identidad por ítem, duplicados, eliminación,
  reemplazo por serializer y salida del modo. KBRD sigue serializando campos
  explícitos; los leases/identidades son transitorios y no entran al archivo.
- BoardTagScanner completo: start/cancel/join, progreso atómico, checks por
  entrada/archivo/tag/chunk y publicación con mutex contra cancelación; temporal
  CREATE_NEW propio y reemplazo al completar, con limpieza ante fallo/cancelación.
- AsyncDirectoryScanner completo: límites de lectura, publicación/callbacks,
  marcador thread_local de callbacks para evitar join circular en workers anidados,
  reinicio en hilo dueño y lectura de dirty bajo el mutex de caché.
- Cliente IPC completo y contrato con servidor 012/App/main: overlapped WriteFile,
  presupuesto compartido, cancelación observada, errores compatibles y sin flush
  síncrono. Servidor/wire/ACK/elección no se rediseñan.
- App: productores unidos antes de destruir HWND en WM_CLOSE; destructor y WM_QUIT
  siguen garantizando join antes de dependencias también en inicialización parcial.
- CMake, wrappers, regresiones ampliadas, smoke nativo y test de EXE empaquetado.

## Correcciones de revisión

La revisión detectó y corrigió dentro del diff: acceso privado de la tira mediante
API CancelPendingLoads; sobrecarga animated con argumentos propios; limpieza de
temporal que nunca borra un archivo cuyo CREATE_NEW falló; sort/delete que deben
cancelar hover; lectores que prefieren stream en el proxy WIC; cancelación de
scanner desde su callback anidado que no debe unir al manager que espera ese worker.
También se evita construir colas temporales al podar un loader ya vacío en reposo.

Los comentarios de clases/métodos modificados describen propiedad, hilos, efectos y
límites. No se detectan hallazgos bloqueantes restantes en este alcance. No se
afirma auditoría integral del renderer, escritores, shell o persistencia KBRD.

## Contrato final y límites comprobados

La cancelación es cooperativa. Una llamada WIC/E/S ya admitida puede terminar antes
del siguiente check. No se destruyen referencias mientras la usa el worker ni se
interrumpe un hilo. CancelSearch notifica finished una vez, incluso cancelado;
al volver el join del dueño no quedan callbacks activos. Desde callbacks solo se
solicita cancelación; no se destruye/reinicia el scanner allí. No se admiten
excepciones de callbacks. CancelScan tampoco publica un catálogo parcial.

El presupuesto IPC se aplica a conexión y espera de escritura; se conserva error
busy/missing de 005 si expira conectando y ERROR_TIMEOUT para escritura pendiente.
CancelIoEx se observa antes de liberar almacenamiento; retorno sujeto a completar
esa cancelación de kernel, no a un flush sin límite. Forwarded sigue sin ser ACK.
Referencia primaria: [Named Pipe Client](https://learn.microsoft.com/en-us/windows/win32/ipc/named-pipe-client)
y [overlapped pipe I/O](https://learn.microsoft.com/en-us/windows/win32/ipc/synchronous-and-overlapped-input-and-output).

No se promete un plazo universal para un códec/controlador que no retorna. Esta
frontera se caracteriza con llamada proxy retenida y muestras reales; no queda
como una implementación diferida de AR-005. No se introdujo aislamiento de códecs
en procesos ni se cambiaron cargas síncronas del visor/lectura de un archivo.
No hay mejora global de rendimiento afirmada; la poda añade un recorrido de cola
cuando hay tareas y los leases añaden una asignación pequeña por solicitud.

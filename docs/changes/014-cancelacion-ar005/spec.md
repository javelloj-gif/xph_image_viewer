# 014: Completar AR-005

Fecha: 2026-10-23. Autorización: implementar los faltantes de AR-005, cerrar sus
pendientes en roadmap y entregar ejecutable temporal listo para prueba funcional.
Base de entrada: 6f84dc5, árbol limpio, codex/repository-foundation. Sin delegación.

## Contratos y aceptación

- AC-01: cancelación independiente por consumidor en tira, previews compartidos,
  hover Library/Waterfall y texturas Board. Al retirar/reemplazar un elemento o
  cambiar carpeta/página, retirar tareas obsoletas sin decodificarlas ni invocar
  sus callbacks/uploads. Conservar solicitudes retenidas, identidad y ordenación;
  permitir nueva solicitud para la misma ruta y preservar el resto de consumidores.
- AC-02: comprobar cancelación antes/después de etapas WIC y entre frames/filas
  de composición. Shutdown solicita parada también a tareas activas. Caracterizar
  llamada WIC activa con bloqueo controlado y muestras reales grandes/animadas/fallo:
  se espera el retorno de esa llamada y se omiten las siguientes etapas/resultados.
  WIC no ofrece interrupción universal de códecs; no forzar hilos, soltar referencias
  en uso ni prometer un plazo universal frente a un códec/E/S que no retorna.
- AC-03: Board cancela cooperativamente enumeración, lectura, preparación de catálogo
  y publicación; no publica un catálogo parcial al cancelar. Progreso compartido
  sin carreras. Búsqueda de tags deja de publicar nuevos matches/estado tras cancelar
  y no inicia más operaciones; join antes de destruir dependencias, reinicio y cierre
  repetido. Caracterizar espera de E/S/WIC ya iniciada, sin cancelación forzada.
- AC-04: cliente IPC usa un presupuesto total de conexión/escritura (2000 ms por
  defecto), escritura overlapped cancelable, error explícito de timeout y sin flush
  síncrono. Esperar completion de cancelación antes de liberar buffer/OVERLAPPED.
  Preservar wire UTF-8, estados 005, ausencia de ACK/elección y aplicación exacta.
  Probar receptor conectado sin leer, desconexión, recuperación y timeout de conexión.
- AC-05: regresiones aisladas activas en Release, CTest completo y smoke nativo de
  arranque/cuatro modos/cierre con trabajo; código y comentarios revisados. Metadatos
  y catálogos solo en copias; no modificar fixtures ni endpoint/instancia personales.
- AC-06: Release portable desde staging nuevo con runtime/locales/defaults, extraer
  EXE temporal listo para abrir, comprobar hashes, contenido y arranque real. Registrar
  protocolo para el usuario, resultado, límites y roadmap. Guardar cambios en Git.

No retirar Library, alterar formatos/enums/gestos, introducir ACK ni rediseñar el
renderer. El predicado de vigencia 011 conserva callback con resultado CPU cuando
solo rechaza upload; la cancelación explícita de esta entrega suprime ambos.
El reporte funcional de la base no se atribuye automáticamente al nuevo binario.

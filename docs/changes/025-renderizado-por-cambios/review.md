# Revisión acotada 025

Revisión en sesión principal con lenteuv-architecture-review, sobre 6e66535 y
cambios locales preservados 023/024. Cobertura: App Run/RenderFrame/diagnósticos,
Window invalidación/mensajes, loader entrega, tira Update y sus declaraciones,
regresiones añadidas y recolector nativo. No auditoría integral de todos los
decoders, Board, persistencia, driver o recuperación de dispositivo.

Invariantes revisados:

- El flag atómico comunica necesidad de dibujar, no publica datos del worker:
  las entregas conservan mutex y ejecución UI. Solo UI consume el flag.
- El intercambio del flag conserva nuevas solicitudes concurrentes; validar la
  región nativa evita un WM_PAINT redundante por la invalidación ya dibujada.
  Oculto/minimizado no consume el flag. Show/Size/Paint invalidan al restaurar.
- Se mantienen ProcessPending y temporizadores sin Draw/Present; no se introducen
  esperas infinitas ni cambios de propiedad/hilo. Los uploads siguen en UI,
  incluidos resultados recibidos oculto; no se prometen cero uploads durante carga.
- La última muestra de una propiedad animada invalida aunque Update retire la
  animación; los cambios de opacidad/scroll de tira también invalidan.
- Las entregas de texturas invalidan conservadoramente al drenar un lote, incluso
  si todos sus elementos fueron descartados. Puede producir un frame extra por
  lote cancelado, no una presentación continua sin trabajo nuevo.
- El progreso animado visible conserva fluidez; la reproducción finita completa
  y la reproducción de imagen detrás de otra página no mantienen frames.
- Los diagnósticos de salida/estadísticas se siguen consultando en ticks sin
  dibujo. Los eventos Draw/Present son optativos y describen llamadas CPU, no
  duración GPU ni prueba universal de visibilidad.

Hallazgo de revisión corregido antes de cerrar: invalidar solo al avanzar el frame
de una animación reducía la cadencia de su barra de progreso. Se conservó la
actualización entre frames del host Viewer, con regresiones de progreso/completado.

Limitaciones aceptadas para esta porción: sondeo CPU hasta cada 16 ms; cuotas de
upload y scheduler de notificaciones/vencimientos quedan fuera. No se cambian
calidad, formato, gestos, datos ni política de reproducción por minimización.
El reloj lógico sigue el dt limitado existente, sin nueva garantía de precisión
de reproducción tras bloqueos largos. Resultados medidos en result.md.

# 009: Decoder compartido para carga del visor

Fecha: 2026-10-22. Estado: completado en alcance; [evidencia](result.md). Base ac4a5da y trabajo previo sin
commit preservado. Autorizado continuar optimizaciones; origen OPT-004 de 003.

## Contrato previo y alcance

Nueva entrada síncrona CPU del loader para el visor, con resultado estático o animado
(nunca ambos). Conserva política de extensión IsAnimatedFormat. APIs LoadSync,
LoadAnimatedSync y cargas async mantienen resultados y contratos actuales.

En candidatos animados, abrir una vez y consultar cantidad de frames. Con consulta
correcta y <=1, decodificar frame 0 usando ese decoder. Con >1, ejecutar composición
actual sin cambiar un solo criterio de disposal, transparencia, offsets, delays,
loops, frames fallidos o aceptación de animación parcialmente válida.

Si apertura/consulta/animación no permiten resultado válido, liberar ese decoder y
usar carga estática con decoder nuevo como hacía App. Si falla la decodificación
estática reutilizada, liberar también y reabrir para preservar fallback frente a
codecs con estado. Por tanto: una apertura en caminos normales; dos intentos permitidos
en fallos/fallback. No prometer decoder único incondicional o reintentar sin límite.

App consume resultado conservando ambas ramas existentes: animación válida sube
frames, configura secuenciador/historial/título y retorna antes de sincronizar carpetas;
estático conserva sincronización posterior. Fallo conserva imagen previa y resto de
efectos actuales. Sin migrar a async, cambiar orden de inicio o modificar GPU/render/UI.

No cambios de codecs admitidos, composición APNG/WebP, metadata ni validación de
formatos como efecto secundario. Equivalencia con comportamiento actual no certifica
que toda animación de terceros esté bien soportada por WIC/compositor preexistentes.
Sin ACK/elección atómica/cierre/Library/enums/persistencia.

## Aceptación

- AC-01: salida CPU equivalente al camino previo (App: intento animado + estático),
  ruta, dimensiones, alpha, bytes PBGRA, cantidad/orden de frames, delays, loops y
  duración. PNG/JPEG/GIF nativos obligatorios; APNG/WebP según codec con límites explícitos.
- AC-02: contar invocaciones reales CreateDecoderFromFilename mediante proxy WIC de
  tests, sin hooks/counters productivos. PNG/otro candidato estático pasa de 2 a 1;
  animación válida/noncandidato conservan 1. Apertura fallida, consulta fallida,
  animación vacía y reuse fallido conservan fallback fresco y error final.
- AC-03: GIF sintético con transparencia, offsets, disposal 2/3 y delays; fallos
  inyectados de frames (vacío/parcial) y consulta; corrupto/inexistente; APIs antiguas
  conservadas y App revisado sin efectos nuevos. CTest IPC sigue aplicando bitmap.
- AC-04: Release/CTest completo, revisión, documentación/roadmap. Datos aislados,
  fixtures solo lectura/copias. Medición local opcional de carga completa CPU con
  referencia previa, sin umbral frágil ni mejora global de arranque/recursos afirmada.

Si bytes o metadatos difieren, corregir/retirar cambio antes de aceptar; no degradar
calidad para pasar. No campaña visual nativa ni primera ejecución real en esta porción.

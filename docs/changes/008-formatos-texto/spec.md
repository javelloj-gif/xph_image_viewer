# 008: Formatos DirectWrite internos acotados

Fecha: 2026-10-22. Estado: completado en alcance; [evidencia](result.md). Autorizado continuar optimizaciones
conservando UX/presentación. Base ac4a5da y trabajo R1/R2/003/004/005/006/007 sin commit.
Origen: parte de [OPT-003](../003-diagnostico-rendimiento/source-optimization-review.md).

## Contrato antes de implementar

DrawText y MeasureText reutilizan formatos privados, configurados una vez y nunca
entregados mediante API pública. CreateTextFormat sigue siendo fábrica mutable
independiente en cada llamada, con los mismos parámetros/errores. No cachear layouts,
strings ni medidas; sustitución de espacios por NBSP y límites 10000x1000 intactos.

Clave efectiva: familia exacta, tamaño float y alineaciones efectivas horizontal/
vertical; factory propietaria. Colección null, peso regular, estilo normal, stretch
normal y locale en-us son constantes del contrato privado, no parámetros variables.
Si se amplía ese contrato, ampliar la clave. Enum de alineación desconocido conserva
leading/near por defecto como al crear un formato nuevo. Color, texto, rect, DPI y
transformación no son propiedades variables del formato en estos métodos.

Hasta 32 entradas por Renderer, expulsión FIFO; clave de familia hasta 128 wchar_t.
Nombres más largos, null o tamaños no finitos/no positivos se tramitan sin cachear,
sin truncar ni cambiar validación/errores del camino original. Solo cachear creación
y configuración satisfactorias. Retención acotada de objetos CPU a cambio de evitar
creaciones repetidas; no afirmar reducción de RAM total. Sin memoria por texto/frame.

Limpiar antes de CreateDeviceResources y en DiscardDeviceResources. Además, comparar
factory en lookup para evitar resultados de una factory reemplazada. Mantener una
referencia de identidad evita reutilizar una dirección mientras siga cacheada. Toda
actividad en hilo UI/render, sin locks/hilos/timers nuevos.

## Aceptación

- AC-01: propiedades y medidas iguales a formato recién creado en fuentes/tamaños/
  alineaciones variadas, Unicode, espacios y líneas. Separación de claves, hits,
  expulsión/capacidad/límite de clave, fallos sin caché, factory reemplazada y limpieza.
  Mutar un formato público no cambia objetos internos ni otra llamada pública.
- AC-02: baseline antes de editar producción y bytes idénticos en frames WARP de
  texto (Unicode, alineaciones, tamaños/familias, DPI/resize y expulsión). Conservar
  snapshots 006/007. No certifica compositor/driver/interacción nativa exhaustiva;
  si difiere corregir o retirar porción antes de aceptar.
  Si una captura inicial difiere, contrastar con renderer original recompilado y
  oráculo fresco controlado: no atribuir causa ni excluir diferencias sin comprobar
  si también ocurren sin caché. La comparación controlada exige cero bytes distintos,
  sin tolerancia de calidad ni cambio de tipografía para hacerla pasar.
- AC-03: Release/CTest completo, revisión contra snapshot local y comentarios,
  documentos/roadmap actualizados. Preservar todo trabajo anterior sin commit.
- AC-04: comparar solo obtención/configuración de formatos contra creación nueva
  y medición completa de texto bajo mismos inputs; warmup/mediana sin umbral frágil.
  Registrar límites/retención; sin mejora global de arranque, GPU o reposo atribuida.

## Exclusiones

Cambios de tipografía, shaping, wrapping, calidad/FPS/efectos; nuevas políticas de
layout/animación; índice masonry; decoder; IPC/ACK/elección/cierre; Library/enums/
persistencia. Caché de última medida queda para otra porción. Sin decisiones pendientes.

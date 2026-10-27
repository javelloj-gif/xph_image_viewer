# 025: Evitar dibujo redundante e invisible

Autorización: el 2026-10-26 el propietario pide ejecutar los pasos necesarios
para continuar el diagnóstico y corrección de 024. Esta entrega implementa la
primera corrección medible, conservando calidad, aceleración y datos existentes.

Se separa el sondeo de actualizaciones/entregas del dibujo/presentación. Se mantiene
una espera de hasta 16 ms para entregar trabajo y avanzar temporizadores sin exigir
todavía notificaciones de todos los productores. Un futuro scheduler totalmente
orientado a eventos queda fuera del alcance; no se promete CPU cero en reposo.

- AC-01: después de asentarse una imagen estática, no dibujar/presentar sin cambios.
- AC-02: oculto/minimizado no dibujar/presentar; conservar entregas, cierre e IPC;
  restaurar con un frame completo. Mantener avance lógico de animaciones según el
  reloj actualizado existente, sin introducir pausa implícita al ocultar.
- AC-03: entrada, transiciones, auto-hide, animaciones finitas/infinitas, caret,
  uploads y cambios de página siguen actualizando sin entrada auxiliar.
- AC-04: comparar referencia instrumentada y candidato aislados; registrar
  Draw/Present por proceso y GPU cuando el entorno lo permita. No confundir
  tiempo de llamada CPU con tiempo GPU ni consumo global con atribución por PID.
- AC-05: Release/CTest y regresiones significativas de invalidación/visibilidad,
  con documentación de contratos actualizada.

No reducir FPS durante transiciones, interpolación, transparencia ni presupuestos
de uploads. No alterar formatos, enums persistidos, gestos o rutas de usuario.

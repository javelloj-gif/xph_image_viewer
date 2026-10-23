# Plan: 002 Retirada de Library

Especificación: [spec.md](spec.md). Revisión: [review.md](review.md).
Actualización: 2026-10-23. Entrada: `b1681c0`, rama codex/repository-foundation,
árbol limpio. La instrucción final limita el trabajo a retirar Library, sin diálogos,
opciones nuevas ni migración; sustituye el selector aprobado anteriormente.

## Porciones

- [x] Inventario exclusivo/compartido/histórico actualizado contra código actual.
- [x] Conservar gestor/modelos en core; no repetir extracción ni AR-002/AR-005.
- [x] Retirar página, enum 4 sin alias, miembro/friend/include y fuentes CMake.
- [x] Retirar inicialización, draw/update, eventos, hover/sync y opciones exclusivas.
- [x] Reservar NUM2/2 sin filtrar OnChar; posición radial gris — / Unavailable.
- [x] Retirar únicamente SetPage(Library) del routing IPC de carpetas.
- [x] Adaptar regresiones compartidas existentes a Waterfall/tira/Board; retirar casos
  exclusivos. Retirar selector provisional y sus dependencias/tests auxiliares.
- [x] Actualizar guías vigentes sin reescribir evidencia histórica.
- [x] Release/CTest 7/7 y diff-check finales tras simplificación.
- [x] Fuentes/documentación y continuidad preparadas para guardar en Git.

## Límites

El propietario detuvo la validación ampliada y pidió simplificar. No continuar
campaña UI, packaging o compatibilidad ni abrir decisiones nuevas. Las pruebas
nativas de la versión provisional con selector no validan la versión final.
Los artefactos intermedios que contienen selector quedan obsoletos, sin entrega.
No certificar arranque/drop con preferencia antigua 4: no tiene tratamiento nuevo.
No investigar ni reproducir IPC-014-01; CTest aislado no certifica IPC completo.
La observación visual estaba pendiente al preparar el candidato. El propietario
confirmó después pruebas funcionales exitosas; cierre y límites en result.md.

## Autorización posterior y siguiente sesión

El propietario pidió después un candidato temporal y un prompt de continuidad:
[pruebas funcionales](functional-test.md), [prompt](resume.md).
Candidato `dist/lenteuv_qa_002_20261023_sin_library/lenteuv.exe`, generado desde
`bff3d92`; identidad y comprobaciones en result.md. Reporte funcional exitoso recibido.

- [x] Preparar portable limpio, extraer y comprobar identidad del EXE.
- [x] Guardar protocolo/prompt y condición de limpieza.
- [x] Recoger resultados funcionales del propietario; aceptación global explícita.
- [x] Si son exitosos, actualizar evidencia/cierre y eliminar ejecutables previos
  del proyecto según el ámbito y método de resume.md, conservando el aceptado.

002 cerrado el 2026-10-23. Limpieza realizada: 157 EXE y 8 ZIP; excepciones y
verificación en result.md. IPC-014-01 cerrado por reporte posterior del propietario:
el visor se actualiza al abrir otra imagen desde Windows. Sin investigación ni
cambio de código; no añadir diálogos, opciones ni migraciones. No hay siguiente
paso funcional autorizado ni campaña base que continuar.

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
La observación visual final sigue sin realizarse; no cerrar 002 como aceptación
funcional completa sin evidencia necesaria. Resultado/limitaciones en result.md.

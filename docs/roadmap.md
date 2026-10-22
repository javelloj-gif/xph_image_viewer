# Hoja de ruta

Actualización: 2026-10-22. Prioridad acordada: refactorización. Primero se realiza
la auditoría con la habilidad local y se define una porción verificable de trabajo.
La autorización actual cubre auditoría y documentación; no se ha iniciado la edición de C++.

| ID | Cambio y enlace | Prioridad | Dependencias | Estado |
| --- | --- | --- | --- | --- |
| 001 | [Refactorización base y auditoría](changes/001-refactorizacion-base/spec.md) | Primera, acordada | Auditoría inicial realizada; R1 propuesta | En curso: planificación; implementación pendiente |
| 002 | [Retirada de Library](changes/002-retirar-library/spec.md) | Posterior a la primera porción de 001; orden propuesto | Mapa de servicios compartidos de 001; resolver preferencias antiguas | En definición |
| 003 | [Diagnóstico de arranque, IPC y reposo](changes/003-diagnostico-rendimiento/spec.md) | Posterior; orden propuesto | Revisión Git identificada; medición base antes de cambios que afecten al escenario | En definición |

## Secuencia y dependencias

Los IDs identifican trabajos; solo la prioridad inicial de refactorización está
acordada. No se requiere terminar una refactorización global antes de abordar 002 o 003.
El plan de 001 concretará una primera porción y separará correcciones de otros hallazgos.
Si una refactorización afecta arranque, IPC o recursos, registrar una base acotada
según el protocolo de 003 antes de implementarla; esa medición prepara la validación
y no desplaza la prioridad de refactorización.

Los hallazgos se conservan en el [informe de arquitectura](changes/001-refactorizacion-base/review.md),
se vinculan a tareas del plan o a propuestas posteriores y no se consideran corregidos
por el hecho de estar identificados. Las hipótesis de rendimiento pasan a 003 para
comprobación, sin convertirlas automáticamente en defectos confirmados.

Estados: propuesto, en definición, listo, en curso, en revisión, bloqueado,
completado o descartado. Usar `bloqueado` solo con un impedimento concreto registrado
en el plan. `Completado` requiere la evidencia de aceptación definida para el cambio.

Asignar IDs consecutivos (`001`, `002`, etc.) al registrar cambios. El ID identifica
el trabajo, no su prioridad. Indicar expresamente las prioridades todavía pendientes.
Los asuntos de [problemas conocidos](known-issues.md) son candidatos para investigar,
no una lista de cambios aprobados ni una secuencia obligatoria.

Consultar el [flujo de trabajo](workflow.md) y las [plantillas](changes/README.md).

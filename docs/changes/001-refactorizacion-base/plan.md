# Plan: 001 Refactorización base

Especificación: [spec.md](spec.md). Última actualización: 2026-10-22.
Estado: auditoría inicial completada; R1 propuesta, implementación no iniciada.

## Componentes y diseño

Auditoría estática por componentes, con prioridad a `App`, carpetas/Library/Waterfall,
loader, IPC y persistencia. Revisar otros consumidores según riesgo y registrar
explícitamente la cobertura restante. Usar el diseño actual como contexto, no imponer
una arquitectura nueva. Informe persistente: [review.md](review.md).

## Tareas y comprobaciones

- [x] T-01: Inventariar componentes y cobertura de la auditoría. Criterio AC-01.
- [x] T-02: Ejecutar el agente con la habilidad y verificar sus hallazgos. Criterio AC-02.
- [x] T-03: Proponer la primera porción y mapear hallazgos a trabajo posterior. Criterio AC-03.
- [ ] T-04: Concretar alcance, archivos e invariantes de esa porción y sus pruebas.
  Resolver decisiones materiales antes de edición dependiente. Criterios AC-04/AC-05.
- [ ] T-05: Implementar la porción acordada, revisar y validar. Criterios AC-04/AC-05.
- [ ] T-06: Registrar resultado real, guías afectadas y pendientes. Criterio AC-06.

## Trazabilidad de hallazgos

IDs locales al [informe](review.md); ninguno está corregido por esta auditoría.

| Hallazgo | Tratamiento propuesto | Destino y estado |
| --- | --- | --- |
| AR-001 | Separar ubicación/nombres del gestor compartido | R2 propuesta de 001; preparación de 002, sin rediseñar Library |
| AR-002 | Especificar identidad/vigencia de resultados async y regresiones | Corrección independiente propuesta; revisar a continuación App/tira/Waterfall |
| AR-003 | Medir y aclarar descubrimiento/envío/recepción IPC | Investigación 003; después especificación correctiva propia |
| AR-004 | Extraer búsqueda común de datos preservando algoritmo | R1, primera porción propuesta de 001 |
| AR-005 | Caracterizar colas/cierre; distinguir descarte de cancelación | IPC en 003; campaña de loader/cancelación posterior fuera del alcance inicial |
| AR-006 | Especificar validación de caché con datos sintéticos | Endurecimiento posterior propuesto; en 003 solo investigar si explica tiempos |
| AR-007 | Comentarios exactos y contratos de componentes modificados | Criterio transversal AC-05; cobertura restante progresiva |

El orden R1/R2 es una propuesta de refactorizaciones puras. AR-002/AR-003 tienen
riesgos altos; una reproducción de estabilidad puede justificar adelantar su corrección
sin presentarla como limpieza mecánica.

## Ejecución propuesta de R1

1. Caracterizar el algoritmo existente en árboles temporales antes de sustituirlo.
2. Introducir helper neutral con raíz de EXE como entrada comprobable y adaptar
   los cinco caminos duplicados, sin migrar datos ni alterar política de existencia.
3. Revisar diff y comentarios; ejecutar los casos de selección y build/CTest relevante.
4. Registrar rutas seleccionadas y límites de prueba; comparar tiempos solo si se mide
   una base equivalente. No presentar reducción de duplicación como mejora demostrada.

## Validación

Para la auditoría: comprobar ubicaciones, consumidores y límites; verificar enlaces
y `git diff --check`. No hace falta compilar para validar documentos.
Para implementar: seleccionar pruebas según el diff; usar comandos de desarrollo
para build/CTest y recorridos adicionales cuando afecte UI, concurrencia o persistencia.
Si afecta tiempos o recursos, medir una base antes mediante el protocolo de 003.
No usar CTest de metadatos como prueba de renderizado.

## Riesgos y recuperación

Una reorganización amplia puede mezclar cambios funcionales con mecánicos. Mantener
porciones revisables, datos aislados y cambios ajenos intactos. Separar lo exclusivo
de Library de los servicios que usan otros modos. No renumerar enums persistidos.

## Decisiones y ajustes

- 2026-10-22: prioridad de refactorización acordada; auditoría y creación de requisitos autorizadas.
- Agente especializado autorizado para analizar y escribir únicamente su informe;
  no fuentes, builds ni datos de ejecución.
- Informe inicial recibido y contrastado: se prioriza como recomendación R1 (AR-004)
  y después separación compartida R2 (AR-001); correcciones funcionales en trabajos propios.

## Estado para continuar

- Completado: auditoría inicial, inventario de cobertura, siete hallazgos y propuesta R1/R2.
- En curso: ninguna implementación; requisitos de 002/003 registrados.
- Obstáculo: ninguno para planificar R1; decisiones de compatibilidad de 002 siguen abiertas.
- Siguiente paso: acordar la primera porción R1 descrita en spec y cerrar T-04 antes de editar fuentes.

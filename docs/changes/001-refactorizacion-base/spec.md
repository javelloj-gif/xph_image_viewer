> Estado vigente 2026-10-23: completado en alcance. Véase [cierre y aceptación](closure.md); el texto siguiente conserva decisiones históricas.

# 001: Refactorización base y auditoría de arquitectura

Fecha: 2026-10-22. Estado: auditoría inicial, R1 y R2 completadas; otras porciones pendientes.
Prioridad inicial acordada por el propietario. Tras la auditoría, el propietario autorizó
implementar R1 (AR-004), con caracterización previa y preservación exacta, y después
R2 (AR-001), separación neutral del gestor compartido. Otros hallazgos siguen pendientes.

## Problema y evidencia inicial

El propietario requiere identificar redundancia, responsabilidades mezcladas, código
subóptimo, inconsistencias de nombres y falta de contexto en clases y métodos.
Las guías existentes describen estado compartido mediante `App`, servicios alojados
bajo `library/`, callbacks y formatos persistidos que limitan una reorganización segura.
Son puntos de investigación, no prueba de que toda la arquitectura deba reemplazarse.

## Objetivo

Obtener un mapa revisable de oportunidades y riesgos con la habilidad
[`lenteuv-architecture-review`](../../../.agents/skills/lenteuv-architecture-review/SKILL.md),
priorizar una primera refactorización acotada y definir cómo demostrar que conserva
el comportamiento. Mantener el inventario para continuar por componentes.

## Fuera del alcance

- Rediseñar toda la aplicación, imponer patrones o renombrar masivamente.
- Retirar Library durante la auditoría: ese cambio funcional vive en 002.
- Corregir rendimiento o IPC sin una especificación y evidencia propias.
- Afirmar revisión exhaustiva, pruebas pasadas o mejoras medidas sin realizarlas.

## Comportamiento esperado

El agente especializado lee código y consumidores, registra cobertura y entrega
`review.md` con hallazgos estables `AR-001`, etc. El plan relaciona esos hallazgos
con tareas o trabajos posteriores. La auditoría termina con una propuesta de primera
porción, sin aplicar automáticamente sus recomendaciones.

Las futuras refactorizaciones conservan comportamiento salvo cambios funcionales
expresamente acordados. Cada clase y método nuevo o modificado tiene un comentario
breve y exacto; la cobertura documental preexistente se completa por componentes.

## Restricciones y compatibilidad

Aplican `AGENTS.md`, contratos de hilo principal, propiedad de recursos y preservación
de datos. No cambiar formatos, rutas, valores persistidos o gestos incidentalmente.
No eliminar servicios compartidos por su ubicación en una carpeta de Library.
No afirmar impacto en CPU/GPU/memoria sin medición; transferir preguntas a 003.

## Criterios de aceptación

| ID | Resultado observable | Comprobación |
| --- | --- | --- |
| AC-01 | Inventario de componentes con cobertura completa/parcial/no revisada y siguiente área | Informe contrastado con archivos de `src/` |
| AC-02 | Hallazgos con evidencia, ubicación, impacto, alternativa, coste y validación | Revisión de `review.md` y símbolos citados |
| AC-03 | Primera porción propuesta y relación explícita entre hallazgos y trabajo futuro | Plan y hoja de ruta |
| AC-04 | La porción elegida conserva los contratos acordados | Pruebas y recorridos definidos antes de implementarla |
| AC-05 | Clases y métodos nuevos/modificados cumplen el requisito de comentarios | Revisión del diff; inventario documental del componente |
| AC-06 | Resultado y límites registrados sin equiparar auditoría con corrección | `result.md` y estado de tareas |

AC-01 a AC-03 permiten cerrar la fase de auditoría inicial, no toda la refactorización.
AC-04 y AC-05 solo podrán verificarse sobre la porción que se acuerde implementar.

## Preguntas y decisiones pendientes

- Primera porción elegida: R1; contrato y comprobaciones concretados en el plan.
- Cualquier convención nueva debe justificarse y acordarse; no sustituir la existente por gusto.
- Las decisiones de producto de 002 no se resuelven mediante una refactorización de 001.

## Primera porción acordada: R1, resolución común de datos

Origen: AR-004 del [informe](review.md). Extraer un helper neutral para la búsqueda
actual de `user_data`, usado por Settings, su lectura temprana de diagnóstico,
FolderDataManager y MetadataCache. Implementada y validada según [resultado](result.md).

- Preservar orden, profundidad de seis candidatos, criterio de existencia y fallback
  junto al ejecutable; no endurecer incidentalmente el tipo de candidato admitido.
- Preservar nombres de settings, sesión, favoritos y caché. No crear directorios,
  mover datos a AppData ni cachear el resultado de la búsqueda.
- Excluir i18n, política de permisos, MAX_PATH y formatos; sus contratos no son idénticos.
- Caracterizar con árboles temporales: candidato junto al EXE, candidatos ascendentes
  múltiples, candidato en límite, fuera de límite y ausencia. Comparar selección
  antes/después, sin utilizar datos personales.
- Documentar helper y métodos modificados; comprobar consumidores y build/CTest
  pertinente. Si se mide arranque, mantener revisión e instrumentación identificadas.

Esta porción concreta AC-04/AC-05; no promete mejorar rendimiento ni corregir IPC.

## Segunda porción acordada: R2, gestor compartido en core

Origen: AR-001 del [informe](review.md). Mover `FolderDataManager` y su modelo desde
`src/library/` a `src/core/`, renombrar el miembro de App a `m_folderData`, ajustar
includes/CMake y documentar las responsabilidades y efectos de la API existente.

- Mantener clase, métodos, modelo, disposición, referencias y propiedad del estado.
  Conservar orden de inicialización, `friend`, hilo UI y todos los comportamientos.
- No cambiar algoritmos de escaneo/ordenación, selección, cargas, previews, rutas R1,
  formatos o gestos. Mantener Library y su camino AddFolder propio, con sus diferencias.
- Validar equivalencia mecánica frente al snapshot R1, Release/CTest y recorrido con
  datos aislados; distinguir comparación de código, ejecución y observación visual.
- Aplicar AC-04/AC-05 a esta porción y registrar cobertura/límites en plan/result/review.
  No implica una separación nueva de persistencia/navegación ni corrige AR-002.

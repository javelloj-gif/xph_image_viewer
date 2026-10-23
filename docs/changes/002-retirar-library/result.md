# Resultado: 002 Retirada de Library

Especificación: [spec.md](spec.md). Plan: [plan.md](plan.md).
Actualización: 2026-10-23. Estado: definición actualizada; producto sin implementar.

## Cambios realizados

Se retomó sobre `codex/repository-foundation`, HEAD `8625d95`, árbol inicialmente
limpio. Se actualizaron los documentos antiguos contra el código actual y se creó
[review.md](review.md) con inventario exclusivo/compartido/histórico, cobertura,
riesgos y diseño mínimo. FolderDataManager y modelos permanecen en core; AR-002
y AR-005 no se reabren. Plan por porciones y matriz de validación preparados.

Se formularon juntas las dos decisiones de producto: tratamiento explícito del 4
antiguo en tres preferencias y texto del placeholder/tooltip. Propuestas en spec,
pendientes de respuesta; no son comportamiento aprobado. No se han editado fuentes,
tests, defaults ni guías para anunciar una retirada todavía inexistente.

## Evidencia de aceptación

| Criterio | Evidencia actual | Estado |
| --- | --- | --- |
| AC-01 | Inventariados handler global, prioridad de overlays/páginas y OnChar | Pendiente de implementación y prueba |
| AC-02 | Revisados targets, estilos y tooltip del radial | Pendiente de decisión, implementación y prueba |
| AC-03 | Inventariados arranque, opciones, drop y la rama Library de IPC | Pendiente de retirada; IPC-014-01 excluido |
| AC-04 | Clasificadas fuentes, modelos compartidos, tests y referencias históricas | Inventario completado; retirada pendiente |
| AC-05 | Matriz prevista para Viewer/Waterfall/Board, build/CTest y pruebas nativas | Sin ejecutar sobre 002 |
| AC-06 | Identificadas claves de arranque/drop/debug y sus consumidores reales | Pendiente de decisión y pruebas en copias |
| AC-07 | spec/plan/result/review actualizados; guías vigentes conservan producto actual | Parcial; comentarios y guías finales pendientes |

## Revisión y comprobaciones

Revisión estática de diseño usando la habilidad local; límites y dos riesgos
necesarios en review. Búsquedas `rg` en fuentes, tests, CMake, defaults y documentación;
lectura de APIs/caminos afectados. `git diff --check` sin errores al finalizar.
No hay compilación, CTest, observación visual ni aceptación funcional de 002.
La evidencia de 004..014 continúa histórica y no acredita este cambio.

## Límite IPC

No se investigó, reprodujo ni corrigió IPC-014-01. Se identificó únicamente la
selección Library que deberá retirarse del routing de carpetas. No se atribuye a
las suites aisladas aceptación completa de IPC ni se declara resuelto el reporte.

## Continuidad y cierre

002 permanece abierto. Esperar las dos decisiones explícitas antes de la
implementación dependiente. Después ejecutar T-04a/b/c y la matriz de T-05,
revisar el diff, actualizar guías/roadmap y registrar evidencia por criterio.
No mantener un cast al 4, fallback silencioso o alias operativo para evitar la decisión.

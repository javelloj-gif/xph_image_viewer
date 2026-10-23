# Plan 013

[Especificación](spec.md). Sin delegación; preservar el lote local 001–012.

- [x] Revisar Git, roadmap, 001, inventario, 010/012 y habilidad de arquitectura.
- [x] Registrar contrato de lectura KTAG y cierre antes de modificar producción.
- [x] Caracterizar AR-006 con regresión sobre caché sintética aislada.
- [x] Implementar lectura transaccional y comentarios de responsabilidad.
- [x] Release, CTest completo y revisión del diff; registrar evidencia.
- [x] Reconciliar estados/aceptación y disposición de riesgos sin prometer solución global.
- [x] Guardar cambios en Git y preparar prompt de continuidad.

La revisión de cierre identifica además dependencia de BoardTagScanner respecto a
XmpTagManager (declarado después de BoardPage, destruido antes). Corregir join en
App antes de apagar loader y antes de destrucción de miembros. Sin rediseñar cancelación.
Crear portable nuevo desde staging limpio; conservar artefactos 010–012 y distinguir
la prueba funcional reportada del candidato adicional de esta sesión.

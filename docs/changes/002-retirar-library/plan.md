# Plan: 002 Retirada de Library

Especificación: [spec.md](spec.md). Última actualización: 2026-10-22.
Estado: preparación; pendiente de decisiones de compatibilidad.

## Componentes y diseño

Usar el informe de 001 para separar `LibraryPage` de servicios compartidos como
`FolderDataManager`, sus datos y consumidores. Revisar `App`, menú radial, settings,
opciones, fuentes CMake y guías. No borrar `src/library/` indiscriminadamente.
Origen específico: AR-001 del [informe de arquitectura](../001-refactorizacion-base/review.md).
AR-007 guía los comentarios de los componentes modificados; AR-002 requiere su propia
corrección y no justifica rediseñar una página destinada a desaparecer.

## Tareas y comprobaciones

- [ ] T-01: Inventariar referencias y distinguir exclusivas/compartidas/históricas. AC-03/AC-04.
- [ ] T-02: Resolver preferencias antiguas y texto del placeholder. AC-02/AC-06.
- [ ] T-03: Definir la extracción/movimiento mínimo de servicios compartidos. AC-04/AC-05.
- [ ] T-04: Retirar accesos, código exclusivo y opciones; implementar placeholder. AC-01 a AC-04.
- [ ] T-05: Compilar, ejecutar pruebas pertinentes y recorrer modos restantes. AC-05/AC-06.
- [ ] T-06: Actualizar documentación y comentarios; revisión de arquitectura del diff. AC-07.

## Validación

Antes de implementar, precisar escenarios: arranque normal y desde Explorer, preferencias
de modo, drop, navegación por teclado, clic en placeholder y recorrido de Waterfall/Board/Viewer.
Usar build/CTest según desarrollo, pero validar UI por recorrido. Probar persistencia
sobre copias aisladas; comprobar favoritos/sesión si cambia el gestor compartido.

## Riesgos y recuperación

Los valores de Page están persistidos; no compactarlos. Eliminar componentes solo
tras verificar sus consumidores. Mantener el trabajo en porciones revisables y datos
personales intactos. La investigación de rendimiento se registra aparte en 003.

## Decisiones y ajustes

2026-10-22: Library se retira, Waterfall permanece. NUM2 y `2` sin nueva acción.
No se acepta conservar Library como modo legado ni su fallback específico a Viewer.

## Estado para continuar

- Completado: requisitos funcionales iniciales registrados.
- En curso: no hay implementación.
- Pregunta: tratamiento de valores antiguos y texto de placeholder.
- Siguiente paso: revisar resultados de 001 y resolver T-02 antes de edición dependiente.

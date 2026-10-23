# 002: Retirada de Library

Actualización: 2026-10-23. Estado: completado y aceptado funcionalmente por el propietario.

## Problema y evidencia inicial

El propietario considera redundante el modo Library y pide retirarlo mediante
refactorización, conservando Waterfall operativo. En código, NUM2 y `2` comparten
acceso a Library; el menú radial distingue `LIB` de `WAT`. Hay referencias en
arranque, opciones y preferencias. A HEAD `8625d95`, `library/` contiene solo
`library_page.cpp/.h`; `FolderDataManager` y sus modelos ya residen en `core/`.
AR-002 está corregido en 004/ampliado en 011 y AR-005 implementado en 014.
002 conserva esos contratos; no repite las extracciones ni reabre la base.

## Objetivo

Eliminar el modo y sus referencias funcionales en fuentes y documentación vigente,
preservando las capacidades compartidas que necesitan los modos restantes.

## Fuera del alcance

- Reasignar NUM2 o `2` a una función nueva.
- Desactivar Waterfall, rediseñar el menú o corregir problemas ajenos.
- Mantener `Page::Library` como alias o ruta operativa de compatibilidad.
- Borrar historial Git o documentos históricos para ocultar la existencia anterior del modo.
- Investigar, reproducir o corregir IPC-014-01. La única modificación prevista en
  `OnIpcOpenPath` es retirar la selección de Library en la rama de carpetas;
  no añadir allí un modo sustituto. El fallo permanece diferido por el propietario.

## Comportamiento esperado

- NUM2 y `2` dejan de seleccionar modos y quedan reservados temporalmente. Esta
  regla de navegación no impide escribir el carácter `2` en editores de texto.
- La posición de Library en el menú queda gris, inactiva y sin activación por clic.
  Waterfall conserva su acceso y apariencia operativa; Viewer y Board siguen disponibles.
- Desaparecen la implementación exclusiva de Library, su inicialización, enrutamiento,
  opciones activas e inclusión en build. Los servicios compartidos se conservan en
  core; solo se ajustan nombres/comentarios que describan una dependencia inexistente
  del modo, sin cambiar sus efectos, rutas ni persistencia.
- La documentación vigente refleja los modos disponibles y las teclas reservadas.

## Restricciones y compatibilidad

Última instrucción del propietario, 2026-10-23: limitar el trabajo a eliminar
Library y sus referencias en código, sin cuadros de diálogo ni opciones nuevas.
Esta instrucción sustituye la elección anterior de solicitar un modo para el valor 4.
No se añade fallback, migración, selector ni guardado de reemplazos; las preferencias
antiguas no reciben tratamiento específico. El antiguo 4 deja de designar un modo
soportado. No certificar compatibilidad funcional con ese valor.
Los IDs restantes son Start=1, Viewer=2, Board=3 y Waterfall=5; 4 no tiene enumerador
ni alias. Start conserva su comportamiento preexistente.
Mantener sesión, favoritos, rutas y formatos existentes salvo decisión explícita.
Se admite únicamente el placeholder visual reservado requerido, sin presentar un modo utilizable.

## Criterios de aceptación

| ID | Resultado observable | Comprobación |
| --- | --- | --- |
| AC-01 | NUM2 y `2` no cambian de modo ni reciben función nueva | Recorrido en modos restantes; revisar handlers/overlays |
| AC-02 | Placeholder gris no activable; Waterfall operativo | Hover/clic y navegación con teclado |
| AC-03 | Ningún camino activo intenta iniciar Library | Revisar arranque, menú, opciones, drop e IPC |
| AC-04 | Código exclusivo retirado, servicios compartidos preservados | Búsqueda clasificada de referencias, CMake y revisión del diff |
| AC-05 | Viewer, Waterfall y Board conservan funciones afectadas | Build/CTest y recorridos funcionales pertinentes |
| AC-06 | No se añaden diálogos, opciones nuevas, fallback ni migración de preferencias | Revisión del diff final y búsqueda de referencias |
| AC-07 | Guías vigentes y comentarios coinciden con el resultado | Revisión documental y de clases/métodos modificados |

## Decisiones vigentes

- Retirar Library y referencias funcionales, conservando servicios y modos restantes.
- No introducir diálogos ni opciones nuevas ni tratamiento especial del valor 4.
  El selector implementado provisionalmente y sus tests auxiliares fueron retirados
  tras la instrucción final del propietario; sus pruebas no acreditan la versión final.
- Mantener lo acordado sobre teclas reservadas y posición gris `—` / `Unavailable`.
- Mantener fuera del alcance técnico IPC-014-01 y conservar documentos históricos/atribuciones.
  En el cierre posterior, el propietario reporta funcionamiento actual correcto;
  se actualiza el registro sin investigar ni atribuir una corrección a 002.

Inventario y revisión del diff: [review.md](review.md).

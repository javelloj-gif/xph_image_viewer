# 002: Retirada de Library

Actualización: 2026-10-23. Estado: en definición por compatibilidad pendiente.
Intención funcional acordada; implementación no iniciada.

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

El propietario rechazó conservar referencias funcionales a Library y la propuesta
de convertir específicamente la preferencia antigua de Library a Viewer.
No se ha decidido qué hacer con una configuración existente que contenga el valor
numérico antiguo `4`. No implementar silenciosamente ese fallback ni renumerar los
valores persistidos de Viewer/Board/Waterfall.
Los IDs restantes son Start=1, Viewer=2, Board=3 y Waterfall=5; 4 no tendrá
enumerador ni alias. Start no es una página dibujada actualmente y su rediseño
queda fuera de 002. Las preferencias afectadas son
`page_on_app_launch_from_explorer`, `page_to_open_on_drag_and_drop` y `debug_page`;
esta última se persiste y aparece en opciones, pero no tiene consumidor funcional.
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
| AC-06 | Preferencias antiguas tienen tratamiento explícito y comprobado | Decisión pendiente; prueba con copia de configuración |
| AC-07 | Guías vigentes y comentarios coinciden con el resultado | Revisión documental y de clases/métodos modificados |

## Preguntas y decisiones pendientes

- ¿Qué comportamiento se espera al leer el valor de modo ya retirado en preferencias?
  La refactorización no elimina por sí sola los datos existentes fuera del código.
- Confirmar texto del placeholder y tooltip: recomendación pendiente, posición reservada
  con texto de no disponibilidad, sin prometer un acceso por NUM2.

Consulta conjunta enviada el 2026-10-23, aún sin decisión aplicada:

1. Valor 4: propuesto selector explícito Viewer/Waterfall/Board con guardado solo
   al confirmar; alternativa, error con clave afectada y edición manual antes de
   continuar la operación. Incluye `debug_page`; cancelación y momento exacto de
   resolución se concretarán con la decisión. Ninguna propuesta es un fallback aprobado.
2. Texto: propuesto `RES` / `Reserved (no action)`; alternativa `—` / `Unavailable`.

Inventario y diseño: [review.md](review.md). No implementar comportamiento dependiente
hasta que el propietario resuelva ambas decisiones.

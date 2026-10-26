# 016: Propuestas visuales de visor

Estado: diseño elegido y aceptado por el propietario el 2026-10-26.
[Acuerdos](../product-batch-20261025.md).

Diseño aprobado: dock compacto con iconos Outline; barra informativa centrada sobre
la tira, alineada con la imagen activa; miniatura activa de 117 DIP frente a base
predeterminada de 90 DIP (+30 %), borde cian y sin rótulo permanente Viewing.

## Objetivo

Comparar tres propuestas del panel superior con iconos coherentes con las superficies
oscuras, acento cian y sombras discretas actuales. Mostrar también miniatura activa
mayor y barra informativa inferior sin solapamientos. Mantener nombres en inglés.

## Criterios de aceptación

| ID | Resultado | Comprobación |
| --- | --- | --- |
| AC-01 | Barra horizontal, panel de dos filas y dock compacto completos | Comparación visual de las tres variantes |
| AC-02 | Seis acciones, iconos, hover, activo y Sort & Filter deshabilitada | Inspección e interacción del mockup |
| AC-03 | Activación, ocultamiento y adaptación a ventana pequeña definidos | Propuestas y preview responsive |
| AC-04 | Miniatura activa distinta de multiselección y barra sin solapamiento | Propuestas y preview |
| AC-05 | Elección explícita del propietario | Confirmaciones de dock, iconos, barra centrada, 117 DIP y ausencia de rótulo registradas en result.md |

No implementar el panel en C++ ni sustituir el editor de etiquetas. Los mockups
no prueban UX nativa, hit testing Win32 ni rendimiento. Iconos representados son
referencias visuales; la implementación nativa usará los recursos/patrones del proyecto.

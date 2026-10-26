# Revisión local del diff 017

Fecha: 2026-10-26. Habilidad aplicada: lenteuv-architecture-review.
Alcance: diff local sobre 9a334ad2 en codex/015-roadmap-design; documentos de 015/016
preservados. Revisión en esta sesión, no independiente ni auditoría integral.

Inspeccionados Settings/defaults/parser sin cambios, OptionsMenu, App cierre y
routing de doble clic, Viewer dibujo/contextual/teclas, tira Draw, traducciones,
CMake, wrapper y regresiones. Se siguieron delegación de Waterfall y precedencia
de Board/tira/radial. No se revisaron de nuevo internals del loader, seguridad de
formatos o todos los contratos anteriores: no hay modificaciones en esas áreas.

AR-017-01, defecto confirmado por ejecución, prioridad media: doble clic dentro
de Settings llegaba a Viewer y alternaba fullscreen detrás del overlay. Ubicación:
App::OnMouseDoubleClick en src/core/app.cpp. Corrección mínima: guard de opciones/
etiquetas antes de páginas; conserva orden de controles existente. Regresión real
sobre ventana propia y recorrido visual del candidato final confirman Settings.
No añade jerarquías ni propiedad nueva; solo UI en principal. Resuelto en alcance.

Revisión final: sin otros hallazgos accionables de código en los caminos inspeccionados.
IDs retirados no reutilizados, enums/KTAG/KBRD y resolución de datos intactos;
retiradas exclusivas sin tocar la estética de etiquetas ni dim de Waterfall.
Comentarios de responsabilidad revisados en declaraciones modificadas y test nuevo.
Load conserva su efecto previo de Save cuando falta el archivo; el test lo caracteriza.

Release/CTest 9/9; cobertura visual y límites exactos en result.md. El empeoramiento
de las primeras series bloqueó AC-06. Tras liberar recursos y pedir repetición,
el propietario obtuvo una condición donde los rangos solapan, con mediana aún
mayor. El resultado dispone AC-06 con ese límite, sin defecto causal demostrado
ni certificación de equivalencia/rendimiento por revisión estática.

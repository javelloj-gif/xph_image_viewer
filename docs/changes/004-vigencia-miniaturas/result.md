# Resultado: 004 Vigencia de miniaturas

Estado: completado. Especificación: [spec.md](spec.md). Plan: [plan.md](plan.md).
Fecha: 2026-10-22. Base: `ac4a5da` más R1/R2 sin commit, preservadas en la misma rama.

## Resultado

La tira y los previews compartidos de Waterfall/Library usan tokens únicos por
solicitud/ruta. Los callbacks buscan el elemento actual por token y ruta, sin escribir
por índices capturados. Ordenar o desplazar conserva la solicitud con el elemento;
eliminar/reemplazar/recrear no admite el resultado anterior. La entrega vigente
consume el token y termina isLoading; una segunda entrega es ignorada.

SetFileList conserva caché y solicitudes de las entradas retenidas una vez por
ocurrencia. Las rutas repetidas pueden compartir texturas/fallos ya cacheados, pero
nunca duplican una solicitud pendiente. CompletePreview centraliza el resultado
compartido; cada página conserva su tamaño y sus efectos de layout/redraw vigentes.
No cambia loader, ordenación, gestos, formatos, rutas ni retirada de Library.

## Evidencia y reproducción

Primero se añadió el harness con acceso amigo a las colas/modelos y se ejecutó
contra los callbacks aún sin corregir. `ctest --preset release -R ui.async_previews
--output-on-failure` falló con **stale A completion must not mark replacement B failed**:
se pidió A, se reemplazó por B en una lista del mismo tamaño y se entregó el resultado
fallido de A. Esto reproduce una asociación incorrecta en código productivo, sin
provocar un crash ni usar datos personales. La primera prueba permanece en el harness.

Después de corregir se ejecutó:

```powershell
cmake --preset windows-x64
cmake --build --preset release
ctest --preset release --verbose
git diff --check
```

Entorno: Windows x64, MSVC 19.43.34809 (VS2022 Build Tools), SDK 10.0.22621.0,
CMake 4.4.2. Compilación completa correcta. **CTest 3/3 pasó**: metadata.tagging
(copias aisladas), common.user_data_paths (19 checks) y ui.async_previews
(98 checks, incluidos preparación y control de colas). No se desactivaron assertions.

| Criterio | Evidencia | Resultado |
| --- | --- | --- |
| AC-01 | Tira vacía/menor, reemplazo A/B, retorno A/B/A, reordenamiento real Rearrange, fallo, bitmap válido y caché/ocurrencias repetidas | Pasó |
| AC-02 | Ambas páginas: sort real, carpeta desplazada/borrada, preview desplazado, reconstrucción con misma ruta, ruta diferente; solicitud compartida; App::DeleteImage en memoria | Pasó |
| AC-03 | Segunda entrega no modifica un resultado ya consumido; solicitud nueva conserva isLoading frente al resultado anterior | Pasó |
| AC-04 | Callbacks emitidos por LoadThumbnail/LoadPreviewTexture y entregados por ProcessPendingTextures; build/CTest | Pasó |
| AC-05 | Comentarios, revisión acotada, guías y roadmap con recomendación 003/IPC | Pasó; véase [review.md](review.md) |

El harness enlaza todas las implementaciones productivas salvo wWinMain, sin stubs.
App no se inicializa como aplicación. Usa WIC con cero workers; ejecuta una tarea real
de decodificación fallida y conserva su callback para entregarlo después de las mutaciones.
Los resultados sintéticos de previews omiten upload al no tener píxeles válidos;
la tira recibe además un bitmap D2D real 1x1 creado en el hilo del test con WARP software.
Una ventana solo de mensajes recibe las invalidaciones sin mostrar UI ni afectar al desktop.
No se borran archivos: App::DeleteImage aquí solo modifica listas en memoria.

## Revisión y límites

La revisión con lenteuv-architecture-review no encontró hallazgos nuevos bloqueantes.
AR-002 queda resuelto en los tres callbacks indexados y su estado compartido. Los
callbacks de hover por ruta, otros consumidores async y cierre/cancelación no se certifican.
Los resultados obsoletos todavía pueden ser decodificados/subidos antes de descartarse;
no se cambia ni se mide ese coste (AR-005 separado).

No se midieron rendimiento ni carpetas grandes. No hubo recorrido visual, swapchain
ni carga real de imágenes del usuario; la evidencia determinista comprueba asociación
y estado de callbacks, no UX completa ni concurrencia real entre workers. El smoke
visual R2 permanece pendiente, separado de los criterios ejecutados aquí.

## Siguiente paso

Roadmap actualizado: **003 empezando por diagnóstico IPC (AR-003)**. Identificar revisión
y datos, ejecutar matriz de aperturas y comprobar la imagen del receptor antes de
cambiar el protocolo. 002 conserva las decisiones pendientes sobre valor antiguo 4
y placeholder. AR-003/005/006 y cobertura documental restante siguen sin corregirse.

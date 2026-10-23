# Plan: 009 Decoder compartido

[Spec](spec.md). Fecha: 2026-10-22. Estado: completado en alcance.

- [x] Revisar Git, habilidad, AGENTS/guías/008 y rutas App/loader/composición.
- [x] Guardar snapshot previo. Extraer helpers que reciban decoder sin modificar
  cuerpos de decodificación/composición; entrada nueva y consumo App. AC-01/03.
- [x] Harness separado dentro del EXE existente, proxy WIC para contar/injectar
  fallos sin cambiar producto. Fixtures propios/copias y GIF/PNG pequeños sintéticos.
  Comparación contra APIs previas y aserciones independientes de GIF. AC-01/02/03.
- [x] Configure/Release/CTest, medición acotada, revisión y documentos. AC-04.

Propiedad: ComPtr local por operación; liberar decoder de intento antes de fallback.
Resultado solo CPU, recursos GPU siguen en hilo UI. Sin modificar workers/colas.
LoadAnimatedSync original consulta count y delega mismo compositor; LoadSync original
abre y delega mismo frame 0. No presuponer IsValid exige >1 frames: hoy basta no vacío.

Evidencia local en build/optimization-009 y build/test-runs/decode-<id>, ignorados.
CTest prepara copia de EXE, user_data y fixtures. Probar siempre PNG/JPEG/GIF pequeños;
para fixtures opcionales estimar canvas*frames y omitir >32 MiB antes de decodificar
ambos caminos. No instalar codecs ni medir todo WebP grande sin presupuesto.

Recuperación: retirar solo diff 009 desde snapshot, sin reset/restauración global.
Mantener R1/R2/003..008 y datos locales. Resultado: [evidencia](result.md),
[revisión](review.md). Release/CTest 5/5; 294 checks decoder más 161701 previos.
APNG disponibles exponen un frame; WebP grande omitido por presupuesto, WebP pequeño
de dos frames validado. Compositor y cuerpo estático idénticos tras extracción.

Siguiente recomendado: especificar inicialización secundaria mínima (OPT-005),
siguiendo prerrequisitos de HWND/callbacks/IPC y conservando errores de 005. No introducir
ACK/elección/cierre. Índice masonry, última medida, uploads y presentación separados.

> Estado vigente 2026-10-23: completado. Aceptación funcional general reportada por el propietario; candidato del lote regenerado en 012 y nuevo candidato 013 preservando anteriores. Los pasos de continuidad de abajo son históricos; usar [prompt vigente](../013-cierre-base/resume.md).

# Plan: 010 Inicialización secundaria mínima

[Spec](spec.md), [revisión](review.md), [resultado](result.md). Fecha: 2026-10-22.
Estado: implementado; en revisión, aceptación visual de usuario pendiente (AC-03).

- [x] Revisar Git, AGENTS, habilidad/referencias, guías, 009 y contratos 005.
- [x] Seguir App/Window/IPC/main y prerrequisitos de servicios; fijar orden y errores.
- [x] Guardar snapshot de fuentes y binario de referencia bajo build/optimization-010;
  registrar hashes de referencia antes de editar y comparar AC-05 después. No usar HEAD como referencia
  del producto 009: todo el trabajo posterior a ac4a5da está sin commit.
- [x] Ampliar harness IPC aislado para secundarios por App y arranque temprano,
  bypass y fallo parcial. Evitar hooks/counters permanentes; reutilizar acceso de tests.
  Comprobar reentrada y destrucción durante implementación. AC-01..04.
- [x] Mover bloque de servicios conservando orden; comentar responsabilidad y
  prerrequisitos de Initialize en declaración. Mantener Window/IPC/main si no requieren
  corrección dentro del contrato. Revisión específica de callbacks. AC-01..04.
- [x] Ejecutar configure/Release/CTest, comparación acotada y smoke nativo automatizado;
  registrar evidencia por criterio y revisar diff. AC-05/06.
- [x] Actualizar contexto observado, resultado, revisión y roadmap solo con evidencia.

Validación: cmake --preset windows-x64; cmake --build --preset release;
ctest --preset release --output-on-failure. Fixtures solo lectura/copias, EXE y
user_data propios, pipes privadas del harness; no competir con instancia personal.
Recuperación: retirar solo diff 010 desde snapshot, nunca reset global del checkout.

## Continuidad y entrega de pruebas de usuario

Actualización tras 011: OPT-006 implementado y validado en
[011](../011-vigencia-antes-upload/result.md). Portable nuevo y prompt en ese cambio;
el paquete/evidencia de 010 se conservan. Aceptación visual 010 continúa pendiente.
La siguiente acción de implementación descrita debajo refleja la entrega original;
usar [continuidad 011](../011-vigencia-antes-upload/resume.md) al retomar.

Siguiente acción: recoger aceptación visual de usuario del portable 010 (AC-03);
en paralelo, especificar OPT-006 antes de modificar upload/callbacks. No cerrar
010 como completado hasta registrar el recorrido visual pendiente.
Pendientes independientes: índice masonry, última medida, uploads OPT-006 y scheduler
OPT-007; 003/AR-005/006/007 y límites 008 permanecen abiertos. Library espera decisión
del valor antiguo 4 y placeholder/tooltip. 009 permanece completado en su alcance.

Requisito del usuario: una vez terminadas de implementar todas las correcciones del
lote, generar un ejecutable temporal para pruebas de usuario. No declarar terminado
todo el roadmap por cerrar 010. Al finalizar el lote, compilar Release y preparar
portable aislado desde staging nuevo conforme a docs/development.md, con runtime,
locales y defaults limpios. Entregar ruta absoluta del EXE, hash, alcance y protocolo
de prueba; no instalar, sustituir asociaciones ni copiar datos personales. Mantener
artefactos bajo build/ o dist/ ignorados. La validación de usuario queda por realizar.

## Evidencia y ajuste de diseño

Release/CTest 5/5; diez escenarios nativos de arranque, siete con dibujo de cuatro
modos y tres fallos inducidos HWND/WIC/renderer. Reentrada temprana se induce en tests,
no se declara reproducida espontáneamente. Callback temporal propietario de copias
antes de IPC evita perder/aplicar temprano esas rutas; handler definitivo al completar
contenido/timer. Sin nuevo estado miembro ni API IPC. Spec actualizada.
Medición local no muestra reducción clara de latencia; consultar result.md.

- [x] Aceptación funcional general reportada por el usuario el 2026-10-23; sin matriz específica de gestos/flags.
- [x] Portable candidato 010 generado desde staging nuevo y extraído en
  dist/lenteuv_qa_010_20261022; SHA256 y límites en resultado.
- [x] Candidato regenerado en 012 y 013 al cerrar el lote; el paquete histórico 010
  no incluye futuras correcciones de uploads/scheduler/masonry/última medida.

Al probar reenvío, cerrar otras instancias de LenteUV antes de iniciar el candidato:
el endpoint productivo es compartido. Los tests automatizados usan pipes privadas.
La aceptación visual no se sustituye por el smoke automático ni las pruebas CPU/WARP.

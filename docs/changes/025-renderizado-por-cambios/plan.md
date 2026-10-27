# Plan 025

- [x] Retomar Git y documentación 023/024; preservar cambios anteriores.
- [x] Seguir fuentes de invalidación, entregas y temporizadores.
- [x] Instrumentar Draw/Present y conservar referencia aislada antes de corregir.
- [x] Capturar línea base y disponibilidad real de métricas GPU.
- [x] Consumir invalidación pendiente solo al dibujar visible; mantener sondeo CPU.
- [x] Invalidar por entrada, animaciones, transiciones de tira y entrega de texturas.
- [x] Regresiones de entregas ocultas, animación completa/progreso y entrada; Release/CTest 9/9.
- [x] Comparación nativa final, Release/CTest 9/9 y smoke nativo de 10 escenarios.
- [x] Revisión del diff/documentación y portable limpio con identidad del EXE medido.

Decisión: no reemplazar el sondeo por espera indefinida en esta entrega. Los
productores actuales publican desde Update/ProcessPending; conservar ese contrato
evita perder resultados. El flag de redibujo es atómico para solicitudes de workers,
pero UI y creación de texturas permanecen en el principal.

Revisión final: conservar la barra de progreso animada fluida también entre frames
de origen, mientras esté en un host Viewer. Se incorporan regresiones de esa
invalidación y de reproducción detrás de otra página.

Línea base válida: gpu-idle-e1949581dcb844d4affb6ab5c2247778. Los intentos previos
se descartan por error transitorio PDH o falta de identificación válida del HWND
oculto/cierre. El recolector verifica PID y retiene solo muestras PDH válidas.
Fallo inicial de CTest en concurrent real App secondaries, coincidente con
IPC-003-CONCURRENT-01; recheck de esa suite y ejecución final completa correctos.

Implementación y validación automatizada/nativa acotada completadas. El propietario
marca las pruebas satisfactorias y acepta la entrega el 2026-10-26. No se agrega
un nuevo contraste GPU-Z ni se amplía la cobertura de result.md por esa aceptación.
Autoriza commit y merge local; publicación remota y ajustes finales diferidos a
próximas sesiones cortas. Véase [closure.md](closure.md).

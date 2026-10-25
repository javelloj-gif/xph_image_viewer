# 003: reporte del propietario — 2026-10-24

**Decisión vigente del propietario, 2026-10-24:** «Basado en los criterios
presentados, procede con el cierre de 003». Confirma que, con las tareas anteriores,
hay una base sólida para nuevas modificaciones funcionales. Acepta el cierre por
alcance con portable final, límites de rendimiento medidos y seguimiento separado
del arranque y del harness IPC. Esta decisión sustituye el bloqueo anterior de
cierre; no afirma cumplimiento de 100 ms ni corrección de la intermitencia.
Se conservan los seis OK funcionales reportados. Ver [closure.md](closure.md).

## Registro anterior al cierre (histórico)

El propietario identifica el ejecutable como el candidato recién generado de 003,
entregado en `dist/lenteuv_qa_003_20261024_async_cache/lenteuv.exe` (fuentes b0e98f8;
hash de entrega en result.md). No aporta una nueva comprobación de hash del EXE usado.

## Reporte vigente posterior — 2026-10-24

El propietario informa múltiples pruebas con el SO recién iniciado, sin aportar
una cantidad ni una serie cronometrada. Confirma:

| Área | Reporte |
| --- | --- |
| Zoom/paneo | OK |
| Tira | OK |
| Navegación rápida | OK |
| Waterfall | OK |
| Board | OK |
| IPC | OK |

**La apertura inicial de aproximadamente 3000 ms no es aceptable.** Este reporte
sustituye la aceptación anterior del rendimiento inicial; no autoriza cerrar 003.
La mejora posterior con caché preparada sigue reportada. No se infiere una matriz
adicional de formatos, animaciones o restauración/cierre. El IPC correcto en uso no
diagnostica ni descarta IPC-003-CONCURRENT-01 del harness.

Tras indicar otro reinicio y que el candidato no se había iniciado en la sesión,
autoriza al agente una sola medición y análisis de causas/soluciones. Se realizó
una única invocación: original enviado a 1932 ms y confirmación DXGI observada a
1956 ms, sin repetición. [Condiciones, desglose y propuestas](first-session-analysis.md).
No se atribuye la diferencia con sus 3000 ms a una mejora ni se invalida su reporte.

Posteriormente autoriza preparar ETW y fija `F:\Pictures\FotosTEMP` como entrada,
sin alterar multimedia. Al indicar proceder otra vez, Preflight confirma otro boot
y cero procesos del visor. [Captura ETW única](etw-result.md) completada: original
enviado 2216 ms, DXGI observado 2237 ms, espera del creador de 1426 ms en Defender.
No se repite ni se infiere nueva aceptación del rendimiento a partir del diagnóstico.

## Observación y aceptación anteriores, sustituidas en rendimiento inicial

Reporta una carga inicial de alrededor de **3 segundos en un SO recién iniciado**.
Una vez ejecutados los procesos de caché/miniaturas, la carga resulta
considerablemente más rápida, también en Waterfall. Considera el rendimiento
aceptable, aunque no alcance su estándar de oro para Windows.

Se registra como observación manual de primera apertura tras iniciar el SO y
aceptación del rendimiento percibido, incluida la espera inicial y la mejora
posterior. No se aportan instrumento, repeticiones, archivo/configuración exactos
ni separación de ventana, original y miniaturas. No constituye una serie comparable
según first-session-protocol.md ni permite calcular p95, certificar 100 ms,
calificar el disco como frío o atribuir causalmente los 3 s a la caché/renderer.
Las mediciones instrumentadas anteriores conservan sus condiciones y resultados.

## Alcance pendiente en el reporte anterior (histórico)

El reporte no confirma individualmente el recorrido funcional de functional-test.md
(calidad, gestos, animaciones, Board, IPC y restauración/cierre). Tampoco dispone
expresamente la intermitencia IPC-003-CONCURRENT-01 del harness. No se informa un
nuevo fallo funcional reproducible ni se autoriza otra campaña de optimización.

003 permanece abierta por esos puntos y por la evidencia comparable de primera
sesión, o su disposición explícita por el propietario. No volver a pedir como
desconocida la experiencia inicial ya reportada ni su aceptación del rendimiento.

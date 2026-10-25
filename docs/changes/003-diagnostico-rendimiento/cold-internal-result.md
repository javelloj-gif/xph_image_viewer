# Arranque frío del candidato interno — 2026-10-24

**Una apertura real tras otro reinicio: 1995,63 ms desde invocación hasta envío
del original completo; 473,18 ms desde main.entry. 003 permanece abierta.**

## Preparación y ejecución

Windows arrancó a las 17:20:23,500 America/Bogota. Se comprobó el boot antes de
abrir producto; es posterior al usado para el harness y la serie comparada. No
había procesos LenteUV/harness activos. El propietario autorizó la primera apertura.
No se ejecutaron harness, warmups, builds ni otros EXE del proyecto antes de medir.
La ausencia de procesos por sí sola no prueba ausencia de ejecuciones ya cerradas;
la clasificación combina el contexto del propietario, boot y acciones de esta sesión.

Se usó el staging del candidato nuevo preparado antes del reinicio, conservando
settings, sesión y caché. EXE SHA256 verificado **después** de medir:
`874F5783B219E24FE1AD74B330BBF9A344EF2DAB0E3664738CED579CF5FF50D6`.
Antes de abrir solo se comprobaron metadatos de los inputs, todos anteriores al boot;
no se hasheó/copió/reconstruyó el EXE ni se enumeró/decodificó la colección.

Ejecutor `scripts/measure_cold_internal_startup.ps1 -ConfirmFirstLaunch`:
una apertura directa normal, `--multiple`, carpeta explícita acordada, Viewer
fullscreen, logging desactivado. Solo el recolector ETW previamente validado pidió
elevación UAC; el producto heredó token normal. Se esperó la autorización del
recolector sin iniciar el visor. Invocación a las 17:25:38,403 America/Bogota.
Cierre automático normal después de presentar original y 2500 ms de asentamiento.
Salida 0, hash esperado y guard exclusivo de boot conservado. Ningún reintento.

Artefactos locales privados:
`build/startup-internal-003-cold-463bf53192bb4f34aff618d6b62c84f4/`:
capture.json, qpc.csv, summary.json, startup.etl, trace-statistics.txt,
collector-result.json, defender-report.json/clixml y file-audit.json.
Guard: `build/startup-etw-003-preparation/attempt-639284772235000000.json`.
No mezclar con la captura histórica de otro EXE/boot ni repetir en esta sesión.

## Tiempos observados

| Evento/intervalo | ms |
| --- | ---: |
| Invocación → main.entry | 1522,44 |
| Invocación → ventana mostrada | 1864,39 |
| Invocación → original enviado | **1995,63** |
| main.entry → original enviado | **473,18** |
| Enumeración y ordenación de carpeta | 277,89 |
| Inicialización completa del renderer | 254,98 |
| D3D11CreateDevice dentro del renderer | 248,66 |
| Decodificación del original | 76,90 |
| Subida GPU del original | 27,96 |

La enumeración empezó a 1535,64 ms y acabó a 1813,53 ms desde invocación.
El renderer empezó a 1592,35 ms y acabó a 1847,33 ms: unos **221 ms de solapamiento
real**. Solo hubo un escaneo. Se mantiene el original completo, sin cambiar efectos,
calidad ni backend. La decodificación seleccionada aún espera a la publicación UI
del snapshot después del renderer.

El original se marcó tras Present1 correcto (present ID 10). No se extrajo en esta
porción la confirmación DXGI de presentación física; 1995,63 ms es envío, no una
certificación visual del instante en pantalla.

## Espera previa e integridad

Get-MpPerformanceReport sobre el ETL registra FileScan del EXE de **1385,61 ms**
y RealTimeScan OnOpen previo de 40,82 ms. El FileScan cae dentro de la demora
anterior a main.entry. Esto corrobora actividad importante de Defender, pero no
se analizó aquí el stack/CSwitch para atribuir causalmente cada milisegundo de los
1522,44 ms. No sustituir esta distinción por la atribución completa de la captura
histórica, ni restar automáticamente duraciones solapadas. No se cambió seguridad.

ETW: cero eventos/buffers perdidos; QPC: cero eventos descartados. Auditoría del
FileIO del PID del visor durante toda la traza en la carpeta acordada: cero Write,
Delete, Rename, SetInfo, SetEA o SetSecurity. Los FileIoCreate observados son aperturas,
no prueba de creación de archivos. No se hizo hash completo antes/después de medios.
Las escrituras del producto se mantienen fuera de la carpeta multimedia.

## Interpretación y estado

La captura histórica del candidato anterior registró 2215,70 ms totales y 681,21 ms
después de main.entry. La nueva observación es aproximadamente 220 ms menor en total
y 208 ms menor dentro de la aplicación. **Es una diferencia observada entre dos
sesiones, no un porcentaje causal certificado**: difieren EXE, settings/staging,
sesión/caché y flag multiple, además de la variabilidad del boot/almacenamiento.
La evidencia más directa de esta intervención es el solapamiento registrado, que
sí ocurrió con enumeración fría costosa; la serie preparada no lo demostraba como
ganancia de latencia.

No se eliminaron los 789,46 ms hipotéticos ni se acredita 100 ms. El coste dominante
total sigue siendo anterior a main.entry; dentro de la aplicación domina crear D3D
y la decodificación/subida aún posteriores. No hay aceptación del propietario ni
cierre de 003, y esta medición no constituye distribución estadística (n=1).
No se creó paquete nuevo ni se reemplazó el candidato anterior de dist.

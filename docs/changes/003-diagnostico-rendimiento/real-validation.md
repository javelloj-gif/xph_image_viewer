# Validación real posterior a 005 — 2026-10-22

Seguimiento posterior: [reproducción aislada AR-008](ar008-reproduction.md). Las
observaciones de este documento son históricas; seis nuevos arranques produjeron
JPEG sin F11/resize y permitieron constatar discrepancia de captura/oclusión.
No se reprodujo un fallo persistente del producto; no se modificaron fuentes.

Evidencia de 003; [resultado](result.md), [plan](plan.md), [revisión](review.md).
Sin cambios de fuentes ni recompilación. Git: ac4a5da más R1/R2/003/004/005 sin commit.
EXE Release SHA-256 `400045F207A8775598368A5B5430BED2C33C117C27DC3BB97A722934C544A5C4`,
igual al validado en 005. CTest 4/4 es evidencia previa, no ejecución nueva.

## Entorno, datos y protocolo

CIM informó AMD Ryzen 7 5700X3D, 8 núcleos/16 procesadores lógicos y NVIDIA GeForce
RTX 4070 Ti SUPER. No se comprobó qué adaptador eligió D3D. El log confirma dispositivo
feature level B100 y swapchain; no se usa el WARP del harness. DPI informado: 168.
No ficha de almacenamiento ni códecs opcionales. El propietario confirmó uso anterior
de LenteUV en esta sesión: ninguna apertura es primera real, ni se midió latencia
de arranque. Caso lento habitual aún pendiente de concretar.

Cuatro directorios primary/multiple/lite/failure bajo build/validation-003-<id>, cada
uno con copia del EXE, locales, imágenes y user_data propios. Settings de
packaging/default-settings.json con fullscreen=false, opacidad=1, enable_file_log=true
y debug_ipc=true; instancia única=true, cierre a bandeja=false, sesión vacía inicial.
No datos personales ni cambios de asociaciones. Solo se abrió la pipe productiva tras
comprobar fuera del sandbox ausencia de procesos LenteUV y del endpoint. Se verificó
la ruta del PID propio antes de enviar. El sandbox no veía los procesos externos:
su ausencia allí no demostraba que hubieran terminado.

Fixtures copiados: noname.jpg, 550×368, SHA-256
`DDBDB9CDB5F109C567D2AAFD076288CD57187E7A97D94898C72591DBFE28235C`;
linux.png, 1979×2186, renombrado imagen Ω.png, SHA-256
`6EF38C960CE9019E5F2A0D08FCA6A60930267128C25D583D6426141FCFAE9264`.
Settings finales: `250201A6240E669274B07AFED3D622E2F70880FA16CB9FDB8E5583E1CCB1D58F`.
Logs, capturas y muestras quedan locales e ignorados; solo se versiona este resumen.

## Aperturas y presentación

| Escenario | Evidencia observada |
| --- | --- |
| Arranque normal, --windowed --opaque seed.jpg | PID 4704, renderer/swapchain y carga correcta en log. Start-Process Hidden impidió comprobar la ventana inicial; no fue un crash |
| Segunda invocación con imagen Ω.png | Emisor PID 36724, salida real 0. Receptor conserva PID 4704; tras activar su ventana la captura muestra Tux y título con PNG/dimensiones |
| --multiple --windowed --opaque seed.jpg | PID 31768 y ventana independientes; renderer/swapchain y JPEG de paisaje presentado |
| --lite --windowed --opaque seed.jpg | PID 33520 y ventana independientes; dos capturas iniciales negras en área cliente. F11 mostró JPEG; al volver a ventana permaneció visible. No se certifica presentación inicial sin intervención |
| Receptor tras los flags | Conservó PID e imagen PNG; captura confirma Tux. Logs de los flags sin inicialización IPC |
| Pipe ocupada controlada, ejecutable real | Sin procesos previos, NamedPipeServerStream de una instancia y cliente retenido ocupan lenteuv. PID 35580 muestra diálogo «LenteUV — Error de envío», texto Win32 231. Aceptar y WaitForExit registran salida 2 real de wWinMain |
| Limpieza | Escape en las tres ventanas propias; logs de cierre completo, centinela/join del receptor. Tras aceptar error: cero procesos LenteUV y pipe productiva ausente |

UI mediante habilidad computer-use y sky.list_windows/activate_window/get_window_state,
con entradas dirigidas a la ventana retornada. Una captura del receptor sin activarlo
mostró otra aplicación ocluyente y se descartó como evidencia visual. Solo capturas
activadas y con contenido verificable sustentan presentación. La observación negra
de --lite queda como indicio AR-008, sin diferenciar todavía captura/oclusión/redraw.
No se atribuye a IPC ni se corrige incidentalmente. Invocación directa con ruta:
Explorer y asociaciones de archivo no se probaron.

## Reposo acotado (AC-04 parcial)

PNG estático visible, solo PID 4704 de LenteUV; sin interacción ni nuevas pruebas UI
durante la serie válida. Asentamiento adicional de 5 s y tres intervalos de 10 s:
Stopwatch y Process.Refresh/TotalProcessorTime/WorkingSet64/PrivateMemorySize64.
CPU un núcleo = 100·delta CPU/segundos; normalizada = anterior/16. JSON local:
primary/idle-samples.json. La serie inicial solapada con arranques de los flags se
descartó y se conserva como idle-samples-discarded-overlap.json.

| Muestra | Duración s | CPU s | CPU un núcleo % | CPU normalizada % | Working set MiB | Privada MiB |
| --- | --- | --- | --- | --- | --- | --- |
| 1 | 10,00245 | 0,28125 | 2,812 | 0,176 | 86,762 | 108,895 |
| 2 | 9,99410 | 0,56250 | 5,628 | 0,352 | 86,762 | 108,895 |
| 3 | 10,00849 | 0,46875 | 4,684 | 0,293 | 86,762 | 108,895 |

Serie válida: 10:44:16–10:44:46 America/Bogota (JSON en UTC), 2026-10-22.
Se conservaron aplicaciones concurrentes del usuario. Diagnóstico activo incluye
escritura por frame, coste no aislado. Sin GPU/VRAM, energía, perfiles, control de
frecuencia ni repetición en otra sesión. RAM: instantánea al terminar cada intervalo,
no pico. No prueba reposo cero, fuga ni consumo típico. Sin comparación ni mejora.

## Aceptación y continuación

AC-01 parcial: ficha/protocolo de esta muestra, falta caso habitual/almacenamiento.
AC-02 pendiente: primera real/posteriores medidos. AC-03 ampliado con presentación,
flags y fallo real; Explorer y presentación inicial --lite pendientes. AC-04 parcial:
CPU/memoria instrumentadas, sin GPU/VRAM. AC-05 parcial: revisión e indicio AR-008
registrados; 003 permanece abierto. Sin cambios de fuentes, ACK, elección, cierre,
Library ni contratos persistidos. Siguiente: separar captura de defecto real de
presentación y coordinar caso habitual/primera ejecución en otra sesión.

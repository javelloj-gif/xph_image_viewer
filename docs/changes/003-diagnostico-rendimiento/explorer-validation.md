# Explorer, asociación y muestra de reposo sin logging

Fecha: 2026-10-22. Estado: porción ejecutada con ayuda manual para Abrir con;
003 permanece abierto. [Plan](plan.md), [revisión](review.md),
[protocolo de primera sesión](first-session-protocol.md).

## Caso reportado y binarios

El propietario confirmó apertura habitual por doble clic en imagen, mediante la
asociación de Windows. Reportes en carpetas con menos de 100 y con más de 1000
elementos, formatos/tamaños múltiples. Es reporte, no reproducción ni estadística.
Faltan archivo/carpeta representativos y distinguir espera anterior a ventana o a
imagen. El usuario ya había usado LenteUV en esta sesión: no primera ejecución real.

Lectura fuera del sandbox: UserChoice de .jpg y .png = Applications\lenteuv.exe;
su comando apunta al portable habitual fuera del checkout. SHA-256 del EXE asociado:
`15F90CCD654C7B9ECA4F3C121F8AFDD7A1182195DAD3A52536B9454F6FD03097`.
Build 005: `400045F207A8775598368A5B5430BED2C33C117C27DC3BB97A722934C544A5C4`.
Los hashes difieren; no se conoce la revisión del portable por ese dato ni se afirma
que carezca de una corrección concreta. No extrapolar validación de 005 al doble clic
con el portable habitual. No se sustituyó ese EXE ni se usaron sus datos personales.

## Preparación y entrada Shell

Git comprobado: ac4a5da/codex/repository-foundation más diff R1/R2/003/004/005 preservado,
sin commit. Sin fuentes modificadas, build ni CTest nuevos; sigue evidencia previa 4/4.
Se creó build/explorer-003-<id> con copia idéntica de 005 renombrada
lenteuv_003_explorer.exe, locales, user_data y carpeta «Caso Explorer Ω». Nombre
distinto para no reutilizar Applications\lenteuv.exe del portable habitual.
Fixtures: 01-paisaje.jpg (noname.jpg, 550×368) y 02-imagen Ω.png (linux.png, 1979×2186).
Hashes de fixtures iguales a real-validation.md. Settings: packaging defaults con
fullscreen=false, opacidad=1, enable_file_log=false, debug_ipc=false y sesión vacía.
No flags: AppConfig conserva frameless/translucent=true; difiere de la muestra
anterior --windowed --opaque. Logging desactivado porque Explorer controla cwd.

Preflight fuera del sandbox: ningún LenteUV ni pipe productiva. UI con computer-use:
pestaña propia de Explorer y Abrir con → Elegir otra aplicación. El selector se
mostró, pero no ofreció ventana controlable al helper; error de recuperación:
`foreground window did not report a process id`. Una consulta UIA también dejó de
producir estado útil. No se recurrió a automatización nativa alternativa.

El usuario completó selección del EXE aislado con Solo una vez, nunca Siempre,
primero para JPEG y luego para PNG, dejando el receptor abierto. Confirmó visor
abierto y después «Hecho; veo Tux». El agente observó las imágenes al activar la
ventana; capturas ocluidas por Codex se descartan como evidencia de presentación.

## Evidencia de aplicación y preservación

| Comprobación | Resultado |
| --- | --- |
| Primera apertura JPEG desde Abrir con | Proceso 13188, ruta exacta del EXE aislado y CommandLine con JPEG/espacios/Ω; captura activa muestra paisaje |
| Segunda apertura PNG desde Abrir con | Mismo HWND 9308684 y mismo PID 13188 al medir; un único proceso lenteuv* activo. Título PNG y captura activa con Tux; confirmación nativa del usuario |
| Emisor transitorio PNG | No se capturó PID ni exit code; no se inventa un 0. Evidencia de aplicación = imagen nueva en receptor propio, no salida inferida |
| UserChoice y comando JPG/PNG | Snapshot anterior, intermedio y posterior idénticos; no cambio de predeterminados. No se certifica ausencia de escrituras MRU/OpenWith de Windows |
| Cierre | Escape solo al visor propio; proceso 13188 termina. Código de salida no disponible al observar Process externamente; no se considera 0 por ausencia de proceso |
| Limpieza | Verificación externa: cero procesos lenteuv* y pipe ausente. La ventana Explorer original ya no figuraba en inventario; se dejó la ventana nueva de la carpeta de ensayo que apareció durante la intervención manual |

La entrada Shell queda comprobada con elección de una sola apertura, no como doble
clic usando la asociación predeterminada del build nuevo. No se modificó esa asociación
para fabricar equivalencia. No hay ACK ni conteos exactos en esta sesión sin logging:
la regresión de duplicados sigue apoyada en 005, no en el recuento de ventanas.

## CPU, memoria y contadores GPU sin logging

Get-Counter -ListSet '*GPU*' falló dentro del sandbox con c0000bc8; la misma consulta
fuera enumeró GPU Engine y GPU Process Memory. Es diferencia del entorno, no fallo
del producto ni ausencia de hardware. No se instalaron instrumentos ni cambiaron
permisos/configuración del sistema. Entorno hardware de real-validation.md.

Con PNG estático en el PID 13188 y logging=false, asentamiento adicional de 5 s.
Tres ventanas: Get-Counter -SampleInterval 1 -MaxSamples 11 para
GPU Engine/Utilization Percentage y GPU Process Memory/Dedicated Usage/Shared Usage.
Filtrar InstanceName por pid_13188_, conservar Status y CookedValue. Cada ventana
produjo 187 muestras propias (15 engines y dos gauges × 11), todas Status=0; cero
errores de consulta. Un adaptador/phys_0 por las instancias de este PID; no mapeo
instrumentado de LUID a la GPU comercial. JSON locales idle-no-logging.json y
gpu-own-samples.json; solo datos propios persistidos.

CPU con TotalProcessorTime/Stopwatch, WS/private bytes tras Process.Refresh.
CPU un núcleo = 100·delta CPU/segundos; normalizada = anterior/16. Los tiempos reales
incluyen consulta/inicialización de PDH; no se fuerzan a 10 s. Los 11 puntos GPU
abarcan aproximadamente 10 s por ventana, no exactamente todo el intervalo CPU.
Sin acciones UI del agente durante la serie; foco/oclusión no registrados continuamente.

| Ventana | Duración CPU s | CPU un núcleo % | CPU normalizada % | WS MiB | Privada MiB | Media engine 3D % |
| --- | --- | --- | --- | --- | --- | --- |
| 1 | 13,4722 | 4,1753 | 0,2610 | 86,7773 | 108,3125 | 2,7541 |
| 2 | 11,4406 | 2,0486 | 0,1280 | 86,7773 | 108,3125 | 2,4059 |
| 3 | 11,4463 | 4,3682 | 0,2730 | 86,7930 | 108,3438 | 2,4945 |

En el último punto de cada ventana, Dedicated Usage=53 919 744 bytes (51,421875 MiB),
Shared Usage=4 464 640 bytes (4,2578125 MiB), Status=0. Engine Copy 4 tuvo media
0,00330–0,00338 %; los otros 13 engines, 0 en estas muestras. El máximo 3D muestreado
fue 3,7712 %. No sumar porcentajes de engines ni presentar la media 3D como total GPU.
Dedicated/Shared son esos contadores de proceso: no presupuesto, pico, residencia
física garantizada de VRAM ni memoria de toda la máquina. WS/privada son instantáneas.
Timestamps GPU: 11:27:59–11:28:33 America/Bogota (JSON UTC), 2026-10-22.

No comparación con la serie instrumentada: flags/ventana, logging, instrumentos y
duraciones difieren. Coste de observar no aislado, aplicaciones ajenas concurrentes,
una sola sesión/configuración. No prueba consumo típico, fuga, causa del arranque
lento ni mejora de rendimiento.

## Estado de aceptación

AC-01 parcial: entrada habitual/cantidades y binarios identificados; faltan datos
representativos y fase de demora. AC-02 pendiente, protocolo de otra sesión preparado.
AC-03 ampliado: Shell/Abrir con y JPEG→PNG aplicado en receptor propio, predeterminados
conservados; doble clic del nuevo build por default no probado. AC-04: base acotada
obtenida para esta configuración, con CPU/memoria/GPU por PID sin logging y límites
del instrumento explícitos; no equivale a campaña general de consumo o residencia.
AC-05 parcial: revisión/evidencia y roadmap actualizados.
Sin hallazgos nuevos confirmados de código; 003 continúa abierto.

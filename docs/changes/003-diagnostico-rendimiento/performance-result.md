# 003: comparación de arranque y miniaturas

Resultado, 2026-10-24: el original completo mejora aproximadamente 20–22% en mediana,
pero no acredita la meta de 100 ms. La caché preparada acelera Waterfall; su primera
generación tiene un coste mayor que la base. Series del 23–24, America/Bogota.
Los 500 ms de apertura y 1–5 s de miniaturas iniciales son estimaciones del
propietario, separadas de la instrumentación siguiente.

## Revisión, entorno y muestra

Base funcional 9b1d7e8; base instrumentada reproducible 5d1f67f en
`codex/003-instrumented-baseline`, sin optimizaciones de comportamiento.
La implementación reside en `codex/003-arranque-cache`. Cada manifest local
identifica revisión, SHA256 del EXE, settings, entradas y hashes de fotografías.
El candidato aceptado 002 se conserva intacto y no se utiliza como base medida.

Windows 10.0.26300.0 x64; AMD Ryzen 7 5700X3D, 16 procesadores lógicos;
68 638 838 784 bytes de RAM física reconocida por el sistema; NVIDIA GeForce RTX
4070 Ti SUPER (identificación local del driver). Release/MSVC 19.43, SDK 22621.
Fullscreen y mismo monitor, miniatura 50, orden 0, opacidad 0,85. Logging de
producto desactivado, trazado QPC activado, instancia única desactivada para
aislar cada proceso. No se cambian asociaciones de Windows.

Caso real autorizado: 7749 elementos, 6694 JPEG. Lecturas de originales en F:;
settings, sesión, cachés y resultados exclusivamente en árboles nuevos de build.
Selección por proximidad al inicio/centro/final, tamaño 4,25–5,75 MB y resolución
comparable (±10%). Índices ordinales sin distinguir mayúsculas, base cero:

| Etiqueta | Índice | Dimensiones | Bytes |
| --- | --- | --- | --- |
| A | 1 | 3072 × 4096 | 5587922 |
| B | 3849 | 4080 × 3072 | 4723424 |
| C | 6647 | 3072 × 4096 | 4398799 |

Estos JPEG son aproximadamente 12,5 MP, no 24 MP. No se publica el nombre ni
el contenido privado; los hashes/nombres completos permanecen en manifests locales.

## Método e interpretación

30 aperturas posteriores por posición, versión y escenario; un warmup excluido
por posición. Mediana convencional y p95 por rango más próximo (posición 29/30),
máximo y n explícitos. La caché preparada es propia de cada escenario. Las pruebas
de caché de aplicación vacía usan namespaces nuevos, sin borrar cachés del SO.
Las muestras de recursos y reutilización en el mismo proceso son series separadas.
No se ejecutan compilaciones/tests intensivos simultáneamente con la serie usada.

QPC distingue invocación del ejecutor, creación Win32 de proceso y entrada en
wWinMain. El frame original se marca únicamente cuando DrawBitmap lo dibujó y
Present1 devolvió S_OK. La confirmación DXGI exige el mismo present ID; el instante
en que se observa es una cota superior, no tiempo óptico del panel. Un HWND,
WaitForInputIdle o Present1 por sí solo no prueban visibilidad. Zoom/paneo y calidad
se revisan además funcionalmente; la métrica no certifica toda la interacción.

Tira: primera miniatura GPU lista desde solicitud; área visible desde original
enviado hasta un frame con sus texturas completas y desplazamiento estabilizado.
Waterfall: primera miniatura GPU lista y área visible completa, con geometría
estable en dos frames, desde la demanda de ese modo. Son métricas diferentes.
Un timeout sin frame de área completa permanece sin medida de terminación;
no se convierte en un tiempo de 10 s ni se excluye silenciosamente del resultado.

E/S es lógica del proceso (GetProcessIoCounters), no lectura física del disco.
El contador de decode registra intentos iniciados, incluidos los cancelados; no
equivale al número de originales completamente leídos/decodificados.
RAM es peak working set observado y CPU es tiempo acumulado muestreado; estas
medidas incluyen el trabajo hasta la salida del escenario, cuyas duraciones
pueden diferir. GPU se toma por PID en procesos separados de 15 s, a partir del
segundo 3; uso dedicado/compartido atribuido no equivale a residencia física
completa. Falta de contador se informa como no disponible, nunca como cero.

## Series exploratorias conservadas

Las primeras series tenían tamaños distintos o actividad de compilación y no se
usan para afirmar mejoras causales. La serie 18184d1 detectó un criterio de tira
demasiado estricto ante plateau float lejano; se corrigió solamente la medición,
en ambas versiones. La serie 34f39ca permitió identificar coste de recencia
síncrona; la implementación final agrupa esos UPDATE en el writer. Sus cifras
previas del candidato no se atribuyen al candidato final.

## Límites de aceptación

La primera apertura real de una nueva sesión de Windows está pendiente de
coordinación; estas series son posteriores en una sesión ya utilizada. No se
vacían caches del SO, reinicia Windows ni instala un proceso residente.
La aceptación visual del propietario sobre 003 también está pendiente según
[functional-test.md](functional-test.md). Se conserva el fallo intermitente
IPC-003-CONCURRENT-01 del harness, distinto del reporte manual 014 cerrado.

## Resultados instrumentados

Todos los tríos siguientes son **mediana / p95 / máximo**, en ms salvo indicación.
Cada celda de latencia principal tiene n=30. Las versiones comparten hashes de
configuración y de los tres originales; se comprobó esa igualdad antes del resumen.

### Original completo, desde la invocación

| Posición | Base | 003 preparada | 003 vacía |
| --- | --- | --- | --- |
| A | 385.73 / 402.38 / 403.68 | 299.70 / 327.00 / 330.39 | 300.87 / 319.15 / 336.54 |
| B | 376.23 / 390.51 / 401.82 | 297.55 / 318.77 / 327.29 | 296.45 / 327.46 / 352.86 |
| C | 369.05 / 382.19 / 391.92 | 297.23 / 325.27 / 327.28 | 297.29 / 305.86 / 315.56 |

Es envío del frame del original. La cota superior de confirmación DXGI del mismo
present ID, también desde invocación (30/30 por posición en Viewer), fue:

| Posición | Base confirmada | 003 preparada confirmada |
| --- | --- | --- |
| A | 402.59 / 418.39 / 424.13 | 314.81 / 337.41 / 341.01 |
| B | 394.58 / 410.28 / 417.74 | 311.36 / 337.59 / 340.10 |
| C | 386.65 / 402.73 / 410.43 | 310.81 / 335.38 / 343.05 |

Desde creación Win32 del proceso y desde entrada en wWinMain, respectivamente:

| Posición | Base creación | 003 creación | Base entrada | 003 entrada |
| --- | --- | --- | --- | --- |
| A | 385.37 / 402.00 / 403.30 | 299.28 / 326.62 / 330.04 | 368.09 / 384.67 / 384.69 | 282.03 / 309.19 / 312.84 |
| B | 375.85 / 390.15 / 401.31 | 297.18 / 318.41 / 326.92 | 358.89 / 373.45 / 382.03 | 279.58 / 300.74 / 309.42 |
| C | 368.67 / 381.84 / 391.56 | 296.83 / 324.92 / 326.90 | 351.44 / 365.26 / 374.57 | 279.52 / 307.15 / 309.84 |

**100 ms no acreditados.** En 003 preparada la inicialización del renderer tiene
medianas de 218–221 ms; la decodificación original, 61–77 ms, se solapa con ella;
la subida original ronda 17–18 ms. No se deben sumar etapas que se solapan. El
renderer es el coste dominante medido; esta traza no desglosa sus llamadas internas
ni atribuye la demora a un driver concreto. No se reduce resolución para cumplir la meta.

### Navegación y tira

Navegación disponible desde invocación, independiente del primer original:

| Posición | Base | 003 preparada |
| --- | --- | --- |
| A | 372.55 / 389.32 / 390.62 | 306.75 / 335.02 / 338.28 |
| B | 362.17 / 377.22 / 388.37 | 303.87 / 326.90 / 335.63 |
| C | 356.35 / 369.43 / 379.03 | 304.67 / 333.14 / 337.51 |

Tira: destino preparado y área completa desde el original enviado. La animación
conservada explica que preparar sus píxeles tarde menos que terminar de desplazarla.

| Posición | 003 destino preparado | Base área | 003 preparada área | 003 vacía área |
| --- | --- | --- | --- | --- |
| A | 19.65 / 25.57 / 25.74 | 412.33 / 415.34 / 415.70 | 398.38 / 401.44 / 401.85 | 399.58 / 530.52 / 1214.88 |
| B | 24.79 / 30.56 / 35.75 | sin terminación observada (0/30) | 946.26 / 950.84 / 952.04 | 948.47 / 953.77 / 1381.32 |
| C | 27.11 / 30.82 / 31.51 | sin terminación observada (0/30) | 957.40 / 961.01 / 961.11 | 956.67 / 962.30 / 975.34 |

Base B y C: 30/30 sin marca de terminación dentro de la ventana de seguridad de
10 s; son observaciones censuradas, sin mediana/p95 de terminación. No se sustituyen
por 10 s. Se detectaron colas de miniaturas intermedias que seguían consumiendo
trabajo. La posición/área se estabiliza; este criterio no exige terminar toda la
transición de opacidad ni demuestra píxeles físicos del monitor.

### Waterfall

Se mide la entrada al viewport superior, con columnas automáticas iguales en ambas
versiones; las tres imágenes iniciales no representan tres posiciones de scroll.

| Posición de apertura | Base área | 003 preparada área | 003 vacía área |
| --- | --- | --- | --- |
| A | 672.13 / 687.54 / 704.63 | 62.50 / 67.23 / 306.82 | 1018.82 / 1220.12 / 1227.34 |
| B | 672.55 / 683.92 / 690.09 | 63.04 / 72.12 / 78.88 | 973.74 / 1951.72 / 1970.29 |
| C | 671.78 / 680.74 / 684.44 | 62.47 / 68.73 / 69.67 | 980.23 / 1851.79 / 1966.62 |

La mejora preparada es clara, pero **Waterfall vacío empeora**: medianas cercanas a
1 s frente a 0,67 s y p95 de hasta 1,95 s. Vacía incluye crear SQLite nuevo, generar
miniaturas y empezar su persistencia; se mantienen tres workers de miniaturas para
reservar capacidad al original. No se atribuye todo el aumento a una sola causa ni
se presenta esta entrega como superior en toda condición. El máximo preparado de
A (306,82 ms) se conserva, aunque su p95 sea menor.

Primera miniatura GPU lista desde solicitud (no es confirmación de pantalla):

| Posición | Base tira | 003 preparada tira | Base Waterfall | 003 preparada Waterfall |
| --- | --- | --- | --- | --- |
| A | 36.95 / 40.39 / 42.64 | 1.52 / 1.83 / 1.86 | 37.96 / 41.86 / 42.40 | 3.32 / 4.06 / 243.32 |
| B | 31.73 / 56.15 / 72.69 | 1.48 / 1.85 / 1.99 | 35.58 / 39.69 / 43.67 | 3.27 / 3.68 / 4.17 |
| C | 29.83 / 58.06 / 61.72 | 1.70 / 2.20 / 2.38 | 36.07 / 46.84 / 47.23 | 3.37 / 7.52 / 9.40 |

### Trabajo, E/S y memoria

Medianas por apertura preparada (n=30 por fila). Incluyen el trabajo hasta la salida
del escenario; la base B/C de tira permaneció unos 10 s sin completar, mientras 003
salió antes. No son tasas por segundo ni una comparación universal de reposo.

| Escenario/posición | Intentos decode base/003 | Lectura lógica MiB base/003 | Escritura lógica MiB base/003 | CPU ms base/003 | Peak WS MiB base/003 |
| --- | --- | --- | --- | --- | --- |
| viewer A | 1.00 / 1.00 | 5.38 / 5.97 | 0.00 / 0.03 | 414.06 / 429.69 | 130.68 / 132.10 |
| viewer B | 1.00 / 1.00 | 4.56 / 5.75 | 0.00 / 0.04 | 453.12 / 507.81 | 131.05 / 133.24 |
| viewer C | 1.00 / 1.00 | 3.56 / 4.72 | 0.00 / 0.01 | 390.62 / 406.25 | 130.69 / 132.25 |
| strip A | 29.00 / 1.00 | 87.92 / 6.46 | 0.01 / 0.25 | 1351.56 / 476.56 | 133.02 / 133.80 |
| strip B | 1027.00 / 42.50 | 3658.50 / 117.28 | 0.11 / 0.50 | 41632.81 / 1914.06 | 137.46 / 163.20 |
| strip C | 855.00 / 29.50 | 3209.57 / 134.59 | 0.19 / 0.57 | 38250.00 / 1890.62 | 141.01 / 135.87 |
| waterfall A | 77.00 / 1.00 | 321.77 / 13.88 | 0.00 / 0.50 | 3937.50 / 703.12 | 146.87 / 148.90 |
| waterfall B | 77.00 / 1.00 | 320.95 / 13.06 | 0.00 / 0.50 | 3929.69 / 750.00 | 145.76 / 147.49 |
| waterfall C | 77.00 / 1.00 | 319.95 / 12.06 | 0.00 / 0.50 | 3953.12 / 695.31 | 146.76 / 148.85 |

Viewer preparado mantiene una decodificación original por apertura y prepara
28–53 miniaturas desde caché. La tira conserva solicitudes de posiciones visibles
intermedias durante la animación: puede tener misses, aunque el destino ya esté
preparado. No se precalcula toda la carpeta para evitar esos misses. Waterfall
preparado tiene mediana de un intento original, frente a 77 intentos totales en la
base; las cancelaciones/fallos se cuentan aparte en CSV. No hubo fallos de miniaturas
en las series principales.

Los procesos de recursos se mantuvieron 15 s tras el primer frame, con **caché
privada de primer uso** (n=1 por posición/versión, no p95 significativo):

| Posición | CPU ms base/003 | Peak WS MiB base/003 | Lectura lógica MiB base/003 | Escritura lógica MiB base/003 |
| --- | --- | --- | --- | --- |
| A | 1234.38 / 1578.12 | 130.93 / 128.19 | 5.38 / 87.93 | 0.00 / 1.74 |
| B | 1015.62 / 2984.38 | 126.83 / 130.78 | 4.56 / 181.49 | 0.00 / 3.57 |
| C | 2953.12 / 5843.75 | 130.57 / 139.39 | 3.56 / 148.55 | 0.00 / 3.47 |

Aquí 003 sí hace más trabajo que la base: prepara 28–53 miniaturas después de mostrar
el original. Su CPU/E/S inicial adicional queda registrada; no se vende el arranque
asíncrono como eliminación de ese trabajo. El trazado QPC está activado en ambas
versiones; esta serie incluye arranque, preparación y renderizado, no certifica
consumo habitual en reposo sin diagnóstico ni un presupuesto global de RAM/VRAM.

GPU por PID (mediana / máximo; 5–6 muestras válidas por celda):

| Versión/posición | 3D % | Dedicada MiB | Compartida MiB |
| --- | --- | --- | --- |
| base A | 3.52 / 4.04 | 96.43 / 96.43 | 3.98 / 3.98 |
| base B | 1.81 / 4.17 | 96.24 / 96.24 | 4.45 / 4.72 |
| base C | 13.97 / 15.21 | 96.42 / 96.42 | 3.98 / 3.98 |
| candidate A | 4.01 / 4.78 | 98.12 / 98.12 | 4.09 / 4.09 |
| candidate B | 2.45 / 4.09 | 99.51 / 99.51 | 4.39 / 4.39 |
| candidate C | 14.20 / 15.68 | 99.59 / 99.59 | 4.21 / 4.21 |

Cada archivo GPU contiene además una lectura final inválida, excluida y conservada
en crudo. Los valores atribuidos no prueban residencia física completa. La memoria
dedicada sube unos 2–3 MiB en esta muestra; no se afirma reducción global de GPU.
Las cachés preparadas de estos escenarios ocupan aproximadamente 3,1 MB (tira con
tres destinos) y 4,5 MB (Waterfall de entrada), no toda la colección. En las series
vacías, el total de disco incluye muchos namespaces independientes del ejecutor y
no representa la huella de una sola caché normal. Se verificó LRU bajo una cuota
inyectada de 4 MiB; no se llenó un volumen real ni se cambiaron ACL de fotografías.

### Identidad y reproducción

- Base 5d1f67ff48bdbdcf09b49fbe3a87d7d41370e992; EXE
  `A12448EED6A1A01872449AF03F5800097FAE52A5174FD04646936FDD30274628`.
- Fuentes 003 b0e98f8933729027f1620b4c671f74ec604ed518; EXE
  `A8C54E3B42B5811EA17CCF347AA91A7FFF4C2808BF19EE45BD4E6C7F0AF04213`.
- Base válida: `build/perf-003-final-34f39ca/base-*-warm`. Las series del
  candidato 34f39ca/cc5db31 son exploratorias y no se mezclan con b0e98f8.
- 003 preparada/recursos: `build/perf-003-final-b0e98f8`. Vacía n=30:
  `build/perf-003-empty30-b0e98f8`. Los ensayos vacíos iniciales n=3 se excluyen.
- Manifests, CSV, traces, contadores y hashes de datos quedan locales. El campo
  Dirty puede reflejar documentación/ejecutores en curso; no hubo cambios de
  fuentes productivas ni recompilación durante las series usadas.

Reconstruir la base desde `codex/003-instrumented-baseline` en un checkout aislado;
compilar Release de ambos árboles. Ejecutar la matriz secuencial sin builds/tests
simultáneos, usando rutas absolutas y un RunRoot nuevo bajo build:

```powershell
./scripts/run_startup_comparison.ps1 -ReferenceExecutable <EXE-base> -ReferenceRevision 5d1f67ff48bdbdcf09b49fbe3a87d7d41370e992 -CandidateExecutable <EXE-003> -CandidateRevision b0e98f8933729027f1620b4c671f74ec604ed518 -Images <coleccion-local> -RunRoot <nuevo-bajo-build> -Repetitions 30 -ColdRepetitions 30
```

Para otro build usar su revisión/hash real, no reutilizar automáticamente la etiqueta
anterior. El ejecutor no cambia asociaciones, reinicia Windows ni purga cachés del SO.

### Reutilización dentro del proceso

Diez ciclos por posición, modo y versión, con los dos viewports previamente
preparados (120 observaciones válidas). Demanda mediante mensajes nativos al HWND
propio; frame enviado con el área completa. No representa otra apertura de proceso
ni una medición óptica. No hubo nuevas decodificaciones, hits de disco ni fallos
durante esos ciclos: las texturas en memoria ya resolvían ambos casos.

| Posición | Base tira | 003 tira | Base Waterfall | 003 Waterfall |
| --- | --- | --- | --- | --- |
| A | 11.02 / 12.74 / 12.74 | 9.98 / 12.93 / 12.93 | 14.21 / 18.56 / 18.56 | 12.74 / 16.09 / 16.09 |
| B | 8.94 / 12.09 / 12.09 | 11.11 / 13.57 / 13.57 | 13.79 / 21.80 / 21.80 | 14.48 / 18.43 / 18.43 |
| C | 9.70 / 13.89 / 13.89 | 9.39 / 13.73 / 13.73 | 14.89 / 40.60 / 40.60 | 13.37 / 24.33 / 24.33 |

El primer intento de base C no tenía ambos viewports preparados tras 30 s: sus
20 filas se conservan aparte, con una tira sin entrega en la ventana de 350 ms y
58 intentos decode todavía activos. No se mezclan con reutilización caliente. Se
repitió solo esa posición con 60 s de preparación, obteniendo ambos viewports
listos antes de los ciclos y cero nuevas decodificaciones. Las demás posiciones
usaron 30 s. Esta preparación no se oculta como tiempo de reutilización.

Datos locales: `build/perf-003-reuse-b0e98f8` y complemento
`build/perf-003-reuse-base-C-b0e98f8`. Reproducir con
`scripts/run_startup_reuse_benchmark.ps1`, raíces de comparación correspondientes,
Repetitions=10 y PreparationSeconds=30; para base C, Variants=base, Positions=6647
y PreparationSeconds=60. Las trazas comprueban readiness anterior a los ciclos.

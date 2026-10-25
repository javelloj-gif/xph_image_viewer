# 003: primera apertura instrumentada y opciones — 2026-10-24

**Seguimiento posterior:** [ETW en otra primera sesión](etw-result.md) mide original
enviado 2216 ms y DXGI observado 2237 ms; identifica espera del creador de 1426 ms
en Defender con FileScan del EXE de 1408 ms. Directorio explícito frente a la apertura
sin argumentos inferior: no comparación causal de versiones. Este documento conserva
la evidencia y las hipótesis de la captura QPC anterior, sin reatribuir sus tiempos.

## Resultado y condiciones

Se realizó **una sola invocación**, sin calentamiento, repetición, compilación ni
ejecución previa del visor por el agente. El propietario indicó que Windows había
sido reiniciado y LenteUV no se había abierto. Comprobación fuera del sandbox:
cero procesos lenteuv; último arranque del SO 2026-10-24 01:53:38 America/Bogota.
Inicio de la medición aproximadamente 02:33:22 de esa misma zona horaria.

Candidato: `dist/lenteuv_qa_003_20261024_async_cache/lenteuv.exe`, fuentes b0e98f8,
SHA256 posterior a la medición
`A8C54E3B42B5811EA17CCF347AA91A7FFF4C2808BF19EE45BD4E6C7F0AF04213`.
No se copió ni calculó el hash del EXE antes de abrirlo. Se leyeron instrucciones,
configuración y sesión antes de lanzar; esto no garantiza cachés del SO intactas.
No se purgó ninguna caché ni se reinició/cerró sesión. Primera apertura del visor
en la sesión no significa primera actividad de GPU/disco de todo el sistema.

Se lanzó el EXE existente **sin argumentos**, desde su directorio, con configuración,
sesión y caché existentes. Viewer, fullscreen, orden por nombre, miniatura 50,
opacidad 0,85, instancia única activada y logging desactivado. Restauración de dos
carpetas: 6694 y 2872 imágenes admitidas. No se abrieron ni modificaron originales
para preparar la prueba. No es el escenario histórico de archivo explícito con
datos aislados: no se atribuye la diferencia exclusivamente al reinicio.

Instrumentación QPC ya incluida, salida absoluta y cierre normal 2500 ms después
del original enviado; límite de seguridad de 15 s desde el primer RenderFrame.
Sin forzar cambio de modo, sin namespace alternativo de caché, sin --multiple.
Salida 0, 0 eventos descartados; settings y sesión conservan hashes antes/después.
El proceso propio terminó; no se lanzó otro LenteUV para analizar esta captura.

Artefactos privados y ejecutor de un solo intento, con guard persistente, bajo
`build/first-session-003-20261024-single/`: trace.csv, resources.csv, manifest.json,
settings/session previos y measure-once.ps1. No versionarlos. El campo InvocationUtc
del manifest se escribió después de Process.Start; para el instante de invocación
se usa la correlación QPC/FILETIME, no ese rótulo. Incertidumbre de correlación
4430 microsegundos; no afecta las diferencias entre eventos del mismo reloj QPC.

## Línea de tiempo

Una observación (n=1), sin mediana ni p95. Milisegundos redondeados desde invocación:

| Evento | ms |
| --- | ---: |
| Creación Win32 del proceso, correlación FILETIME/QPC | 27 |
| Retorno de Process.Start al ejecutor | 1270 |
| Primer marcador main.entry | 1325 |
| Inicio del renderer | 1337 |
| Fin del renderer | 1544 |
| Solicitud de mostrar ventana | 1556 |
| Inicio de enumeración de la primera carpeta | 1558 |
| Fin de esa enumeración, incluida ordenación | 1783 |
| Solicitud del original | 1818 |
| Fin de decodificación del original | 1891 |
| Inicio / fin de subida del original | 1909 / 1928 |
| Frame con original enviado (Present1 S_OK) | **1932** |
| Primera solicitud de miniatura de la tira | 1932,35 (después del original) |
| Confirmación DXGI observada para el mismo present ID | **1956** |

Duraciones: renderer 207,80 ms; primera enumeración 224,31 ms (ordenación 3,01 ms);
decode original 72,74 ms; espera entre decode y upload 18,06 ms; upload 19,02 ms.
Segunda carpeta: enumeración 35,42 ms, en paralelo con aplicación del primer snapshot.
Apertura de SQLite: 13,48 ms, solapada con el renderer. Hubo 27 solicitudes de
miniatura, 0 hits y 27 intentos de escritura de caché durante la ejecución completa; no son
coste anterior al primer original ni prueba de que se borró la caché.

El primer HWND se observó por sondeo a 1570 ms; no se usa para certificar visibilidad.
DXGI es la cota superior de observación de la presentación correlacionada, no tiempo
óptico ni validación de zoom/paneo. Los múltiples arranques de unos 3000 ms reportados
por el propietario conservan su evidencia manual; este caso de 1932/1956 ms no los
desmiente ni permite calcular su dispersión. No se midió Waterfall en otra apertura.

## Causas delimitadas

1. **Intervalo dominante sin desglose: 1325 ms antes de main.entry**, aproximadamente
   69 % del tiempo hasta enviar el original. De ellos, unos 1298 ms son posteriores
   a la creación Win32. No es correcto atribuirlos al cuerpo de App::Initialize.
   El marcador está en wWinMainImpl: también quedan antes de él el loader/CRT,
   inicializadores globales, PreInitDebugIpc y la inicialización del propio recorder.
   Los imports PE son ordinarios, sin Delay Import Directory; incluyen D3D/D2D,
   DirectWrite, DirectComposition, SQLite, Cabinet, Shell/Property System y VC++.
   No se midió el coste individual de cargar esas DLL ni sus dependencias.
   E/S/fallos de página, comprobaciones de seguridad y planificación son hipótesis,
   no causas confirmadas. No hay ETW de stacks/disco/antimalware: el token no era
   administrador y no se obtuvo una captura kernel elevada antes de la invocación.
2. **Serialización confirmada en restauración sin argumentos.** App::Initialize
   llama LoadSession después de inicializar el renderer/páginas. CompleteFolder
   aplica catálogo y SetCurrentFolderIndex solicita entonces el original. Así se
   suman renderer y enumeración en este camino; poner la CPU fuera de UI no implica
   solapar estas etapas. El archivo explícito sí empieza a decodificar antes del
   renderer. La enumeración observada de 224 ms y los 35 ms hasta image.request son
   el siguiente frente concreto; los 35 ms incluyen planificación, publicación y
   preparación UI, sin atribución individual demostrada a SetFileList.
3. **Renderer y original tienen costes menores y cuantificados:** 208 ms de renderer,
   73 ms de decode y 19 ms de upload. La traza no separa D3D11CreateDevice,
   DirectWrite, swapchain y DirectComposition. No demuestra 2–3 s dentro del driver.
4. **Miniaturas no explican la espera inicial en este caso.** Las demandas de tira
   empiezan después del frame original y SQLite abre en paralelo en 13 ms. La mejora
   posterior de Waterfall sigue siendo válida; optimizar solo thumbnails no ataca
   los 1325 ms dominantes de esta apertura.

## Opciones y compromisos

| Prioridad / opción | Beneficio buscado | Coste, límites y validación |
| --- | --- | --- |
| 1. Captura ETW previa al siguiente arranque disponible, con CPU sampled/precise, image loads, disco/faults; analizador Defender solo si corresponde | Atribuir el intervalo previo a main.entry a stacks, carga de módulos, E/S o seguridad | Requiere permisos elevados y otra primera sesión disponible; no repetir ni reiniciar ahora. Coste de observación explícito. No se recupera retrospectivamente el desglose de esta captura |
| 2. Arrancar la enumeración de restauración antes del renderer, reteniendo snapshots hasta que UI/páginas estén listas | Solapar los 224 ms de catálogo con los 208 ms de renderer | Ahorro potencial del orden de 200 ms bajo esas duraciones y ausencia de contención, no demostrado. Preservar generaciones, prioridad de ruta explícita, orden de selección, sesión pendiente y cierre; no publicar UI desde trabajador ni provocar reentrada prematura |
| 3. Diferir imports y servicios prescindibles, por ejemplo SQLite/Compression y funciones de metadatos, junto con su primer uso | Reducir trabajo previo al original si ETW confirma coste de esas dependencias | /DELAYLOAD por sí solo mueve el coste a la primera llamada; abrir la caché inmediatamente podría anular el beneficio. Riesgo de errores de carga tardíos y mayor latencia de primeras miniaturas. D3D/D2D siguen siendo necesarios para el original completo; sin promesa sobre los 1325 ms |
| 4. Reducir coste de catálogo: revisar enumeración nativa por lotes y construcción/publicación de listas | Reducir los 224 ms de catálogo y el intervalo de 35 ms hasta solicitar original | Hay que medir E/S frente a CPU, comprobar si la STL ya usa información cacheada y conservar extensiones, sort/ties, Unicode, enlaces, cambios y cancelación. Ordenar solo costó 3 ms; optimizar exclusivamente el comparator tiene poco margen en este caso |
| 5. Instrumentar llamadas del renderer y construir directamente el tamaño final si resize inicial resulta costoso | Localizar y reducir parte de los 208 ms sin cambiar backend/calidad | Los 208 ms no equivalen a ahorro realizable. Cambios de orden/fullscreen requieren validar DPI, transparencia y device loss. Cambiar a WARP/otro backend solo desplaza costes y exige comparación propia |
| 6. Catálogo o ruta inicial persistentes con reconciliación asíncrona | Quitar la dependencia del catálogo completo para presentar una imagen inicial | Más estado, invalidación y posible cambio de cuál imagen se restaura. Una ruta anterior no garantiza el primer archivo según el orden actual; no implementarlo como cambio incidental ni alterar session.txt sin acuerdo |
| Alternativas fuera de los contratos actuales: proceso residente o vista inicial GDI/reducida | Proceso residente amortiza arranque; otro camino puede adelantar una vista | Residente consume recursos y cambia el modelo autorizado; vista reducida/otro renderer puede cambiar calidad, efectos y transición. Mostrar una ventana vacía no satisface original utilizable. No se implementan en este diagnóstico |

Recomendación: atribuir primero el intervalo dominante y preparar el solapamiento
de restauración como mejora acotada independiente. No prometer 100 ms ni presentar
el renderer como única causa. No desactivar seguridad ni recomendar exclusiones
sin evidencia; firmar/empaquetar puede ser pertinente solo si el perfil acredita
coste de confianza y no garantiza eliminar análisis o carga fría.

Fuentes primarias para mecanismos, no evidencia causal de este equipo:
[CRT](https://learn.microsoft.com/en-us/cpp/c-runtime-library/crt-initialization),
[WPA CPU/waits](https://learn.microsoft.com/en-us/windows-hardware/test/wpt/cpu-analysis),
[/DELAYLOAD](https://learn.microsoft.com/en-us/cpp/build/reference/linker-support-for-delay-loaded-dlls),
[Defender analyzer](https://learn.microsoft.com/en-us/defender-endpoint/tune-performance-defender-antivirus).

## Estado y alcance

El propietario reporta múltiples pruebas tras iniciar el SO: zoom/paneo, tira,
navegación rápida, Waterfall, Board e IPC correctos. **Revoca la aceptación de
rendimiento inicial:** unos 3000 ms no son aceptables. No se infieren más formatos,
animaciones ni restauración/cierre que los confirmados. El reporte IPC correcto
no diagnostica el fallo intermitente previo del harness.

003 sigue abierta por rendimiento inicial. Esta porción autoriza medición/análisis
y propuestas, sin implementar esas opciones ni generar otro candidato. No se
recompila ni repite CTest porque no hay cambios de fuentes. Revisión acotada y
hallazgos en review.md; continuidad y planes actualizados.

# 003: preparación de ETW para la próxima primera apertura

Estado 2026-10-24: **preparación y captura única del visor completadas**.
[Resultado ETW y atribución](etw-result.md): 2216 ms hasta envío del original,
2237 ms hasta DXGI observado; espera del creador de 1426 ms atribuida a Defender.
No repetir este procedimiento por inercia; la oportunidad de esa sesión ya se usó.
Las instrucciones inferiores conservan el protocolo de la captura realizada.
El propietario autorizó preparar la opción 1 del análisis y fijó como entrada
`F:\Pictures\FotosTEMP`, prohibiendo alterar cualquier archivo multimedia allí.
No se reinició Windows ni se volvió a abrir LenteUV durante la preparación;
la ejecución posterior se hizo tras el nuevo arranque autorizado por el propietario.

## Herramientas y prueba del recolector

- Instalado solo Windows Performance Toolkit desde el ADK oficial Microsoft
  10.1.26100.9457, instalador con Authenticode válido de Microsoft. Salida 0,
  sin reinicio. Incluye WPA, WPAExporter y Xperf; WPR del sistema ya existía.
- Perfiles `GeneralProfile.Verbose` y `FileIO.Verbose`, modo archivo, inspeccionados
  y exportados como referencia local. Incluyen CPU sampled, CSwitch/ReadyThread,
  loader/image loads, disco, hard faults, FileIO y proveedores antimalware.
  No se añade otro recorder Defender simultáneo en esta primera captura.
- Tres comprobaciones reales del recolector sin producto: la última corresponde
  a la versión final de los scripts. Start/stop/merge correctos, ETL decodificable,
  **0 eventos y 0 buffers perdidos**; 157286400 bytes en la última traza de ensayo.
  El intervalo de espera de ensayo es 3 s; arranque/rundown/merge amplían el tiempo
  total. Esto no es una medición de LenteUV ni certifica el rendimiento del producto.
- Xperf pudo extraer información de procesos/hilos/imágenes y stacks del ensayo.
  WPAExporter respondió a ayuda fuera del sandbox; su lectura de registro fue
  bloqueada dentro del sandbox. Análisis funcional de la traza probado con Xperf.
- Dos negativas verificadas antes de abrir producto: Capture sin confirmación de
  primera ejecución y Capture con confirmación en la sesión de preparación.
  Cero intentos del producto, cero procesos lenteuv al terminar y WPR propio parado.
- Scripts parseados; enlaces documentales y `git diff --check` comprobados.
  No compilación/CTest: el EXE y las fuentes de la aplicación no cambian.

Artefactos privados bajo `build/startup-etw-003-preparation/` y
`build/startup-etw-003-validatecollector-*/`, fuera de Git. No borrar el ensayo de
primera sesión anterior ni regenerar el candidato. La preparación guarda hash
del EXE, marcas de archivo, snapshots de settings/sesión, versión WPR y hashes de
los scripts validados. La preparación actual corresponde a la sesión ya consumida;
una captura real exige otro arranque y confirmación explícita del propietario.

## Entrada y preservación

El ejecutor fija el candidato 003 b0e98f8 ya entregado y **un único argumento**:
`F:\Pictures\FotosTEMP`. No selecciona ni decodifica imágenes de antemano y no
recorre toda la colección para calcular hashes antes de la apertura. Se presenta
la primera imagen según la ordenación existente de esa carpeta.

La configuración preparada exige Viewer visible (ID 2), logging desactivado y
`user_data` existente junto al candidato, en E:. Captura conserva caché/sesión;
rechaza cambios posteriores de EXE/settings/sesión antes de abrir, comprobando solo
metadatos de esos archivos en esa fase. Los hashes completos se verifican después
del proceso. No cambia asociaciones, permisos de medios, preferencias ni SO.

No se envían entradas de edición, borrado, etiquetas, Board ni guardado de tableros.
El cierre es normal y automático tras el frame original más 2500 ms. No interactuar
con el visor durante la captura. Los scripts escriben únicamente sus artefactos en
build; LenteUV usa sus datos/cachés normales junto al EXE. El directorio multimedia
solo se suministra como entrada de lectura, sin copiar, convertir ni alterar sus
archivos. No ejecutar tests de metadatos/borrado sobre esa carpeta.

Esta entrada explícita de directorio sustituye, por instrucción del propietario,
el escenario sin argumentos de la primera captura anterior. Registrar la diferencia;
no presentarla como comparación causal ni como cambio de rendimiento del mismo caso.

## Ejecutores y permisos

`scripts/run_startup_etw.ps1` requiere PowerShell 7 y se inicia **sin elevar**.
Solo `scripts/startup_etw_collector.ps1` usa elevación UAC. El helper nunca lanza
LenteUV; el padre lo inicia con su token normal después del handshake ETW listo.

La sesión WPR tiene nombre único `LenteUV003-<guid>`. Start/stop/cancel se restringen
a ella; no se cancela una grabación ajena para forzar el arranque. Un fallo de
permiso, inicio, confirmación de perfiles o handshake impide abrir el producto.
Helper con watchdog de 90 s y guardado del ETL incluso ante pérdida del padre.
Capture espera el proceso hasta 45 s; al expirar solicita cierre normal y preserva
la evidencia sin forzar ni relanzar. El merge puede continuar después del padre;
consultar collector-result.json antes de dar por terminada la captura.

Un archivo exclusivo `attempt-<boot-ticks>.json` en la preparación se crea justo
antes de Process.Start. Protege contra reintentos incluso desde RunRoots distintos
y aunque el lanzamiento falle. No borrarlo para repetir la prueba. La ausencia de
procesos no demuestra que el visor nunca se abrió y cerró: por eso también se exige
confirmación del propietario de la primera ejecución.

## Próxima sesión: procedimiento para el agente

1. Esperar a que el propietario indique un nuevo arranque y confirme que no abrió
   LenteUV. No programar inicio automático ni reiniciar/cerrar sesión por iniciativa
   propia. No lanzar visor, benchmark, EXE auxiliar del proyecto ni hacer warmups.
2. Revisar Git/continuidad sin reconstruir, reemplazar ni hashear previamente el EXE.
   Confirmar espacio suficiente en E: para traza y merge (en preparación había
   aproximadamente 1428 GiB libres; reservar al menos 2 GiB como comprobación operativa).
3. Desde PowerShell 7 normal, raíz del repositorio, ejecutar:

   ```powershell
   ./scripts/run_startup_etw.ps1 -Mode Preflight
   ```

   Debe indicar NewBoot=true, CollectorValidated=true, Administrator=false y
   ExistingLenteuvCount=0. Preflight no abre el visor. No ejecutar Prepare otra vez
   en la sesión nueva: leería el EXE para fijar hash y cambiaría la referencia de boot.
   Si falla por inputs/herramientas cambiados, detenerse y explicar el límite antes
   de consumir la oportunidad; no improvisar fallback o captura sin ETW.
4. Con la confirmación explícita de primera apertura, ejecutar fuera del sandbox:

   ```powershell
   ./scripts/run_startup_etw.ps1 -Mode Capture -ConfirmFirstLaunch
   ```

   Aprobar UAC para el helper ETW. No ejecutar el padre como administrador ni abrir
   manualmente LenteUV: el padre espera el recolector y realiza la única apertura.
5. Conservar RunRoot devuelto: startup.etl, qpc.csv, capture.json, estado/logs WPR y
   estadísticas. Confirmar salida normal, hash final, presencia de main.entry y
   original, pérdidas cero y estado saved. Un fallo posterior no permite reintentar
   en ese boot. La captura podría ser parcial; conservarla como tal.

Los modos Prepare y ValidateCollector son para esta preparación o una nueva
preparación deliberada; ninguno inicia LenteUV. No repetirlos tras el reinicio por
inercia. No hay tareas programadas, servicios ni recolectores persistentes.

## Análisis que debe seguir a la captura

Correlacionar capture.json (PID, FILETIME de creación, invocación QPC y frecuencia)
con qpc.csv y el proceso/hilo principal en ETL. Separar antes de creación, creación
a main.entry, renderer, catálogo, decode/upload y presentación. Limitar primero el
análisis a creación→main.entry para atribuir los 1325 ms históricos sin asumir
que todos se repetirán. Evaluar:

- CPU sampled con stacks: qué módulos/funciones ejecutan trabajo.
- CPU precise/CSwitch/ReadyThread: espera, disponibilidad y quien desbloquea el hilo.
- Carga de imágenes, hard faults y disco: intervalos y archivos implicados;
  no sumar tiempos paralelos ni convertir duración de un módulo en causalidad única.
- Antimalware: correlación temporal y eventos atribuibles; presencia de MsMpEng
  no prueba que el EXE esté esperando a Defender. No excluir archivos ni desactivar
  seguridad. Si la traza no aporta detalle suficiente, conservar la incertidumbre.

Xperf puede generar reportes con `-a process -thread -image`, `-a profile`,
`-a cswitch`, `-a diskio`, `-a hardfault` y `-a stack -butterfly`, filtrando PID/rango
según la ayuda de cada acción. WPA permite inspeccionar las mismas tablas y stacks.
Resolver símbolos públicos Microsoft después de medir, con caché local en build;
no descargar símbolos ni ejecutar herramientas intensivas antes de la apertura.
No hay PDB Release coincidente disponible en el build comprobado; el PDB Debug
existente no corresponde a este EXE. No reconstruir para fabricarle símbolos:
preservar módulo+RVA y declarar el límite para frames privados de LenteUV/driver.

Una primera apertura instrumentada no produce p95 ni comparación causal contra
la traza QPC anterior. Registrar coste del recorder y distinguir Present enviado,
confirmación DXGI y observación óptica. 003 sigue abierta por rendimiento inicial.

Referencias primarias consultadas:
[ADK/WPT oficial](https://learn.microsoft.com/en-us/windows-hardware/get-started/adk-install),
[WPR CLI](https://learn.microsoft.com/en-us/windows-hardware/test/wpt/wpr-command-line-options),
[WPA CPU/waits](https://learn.microsoft.com/en-us/windows-hardware/test/wpt/cpu-analysis).

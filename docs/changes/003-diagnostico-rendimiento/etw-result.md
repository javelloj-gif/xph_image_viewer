# 003: primera apertura con ETW — 2026-10-24

## Resultado

**La creación del proceso estuvo bloqueada unos 1426 ms esperando a Defender.**
La conclusión se sustenta en una espera CSwitch del hilo creador, su stack dentro
de CreateProcess/NtCreateUserProcess/WdFilter, el ReadyThread emitido por MsMpEng
y el análisis del EXE registrado por el analizador oficial de Defender. No se
deduce solo del uso de CPU del antivirus ni de coincidencia temporal entre procesos.

El original se envió a **2215,70 ms** desde invocación; confirmación DXGI del mismo
present ID observada a **2237,34 ms**. Una sola apertura en otro boot, sin warmup ni
repetición. No hay medición óptica, p95 ni mejora implementada por este diagnóstico.
Los aproximadamente 3000 ms reportados por el propietario conservan su evidencia.

## Identidad, escenario y preservación

- Windows inició a las **09:05:32,500 America/Bogota**; invocación alrededor de
  **09:07:10,235**. Fecha 2026-10-24. Preflight confirmó boot distinto de preparación,
  cero procesos lenteuv y recolector validado. El propietario indicó proceder en
  respuesta a la coordinación de nueva sesión sin haber abierto el visor.
- Candidato 003 b0e98f8, mismo EXE entregado, hash posterior
  `A8C54E3B42B5811EA17CCF347AA91A7FFF4C2808BF19EE45BD4E6C7F0AF04213`.
- Argumento único **`F:\Pictures\FotosTEMP`**, conforme a instrucción del propietario.
  Viewer/fullscreen y datos/caché existentes; no se hasheó ni copió el EXE antes de
  medir. Entrada de directorio explícita, diferente de la primera captura anterior
  sin argumentos: no comparar ambas como versiones ni atribuir causalmente su delta.
- Solo el helper ETW elevado; LenteUV con token normal. Perfiles GeneralProfile.Verbose
  y FileIO.Verbose, salida bajo build. ETL de 251658240 bytes, 0 eventos/buffers
  perdidos; QPC con 0 descartes. Estado del recolector saved, salida del visor 0.
- Settings y sesión conservaron sus hashes antes/después. Caché: 27 hits para
  27 solicitudes de tira y una decodificación original; no se purgó ninguna caché.
- Auditoría FileIO del **ETL completo** para PID de LenteUV y la carpeta indicada:
  1838 eventos de lectura y 259 de enumeración; **0 FileIoWrite, Delete, Rename,
  SetInfo, SetEA o SetSecurity** en esa carpeta. Los eventos Create de ETW incluyen
  apertura de archivos existentes; no significan creación de archivos. Los eventos
  de modificación del proceso observados tenían destinos fuera de esa carpeta.
  No se editó, borró, renombró ni convirtió multimedia; no se enviaron entradas al visor.
  No se calculó una comparación binaria previa/posterior de toda la colección.

Artefactos y detalles privados locales bajo
`build/startup-etw-003-capture-dc4ce481224f48dbb9f80b154dae00e9/`: startup.etl,
qpc.csv, capture.json, summary.json, estados/logs WPR, estadísticas, reportes de
procesos/stacks/faults/disco, preentry-events.csv, wait-analysis.json,
createprocess-wait-stack.csv, defender-report.clixml/json y file-audit.json.
El análisis Python local solo lee exports/ETL y escribe informes bajo build;
no abre contenido multimedia ni ejecuta el visor. No versionar estos artefactos.
Guard del boot conservado en build/startup-etw-003-preparation; no reintentar.

## Tiempos y cadena causal

Desde invocación, QPC:

| Evento / tramo | Tiempo |
| --- | ---: |
| Creación Win32 del proceso, correlación FILETIME/QPC | ~35,6 ms |
| Retorno de Process.Start | 1467,04 ms |
| Primer marcador main.entry | 1534,49 ms |
| Renderer: inicio → fin | 1551,09 → 1811,58 ms (260,49 ms) |
| Solicitud de mostrar ventana | 1826,75 ms |
| Primer scan de restauración, cancelado | 1835,72 → 1879,74 ms (~44,02 ms) |
| Scan del destino explícito, con ordenación | 1879,75 → 2083,36 ms (~203,61 ms) |
| Solicitud del original | 2108,51 ms |
| Decode original | 75,21 ms |
| Upload original | 23,63 ms |
| Original enviado | 2215,70 ms |
| Confirmación DXGI observada | 2237,34 ms |

No sumar etapas solapadas: el scan de otra carpeta restaurada continúa en paralelo;
SQLite abre en paralelo al renderer (18,27 ms). Las miniaturas se solicitan después
del original enviado. Desde main.entry hasta original enviado transcurren 681,21 ms.

Evidencia ETW, tiempos en microsegundos desde inicio del ETL:

1. P-Start del visor PID 17948 a **1293561 us**; padre pwsh PID 19128.
2. Su hilo creador **13196** pasa a Waiting/Executive a **1293747 us**.
   Stack inmediatamente anterior: `KernelBase!CreateProcessInternalW` →
   `ntoskrnl!NtCreateUserProcess` → `PspCallProcessNotifyRoutines` → `WdFilter.sys`
   → `FLTMGR!FltSendMessage` → espera kernel. Símbolos públicos descargados después
   de la captura; frames privados de WdFilter conservan módulo/dirección.
3. `Get-MpPerformanceReport -Path startup.etl -TopScans 20` identifica **FileScan de
   lenteuv.exe durante 1407,5852 ms**, 09:07:10,2746912–09:07:11,6822764.
   También un RealTimeScan OnOpen del EXE de 35,0829 ms anterior a P-Start.
4. `MsMpEng.exe`, PID 4688/TID 12740, emite ReadyThread para el hilo creador a
   **2719961 us**; este vuelve a CPU a **2719991 us**. Intervalo fuera de CPU:
   **1426,244 ms**. El hilo principal de LenteUV se crea a **2720262 us**.
5. El loader también espera a Defender en DLLs VC++: esperas individuales de unos
   20–30 ms con ReadyThread de MsMpEng. Son concurrentes en varios hilos; no sumarlas
   como si fueran una única serie.

El bloqueo principal explica aproximadamente el 64 % de los 2216 ms totales y la
mayor parte del intervalo de 1534 ms previo a main.entry. No es trabajo del cuerpo
de App::Initialize. Se observó JIT de otro hilo del padre PowerShell en paralelo;
no debe atribuirse a CPU de LenteUV ni confundirse con la espera explícita del creador.

El EXE no tiene firma Authenticode (NotSigned). El proveedor Engine registra
comprobaciones de confianza/firma y un disparador `SLF:TriggerCloudForFriendlyScans`
con cached=false. Esto documenta el camino de análisis/confianza, **no cuantifica
latencia de red ni demuestra que firmar elimine los 1408 ms**. No se midió SmartScreen
como origen del bloqueo; distinguirlo de Defender Antivirus. El comentario del
analizador sobre low fidelity no se convierte aquí en un veredicto de malware.

## Consecuencias y opciones actualizadas

| Opción | Evidencia que la motiva | Compromisos / siguiente comprobación |
| --- | --- | --- |
| Prototipo de distribución firmada con identidad estable | EXE sin firma y ruta de confianza/antimalware en arranque | Certificado/identidad/proceso de firma y cambios del artefacto. Comparar candidato firmado en otra primera sesión disponible; no garantiza reputación inmediata, ahorro de 1,4 s ni desactivar análisis. No se firma ni compra nada por esta porción |
| Preparar reporte de rendimiento para Microsoft si el coste persiste | Stack/espera, ReadyThread y reporte oficial identifican un análisis síncrono lento | Investigación externa, sin plazo/resultado garantizados. Cualquier envío de EXE/traza requiere autorización; no se envió contenido a terceros |
| Solapar scan del destino con renderer y evitar iniciar trabajo de restauración que se cancela ante destino explícito | En este caso 260 ms de renderer seguidos de 44 ms cancelados y 204 ms de scan necesario | Oportunidad del orden de cientos de ms, no ahorro demostrado. Preservar prioridad/generaciones, selección, sesión pendiente y GPU/UI en principal; no altera por sí sola el análisis del EXE anterior a su hilo principal |
| Afinar renderer/catálogo y diferir servicios prescindibles | Otros 681 ms tras main.entry; ~260 ms renderer y ~204 ms scan principal | Requiere profiling por llamada y regresiones. Diferir imports no evita necesariamente el FileScan del EXE completo; los análisis VC++ medidos son decenas de ms y se solapan |

Recomendación: tratar **distribución/confianza del EXE** y **serialización interna**
como frentes medidos separados. No prometer que una sola optimización del loader
lleve el arranque a 100 ms. Restar aritméticamente el bloqueo de 1426 ms dejaría unos
789 ms en esta muestra; es una ilustración del trabajo restante, no un resultado
medido de otro build ni una predicción garantizada.

No se deshabilitó protección, añadieron exclusiones, cambiaron políticas ni instalaron
certificados. No hay nueva corrección productiva, build, CTest o paquete. 003 sigue
abierta por rendimiento inicial rechazado; preservar los seis OK funcionales.

Fuentes primarias sobre las herramientas y límites de firma, no sustitutos de la traza:
[Get-MpPerformanceReport](https://learn.microsoft.com/en-us/powershell/module/defenderperformance/get-mpperformancereport),
[WPA CPU/waits](https://learn.microsoft.com/en-us/windows-hardware/test/wpt/cpu-analysis),
[firma y reputación SmartScreen](https://learn.microsoft.com/en-us/windows/apps/package-and-deploy/smartscreen-reputation).

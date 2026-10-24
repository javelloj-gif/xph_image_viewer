# Plan: 003 Diagnóstico de rendimiento

**Estado vigente 2026-10-23:** el propietario aprobó [implementation-plan.md](implementation-plan.md):
arranque asíncrono completo, prioridad/cancelación visible, caché persistente exacta
y medición reproducible. P1..P6 implementados/verificados; P7 (comparación/portable)
en curso. Las recomendaciones y pendientes inferiores son históricos; no reabrir
la base ni repetir 010/AR-002/AR-005. Cualquier aceptación restante se registra con
sus límites en result.md; la meta de 100 ms no se presume cumplida.

## Plan histórico

Especificación: [spec.md](spec.md). Última actualización: 2026-10-22.
Estado: diagnóstico IPC histórico y validación real posterior a 005 ejecutados;
arranque real pendiente; base acotada CPU/memoria/GPU sin logging y entrada Shell
ejecutadas con límites documentados.

Seguimiento posterior: [005](../005-entrega-ipc/result.md) corrigió AR-003 en alcance
transporte/error/payload y convirtió la matriz en regresión; Release/CTest 4/4 pasaron.
El resto de este plan conserva decisiones y evidencia históricas de 003. Siguiente
actual: presentación/Explorer/arranque completo de instancias y arranque/reposo real,
sin ACK, elección atómica ni cierre por efecto secundario.

## Componentes y diseño

Seguir `main`/`App::Initialize`, inicialización de renderer/loader, restauración de
sesión, primera carga, IPC y bucle de reposo. Usar los hallazgos de 001 como preguntas,
sin tratarlos como causa probada. Desglosar inicio, ventana e imagen utilizable.

Entradas de la [auditoría de 001](../001-refactorizacion-base/review.md): AR-003 para
IPC; AR-005 en su parte IPC/cierre, dejando la campaña de loader para etapa posterior;
AR-006 como pregunta de coste de carga de caché, sin introducir endurecimiento en esta
investigación. El informe añade preguntas sobre restauración de sesión, carga síncrona
y reposo. No ampliar por ello el alcance inicial a toda navegación o animaciones.

## Tareas y comprobaciones

- [ ] T-01: Acordar caso reportado, datos y ruta de entrada; registrar hardware,
  Windows, almacenamiento, códecs relevantes, revisión, Release y settings. AC-01.
- [ ] T-02: Seleccionar instrumentos y definir fases, repeticiones y observación. AC-01.
- [ ] T-03: Medir arranque inicial real y posteriores; registrar procesos/cachés
  observables sin alterarlos arbitrariamente. AC-02.
- [ ] T-04: Ejecutar matriz instancia existente/segunda apertura/`--multiple`;
  registrar recepción, proceso activo, imagen resultante y fallos. AC-03.
  Parcial: PNG presentado después de IPC y arranques completos --multiple/--lite.
  Lite necesitó F11 para mostrar JPEG en capturas; indicio por investigar.
  Explorer/asociaciones y dos primarios simultáneos no comprobados.
- [x] T-05: Medir reposo con imagen estática y condiciones estables; separar CPU,
  GPU, working set/memoria privada y métricas no disponibles. AC-04.
  Base acotada: tres ventanas sin logging con CPU/WS/privada, GPU por engine y
  Dedicated/Shared Usage por PID; Status=0. Limitaciones y duraciones reales en
  [evidencia](explorer-validation.md). No residencia física/picos ni comparación causal.
- [ ] T-06: Publicar resumen y hallazgos con impacto, confianza, reproducción,
  prioridad sugerida y próxima comprobación/corrección. AC-05.

## Validación

### Porción IPC acordada antes de editar (2026-10-22)

Base: `ac4a5da` más R1/R2/004 sin commit, preservados. Se implementa el siguiente
paso recomendado, diagnóstico AR-003, no elección de instancia, ACK, retry ni cierre.

- `SingleInstanceIpc`: snapshot de etapas por objeto (detección, intento/escritura y
  error, mensajes recibidos/posteados). Contadores atómicos observables, sin cambiar
  booleano de Initialize ni decisiones/reintentos. Pipe productiva igual; miembro
  privado permite al harness amigo usar nombre único. Diagnóstico no confirma apertura.
- `App`: extraer cuerpo existente del callback IPC a OnIpcOpenPath sin cambiarlo;
  callback y harness invocan la misma implementación. Declaraciones documentadas.
- Harness `tests/ipc_diagnostic_tests.cpp` junto al target productivo de 004, modo
  CLI separado `--ipc-diagnostics`, registrado como CTest `ipc.delivery_diagnostics`.
  Procesos sender reales relanzan el mismo EXE con endpoint exclusivo. Padre mantiene
  App, WIC y contexto D2D software WARP, HWND solo de mensajes y bombeo de mensajes;
  no App::Initialize, swapchain ni ventana visible. Verificar ruta y bitmap actuales
  tras el handler real Window/App, no solo exit code del cliente. No prueba presentación.
- Script `scripts/run_ipc_diagnostics.ps1`: run nuevo bajo build, EXE y copias JPEG/PNG
  (incluida ruta Unicode/espacios), user_data local. Sin pipe global, datos personales,
  asociaciones ni cierre de procesos ajenos. Todos los hijos tienen timeout y limpieza.
- Matriz: envío directo como control, cinco segundas aperturas por Initialize con
  servidor asentado, receptor sin bombear temporalmente, ruta inexistente/vacía,
  pares clientes simultáneos. ParseCommandLine verifica flags --multiple/--lite;
  se registra que no sustituye un arranque completo con ese flag. Distinguir transporte,
  post UI, handler y archivo aplicado. Los fallos de producto se reportan como resultados,
  no se exige que toda apertura pase para aceptar el diagnóstico; invariantes del harness sí.
- Configure/build Release/CTest completo, revisión con lenteuv-architecture-review,
  diff/enlaces y resumen de evidencia con hashes de EXE. No medir rendimiento con
  esta instrumentación ni simular primera ejecución real. 003 permanece abierto.

El snapshot añade coste de observación no medido; el entorno controlado es evidencia
del mecanismo IPC, no reproducción automática de la lentitud del propietario.

### Resultado de la porción IPC

- [x] I-01: Instrumentar etapas, conservar protocolo y extraer handler sin cambiar cuerpo.
- [x] I-02: Crear script/harness aislados, procesos propios con timeout y limpieza.
- [x] I-03: Ejecutar matriz: 5/5 aperturas vía Initialize fallaron con error Win32 2;
  controles directos aplicaron JPEG/PNG con bitmap en el receptor. Par concurrente:
  ambos con error 2. Escritura con UI sin bombear no aplicó hasta entregar mensajes.
- [x] I-04: Compilar Release, CTest 4/4, revisión, resultado y roadmap.

El éxito de ipc.delivery_diagnostics valida invariantes y recogida del diagnóstico;
no significa que las aperturas secundarias funcionen. El fallo de producto se mantiene.
Los intentos iniciales corrigieron readiness/normalización de expectativas del harness,
no la lógica del transporte. Véanse [resultado](result.md) y [revisión](review.md).

Conservar mismos datos y configuración entre ejecuciones comparables. Registrar
cada observación y dispersión; no mezclar primeras ejecuciones reales con posteriores.
No considerar el cliente IPC exitoso como demostración de recepción. La instrumentación
y su revisión deben figurar en las mediciones. Logs/perfiles quedan locales e ignorados;
versionar el resumen suficiente para reproducir. No afirmar fuga por un pico de memoria.

## Riesgos y recuperación

No interferir con procesos/datos personales. La primera ejecución real requiere
coordinación con el propietario, no reinicio automático. Si falta un instrumento,
registrar su límite y continuar mediciones independientes.

## Decisiones y ajustes

2026-10-22: se limita la primera etapa a arranque, IPC y reposo. Refactorización
es la prioridad; si una porción de 001 afecta estos escenarios, adelantar solo su base
antes de cambiar el código para conservar comparación.

## Estado para continuar

### Prioridad actual: oportunidades por código (2026-10-22)

- [x] Atender giro del propietario a análisis de fuentes/funcionamiento, dejando
  mediciones de primera sesión y reproducción de síntomas como pendientes conservados.
- [x] Seguir caminos de UI/render, miniaturas/modelo, loader y arranque secundario;
  registrar oportunidades y límites en [source-optimization-review.md](source-optimization-review.md).
- [x] Priorizar equivalencia visual/de interacción: primero trabajo repetido y
  algoritmos; no bajar resolución, interpolación, FPS o efectos.
- [x] Primera porción local especificada e implementada en [006](../006-optimizaciones-locales/result.md).
  Equivalencia y benchmark local registrados; no mejora global demostrada.
- [x] Layouts Library/Waterfall especificados e implementados en [007](../007-layouts-por-cambios/result.md),
  con invalidación compartida, equivalencia de rects/píxeles y medición local.
- [x] Formatos DirectWrite internos acotados implementados en [008](../008-formatos-texto/result.md),
  con fábrica mutable independiente, equivalencia controlada y medición local.
- [x] Decoder compartido OPT-004 implementado en [009](../009-decoder-compartido/result.md),
  con fallback y composición preservados, equivalencia CPU y conteo real WIC.
- [ ] Siguiente recomendado: especificar inicialización secundaria mínima OPT-005;
  índice masonry y última medida del overlay siguen pendientes y separados.

El scheduler OPT-007 tiene potencial para reposo, pero no se autoriza a sustituir
el bucle por Sleep/WM_PAINT sin mapa de invalidación/cargas/timers. Fuentes modificadas
solo en alcances 006..009, trabajo previo preservado y sin commit. Las campañas históricas
de abajo no cambia esta prioridad nueva.

### Explorer y asociación de una sola apertura (protocolo previo, 2026-10-22)

Estado final: [porción ejecutada](explorer-validation.md). El usuario completó los
selectores nativos no controlables por el helper; JPEG y PNG aplicados en receptor
13188, JPG/PNG UserChoice/comando conservados. Default apunta a portable con hash
distinto a 005: no sustituirlo ni atribuirle resultados de ese build. El resto de esta
sección conserva el protocolo previo. [Primera sesión](first-session-protocol.md)
preparada; no ejecutada ni programada automáticamente.

Ampliación antes de medir: Get-Counter GPU falló en sandbox con c0000bc8, pero fuera
enumeró GPU Engine y GPU Process Memory. Si el PNG queda estable en el receptor propio,
registrar tres ventanas con 11 muestras a 1 s (aprox. 10 s) de utilización por engine
y Dedicated/Shared Usage, filtradas por PID y con Status; CPU/WS/private bytes con
Stopwatch/Process en el mismo intervalo. No sumar porcentajes de engines como total
GPU ni equiparar Dedicated Usage a presupuesto/residencia física de VRAM. Conservar
datos locales propios; no capturas/acciones UI durante los intervalos. Es una muestra
de esta configuración sin logging, no comparación con la serie instrumentada anterior.

Crear copia nueva del EXE/hash de 005 con user_data/locales/fixtures JPEG/PNG Unicode
propios bajo build. Abrir desde Explorer mediante Abrir con y selección del EXE local,
con Solo una vez; nunca elegir Siempre, instalar ni escribir asociaciones por script.
Registrar ProgId UserChoice de .jpg/.png antes/después (lectura fuera del sandbox),
proceso/ruta/línea de comandos e imagen presentada. Comprobar ausencia de LenteUV y
pipe antes de la primera apertura. Reabrir PNG con receptor propio existente y
observar cambio de imagen/PID, sin declarar entrega por exit code solamente.
Logging desactivado en esta copia: Explorer controla cwd y no deben escribirse logs
relativos fuera del staging. Sin medir latencia con capturas ni usar la sesión actual
como primera real. Al terminar cerrar solo proceso/tab propios y verificar UserChoice
conservado. Si la UI no permite apertura única sin cambiar default, no confirmar ese
cambio: registrar límite y continuar protocolo del caso lento/primera ejecución.

### Reproducción AR-008 (protocolo previo, 2026-10-22)

Estado final: [diagnóstico acotado completado](ar008-reproduction.md). Seis arranques,
seis salidas 0 y JPEG capturado en todos sin F11/resize. Capturas negras también
sin flag y con --multiple; oclusión y discrepancia con observación nativa constatadas.
No hay fallo persistente de --lite reproducido ni corrección de fuentes sustentada.
El protocolo siguiente se conserva como previo a la ejecución. Siguiente actual:
Explorer/caso habitual/primera real de 003. Reabrir AR-008 si se observa negro/parpadeo
nativamente.

Sin editar fuentes: seis arranques secuenciales en directorios nuevos bajo build,
dos sin flag, dos --multiple y dos --lite. Mismo EXE/hash de 005, JPEG copiado y
settings de la validación anterior (ventana opaca, fullscreen=false, logging activo).
Sesión vacía por caso; no procesos ajenos ni pipe productiva al comenzar cada caso.
Start-Process Normal para evitar el ocultamiento del ensayo anterior. Registrar
argumentos/PID/salida y observar mediante computer-use: captura inicial, activación,
segunda captura asentada sin resize. Ante negro, solicitar contraste visual nativo
del usuario y solo después probar F11; conservar diferencias como observaciones.
Sin captura fiable/contraste nativo no afirmar defecto de producto ni causa de GPU.
Cerrar únicamente la ventana propia entre casos; timeout/limpieza del proceso propio.
Actualizar review/result/roadmap según evidencia, sin corregir por el indicio.

### Validación real posterior a 005 (protocolo previo, 2026-10-22)

Sin cambiar fuentes ni contrato: preparar copias nuevas del EXE Release de 005,
locales, JPEG/PNG y user_data propio bajo build. Comprobar hash, procesos existentes
y ausencia de endpoint productivo antes de arrancar un receptor; no enviar a procesos
ajenos. UI mediante computer-use; no cambiar asociaciones ni configuración del sistema.
Abrir JPEG en arranque completo, reenviar PNG y observar imagen presentada; arrancar
--multiple/--lite con datos separados, observar ventanas independientes y receptor previo.
Registrar PID, argumentos, exit codes disponibles, capturas locales y límites.

Si puede observarse imagen estática, medir tres ventanas de 10 s tras al menos 5 s
de asentamiento usando TotalProcessorTime, tiempo monotónico, WorkingSet64 y
PrivateMemorySize64; CPU como fracción de un núcleo y normalizada por procesadores
lógicos. Sin atribuir GPU/VRAM si no hay contador fiable. Settings/copias/hash y
programas concurrentes forman parte de la ficha; no comparar con otra revisión.
No llamar primera ejecución real a un arranque de estado previo desconocido.
El diálogo de fallo requiere endpoint ocupado controlado y ausencia de receptor real;
no inducirlo si se compromete aislamiento. Cerrar solo procesos propios al terminar.

Estado de esta porción: ejecutada; [evidencia](real-validation.md). PNG presentado,
ambos flags completos, diálogo Win32 231 y salida 2 real. La serie inicial de reposo
se descartó por solaparse con arranques propios; segunda serie sin acciones UI ni
otros LenteUV. El propietario confirmó uso previo en esta sesión: no primera real.

- Completado: diagnóstico IPC, corrección independiente 005; PNG presentado,
  flags completos, diálogo/salida real y muestra CPU/memoria instrumentada.
- Pendiente: primera real/posteriores medidos, caso representativo y fase de demora;
  doble clic predeterminado del build nuevo no probado ni asociación cambiada.
  Explorer/Abrir con y base CPU/memoria/GPU sin logging ejecutados. AR-008: terminada,
  discrepancia de captura constatada; no fallo persistente de --lite reproducido.
- Siguiente actual: tras [006](../006-optimizaciones-locales/result.md),
  [007](../007-layouts-por-cambios/result.md), [008](../008-formatos-texto/result.md)
  y [009](../009-decoder-compartido/result.md), especificar inicialización secundaria
  mínima OPT-005 conservando prerrequisitos de HWND/callbacks y errores de 005.
  La serie de primera ejecución queda preparada, sin ejecución automática: elegir
  instalación/hash y caso representativo según first-session-protocol.md.
  Reabrir AR-008 con síntoma nativo.
  No modificar renderer por la captura negra de esta campaña. 003 abierto.
  Sin ampliación a ACK, elección atómica o cierre.

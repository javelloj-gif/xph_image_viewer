# Revisión de diseño: 010

Fecha: 2026-10-22. lenteuv-architecture-review, sesión actual sin delegación.
Base ac4a5da, rama codex/repository-foundation; trabajo posterior sin commit preservado.

Cobertura: App::Initialize/OnIpcOpenPath y miembros, Window::Create/Show y dispatch,
SingleInstanceIpc::Initialize, traducción main, inicialización/destrucción del loader,
inicializadores de FolderDataManager, XmpTagManager y overlays. Documentos 009,
spec/result 005, oportunidad OPT-005 y guías. Sin auditoría global del cierre,
codecs, escáneres ni rendimiento; no ejecución nueva de producto.

## Hallazgos y tratamiento

- IS-001 — Riesgo, prioridad alta, código confirmado: app.cpp:118 inicia servidor
  antes de enlazar onIpcOpenPath (:155); window.cpp:430 solo invoca callback si existe.
  Adelantar IPC extiende la fase con servicios incompletos. Un despacho temprano puede
  perder petición o consumir servicios no listos. Reentrada no reproducida aquí.
  Propuesta mínima: preservar fase sin bombeo y comprobar despacho/reentrada y petición
  temprana; AC-03 necesario. Coste medio, sin introducir ACK ni cola por hipótesis.
- IS-002 — Riesgo, prioridad media, código confirmado: app.cpp:85 puede fallar WIC
  antes de IPC en el orden actual. Al moverlo, un primario tendrá servidor activo ante
  ese fallo; un secundario ya no ejecutará WIC. Precedencia explícita en spec y fallo
  parcial en AC-04. Coste medio; no afirmar cierre acotado ni cambiar Shutdown.
- IS-003 — Restricción, prioridad alta, confirmada: single_instance_ipc.cpp:36 toma
  HWND y :76 inicia hilo; un mero sondeo adelantado con HWND nulo no es sustituto del
  movimiento. Conservar ventana oculta (window.cpp:21), COM/settings y API existente.
  Coste pequeño; AC-01/02 preservan 005 y el diálogo de main.cpp:82.

Los riesgos tienen tareas y criterios; no son fallos nuevos reproducidos ni bloqueos
de especificación. Diseño acotado listo para implementación y validación. Comentarios
de métodos modificados se comprobarán con el diff de fuentes, todavía inexistente.

## Revisión de implementación y validación posterior

Diff específico contrastado con build/optimization-010/before/app.cpp y app.h,
no con HEAD que carece de 003..009. Producción modificada solo en App::Initialize
(app.cpp:65) y su contrato (app.h:82). Se conserva el bloque de servicios sin cambiar
su contenido, situado ahora después de IPC. Métodos nuevos/modificados del harness
incluyen comentarios de responsabilidad, propiedad y aislamiento.

IS-001 tratado: callback temporal (app.cpp:98) posee copias mediante shared_ptr antes
de iniciar servidor (:106). Callback definitivo y aplicación retenida (:215) solo
tras contenido inicial y timer. No referencias a stack en retorno fallido. El test
StartupCases induce dos posts/despachos JPEG/PNG antes de WIC; no se aplica imagen
entonces y PNG prevalece después sobre argumento JPEG. No es prueba de reentrada
espontánea. Se revisó el diseño antes de aceptar el movimiento con retención local.

IS-002 tratado: orden de errores explícito; tests inducen fallos de ventana, WIC y
renderer mediante imports del EXE de tests. En fallos se deja actuar RAII de App;
el proceso termina sin cargar rutas retenidas. Sin cambiar orden de miembros ni
Shutdown; no demuestra plazo de cierre general o ausencia dinámica de toda fuga.

IS-003 preservado: HWND válido y oculto, settings/i18n/COM existentes, endpoint y
semántica 005 sin diff. Secundarios reales exitosos/fallidos no crean factories WIC
o tags/renderer; favorito sintético no cargado. Código confirma bloque omitido.

Cobertura ampliada: harness IPC/StartupCases y ScopedStartupImport, script aislado,
referencia instrumentada, portable/staging e imports. Diez escenarios de arranque
nativo pasaron y siete dibujaron cuatro modos; Release/CTest 5/5 conserva 009.
No fallos de producto pendientes identificados en el diff; aceptación visual AC-03
pendiente. Medianas no prueban mejora temporal; no afirmar UX, primera real ni reposo.
Sin revisión nueva de escáneres/cancelación, elección atómica, ACK, cierre global,
Library, compositor o scheduler. Los pendientes previos siguen siendo independientes.

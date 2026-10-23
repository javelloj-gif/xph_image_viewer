# Revisión de diseño e inventario: 002

Fecha: 2026-10-23. Base: `8625d95`, rama `codex/repository-foundation`, Git limpio
al iniciar. Evidencia de código y documentación; no ejecución del producto.
Habilidad: [lenteuv-architecture-review](../../../.agents/skills/lenteuv-architecture-review/SKILL.md).

## Cobertura y límite

Leídos AGENTS, workflow, project-context, usage, development, roadmap, spec/plan/result
002 y user-validation 014; criterios/formato de la habilidad. Inspeccionados Page y
propiedad en app.h, referencias Library y caminos afectados de app.cpp, API de
LibraryPage, API/consumidores del gestor core, carga Waterfall, Load/Save de Settings,
controles de modos de OptionsMenu, listas CMake y defaults de distribución.
Búsquedas auxiliares en src/tests/locales/docs/scripts/packaging; las coincidencias
no equivalen a una auditoría completa de esos componentes.

No revisados íntegramente renderer, decoders, escritores de metadatos, serializadores,
catálogos heredados, tests ni todos los cuerpos de LibraryPage. No se investiga
IPC-014-01: la lectura del punto de routing se limita a identificar la retirada.
La revisión del diff de implementación sigue pendiente; aún no existe ese diff.

## Inventario clasificado

| Clase | Referencias y tratamiento previsto |
| --- | --- |
| Exclusiva: página | `src/library/library_page.cpp/.h`: implementación de paneles, previews y hover; retirar ambos. No contienen el gestor core |
| Exclusiva: propiedad/build | `app.h:26,41,142,161`: include, enumerador, friend y miembro; CMakeLists.txt:50,92, fuentes de página. Retirar sin compactar Page |
| Exclusiva: ciclo | `app.cpp:211,308,352,405`: Initialize, animación continua, Update y Draw; retirar ramas Library |
| Exclusiva: sincronización | `app.cpp:663,675,709,716,781,891..896,907,925`: sync, layouts, hover y condición de cancelación; eliminar parte Library preservando Waterfall y modelo |
| Exclusiva: entrada | `app.cpp:1009..1287`: ramas ratón, teclado, borrado/objetivo de tags, NUM2/2 y selección radial. Retirar solo la parte Library, manteniendo prioridad de overlays/editores |
| Exclusiva: presentación | `app.cpp:1426..1465`: LIB, arrays de páginas y tooltip Library. Sustituir posición 1 por placeholder gris sin target activable; texto pendiente |
| Exclusiva: routing IPC | `app.cpp:248`: retirar únicamente SetPage(Library) en rama carpeta; conservar LoadFolder y resto del handler. Sin modo sustituto ni diagnóstico IPC |
| Exclusiva: opciones | `options_menu.cpp:19,169..198`, `settings.cpp:198..200`, packaging/default-settings.json:5..7: nombre, ciclo continuo 1..5 y listas con 4. Retirar opción; conservar IDs y decidir valor antiguo |
| Compartida: modelo | `core/folder_data_manager.*`: FolderInfo, PreviewItem, selección, previews, sesión, favoritos, historial, orden y revisión layout. Conservar ubicación, propiedad y formatos |
| Compartida: nombre heredado | AddFolder recibe `libraryLoading` (h:64..66, cpp:33..82); LoadSession lo pasa true (cpp:172). Renombrar a restauración de sesión sin cambiar selección/guardado/carga. Logs Library directory se vuelven neutrales |
| Compartida: carga | WaterfallPage::LoadPreviewTexture (cpp:370), FolderDataManager::CompletePreview/IsPreviewRequestCurrent/CancelPreviewLoads y CancelHoverLoads de App. Conservar identidad/token/ruta, lease débil y callbacks en UI; no extraer otro servicio |
| Compartida: comentarios | folder_data_manager.h alude a dos layouts y waterfall_page.h:91 a Library. Actualizar al contrato restante; conservar comentarios de responsabilidades/hilo |
| Tests activos | async_preview_tests.cpp:23,186..258,438..644,1010..1013,1113..1202,1255..1350,1444..1445; ipc_diagnostic_tests.cpp:81,428,610. Retirar consumidores/casos exclusivos, adaptar los mixtos a Waterfall sin perder vigencia, mutación, upload y cancelación de modos restantes |
| Guías vigentes | AGENTS, docs/project-context, docs/usage y docs/development contienen cuatro modos o protocolos que los recorren. Actualizarlos con implementación y evidencia; roadmap refleja definición hasta entonces |
| Histórica | docs/changes/001 y 004..014, docs/validation y antecedentes: conservar afirmaciones con su revisión/fecha. No reescribir evidencia antigua como pruebas de 002 |
| Catálogos heredados | locales/base.pot y base.po: msgids, ayuda y referencias Python a library_data.py. Sin consumidores C++ Library tras retirada; conservar material histórico y traducciones ajenas. Revisar búsquedas finales por significado, no exigir cero coincidencias Library |
| Coincidencia ajena | common/pch.h: C++ Standard Library, bibliotecas del sistema y atribuciones: conservar |

## Hallazgos necesarios para el diseño

### R002-01: valor 4 requiere decisión explícita

- Ubicación: settings.cpp:131..133; app.cpp:215,1378; options_menu.cpp:14..22,169..198.
- Evidencia: código. Load conserva enteros sin validar el dominio; arranque/drop hacen
  static_cast a Page. Las opciones incrementan 1..5; debug_page no tiene consumidor
  funcional fuera de persistencia/controles.
- Clase: riesgo de compatibilidad, prioridad alta, confirmado por inspección.
- Consecuencia: borrar solo el enumerador permitiría un modo sin Draw; quitar solo
  el texto dejaría elegir 4. Convertirlo a Viewer contradice el requisito.
- Propuesta pendiente: elección explícita y guardado al confirmar; alternativa error
  y edición manual. Ninguna aprobada. El diseño no acepta mantener Page::Library,
  un alias ni una página vacía como solución implícita.
- Coste: medio, afecta arranque/drop/opciones y configuración antigua. Validar sobre
  copias las tres claves, combinaciones, cancelación y recarga. Necesario AC-03/06.

### R002-02: retirar Library no autoriza retirar regresiones compartidas

- Ubicación: tests/async_preview_tests.cpp, PreviewRequest/PreviewMutations,
  SharedPreviews/CheckLayouts/LayoutCacheRegressions/ConsumerCancellation;
  tests/ipc_diagnostic_tests.cpp, inicialización y recorridos de páginas.
- Evidencia: código/búsqueda de llamadores. Harness usa la página y mezcla casos con
  Waterfall; las suites de cancelación también dependen de estos cuerpos.
- Clase: riesgo de regresión, prioridad media, confirmado; no fallo ejecutado.
- Consecuencia: eliminar bloques mixtos completos quitaría cobertura todavía válida.
- Propuesta: conservar casos de Waterfall/tira/Board, sustituir solicitudes Library
  solo cuando el escenario compartido siga teniendo sentido y retirar benchmarks/
  snapshots exclusivos sin reinterpretar resultados históricos.
- Coste: medio. Release/CTest y revisión del diff de tests; checks cambiarán de cantidad,
  por lo que no exigir conteos históricos como aceptación. Necesario AC-04/05.

## Diseño inicial y decisión posterior

Conservar Start=1, Viewer=2, Board=3, Waterfall=5, con hueco 4 sin enumerador.
No cambiar Start, rutas, sesión/favoritos, KTAG/KBRD ni propiedad/hilos/cancelación.
OnChar debe seguir entregando 2 a tags/Board; reservar teclas globales no exige
filtrar caracteres. El placeholder debe poder mostrar tooltip sin target, resaltado
activo o propagación del clic a la página. Waterfall mantiene índice radial 2.

La consulta inicial recibió elección de selector y texto — / Unavailable. Después
el propietario detuvo el trabajo ampliado y ordenó eliminar Library sin diálogos
ni opciones nuevas. El selector provisional fue retirado íntegramente; R002-01
queda fuera del alcance final por esa instrucción. No se afirma compatibilidad con
preferencias antiguas 4 ni se añade fallback/migración. El texto reservado permanece.

## Revisión del diff final

Evidencia de código, sobre b1681c0 más diff local simplificado:

- LibraryPage y sus dos fuentes eliminados, igual que include/friend/miembro/enum,
  CMake, inicialización/update/draw y ramas exclusivas de entrada/sync/hover.
- NUM2/2 no seleccionan modo. OnChar y editores sin cambios. Placeholder gris y
  sin target/activación; clic y doble clic no llegan al contenido de fondo.
- Save/defaults/GetPageName/ciclo de opciones ya no ofrecen Library/4; no se añadieron
  opciones ni diálogos. Sin cambios finales en main, loader o serializadores.
- Gestor/modelos core conservados; solo nombres/logs/comentarios heredados ajustados.
  Waterfall conserva tokens/rutas, revisión de layout y leases de cancelación.
- R002-02 atendido: regresiones de Waterfall/tira/Board conservadas; casos exclusivos
  retirados. Los snapshots de layout actuales generan 12 frames, frente a 24 históricos.
- Único cambio de OnIpcOpenPath: retirar SetPage(Library) al abrir una carpeta.
  Ninguna investigación o corrección de IPC-014-01.
- Búsqueda final en src/tests/CMake/defaults: sin referencias al modo Library;
  C++ Standard Library en pch es una coincidencia ajena y se conserva.

Sin otros hallazgos accionables dentro de este diff por inspección. No se revisa
el repositorio completo ni se atribuye mejora de rendimiento. La eliminación de
compatibilidad específica del 4 es una decisión de alcance del propietario, no
una corrección demostrada de datos antiguos. Validación final en result.md;
observación visual final pendiente en ese momento tras detener la campaña ampliada.

## Cierre posterior, 2026-10-23

El propietario confirma pruebas funcionales exitosas del candidato de 002 y que
el visor muestra la nueva imagen al abrirla desde Windows. Se registra aceptación
funcional global, sin matriz individual ni diagnóstico causal de IPC-014-01.
No hay nuevo diff de código que revisar: esta sesión actualiza evidencia y ejecuta
la limpieza condicional autorizada. Cantidades, excepciones y verificación en
[result.md](result.md). No se reabre la auditoría base ni el diseño descartado.

# Resultado: 018 — Tira y barra de información

Especificación: [spec.md](spec.md). Plan: [plan.md](plan.md).
Estado al 2026-10-26: **implementado y comprobado en el alcance descrito**.
Default vigente: **44 DIP base / 57,2 DIP activa**, ajustado posteriormente por
petición del propietario. Nueva identidad y pruebas en la ampliación final; las
mediciones y recorridos iniciales siguientes pertenecen al candidato de 90/117.
Rama codex/015-roadmap-design, HEAD 2d19d3d; diff funcional de 018 sin commit.
Se preservan los diez archivos locales de 015/016, comprobados por SHA256, los
cambios previos de índice/contexto/roadmap/lote y la retirada de debug_ipc.txt.
No se hizo reset, limpieza general, cambio de rama ni paquete nuevo en dist.
019–022 no se implementaron.

## Comportamiento y contratos iniciales (90/117)

- Settings y defaults del paquete/fallback usan 90 DIP si falta preferencia.
  Los tamaños guardados se conservan. Una sola ocurrencia de imagen aplicada mide
  130 % (117 DIP con base 90), cuadrada, cian 2 DIP y plena; hover/m_selected no
  determinan esa identidad. La selección usa check separado, sin rótulo permanente.
- ThumbnailStrip calcula rectángulos monótonos O(1), con 6 DIP entre vecinos;
  comparte geometría para dibujo, visibles por bisección, clip, hit testing,
  selección de caja, inserción y límites de scroll. La envolvente es dinámica;
  DPI/tamaño preferido actualizado recentra la activa. El scroll manual puede
  sacar miniaturas parcialmente del cliente, con recorte y hit testing coherentes.
- Identidad presentada independiente del índice solicitado mientras carga un
  original. ApplyLoadedImage sincroniza también el retorno temprano animado;
  no se cambia la generación/latest-wins, orden de entrega ni selección de página.
- La reordenación opera por ocurrencias para no perder rutas duplicadas, conserva
  la interpretación anterior del destino y la identidad de la activa. SetFileList
  mantiene su contrato de reutilización por ocurrencias y tokens/leases existentes.
- ViewerPage usa la envolvente real para la barra y margen mínimo 12 DIP. Ancho y
  posición acotados al cliente; nombre elidido antes que dimensiones/zoom/frame/
  índice. Puede usar dos líneas en cliente estrecho. Nombre completo en tooltip
  envuelto, con rueda para recorrerlo si la altura disponible no basta.
- Tira/barra/tooltip mantienen el hover estable; el fade continúa al salir. Barra
  y tooltip consumen sus clics/doble clic; M sigue controlando información.
  Waterfall conserva la cuadrícula/modal y su barra sin reservar una tira que
  todavía no se compone allí. Editor T y radial conservados.
- Conversión DIP local mediante DPI de Window: renderer, target y eventos Win32
  mantienen píxeles de cliente. Sin conversión global de UI. No se modifican
  loader/workers, caché, prioridades, variantes 100/200, resolución de miniaturas,
  enums, KTAG/KBRD, rutas de datos, zoom de 017, licencia ni atribuciones.

## Identidad y ejecución iniciales (90/117)

Candidato final Release SHA256:
`5F08062785AD596872047ADEEE64A1E1DA4B5D5A6B693010FF27629AEC84FCB9`.
EXE `build/windows-x64/bin/Release/lenteuv.exe` y copia aislada
`build/session-018/ui/candidate.exe`. Fuentes 2d19d3d + diff local 018; los últimos
ajustes después del build son comentarios, sin cambio funcional. Windows
10.0.26300 x64, VS 2022, SDK 10.0.22621.0, Release; 16 procesadores lógicos.

- `cmake --build --preset release`: correcto. Logs locales build.log,
  build-tests.log, build-corrections.log, build-final.log y build-delivery.log,
  todos bajo build/session-018. Configuración existente windows-x64 conservada.
- `ctest --preset release --output-on-failure`: **9/9, 17,32 s** sobre el final,
  log `build/session-018/ctest-delivery.log`. También 9/9 en pasadas intermedias.
  Primera pasada 8/9: el test nuevo de rueda suponía llegar al extremo desde un
  destino antiguo en una sola entrada. Se corrigió su preparación/recorrido para
  comprobar el gesto real por pasos conservado; no se cambió la rueda para el test.
- `settings.preferences`: **45 checks** de Settings (15 más que 017), incluidos
  default 90, clave omitida, tamaños guardados 30/50/73/90/100 y round trip. Más
  las 36 regresiones existentes de controles/zoom. EXE/defaults/user_data propios;
  resolución desde EXE comprobada antes de escribir, sin preferencias personales.
- `ui.async_previews`: **3251 checks nuevos**, total **95461**. Tamaños guardados,
  96/144/192 DPI simulados, clientes 320/480/736/1024 DIP; activa primera/medio/
  última, fracciones, intervalos frente a recorrido completo, geometría/hit ampliado,
  margen de barra, modal sin tira, pending/applied, duplicados, caja de selección,
  reordenación, límites de rueda y truncamiento Unicode. Regresiones anteriores de
  identidad, cancelación y carga conservadas. `loader.startup_cache`: 100 checks;
  se adaptó solo su destino calculado a la geometría real en lugar del antiguo 50.
- `scripts/run_ipc_diagnostics.ps1 ... -Startup`: **diez escenarios nativos**
  correctos, fallos inyectados esperados y cierre pendiente; log native.log y árbol
  native/ en build/session-018. No reapareció IPC concurrente en las pasadas de 018;
  no se declara resuelto su seguimiento histórico por ese resultado.
- `--optimization-snapshot build/session-018/final-snapshot.bgra`: **12 frames WARP**,
  anchos físicos 320/801 y ventanas simuladas 96/144 DPI, context/targets a 96 DPI
  como producción. Evita mezclar DPI del contexto con píxeles de cliente. PNG de
  inspección local, no assets nuevos ni comparación de igualdad contra el dibujo
  retirado. Inspección de 320/801: activa/borde, clipping y barra acotada/separada.
- Diff sin errores de whitespace y enlaces relativos existentes; revisión de
  responsabilidades/contratos en [review.md](review.md). No auditoría integral.

## Recorridos nativos

Aplicación real mediante computer-use, con EXE y copias propias PNG/JPEG/GIF y
nombre largo Unicode en build/session-018/ui. Capturas de ventanas reales, no
mockups. Las condiciones visuales principales se verificaron durante las pasadas
previas y se volvió a probar el candidato final en controles/modal.

Observado: activa 117 y vecinas 90 con borde/check separados; barra alineada y
separada; doble clic sobre miniatura consume sin fullscreen y sobre imagen alterna
compacto/fullscreen. En compacto el nombre se trunca y el tooltip muestra el nombre
completo; doble clic sobre barra no alterna fullscreen. El primer recorrido mostró
que el autohide movía la barra al consultar el nombre: corregido reteniendo hover
de tira/barra/tooltip y verificado estable en una pasada posterior.

Navegación End hasta GIF: marca anterior durante carga, nueva marca al aplicar,
frames visibles. JPEG compacto estrecho conserva barra y activa con vecinos
recortados al cliente. Rota con R; M oculta información. Rueda en tira ya visible
cambia scroll/recorte. Una primera rueda al entrar en tira oculta llegó al zoom del
visor (107 %): no se presenta esa entrada como scroll de tira; los controles ocultos
conservan su contrato de no interceptar. Hover/clic posterior en tira visible la
recorre correctamente.

Waterfall muestra cuadrícula sin cambios y abre JPEG modal, con nombre cerca del
borde inferior y sin espacio reservado a tira. NUM1 regresa a Viewer; R/M/Settings
operativos. T abre el editor sin rediseño; se cerró sin guardar. No se escribieron
metadatos ni se borraron imágenes. La última ventana aislada queda disponible para
revisión; sus preferencias son de prueba y pueden haber cambiado durante el recorrido.

Límites: una configuración física de pantalla, sin cambiar escalado del SO ni
certificar otro monitor/DPI físico; las tres escalas adicionales son del harness.
Drag/multiselección/duplicados se comprueban en código real del harness, sin afirmar
una matriz exhaustiva de gestos físicos. No se certifican todos los frames, pérdida
de dispositivo ni UX universal. Los checks no convierten build/CTest en prueba visual.

## Rendimiento: candidato, sin nuevas ejecuciones de base

Referencia conservada de 017, SHA256
`3021B349F6BFA5223C3C7E89D506A88DD97041EA9CD04ECF17F89ABDCA207FEB`,
[serie solicitada tras liberar recursos](../017-simplificacion-funcional/result.md).
No se ejecutó, compiló ni midió de nuevo ese EXE ni las bases 003/anteriores.

Ejecutor individual ya conservado `build/session-017/measure-candidate.ps1`,
Candidate=EXE final, Images=carpeta acordada en solo lectura, RunRoot nuevo
build/session-018/startup, Pairs=8. Fullscreen, carpeta/sesión propios, --multiple,
logging off, un acondicionamiento excluido, QPC tras Present1, misma receta general
que 017. Nueve salidas 0 y un escaneo por proceso. Log performance.log y results/CSV
locales. No hubo builds/tests o la ventana propia de UI simultáneos con la serie.

| Métrica (ms) | 017 conservado, n=8 | 018 candidato, n=8 |
| --- | ---: | ---: |
| main.entry → envío original, mediana | 356,13 | 355,33 |
| Mismo intervalo, mínimo–máximo | 344,40–375,32 | 343,51–367,34 |
| Invocación → envío original, mediana | 369,96 | 369,16 |
| Renderer, mediana | 234,75 | 231,80 |
| D3D11CreateDevice, mediana | 226,24 | 223,31 |
| Decode original, mediana | 57,63 | 56,16 |
| Upload, mediana | 18,89 | 16,22 |

No se observa empeoramiento en esta serie; diferencias pequeñas/rangos solapados
no demuestran mejora causal ni equivalencia universal. Defaults visuales distintos
(90 frente a 50), cachés/staging propios, carga de fondo no controlada y monitor/DPI
sin cotejo histórico completo limitan comparabilidad. No nueva primera apertura
fría ni p95 retrospectivo; no se usa una tolerancia porcentual.

Comprobación adicional de tira, solo candidato: ejecutor run_startup_benchmark
adaptado localmente en build/session-018/measure-strip.ps1, Scenario=strip,
Cache=warm, Warmups=1, Repetitions=2, tres posiciones representativas de la colección
(6694 JPEG en manifest). Primer intento del script original falló antes de arrancar
por fallback de DLL a una entrega 002 ya ausente; se conservó el fallo y se ajustó
solo la copia local a las DLL conservadas de la entrega final 003. No se ejecutó
ningún EXE de esa entrega ni se añadió dependencia del producto a paquetes históricos.

Artefactos: strip-readiness-final/manifest.json, samples.csv y nueve trazas;
strip-readiness-final.log. Tres acondicionamientos separados de seis muestras,
todos procesos propios con salida 0 y cero fallos de miniaturas.

| Posición | n | Preparación destino tras original, mediana ms | Envío de tira tras original, mediana ms |
| --- | ---: | ---: | ---: |
| A | 2 | 10,33 | 462,88 |
| B | 2 | 11,73 | 921,13 |
| C | 2 | 19,64 | 929,04 |

La segunda columna temporal incluye scroll/visibilidad y asentamiento, no solo
CPU de generación. La tabla Summary del script resume envío del original, no la
tira: arriba se usan las columnas Strip* del CSV. Sin inferir percentiles fiables
con n=2. Working set pico mediano por posición: 142452736 / 145422336 / 139474944
bytes; CPU muestreada: 773,44 / 1335,94 / 1031,25 ms por proceso hasta salida.
Son observaciones del candidato, no comparación histórica ni uso estable. GPU/VRAM
no muestreadas. No se atribuye ahorro global al dibujo ni se rellenan referencias
faltantes con series de otro hash. No se abre una campaña general de Waterfall/003.

## Disposición

| Criterio | Evidencia |
| --- | --- |
| AC-01 | Defaults y persistencia 45 checks; tamaños/130 % en matriz geométrica |
| AC-02 | Identidad, hover/multiselección/duplicados en regresiones; borde/tamaños/check en WARP y nativo |
| AC-03 | Visibles diferenciales, hits, scroll, caja y reordenación reales; recorrido de bordes/doble clic/scroll visible |
| AC-04 | Geometría de barra y margen en matriz; snapshots estrechos; nombre completo y precedencia en nativo |
| AC-05 | Release/CTest 9/9, diez escenarios nativos y recorridos delimitados arriba |
| AC-06 | Serie de arranque frente a 017 sin empeoramiento observado; tira solo candidato y límites históricos explícitos |

Guías usage/project-context/development, índice y roadmap actualizados. Implementación
018 terminada en el alcance comprobado; cambios pendientes de commit. No se inicia
019 ni se añade dock, iconos, marca o gestos posteriores por efecto secundario.

## Ajuste de tamaño solicitado tras revisar la evidencia — 2026-10-26

El propietario confirmó haber comprobado la evidencia y pidió sustituir los tamaños
iniciales por la mejor aproximación a su captura de Settings. Se usó únicamente
la posición del slider; el texto de fondo y los demás checks no son instrucciones
ni parámetros que deban copiarse.

Pista aproximada x=289–949, control x=421: avance 20 %. Código verificado en
OptionsMenu::BuildControls/Draw: interpolación lineal entre 30 y 100.
Default elegido: **30 + 0,20 × 70 = 44 DIP**. Activa al 130 %: **57,2 DIP**.
Es una aproximación visual, no una medición exacta de escala física del PNG.

Cambios limitados a los valores de Settings, packaging y fallback de tira; se
conservan ACTIVE_SCALE=1,3, borde, geometría, barra, slider 30–100 y preferencias
guardadas. 90 sigue siendo un valor guardado válido; no se fuerza un reset.
Se añadió 44 a las matrices existentes y se adaptaron sus expectativas de default.
Spec/plan/guías/roadmap/lote se actualizan; 016 y las mediciones iniciales quedan
como historial, sin atribuirlas al nuevo candidato.

Candidato Release SHA256:
`B48BCF8D173E0EB191E3B77608731CD2A36DD848268D7F34E40F67A11A9F0078`.
`cmake --build --preset release`: correcto, build/session-018/build-default-44.log.
`ctest --preset release -R 'settings.preferences|ui.async_previews|loader.startup_cache'
--output-on-failure`: **3/3, 4,28 s**, ctest-default-44.log en ese directorio.
48 checks de Settings; 3899 checks de presentación dentro de 96109 de previews;
100 de startup/cache. Comprueban default, valores guardados y geometría/entradas
con base 44. No se repitió la suite completa de nueve tests para este ajuste.

**Sin nuevas pruebas de rendimiento**, por instrucción expresa del propietario.
Tampoco se atribuye al nuevo hash el 9/9 o la matriz visual del binario anterior.
Copia limpia para revisión: build/session-018/default-44/candidate.exe, con
user_data/fixtures propios. No se modifican datos personales ni se reinicia la
ventana anterior que puede estar usando el propietario.

## Continuidad posterior

019–022 se implementaron después de este cierre, por autorización expresa. Su evidencia vive en [022](../022-neutralizacion-marca/result.md); no se atribuye a las compilaciones históricas de 018. El default 44/57,2 y las preferencias guardadas permanecen.


## Corrección visual conjunta con 020

Por petición posterior: texto flotante original 13 unidades cliente, sin
multiplicación por DPI, y gap 3 DIP (antes 6). Se usa una sola geometría para
visibles, dibujo, selección, scroll y reordenación. No se fuerzan preferencias.
Release/CTest 9/9 y 12 snapshots WARP del candidato A975803A…; evidencia y límites
en [020](../020-navegacion-superior/result.md). No se atribuyen a este hash las
mediciones previas ni se repite rendimiento para la corrección.

El propietario aprobó expresamente la corrección de fuente original y separación el 2026-10-26, junto con los ajustes del dock; [registro de aprobación en 020](../020-navegacion-superior/result.md). No se infieren nuevas mediciones ni pruebas adicionales.

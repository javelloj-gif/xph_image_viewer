# Resultado: 017 — Simplificación funcional

Especificación: [spec.md](spec.md). Plan: [plan.md](plan.md).
Estado al 2026-10-26: **implementación y comprobaciones de 017 completadas con
los límites de evidencia descritos abajo**. La primera comparación quedó bloqueada;
el propietario liberó recursos y solicitó expresamente repetir. Esa nueva serie
ya solapa el rango histórico y no reproduce la separación completa anterior.
No se demuestra equivalencia causal ni ausencia universal de regresión.
No se implementaron 018–022.

## Código y compatibilidad

Rama comprobada: `codex/015-roadmap-design`, HEAD
`9a334ad2b775bd5fe7244e805384ea4bed2038cd`. Se preservaron los documentos locales
015/016, lote/índice/contexto/roadmap previos y la eliminación de debug_ipc.txt.
Durante la implementación no se hizo reset, limpieza general, cambio de rama
ni paquete en dist. La confirmación posterior en Git fue solicitada por el
propietario al indicar proceder secuencialmente; incluye solo archivos/hunks de
017, conservando documentos previos compartidos y de 015/016 sin confirmar.

- Settings y defaults de packaging usan animated_zoom=true cuando falta preferencia.
  Se respetan true/false existentes y effects. Las flechas conservan su camino propio.
- Se retiraron cuatro miembros/claves de declaración, lectura y escritura; el parser
  y resolución de datos permanecen intactos. Claves antiguas desconocidas se ignoran;
  el guardado solo escribe preferencias vigentes. La ausencia del archivo conserva
  el contrato previo: Load guarda defaults y retorna false.
- Se retiraron controles/traducciones exclusivos, acciones contextual 6/8 (sin
  reutilizar sus IDs), C/I, estado de inversión y geometría de esquinas. El botón
  de cierre usa el rojo previo; la tira conserva etiquetas/reflejos/clips sin fondo
  opcional. Settings, etiquetas y atenuación de Waterfall siguen presentes.
- Viewer alterna fullscreen con doble clic izquierdo siempre. App conserva el orden
  radial/páginas/tira; añade antes un guard de opciones/etiquetas para impedir que
  un doble clic sobre un modal alcance el visor. Este fallo se reprodujo sobre el
  primer candidato y se verificó corregido sobre el final.
- No se modificaron workers, loader, cancelación, caché, prioridades, enums,
  formatos KTAG/KBRD, atribuciones ni rutas. Thumbnail_width sigue siendo 50.

## Pruebas ejecutadas

Candidato Release compilado con VS 2022, SDK 10.0.22621.0, Windows 10.0.26300 x64.
SHA256 del producto comprobado en UI y mediciones:
`3021B349F6BFA5223C3C7E89D506A88DD97041EA9CD04ECF17F89ABDCA207FEB`.
Fuentes: HEAD citado más diff local 017; los últimos ajustes a cabeceras fueron
solo comentarios de responsabilidad, sin cambio de comportamiento tras el build.

- `cmake --preset windows-x64` y `cmake --build --preset release`: correctos.
  Logs locales `build/017-build.log` y `build/017-build-final.log`.
- `ctest --preset release --output-on-failure`: pasada final **9/9, 17,57 s**,
  `build/017-ctest-final.log`. La primera pasada fue 8/9, 16,22 s: fallo
  `concurrent real App secondaries` de IPC. Repetición solo de IPC: 1/1, 7,93 s
  (`build/017-ipc-recheck.log`). No se declara corregido IPC-003-CONCURRENT-01
  ni se atribuye su intermitencia a 017; no se tocó ese componente.
- Nueva `settings.preferences`: **30 checks de Settings + 36 de controles**,
  activos en Release. EXE/defaults copiados a árbol con user_data y marcador propios;
  comprueba resolución desde EXE antes de toda escritura. Ausencia, clave omitida,
  true/false, round trip de claves antiguas, effects/opacidades/idioma/tamaño/orden
  y defaults de paquete. El JSON fuente y preferencias personales no se escriben.
  Los controles usan App/Viewer reales sobre ventana oculta propia sin Initialize:
  rueda, punto, 1:1 y región en las cuatro combinaciones de preferencias, camino
  independiente de flechas, C/I sin handler, P, doble clic y guard de Settings/radial.
- `scripts/run_ipc_diagnostics.ps1 -Executable build/windows-x64/Release/async_preview_tests.exe
  -Fixtures tests/fixtures -RunRoot build/session-017/native -Startup`: diez
  escenarios nativos correctos, incluidos fallos inyectados esperados y cierre con
  trabajo pendiente. Log `build/017-native.log`. Se ejecutó antes del guard final;
  el cierre/cargas no se modificaron después. CTest final cubre el candidato final.
- `git diff --check` sin errores. Búsqueda en src/locales/packaging sin los símbolos
  retirados; las únicas apariciones activas de las claves están en datos/asserts de
  regresión. [Revisión local](review.md), sin hallazgos de código pendientes en alcance.

## Recorrido visual nativo

Staging propio `build/session-017/ui`, EXE candidato, copias de PNG/JPEG/GIF de
fixtures, user_data/defaults propios y clave antigua doubleclick_toggle=false.
Se usó la habilidad computer-use. Una invocación inicial de launch devolvió
`computer-use request budget exhausted`; la ventana sí existía y list_windows
permitió recuperar el control. No fue un fallo de producto.

Observado en ventanas/capturas del candidato:

- Viewer compacto con PNG alfa; cierre rojo. Doble clic en imagen pasa a fullscreen
  aun con clave antigua false; Settings general/Viewer sin filas retiradas, zoom
  marcado, checkbox effects conservado. Contextual sin inversión/esquinas y con P/G/M/R.
- Primer candidato: doble clic dentro de Settings cambiaba fullscreen detrás del
  modal. Tras el guard, el mismo gesto conserva tamaño/estado y Settings visible.
- Waterfall conserva cuadrícula y atenuación; clic en JPEG abre visor modal y doble
  clic sobre imagen alterna a fullscreen. No se añadió composición de tira de 019.
- Rueda cambia zoom con default nuevo, G dibuja tercios, C/I no producen efecto
  visual. Navegación por derecha pasa PNG → JPEG → GIF. GIF muestra frames distintos
  y Espacio muestra estado de pausa. Board abre la imagen y su doble clic vuelve
  al visor conservando tamaño compacto, sin fullscreen forzado.
- Cierre por Escape de los procesos propios al terminar. No se escribieron metadatos
  ni se borraron imágenes. Los controles se probaron sobre copias.

Límites: capturas puntuales no certifican fluidez ni todos los frames. No se hizo
una matriz visual exhaustiva de DPI/monitores, todos los overlays/selecciones o las
cuatro combinaciones de zoom; estas últimas tienen regresión de estado/animaciones
real en el harness. No se atribuye cobertura visual a CTest ni a la compilación.

## Rendimiento: solo candidato y referencia conservada

T-01 satisfecha por decisión expresa del propietario. **No se ejecutó, compiló ni
midió de nuevo la base**. Referencia final SHA256
`874F5783B219E24FE1AD74B330BBF9A344EF2DAB0E3664738CED579CF5FF50D6`:
[serie posterior](../003-diagnostico-rendimiento/internal-startup-result.md),
[frío](../003-diagnostico-rendimiento/cold-internal-result.md),
[métricas históricas y límites](../003-diagnostico-rendimiento/performance-result.md).
No se usan las series del EXE anterior de ese último informe como si fueran del final.

Se preparó `build/session-017/measure-candidate.ps1` desde el ejecutor existente
compare_startup_folder, retirando parámetro/iteración de baseline y conservando
solo candidate. Artefacto local, no nuevo framework. Reproducibilidad: candidato
Release, `-Images F:/Pictures/FotosTEMP -Pairs 8`, RunRoot nuevo en build, fullscreen,
carpeta explícita en solo lectura, sesión propia, defaults actuales, logging off,
--multiple, QPC, un acondicionamiento excluido, 250 ms de asentamiento y salida 0.
Confirmación acotada del mismo caso con Pairs=3, sin repetir otros escenarios.
Resultados/CSV/settings en `build/session-017/startup` y `startup-confirm`;
logs `build/017-performance.log` y `build/017-performance-confirm.log`.

| Métrica (ms) | Referencia final conservada, n=8 | Candidato, n=8 | Confirmación candidato, n=3 |
| --- | ---: | ---: | ---: |
| main.entry → envío original, mediana | 334,69 | 385,45 | 434,97 |
| Mismo intervalo, mínimo–máximo | 317,00–362,31 | 364,71–405,31 | 377,62–445,17 |
| Invocación → envío original, mediana | 347,08 | 400,65 | 451,30 |
| Renderer, mediana | 204,22 | 255,78 | 256,29 |
| D3D11CreateDevice, mediana | 199,03 | 246,22 | 246,66 |
| Decode original, mediana | 57,40 | 61,90 | 61,10 |
| Upload, mediana | 17,30 | 16,58 | 17,43 |

Los 22 procesos nuevos (incluidos tres acondicionamientos separados) salieron 0 y
registraron un escaneo por arranque. Presentación significa envío después de
Present1 correcto, sin certificar aparición física. No hubo builds/tests paralelos
a las mediciones. La referencia fría n=1 es 1995,63 ms totales / 473,18 internos;
no se repitió ni se calcula rango/p95 ni se compara causalmente con sesiones calientes.

Comparabilidad limitada: mismo equipo y ruta/protocolo de carpeta, sin acreditar
igualdad de carga de fondo, monitor/DPI o inventario de archivos desde la referencia.
Había otras aplicaciones gráficas abiertas; no se cerraron aplicaciones del usuario.
La referencia usa animated_zoom=false; el candidato true, sin gestos de zoom durante
la métrica. Staging y caché propios por serie; SO sin purgar. No se midieron GPU/VRAM,
CPU/RAM ni tira/Waterfall/rendimiento de navegación del EXE final en esta porción:
la referencia final enlazada no aporta una serie comparable para todas esas métricas.
No se importan valores de otro hash ni se afirma mejora por retirar geometría.

### Repetición solicitada tras liberar recursos

El propietario informó que había liberado recursos de cómputo y pidió repetir.
Se ejecutó únicamente la misma serie del candidato, sin build ni base, con Pairs=8,
un acondicionamiento excluido y nueva raíz `build/session-017/startup-resources-released`.
Log `build/017-performance-resources-released.log`. Mismo hash del candidato,
carpeta, defaults, fullscreen y protocolo; nueve salidas 0 y un escaneo por proceso.

| Métrica (ms) | Referencia conservada, n=8 | Candidato con recursos liberados, n=8 |
| --- | ---: | ---: |
| main.entry → envío original, mediana | 334,69 | 356,13 |
| Mismo intervalo, mínimo–máximo | 317,00–362,31 | 344,40–375,32 |
| Invocación → envío original, mediana | 347,08 | 369,96 |
| Renderer, mediana | 204,22 | 234,75 |
| D3D11CreateDevice, mediana | 199,03 | 226,24 |
| Decode original, mediana | 57,40 | 57,63 |
| Upload, mediana | 17,30 | 18,89 |

Disposición de AC-06: comprobación acotada completada; se retira el bloqueo de la
primera serie porque la separación completa por encima del rango histórico ya no
se reproduce tras el cambio de condición informado por el propietario. Los rangos
solapan; la mediana sigue 21,44 ms por encima y algunas muestras exceden el máximo
histórico. Eso no se oculta ni se convierte en equivalencia estadística. No hay
umbral porcentual inventado ni atribución causal al código o a la liberación de
recursos. No se agrupan las tres series con diferentes condiciones como una muestra.

La decisión cubre este arranque posterior con la evidencia disponible, no frío,
UX universal, GPU/VRAM o cada ruta de navegación. Si se exige demostrar equivalencia
causal bajo entornos idénticos, esa conclusión sigue sin estar sustentada por estas
series históricas; no se mide otra vez la base para fabricarla. No se cambió renderer,
efectos, prioridades, calidad ni seguridad para perseguir las cifras.

## Disposición de aceptación

| Criterio | Evidencia / estado |
| --- | --- |
| AC-01 | Defaults, true/false y combinación con effects comprobados en regresión; default visible en Settings y rueda nativa |
| AC-02 | Retirada por diff/búsqueda y recorridos Settings/contextual/C/I; superficies compartidas conservadas |
| AC-03 | Viewer/Waterfall nativos con clave antigua false; guard modal corregido y comprobado; Board conserva su semántica |
| AC-04 | 30 checks aislados de persistencia/compatibilidad y defaults; sin datos personales |
| AC-05 | Release, 9/9 CTest y recorridos anteriores; fallo IPC intermitente inicial conservado, límites visuales explícitos |
| AC-06 | Comparación acotada completada: tras liberar recursos y repetir por petición expresa, rangos solapados; separación anterior no reproducida. Mediana mayor y límites conservados, sin equivalencia causal afirmada |

Guías usage/project-context/development y roadmap actualizadas. 017 completado en
el alcance implementado y comprobado descrito; no certificación universal. El diff
queda revisable; 015/016 y sus documentos siguen preservados.

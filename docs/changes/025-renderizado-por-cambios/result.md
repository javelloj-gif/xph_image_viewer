# Resultado 025 — 2026-10-26

Implementado sobre 6e66535 con cambios previos 023/024 preservados. El candidato
conserva el fix visual 023. La validación inicial no publicó ni instaló el paquete;
el cierre local autorizado se registra al final.

## Comportamiento

Window retiene invalidación atómica por solicitudes e input/paint. RenderFrame
continúa entregando carpetas/texturas y avanzando estado/temporizadores, pero
consume el flag solo visible y sin minimizar. Run espera hasta 16 ms cuando no
dibuja. Se invalidan también revisiones de carpetas, lotes del loader, propiedades
animadas y transiciones de tira. La reproducción animada visible conserva su
barra de progreso fluida; completada o detrás de otra página no dibuja por sí sola.
No se cambian calidad, aceleración, FPS de transiciones, gestos ni persistencia.

## Comparación nativa

Misma GPU RTX 4070 Ti SUPER, driver 616.56; PNG sintético 2560×1440 y ventana
1280×720 exterior. Copias separadas con user_data/seed propios, modo --multiple
--windowed. No builds/tests durante capturas. Referencia instrumentada antes de
corregir: EXE SHA256 `DD072A78D471AF4D961966E740EC03E9A875C197BCF5878FC669CFBF4C15B06D`.
Final: `25A0FE8581830E54069DC2DE8C97435C0D37E34B7E249B9F7285A36A654F298D`.

| Estado asentado | Present/s referencia | Present/s final | Media máximo motor por PID referencia | Final |
| --- | ---: | ---: | ---: | ---: |
| Visible | 36,67 | 0 | 3,1794% | 0% |
| Dock retenido por Tab | 143,95 | 0 | 8,1039% | 0% |
| Minimizado | 37,38 | 0 | 0,9481% | 0% |
| Oculto | 36,26 | 0 | 1,1910% | 0% |
| Restaurado y estable | 35,39 | 0 | 1,5480% | 0% |

Ventanas QPC de referencia: 8,344 / 7,579 / 7,518 / 7,556 / 7,573 s.
Final: 8,165 / 7,510 / 7,485 / 7,470 / 7,504 s. Se contaron todas las llamadas
Draw/Present en esas ventanas; no se perdieron eventos de traza ni hubo errores
Present. El final tuvo cero Draw además de cero Present en los cinco tramos.
Cada tramo tiene solo tres muestras PDH válidas, sin errores PDH en las capturas
comparadas. Los porcentajes son el máximo entre instancias de motor del PID,
promediado entre esas muestras, no utilización total de GPU ni tiempo GPU.

El original ya estaba aplicado antes del primer tramo. Entre fin del tramo oculto
y comienzo del restaurado estable se registraron 109 Presents durante los 3 s de
asentamiento: restaurar no quedó congelado. El proceso final produjo 216 Draw
en toda la ejecución, incluyendo arranque y transiciones. Cierre normal, salida 0.
Los contadores no certifican por sí solos todos los píxeles visibles.

Actividad global NVIDIA media por tramo, referencia/final: 43,67/1,67%,
26/5%, 9/1,67%, 10,67/0%, 24/6%. No se atribuye toda esa diferencia al visor:
dispositivo global, clocks adaptativos y otras aplicaciones/composición varían.
La evidencia causal más directa es la retirada de llamadas redundantes del PID.

## Evidencia y aceptación

- AC-01/02/04: comparación anterior, HWND verificado por PID y estados nativos
  registrados. Se conservan entregas ocultas y se dibuja al restaurar.
- AC-03/05: Release y **CTest 9/9** final, 17,87 s; ui.async_previews añade
  **14 checks** de entrega oculta sin target dibujable, invalidación por input/paint,
  último tick de propiedad animada, animación finita completa, progreso entre frames
  y animación detrás de otra página, además del fade de la tira sin entrada auxiliar.
- Smoke nativo `run_ipc_diagnostics.ps1 ... -Startup`: **10 escenarios correctos**,
  incluidos modos Viewer/Waterfall/Board, entrega GPU tras IPC con ventana oculta,
  hide-to-tray/cierre y fallos controlados de inicialización.
- Revisión acotada: [review.md](review.md). Diff sin errores de whitespace.

Primera ejecución CTest: 8/9, fallo «concurrent real App secondaries». El caso
coincide con el seguimiento intermitente aceptado IPC-003-CONCURRENT-01; no se
demuestra su causa ni se declara corregido. Recheck de la suite: 1/1, 7,80 s;
ejecución completa posterior: 9/9. No se desactivaron checks.

## Identidad y artefactos locales ignorados

- Referencia válida: `build/gpu-025-reference/gpu-idle-e1949581dcb844d4affb6ab5c2247778`.
- Final: `build/gpu-025-final/gpu-idle-93334304d89646b0a4cd1e5b727e5158`.
  manifest Completed=true y hash del recolector
  `96DCFA88DDF71A589D168E2B8FFA17E6A2B58350EE2F1A53D3C4CD66D5345DE9`.
- Un candidato intermedio 680701BF… también tuvo cero frames en tramos estables;
  se volvió a medir el final tras preservar la barra de progreso animada.
- Intentos anteriores de referencia descartados por muestras PDH inválidas o
  HWND=0/cierre incompleto del recolector; no se usan en las tablas. La comparación
  válida usa ventana propia, estados confirmados y cierre normal.
- Logs: `build/session-025-final-build.log`, `build/session-025-ipc-recheck.log`,
  `build/session-025-native-smoke.log`; CSV/summary.json en las carpetas indicadas.
- Portable limpio: `dist/image_viewer_025_gpu_20261026.zip`, SHA256
  `A2E6B492BA8941A7CF03FB774B673D5172EEE5709BB263E6AD7A22B6E2365409`.
  Staging nuevo `build/staging/519945c867304c69a729ee7c69491954`; su EXE coincide
  con el medido. Runtime/defaults/locales, sin datos personales ni caches previas.

## Límites y continuidad

Una ejecución corta comparada por binario, tres muestras de sensores por estado;
no es una campaña de 60 s × 3 ni prueba universal de carga/energía. No se midió
duración de comandos GPU ni se garantiza 0% de todo el dispositivo. La aceptación
posterior del propietario no agrega un nuevo log ni una matriz de escenarios reales.
El sondeo CPU de hasta 16 ms permanece; cargas nuevas aún pueden subir texturas
ocultas y generar picos legítimos. Quedan fuera cuotas de uploads, animaciones
grandes, carpetas masivas, oclusión por otras ventanas y pérdida de dispositivo.
No se certifica toda UX mediante build/tests ni se cambia el dt limitado existente.

## Aceptación del propietario y cierre local — 2026-10-26

El propietario indica «Marca las pruebas como satisfactorias» y «Realiza
respectivos commit y merge». Se marcan **satisfactorias y aceptadas** las pruebas
de la entrega 023–025, con la evidencia y límites anteriores intactos. Esta
aceptación no reescribe el fallo intermitente IPC ni lo declara corregido.

Integración local en `codex/015-roadmap-design` mediante la rama
`codex/025-gpu-idle-approved`. No hacer push: el propietario reserva los ajustes
finales previos a publicar en el repositorio maestro para próximas sesiones cortas.
Continuidad y alcance cerrado en [closure.md](closure.md).

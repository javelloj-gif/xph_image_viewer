# Revisión de picos GPU — 2026-10-26

## Cobertura y evidencia

Base: 6e66535; Git inicialmente limpio. Cambio local 023 solo ajusta la leyenda.
Habilidad aplicada: lenteuv-architecture-review, sin delegación. Guías leídas:
AGENTS, contexto, desarrollo, workflow, roadmap, problemas conocidos; antecedente
OPT-007 de la revisión de oportunidades 003, que ya advertía presentación continua.

Inspeccionados: Run/RenderFrame, composición Viewer, UpdateDock/DrawDock, OnPaint,
entrega de imagen; Window::RequestRedraw y WM_PAINT/WM_SIZE; dispositivo/swap chain,
EndFrame/DrawBitmap; ViewerPage::Update/Draw/DrawImageFitted/DrawOverlayInfo y
HasActiveAnimations; ThumbnailStrip::Update/HasActiveAnimations; Waterfall::Update
y rutas de dibujo; AnimationController/FrameSequencer; entrega/upload de ImageLoader.
Fuera de cobertura completa: Board, todos los callbacks de escáneres, driver,
residencia VRAM y ejecución GPU/DWM. No se auditó el proyecto Python histórico.

Hardware reportado: RTX 4070 Ti SUPER 16 GB, monitor LG27GL850 144 Hz. Falta
confirmar resolución/configuración efectiva, driver, EXE medido y escena.

## Log aportado

Original leído sin modificar: GPU-Z Sensor Log.txt, 73 431 bytes. SHA256:
`1DA83D9D883807BDE2D4629C15953BCCED402D67231E4A5BFA805738D2C3E005`.
No se copia el CSV a Git. Análisis con csv/statistics de Python estándar,
script auxiliar local ignorado en `build/session-023/analyze_gpu_log.py`.

368 muestras entre las etiquetas 2026-10-26 16:09:54 y 16:16:02 (6 min 8 s).
Las horas se transcriben del archivo: no declara zona horaria. Cadencia nominal
de 1 s, con diferencias observadas de 0/1/2 s; no asumir una serie perfecta.
Media por muestra, no ponderada por tiempo; p95 por rango más cercano.

| Intervalo de etiquetas | Muestras | Media GPU | p95 | Máximo | Muestras en 0% |
| --- | ---: | ---: | ---: | ---: | ---: |
| Registro completo | 368 | 18,73% | 48% | 60% | 60 |
| 16:10:00–16:10:59 | 60 | 18,55% | 38% | 47% | 7 |
| 16:11:00–16:11:59 | 59 | 20,71% | 44% | 51% | 0 |
| 16:12:00–16:12:59 | 60 | 26,78% | 53% | 60% | 0 |
| 16:13:00–16:13:59 | 60 | 27,68% | 54% | 59% | 0 |
| 16:14:00–16:14:59 | 60 | 20,10% | 48% | 54% | 5 |
| 16:15:00–16:15:59 | 60 | 1,32% | 5% | 5% | 40 |

14 muestras >=50%; 12 estrictamente >50%. Máximo 60% a las 16:12:37.
Entre 16:11 y 16:13 la carga no llega a cero: hay actividad durante minutos,
no solo un máximo aislado de apertura. Al final vuelve a 0–5%, pero el archivo
no identifica qué acción produjo esa transición. Temperatura GPU: 45,5 °C al
inicio, máximo 52,7 °C, 50,3 °C al final; no implica por sí sola potencia elevada.

Solo hay temperaturas, ventiladores y GPU Load. **No contiene frecuencias,
potencia, VRAM, motor, PID ni eventos de abrir/minimizar/cerrar**. No se puede
deducir un estado de energía ni atribuir todo el porcentaje al visor. El usuario
confirma persistencia minimizada; es evidencia de reporte, no un tramo etiquetado
independientemente en el CSV. El log confirma el patrón del dispositivo.

## Segundo log con eventos reportados — 2026-10-26

El propietario proporciona una nueva captura en la misma ruta, con apertura a
17:52:30, minimización a 17:53:30 y cierre a 17:54:15. Estos eventos proceden
del reporte del usuario, no de instrumentación del proceso. El archivo actual
sustituye al anterior; su resumen histórico y hash se conservan arriba.

Identidad del segundo archivo: 30 845 bytes, SHA256
`899DB74848EBE2CD6AC5787372FA512899701DD30D2F3813BA8D5A8C4FC2155F`.
154 muestras de 17:51:48 a 17:54:21, todas separadas por un segundo. Misma selección
de sensores: no incorpora clocks, potencia, PID, motor ni memoria utilizada.
Lectura sin modificaciones; script local `build/session-023/analyze_gpu_stages.py`.

Se asigna cada muestra al estado vigente según su etiqueta temporal, con límites
inclusivos al inicio y exclusivos al siguiente evento. El muestreo de sensores
y el reporte de acciones no garantizan sincronización subsegundo. p95 por rango
más cercano y media aritmética por muestra; no se estima energía.

| Estado según horas del usuario | Etiquetas incluidas | n | Media | Mediana | p95 | Máximo |
| --- | --- | ---: | ---: | ---: | ---: | ---: |
| Antes de abrir | 17:51:48–17:52:29 | 42 | 4,38% | 3% | 14% | 31% |
| Visible | 17:52:30–17:53:29 | 60 | 29,17% | 37% | 50% | 51% |
| Minimizado | 17:53:30–17:54:14 | 45 | 9,27% | 8% | 16% | 45% |
| Cerrado | 17:54:15–17:54:21 | 7 | 0,43% | 0% | 3% | 3% |

Sensibilidad a transiciones: quitando los primeros cinco segundos de cada tramo
abierto, visible conserva media 31,09% (55 muestras, máximo 51%) y minimizado
9,20% (40 muestras, máximo 18%, p95 15%, mínimo 1%). El 45% minimizado corresponde
exactamente a 17:53:30 y no debe describirse como pico sostenido tras minimizar.
Visible tiene cuatro muestras >=50%, dos estrictamente >50%. Cerrado tiene seis
muestras de 0% y una de 3%; solo se registraron siete segundos tras el cierre.

Conclusiones de esta captura:

- Abrir, minimizar y cerrar coinciden con diferencias claras en la carga global.
  La fase visible no es únicamente una ráfaga de los primeros cinco segundos.
- Minimizar reduce la carga media aproximadamente un 68% frente a visible, pero
  la actividad residual persiste durante el tramo posterior a la transición.
  No se reproducen picos >50% en la fase minimizada de esta captura.
- El cierre coincide con retorno a 0–3%, lo que fortalece la relación temporal
  con el visor. El periodo posterior es corto y la base anterior ya tenía picos:
  no restar medias para declarar una contribución exacta del proceso.
- AR-001 y AR-003 ganan prioridad diagnóstica. La reducción al minimizar también
  indica que el coste efectivo depende de visibilidad/composición; no demuestra
  que todas las llamadas de dibujo tengan igual coste en los dos estados.
- No demuestra que el dock estuviera retenido, que hubiera uploads o que fueran
  estos los responsables. Falta atribución por PID/motor y medir Draw/Present.

La recomendación sigue siendo presentar por cambios y suspender dibujo invisible
conservando entregas/IPC. No se implementa aquí ni se promete 0% global de GPU.

## Hallazgos

### AR-001 — Dibujo/presentación incondicional en reposo

Ubicación: `src/core/app.cpp:294` Run, :318 RenderFrame incondicional, :339
RenderFrame; `src/rendering/renderer.cpp:284` Present1. Código confirmado;
defecto de eficiencia, prioridad alta. Se borra el target completo, dibuja y
presenta incluso sin cambios. needsContinuousRender solo decide esperar hasta
16 ms al final. RequestRedraw invalida HWND, pero OnPaint (:1010) está vacío:
la invalidación no decide si producir un frame.

Consecuencia candidata: repetir escalado/dibujo/composición de la misma imagen.
No son 60 FPS exactos: Present puede esperar y los mensajes despertar antes.
Propuesta principal: separar actualización/entrega de dibujo/presentación y
presentar cuando cambie el resultado visual o avance una transición.
Coste medio/alto: inventario completo de cambios, notificaciones y vencimientos;
riesgo de resultados congelados si se omite un productor. Conservar contratos
del hilo UI. Validar por PID los Presents y consumo en reposo, además de carga,
navegación e IPC sin entrada adicional. Candidato posterior, no implementado.

### AR-002 — Retener UI se interpreta como animación continua

Ubicación: `src/core/app.cpp:322–328` y :1453 UpdateDock;
`src/ui/thumbnail_strip.cpp:101`. Código confirmado; defecto de clasificación,
prioridad alta. Hover/foco/menús mantienen m_dockRemaining=0.9, razón para omitir
la espera aunque alpha=1. Editar una nota también activa la ruta continua. La
tira trata un plazo de inactividad como animación durante fases visualmente quietas.

Consecuencia candidata: más frames según refresco/Present y posición del puntero.
Present1(1,...) sincroniza con refresco, no fija 60 FPS: con 144 Hz el escenario
continuo podría producir más frames, sin afirmar que se hayan medido 144 FPS.
Propuesta: separar transición, retención y próximo auto-hide; despertar en el
vencimiento. Coste pequeño/medio dentro del scheduler; comprobar Tab, hover,
menús, fades, caret y efectos on/off. Por sí sola no elimina AR-001.

### AR-003 — Ocultación/minimización sin suspensión explícita

Ubicación: `src/core/window.cpp:271` ignora SIZE_MINIMIZED;
CheckMinimized (:122) no participa en Run/RenderFrame. El target conserva tamaño
y estos métodos carecen de guard de visibilidad. Bandeja usa Show(SW_HIDE)
en `src/core/app.cpp:197`. Código confirmado; riesgo, prioridad media/alta.
El coste real oculto depende de DXGI/DWM/driver y no está medido. Coincide con
el escenario reportado de actividad persistente minimizada, sin probar causalidad.

Propuesta: suspender Draw/Present oculto/minimizado conservando entregas, IPC,
cancelación y cierre; invalidar al restaurar. Coste medio y decisión explícita
de si las animaciones avanzan en tiempo real o se pausan mientras no son visibles.
Validar bandeja, restauración e IPC con trabajo pendiente. No resolver solo con
DXGI_STATUS_OCCLUDED: Microsoft indica que no llega con flip model, usado aquí.

### AR-004 — Uploads por lotes sin presupuesto por frame

Ubicación: `src/loader/image_loader.cpp:597–665`. Código confirmado; riesgo,
prioridad media. ProcessPendingTextures drena lotes disponibles sin cuota de
bytes/tiempo; una animación sube todos sus frames faltantes. No hay evidencia
de resubir todas las texturas por frame: se comprueba HasGpuBitmap en miniaturas
y se omiten frames animados ya cargados.

Hipótesis: copias/residencia de grandes lotes causan ráfagas durante carga; no
explica por sí sola actividad indefinida sin solicitudes. Medir bytes/tiempo
antes de introducir cuotas y prioridad visible. Coste medio/alto: más demora
para miniaturas, callbacks diferidos y cancelación más compleja. Mantener creación
GPU y callbacks en UI; validar obsolescencia y cierre. Trabajo condicionado a medición.

### AR-005 — Una animación terminada conserva el criterio continuo

Ubicación: `src/loader/frame_sequencer.cpp:20,36,61–62`, Run :325 y
`src/waterfall/waterfall_page.cpp:78`. Código confirmado; defecto de criterio,
prioridad media. Agotar loops marca complete pero no paused. Update deja de
avanzar, mientras Run y hover aún consultan !IsPaused. No explica una imagen
estática ni se ha reproducido este escenario en GPU.

Propuesta: actividad efectiva que descarte complete/empty y próximo vencimiento
de frame, conservando pausa/reinicio/progreso. Coste pequeño/medio; validar
animación finita/infinita/pausada y hover. Candidato posterior.

## Soluciones y tradeoffs

Viewer usa interpolación cúbica al no activar pixelado; Waterfall también usa
alta calidad. Repetir escalado y dibujo de canvas grandes puede amplificar
AR-001; composición alfa/DWM añade otro coste candidato. No están cuantificados.

| Opción | Beneficio esperado | Coste o tradeoff |
| --- | --- | --- |
| Presentación por cambios y vencimientos | Evita frames innecesarios conservando GPU y calidad | Recomendación principal; requiere notificar todos los productores y temporizadores |
| Suspender dibujo oculto/minimizado | Evita trabajo invisible | Definir política de animación y restauración; mantener entregas/IPC |
| Corregir criterios de actividad | Evita retenciones falsas | No elimina el dibujo periódico de AR-001 |
| Límite global de FPS | Mitigación sencilla | Reduce fluidez/respuesta y sigue dibujando en reposo; no opción inicial |
| Presupuesto de uploads | Suaviza ráfagas durante carga | Retrasa miniaturas/frames y complica cola/cancelación |
| Cachear escalado o capas | Reduce coste cuando sí hay frames necesarios | Más VRAM e invalidación por zoom, DPI, rotación y tamaño |
| Rectángulos sucios DXGI/D2D | Reduce región repintada | Complejidad con transparencias, transformaciones y buffers; después de evitar frames enteros |
| Reducir interpolación/transparencia | Puede abaratar cada frame | Cambia calidad/estética; no justificado antes de medir |
| CPU/WARP | Contraste funcional | Traslada carga a CPU, puede empeorar energía/fluidez y conserva el problema del scheduler |

Aceleración GPU no exige renderizar contenido estable. VSync sincroniza pero no
detecta cambios. Una espera arbitraria mayor puede empeorar respuesta; dormir
indefinidamente sin notificaciones deja cargas sin aplicar. ProcessPending de
carpetas y loader se ejecuta hoy desde RenderFrame: deben conservarse entregas
sin presentar, con despertares de búsquedas/escáneres y plazos UI. HasActiveAnimations
de Viewer solo cubre el fade final del texto de zoom: todavía no enumera todos
los vencimientos de metadatos/overlays.

## Medición propuesta, no ejecutada

1. Tiempos recibidos para el segundo log; falta registrar hash/Release,
   driver, resolución/Hz efectivos, tamaño/opacidad, contenido y caché.
2. Separar carga de reposo; esperar a colas vacías y fades terminados. Capturar
   60 s y repetir tres veces por caso, mismos datos y sin builds/tests concurrentes.
3. Matriz: app cerrada; estática con puntero fuera; dock retenido; menú; minimizada;
   bandeja; animación pausada/finita/infinita; Waterfall cargado; Board; scroll;
   carga de muchas miniaturas como escenario separado. EXE/datos aislados.
4. Correlacionar GPU-Z con PID/motor (3D/Copy), DWM y otros procesos con ETW/GPUView
   o equivalente. Añadir clocks/potencia/VRAM al próximo registro. Porcentaje
   aislado no demuestra consumo energético ni proceso responsable.
5. Instrumentación optativa posterior: motivos de despertar, Draw/Present,
   bytes/tiempo de upload y colas. Tiempo CPU alrededor de Present no es tiempo GPU.
6. Comparar media/p95/máximo, distribución y frames/s. No extrapolar al contador
   NVIDIA/GPU-Z la definición del porcentaje del Administrador de tareas.

Aceptación propuesta para una implementación posterior: estática asentada sin
nuevos Draw/Present hasta cambio/evento que lo requiera; oculto sin dibujo;
restauración y callbacks visibles sin entrada adicional; mantener IPC, fades,
caret, animaciones, scroll y calidad. Acordar umbrales GPU/energía con la línea
base; no prometer 0% del dispositivo completo.

## Fuentes primarias

- [Present1 y sincronización](https://learn.microsoft.com/en-us/windows/win32/api/dxgi1_2/nf-dxgi1_2-idxgiswapchain1-present1).
- [DXGI_STATUS y límite de occlusion con flip model](https://learn.microsoft.com/en-us/windows/win32/direct3ddxgi/dxgi-status).
- [GPUView y correlación CPU/GPU](https://learn.microsoft.com/en-us/windows-hardware/drivers/display/using-gpuview).
- [Microsoft: porcentaje del motor más ocupado en Task Manager](https://devblogs.microsoft.com/directx/gpus-in-the-task-manager/).

Revisión completa dentro de cobertura; atribución causal pendiente. No se modificó
el scheduler ni se demostró una mejora de consumo GPU.

# Plan: 017 — Simplificación funcional

Especificación: [spec.md](spec.md). Preparado el 2026-10-26.
Estado: T-02–T-08 completadas en alcance; repetición expresa tras liberar recursos
y límites de comparación registrados. Ver [resultado](result.md) y [revisión local](review.md).

## Base y fronteras

Git base: 9a334ad2b775bd5fe7244e805384ea4bed2038cd; trabajo local previo de 015/016
en codex/015-roadmap-design. Preservar ese diff y la retirada de debug_ipc.txt.
Referencia binaria: portable final 003, hash registrado en 015. Por instrucción
expresa del propietario del 2026-10-26, reutilizar las mediciones conservadas de la
última modificación: no volver a ejecutar, compilar ni medir la base sin cambios.
El staging nuevo bajo build/session-017 se destina al candidato y sus pruebas.

Settings posee preferencias; App coordina entradas; ViewerPage implementa gestos
y overlays; OptionsMenu compone controles; ThumbnailStrip dibuja su superficie.
Mantener esas responsabilidades y `friend`, sin nuevo framework ni abstracciones.

## Tareas de implementación

- [x] T-01 / AC-06, referencia: adoptar identidad y mediciones conservadas de la
  última modificación, enlazadas abajo. Sin nuevas ejecuciones de la base. La
  comprobación de regresión del candidato permanece pendiente en T-08.
- [x] T-02 / AC-01,04: poner animated_zoom=true en código y defaults de packaging;
  retirar los cuatro campos persistidos exclusivos doubleclick_toggle,
  desaturated_corner_buttons_and_corner_menus, draw_control_panel_backplate y
  show_cyberpunk de declaración/carga/guardado/defaults. No cambiar el parser ni
  los restantes valores. Claves desconocidas continúan ignoradas.
- [x] T-03 / AC-02,03: quitar las filas correspondientes de Settings y las entradas
  Invert Colors/Cyberpunk del contextual; retirar m_invertImage y sus toggles I,
  DrawCyberpunkCorners y toggles C; mantener IDs de acciones restantes sin reutilizar
  los retirados. Doble clic izquierdo de ViewerPage alterna fullscreen siempre,
  conservando la precedencia de entrada existente en App y páginas.
- [x] T-04 / AC-02,05: simplificar color del botón de cierre al rojo existente y
  retirar solo el bloque opcional de fondo de tira. Conservar sombras, reflejos,
  clips, textos, tercios, overlays y todas las superficies no exclusivas.
- [x] T-05 / AC-02: eliminar cadenas exclusivas de POT/PO activos y texto de ayuda C;
  actualizar responsabilidades/comentarios de métodos modificados preferentemente
  en declaración. No eliminar referencias a estética cyberpunk de otros componentes.
- [x] T-06 / AC-01,04: añadir regresión de preferencias al harness productivo
  existente, mediante modo --settings-preferences-tests, script aislador y CTest
  settings.preferences. Usar Settings real, sin App::Initialize ni servicios de
  arranque, con EXE copiado y user_data propio junto a él. El script crea el árbol
  antes de arrancar; verificar que la resolución selecciona esa raíz antes de escribir.
- [x] T-07 / AC-05: retirar del snapshot de tests/async_preview_tests.cpp la
  asignación del campo backplate eliminado; ajustar únicamente expectativas del
  efecto retirado. Mantener los checks de visibilidad, identidad, cancelación y layout.
- [x] T-08 / AC-01..06: Release/CTest, recorridos nativos y comparación con base;
  corregir fallos, completar result y actualizar guías/roadmap con evidencia real.

No añadir marca histórica a nombres nuevos; settings.preferences y session-017
son neutrales. No tocar loader, workers, caché ni presupuestos de decodificación.

## Regresión de configuración

Casos con checks activos en Release:

1. Sin settings: construcción conserva animated_zoom=true; Load falla por ausencia
   sin modificar ese default. Un archivo válido sin esa clave conserva true.
2. Archivos con animated_zoom=false y true: carga y round trip conservan cada valor.
3. Archivo antiguo con doubleclick_toggle=false y los otros tres flags retirados
   true: carga válida; round trip omite las cuatro claves, sin fallo ni reseteo
   de animated_zoom, effects, thumbnail_width, opacidad, idioma y sort_mode.
4. Archivo packaging/default-settings.json copiado: zoom animado habilitado y
   preferencias conservadas; su contenido no modifica el JSON fuente.

El harness solo escribe archivos sintéticos propios y se ejecuta en una copia
aislada por corrida. No cambiar temporalmente datos reales ni fiarse del cwd: la
resolución de Settings parte de la ubicación del EXE.

## Validación nativa y rendimiento

Configurar/build Release y ejecutar `ctest --preset release --output-on-failure`.
Registrar conteos y fallos reales, incluido el seguimiento IPC intermitente si
reaparece; no atribuirlo al cambio sin contraste. No reconstruir en paralelo con
benchmarks ni mezclar muestras de distintas configuraciones.

Recorridos en staging propio: Viewer y modal Waterfall, fullscreen/compacto, doble
clic sobre imagen, tira y overlays, menú contextual y Settings, C/I sin efecto,
G/P/M/R y navegación conservados, zoom rueda/teclado/1:1/región con effects true/false
y animated_zoom true/false, estáticos/animados y cierre con carga pendiente.
Board conserva sus gestos, sin fullscreen forzado al editar por doble clic.

Referencia conservada: [cierre e identidad 003](../003-diagnostico-rendimiento/closure.md),
[última serie posterior](../003-diagnostico-rendimiento/internal-startup-result.md),
[último frío](../003-diagnostico-rendimiento/cold-internal-result.md) y
[métricas y límites](../003-diagnostico-rendimiento/performance-result.md).
Seleccionar en cada informe los resultados del EXE final con SHA256
874F5783B219E24FE1AD74B330BBF9A344EF2DAB0E3664738CED579CF5FF50D6;
no mezclar cifras de candidatos anteriores como si describieran ese binario.
015 ya comprobó preservación de esa identidad. Solo consultar informes/manifests;
no copiar ni lanzar la base para producir muestras nuevas.

Concentrar las comprobaciones nuevas en el candidato: build/CTest, recorridos
afectados y mediciones proporcionales que puedan contrastarse con las conservadas.
Reutilizar entradas, monitor/DPI, settings, caché y protocolo documentados cuando
estén disponibles. No ejecutar run_startup_comparison.ps1 con ambos binarios ni
los harness de comparación que vuelvan a lanzar la base. Usar el modo individual
del ejecutor correspondiente únicamente para el candidato, sin una campaña general
nueva ni repeticiones arbitrarias adicionales. Las pruebas de UX incluyen zoom
animado; distinguir el nuevo default de la configuración de cada métrica histórica.

Por métrica, registrar fuente, identidad, n y dispersión disponible de la referencia
y las condiciones del candidato. No fabricar p95 o rango de variación a partir del
frío n=1. Diferencias de entorno/configuración limitan la comparación y deben quedar
explícitas; no son motivo para medir otra vez la base. La ausencia de GPU/VRAM u
otra métrica histórica se declara como límite, sin exigir instrumentación nueva
para rellenar retrospectivamente esa referencia. Un empeoramiento reproducible del
candidato fuera de la variación histórica disponible bloquea la entrega; confirmar
solo el caso del candidato afectado. No introducir tolerancia fija de 5/10 % ni
afirmar ausencia universal de regresión con evidencia que no la cubre.

## Documentación, cierre y recuperación

Actualizar usage (doble clic siempre; C retirado), tabla de preferencias y
descripciones de Viewer en project-context; development por settings.preferences.
Conservar evidencia histórica de 001–016 y nombres de paquetes anteriores.
Guardar logs/settings/capturas/CSV solo en build; result enlaza criterios con
resumen reproducible. No generar otro portable en dist para este plan documental.

La recuperación del código revierte exclusivamente el futuro diff de 017, nunca
los cambios de 015/016 ni user_data. La migración omite claves retiradas solo al
guardar preferencias desde el producto; las pruebas no afectan datos personales.

T-08: Release y CTest 9/9, diez escenarios nativos de arranque, recorridos afectados
realizados; cobertura exacta y límites en result. Las primeras series del candidato
repitieron tiempos superiores al rango histórico; tras liberar recursos, el propietario
pidió repetir y la serie n=8 solapó el rango conservado (mediana aún mayor).
AC-06 se dispone como comprobación acotada, sin equivalencia causal afirmada. No se hicieron
nuevas ejecuciones ni compilaciones de la base. No iniciar más series generales,
repetir el frío ni modificar renderer como efecto secundario de 017.

Ajuste local de T-03: el recorrido reprodujo fullscreen detrás de Settings por
falta de guard modal en App::OnMouseDoubleClick. Se añadió el guard de opciones/
etiquetas antes de páginas y una regresión de Settings; se conservan radial, tira,
Board y Waterfall en su orden existente. No cambia otros gestos.

Continuidad: 017 completado en alcance y evidencia descritos, con comparación
limitada a arranque posterior. No iniciar automáticamente 018 ni optimizaciones de
003. El resultado conserva las tres series y su causa no determinada. No hay paquete
nuevo ni commit de cambios ajenos. La confirmación posterior solicitada incluye
solo el diff exclusivo de 017; los documentos compartidos con 015/016 se preservan.

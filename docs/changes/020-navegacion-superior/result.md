# Resultado 020

Implementado sobre 019 y 018 locales, HEAD 2d19d3d. Dock Outline original con seis acciones, hit testing y geometría por DPI, objetivos 44 DIP, tooltip, foco Tab/flechas/Enter/Escape, espera 900 ms y transición 180 ms respetando effects. Sort & Filter deshabilitado. Retirados estado, dibujo y eventos exclusivos del radial. Viewer añadido a menús de Waterfall/Board; NUM1 conservado.

Accesibilidad MSAA: toolbar y seis hijos simples con nombre/rol/estado/geometría/acción. El proveedor COM conserva lease débil, sin propiedad de App/GPU, y rechaza llamadas tras expirar el dueño. WM_GETOBJECT se entrega en UI; enlace oleacc. Regresiones comprueban ObjectFromLresult, nombre, deshabilitado y dueño expirado, además de geometría a 96/144/192 DPI, ancho 320/480/736/1024, espera, foco, comandos y precedencia modal.

Release correcto; dos suites afectadas 2/2, 2,62 s, build/session-020/build-final.log y ctest-final.log. Hash D0A165905E7D872BAB3E7218D969E1B93A5B4FE6CEE8B61CBFF4CD8C3EAEFD13. Recorrido previo de esta porción (8B4BFD2E…): Tab muestra dock, rotación real, Settings abre y mantiene panel. El árbol accesible de Windows enumera los seis nombres y filtro disabled. Se detectó tooltip de foco anterior al usar ratón y se corrigió; el hash final tiene regresiones automatizadas, el recorrido final se repetirá al cerrar el lote. No atribuir al hash final toda la UI de la revisión previa.

AC-01..04 implementados y comprobados con las coberturas anteriores; recorrido final y matriz extendida del lote se registrarán al cierre. Sin campaña de base ni métricas nuevas por esta porción. Sin paquete/commit. No certifica un lector de pantalla concreto ni todos los DPI físicos.


## Corrección visual posterior del propietario — 2026-10-26

Estado: implementada y comprobada en el alcance descrito. La nueva instrucción
sustituye el anclaje superior izquierdo por **centrado superior**. Se conserva el
historial 016. La captura adjunta coincide píxel a píxel con concepts/outline.png
de 016 (aunque la codificación PNG y su hash difieren); no se interpretan sus
labels como comandos o nuevas funcionalidades.

Se sustituyen los símbolos genéricos por vectores originales detallados:
columnas con fotos/divisiones, tres fotografías libres, arco de giro con cuadro,
dos etiquetas con agujeros, ordenación con triángulo/embudo y engranaje continuo
de seis dientes. Se conservan las seis acciones, filtro deshabilitado, auto-hide,
teclado y MSAA. Trazos redondeados, glifos 28 DIP dentro de celdas 44 DIP,
separadores finos, hover/activo cian y superficie oscura según la referencia.
Dock y tooltip quedan centrados; la franja de activación sigue su ancho, también
al pasar a varias filas por ancho/DPI. No se copia una imagen raster al renderer.

DockIcons posee seis pares de geometrías CPU y estilo de trazo de la factory D2D
actual; los reutiliza hasta cambiar factory. Crea/dibuja en UI, restaura transform
al salir y no toca imágenes, colas, leases, caché, prioridades ni datos persistidos.
App conserva comandos/estado y delega solo el artwork.

Ajustes asociados de 018: texto **13 unidades cliente**, valor exacto leído en
HEAD 2d19d3d previo a 018, sin multiplicación por DPI; gap **3 DIP** en la geometría
compartida, mitad de 6. Activa +30 %, default 44/57,2 y preferencias guardadas
siguen intactos. El tooltip de nombre usa el mismo tamaño recuperado.

### Ejecución

- `cmake --build --preset release`: correcto; build/session-020-visual-correction/build.log.
- Suites afectadas 2/2, 2,27 s; después CTest completo **9/9, 15,52 s**, ctest-full.log.
  48 checks Settings, 37 de controles y **96320** de previews productivos.
- Regresiones: centro/tooltip acotado a 96/144/192 DPI y ancho 320/480/736/1024;
  separación exacta de 3 DIP entre activa y vecinas; conserva precedencia/comandos.
- `async_preview_tests.exe --dock-snapshot build/session-020-visual-correction/dock.bgra`:
  **12 frames WARP**, dos anchos, 96/144 DPI y tres estados. Verifica texto <=13
  (formato original presente), geometrías dentro del canvas, reutilización de
  geometría y transform restaurado. PNG derivados revisados frente a la referencia:
  detalles de los seis símbolos, colores, divisores, centro, texto y gap.
- Recorrido Computer Use sobre copia aislada: fullscreen y compacto, Tab/árbol
  MSAA con seis nombres y filtro disabled; Rotate, Settings en franja compacta;
  JPEG estrecho con dock en dos filas; doble clic exterior conserva compacto;
  Waterfall sin dock, modal con dock centrado y retorno desde su botón Waterfall.
  Cierre Alt+F4 observado; ninguna ventana QA de esta corrección quedó abierta.
- Smoke de ZIP extraído con test/user_data propios: --multiple y --lite, tres modos,
  NUM2 reservado y salida normal 0. ZIP e instalador generados desde staging nuevo;
  el instalador **no se instaló** en la máquina.

EXE SHA256: `A975803A72C4424F7DD8ECE301640689AD8F55DCD139BFF8926290497A6CCFC1`.
ZIP `dist/image_viewer_020_visual_20261026.zip`, SHA256
`AC24CDCC730E51D66CB0EC43B5EBFE1DDA1AE6D43708E104BF03595E0957F333`.
Instalador actualizado: `dist/image_viewer_setup_0.90.0.exe`.
Los builds, staging, PNG y logs quedan locales ignorados. No se hizo commit.

### Límites

No se repiten mediciones de rendimiento ni se ejecuta/recompila la base histórica.
Las cifras anteriores de 017–022 pertenecen a sus hashes y no prueban rendimiento
del nuevo EXE. El recorrido cubre un monitor físico; el resto de DPI es simulado.
Dos capturas intermedias del modal no mostraron todo su contenido y este reapareció
tras entrada; se registra como límite de la observación, sin diagnóstico de captura
frente a producto ni afirmación de estabilidad universal. Los 12 frames WARP fueron
completos. No se certifica aquí pérdida de dispositivo, actualización de una
instalación antigua ni un lector de pantalla concreto.

Revisión local acotada: [review.md](review.md). Las comprobaciones anteriores
sustentan esta corrección visual; no equivalen a aceptación posterior del propietario.


## Aprobación del propietario — 2026-10-26

El propietario confirma «Cambios aprobados» después de recibir la corrección:
iconos Outline detallados, dock/tooltip centrados arriba, metadatos a 13 unidades
originales y separación de miniaturas de 3 DIP. Se registra como aceptación de
estos cambios y del candidato entregado, sin inferir una matriz de pruebas no
reportada ni ampliar la aprobación a rendimiento o a los límites descritos arriba.

Identidad de la entrega aprobada: EXE A975803A… y ZIP AC24CDCC… (hashes completos
en la sección de ejecución). La aprobación no modifica código ni genera un nuevo
binario. Los cambios permanecen locales, pendientes de commit; no se publica ni
se limpia el workspace como efecto de esta confirmación.

Actualización de cierre, 2026-10-26: el propietario autoriza posteriormente el
commit para cerrar la sesión. La entrega aprobada y su evidencia se conservan;
no se modifica el candidato ni se repiten pruebas o mediciones por este registro.
El diagnóstico de picos de GPU corresponde a la siguiente etapa, aún sin iniciar.

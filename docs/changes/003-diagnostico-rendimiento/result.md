# Resultado: 003 Diagnóstico de arranque, IPC y reposo

**Ampliación vigente, 2026-10-23:** implementación del
[plan autorizado](implementation-plan.md) de arranque asíncrono y miniaturas
persistentes. Release/CTest 8/8 verificados; comparación y entrega en preparación.
Esta porción no está cerrada funcionalmente ni certifica 100 ms. El resultado
histórico inferior conserva su revisión y no describe los nuevos contratos.

**Seguimiento de decoder:** [009](../009-decoder-compartido/result.md) comparte
apertura en carga normal y preserva fallback fresco; bytes/metadata y llamadas WIC
validados, Release/CTest 5/5. Medición local, sin cerrar pendientes globales de 003.

**Seguimiento de texto:** [008](../008-formatos-texto/result.md) implementa formatos
privados acotados con fábrica mutable independiente; equivalencia controlada y medición
local, no mejora global demostrada. Baseline inicial y controles documentados allí.

**Seguimiento adicional:** [007](../007-layouts-por-cambios/result.md) implementa
reutilización de layouts Library/Waterfall con invalidación compartida, mismos rects
y 24 frames WARP idénticos. Solo medición local del cálculo; pendientes 003 conservados.

**Seguimiento de optimización:** [006](../006-optimizaciones-locales/result.md)
implementa reconstrucción indexada, recorrido visible de tira y guard de metadatos
invisibles, con equivalencia offscreen y medición local. La auditoría de abajo fue
previa; no se atribuye mejora de arranque/reposo global ni se cierran los pendientes 003.

**Cambio de prioridad del propietario:** [análisis de fuentes y ruta conservadora](source-optimization-review.md)
completado: reconstrucción de tira, layouts/visibilidad, texto, decoder, arranque
secundario, uploads caducados y presentación continua. Evidencia de código, sin ahorro
cuantificado ni implementación. No se continúa buscando un síntoma concreto como
siguiente actividad. La primera porción recomendada elimina trabajo repetido sin
cambiar calidad/cadencia; scheduler por cambios requiere mapa completo antes de adopción.

**Última porción:** [Explorer y reposo sin logging](explorer-validation.md): JPEG y
PNG vía Abrir con con ayuda manual, asociaciones JPG/PNG conservadas, contadores
CPU/memoria/GPU del receptor propio. Portable asociado y build validado tienen hashes
distintos. [Protocolo de primera sesión](first-session-protocol.md) preparado, sin
medición real de arranque ni cambios de fuentes. AC-04 dispone de base acotada;
003 sigue abierto por arranque/caso representativo y límites restantes de entrada.

Fecha: 2026-10-22. Estado: porción diagnóstica IPC implementada y ejecutada;
003 permanece abierto. Especificación: [spec.md](spec.md). Plan: [plan.md](plan.md).
**Seguimiento actual:** [validación real posterior a 005](real-validation.md): PNG
presentado, flags completos, diálogo/salida 2 real y muestra de CPU/memoria en reposo.
La evidencia histórica de las secciones siguientes se conserva con sus límites.
**Seguimiento AR-008:** [seis arranques aislados](ar008-reproduction.md), JPEG en
todos sin F11/resize; discrepancia de captura y oclusión constatadas, sin fallo
persistente de producto reproducido. No hubo cambios de fuentes ni CTest nuevo.
**En esta porción AR-003 no se corrigió: hubo reproducción controlada.**
Seguimiento: [005](../005-entrega-ipc/result.md) registra la corrección posterior
de transporte/error/payload y la matriz convertida a regresión (5/5, par concurrente,
errores explícitos, sin duplicados; Release/CTest 4/4). La evidencia siguiente
corresponde al binario de 003, no al contrato actual. 003 continúa abierto.

## Implementación

SingleInstanceIpc expone un snapshot por objeto con detección, intento/resultado de
escritura y error Win32, recepción/posteo/fallo de post y estado de escucha. Los campos
son atómicos, observados de forma no transaccional, sin rutas ni persistencia nueva.
La pipe predeterminada conserva exactamente su nombre; el harness amigo usa un endpoint
privado único. Se conservan sondeo, reintentos, booleano, formato, centinela y cierre.
Se corrigieron comentarios/log que prometían un cierre garantizado inexistente.

El cuerpo del callback IPC de App se extrajo a OnIpcOpenPath. Comparación con HEAD:
idéntico omitiendo indentación/espacios. Window y el callback productivo invocan el mismo
handler. Se añadieron acceso amigo y contexto de test para observar imagen/bitmap reales,
sin introducir un modo CLI diagnóstico en el EXE de producto.

El modo --ipc-diagnostics del harness de 004 usa procesos emisores reales, pipe propia,
Window::HandleMessage y App::OnIpcOpenPath reales, WIC y contexto D2D WARP software.
No ejecuta App::Initialize ni crea swapchain/ventana visible. Verifica ruta aplicada y
bitmap disponible; no equivale a observar imagen presentada en la ventana del producto.
El script prepara EXE, JPEG/PNG copiados (Unicode/espacios) y user_data local. Hijos con
plazo de 5 s, cierre de handles y limpieza de procesos propios incluso al fallar.
Los logs/datos quedan bajo build, nunca se usan imágenes ni pipe personales.

## Entorno y revisión

Base: `ac4a5da`, rama `codex/repository-foundation`, más R1/R2/004 sin commit preservados.
La revisión ejecutada incluye la instrumentación de esta porción. Windows NT
10.0.26300.0 x64, MSVC 19.43.34809, SDK 10.0.22621.0, CMake 4.4.2. Backend WARP,
WIC con cero workers; no ficha de rendimiento del hardware. CIM no estuvo disponible;
no se infirieron CPU/GPU reales ni consumo. El coste de instrumentación no se midió.

SHA-256 de los binarios de la validación final:

- lenteuv.exe: `E2F4FD72BBE746D1D550C83FDF92EBE746BB2F40E7C14482D93DED69A6EF20DE`
- async_preview_tests.exe: `3AA700E736B9F6EC3A7E1E071434E092C47BBA97FD854D288F2535489EFAD2CB`

La matriz se ejecuta con el segundo, enlazado contra las mismas implementaciones.
No se presenta el hash del visor como si se hubiera recorrido su UI completa.

## Matriz observada

| Escenario | Observación |
| --- | --- |
| Control directo sin sondeo | Mensaje recibido y posteado; handler aplicó JPEG con bitmap |
| Cinco segundas aperturas mediante Initialize, receptor asentado | 5/5 detectaron receptor e intentaron enviar; error Win32 2 (ERROR_FILE_NOT_FOUND), cero handlers/aperturas |
| Receptor sin bombear temporalmente | Escritura directa exitosa; imagen sin cambiar antes del bombeo; PNG Unicode y bitmap aplicados después |
| Ruta inexistente | Se recibió/posteó y el handler la rechazó; imagen anterior conservada |
| Ruta vacía | Escritura de cero bytes devolvió éxito; servidor no emitió solicitud y no cambió imagen |
| Dos clientes simultáneos con receptor existente | Ambos fallaron con error 2; no se observó primario adicional |
| --multiple/--lite | Parser y rama de bypass del hijo comprobados; no arranque completo de App con esos flags |

Resumen final: 5 intentos por sondeo, 0 escrituras exitosas, 0 aplicaciones;
3 mensajes no vacíos recibidos/posteados (controles JPEG, PNG e inválido), 0 fallos de post.
Los hijos diagnósticos codifican el error como 1000+Win32: exit=1002 significa error 2;
**no** es el exit code observado del wWinMain productivo. Initialize continúa retornando
false tras detectar al receptor aunque falle SendMessage; App/wWinMain conservan su
tratamiento AlreadyRunning. Esa confusión se confirma por código, no se reparó.

Los logs del receptor registraron conexiones sin datos durante los intentos por sondeo.
La secuencia concuerda con el mecanismo estático: Initialize abre/cierra una conexión
para detectar, el servidor atiende ese cliente vacío y cierra/recrea su instancia; la
segunda apertura en SendMessage puede caer en ese intervalo. Ante ERROR_FILE_NOT_FOUND
no reintenta (solo ante ERROR_PIPE_BUSY). Controles directos establecen que el transporte,
handler y decodificación/upload pueden funcionar sin el sondeo previo en este entorno.
No se atribuye a este mecanismo todo fallo IPC histórico ni la lentitud de arranque.

## Validación

```powershell
cmake --preset windows-x64
cmake --build --preset release
ctest --preset release --output-on-failure
ctest --preset release -R ipc.delivery_diagnostics --verbose
git diff --check
```

Release correcto; **CTest 4/4 pasó**: tagging, rutas, 98 checks async y diagnóstico IPC.
El éxito de ipc.delivery_diagnostics comprueba recogida del diagnóstico y controles;
no exige ni afirma que las cinco aperturas con sondeo funcionen. Se repitió el test
IPC tras ajustar el script para PowerShell 5.1/Unicode y se mantuvo la misma matriz.
Los intentos iniciales fallaron en readiness y comparación de rutas del harness;
se corrigieron sus expectativas sin cambiar reintentos ni protocolo productivo.

| Criterio de 003 | Estado |
| --- | --- |
| AC-01 | Concretado para porción IPC; ficha/caso reportado de rendimiento pendientes |
| AC-02 | Pendiente: primera ejecución real y posteriores, sin reiniciar la sesión automáticamente |
| AC-03 | Parcial: matriz aislada con procesos, handler y bitmap; faltan presentación visual, App completo --multiple y elección entre primarios simultáneos |
| AC-04 | Pendiente: CPU/GPU/memoria en reposo |
| AC-05 | Hallazgo AR-003 actualizado con reproducción y propuesta; informe general pendiente |

## Revisión, límites y siguiente paso

[review.md](review.md) registra cobertura, revisión de contratos y AR-003 actualizado.
No hay ACK de apertura ni elección atómica; PostMessage fallido todavía tiene su riesgo
previo de propiedad del payload y cierre conserva IO bloqueante. No se indujeron esos
fallos ni se afirma resolver AR-005. Tampoco se tocó AR-006, Library o preferencias.
No se midieron arranque, memoria o rendimiento, ni se observó presentación de imagen.

Siguiente recomendado: especificar una corrección independiente AR-003 (siguiente ID
005 al registrarla), delimitar el intervalo tras sondeo y hacer distinguible el fallo
al enviar. Definir resultado esperado antes de cambiar App/wWinMain; cubrir propiedad
del payload si falla PostMessage. Evitar mezclar por defecto ACK, mutex de elección,
cancelación de IO o migraciones. La matriz actual debe pasar a exigir apertura válida
cuando se implemente la corrección; no mantener una pérdida de petición como regresión
aceptable del producto. El resto de la investigación 003 continúa abierto.

# Resultado: 003 arranque asíncrono y miniaturas persistentes

**Entrega 2026-10-24:** implementación del [plan autorizado](implementation-plan.md)
en fuentes `b0e98f8`, rama `codex/003-arranque-cache`. Código y evidencia automatizada
entregados; **003 permanece abierto para aceptación del propietario y primera sesión
real de Windows**. La meta de 100 ms no está acreditada. El diagnóstico histórico
inferior conserva su revisión y no describe los contratos nuevos.

## Resultado actual

Decodificación original y enumeración/ordenación CPU fuera de UI, generaciones y
cancelación, publicación GPU/UI en principal y sesión pendiente preservada. Cuatro
workers con techo de tres miniaturas, incluido Board sin nueva variante de disco.
Tira/Waterfall comparten caché exacta 100/200 con SQLite de Windows y Compression API,
cuota adaptativa, LRU y fallos prescindibles. Sin cambios de IDs, formatos KTAG/KBRD,
asociaciones, protocolo IPC, calidad, escalador, efectos ni proceso residente.

La [comparación completa](performance-result.md) contiene n, mediana, p95, máximo,
procedimiento, hashes y límites. Original enviado: medianas 297–300 ms, frente a
369–386 ms de base. El renderer (medianas 218–221 ms) domina el tiempo medido.
Waterfall preparado: p95 67–72 ms frente a 681–688 ms de base; **vacío empeora**,
con p95 1,22–1,95 s y medianas próximas a 1 s. Tira conserva su animación; el destino
preparado no equivale al final del desplazamiento. Se registran CPU/E/S/memoria/GPU
adicionales de primera generación; no se afirma mejora en toda condición.

## Candidato limpio para el propietario

- EXE: `dist/lenteuv_qa_003_20261024_async_cache/lenteuv.exe`.
- ZIP: `dist/lenteuv_qa_003_20261024_async_cache.zip`.
- SHA256 EXE: `A8C54E3B42B5811EA17CCF347AA91A7FFF4C2808BF19EE45BD4E6C7F0AF04213`.
- SHA256 ZIP: `47734AF95ED795C7776A9A39C8407CDF0AC36F77CD182E24A6DA1ECDE9188DB5`.

Mismo EXE medido. Staging nuevo, runtime/imports verificados, locales, licencia y
atribuciones conservados. user_data contiene defaults y sesión vacía, sin cachés,
logs ni datos de ensayo. La prueba nativa del portable usó otra copia bajo build;
la carpeta de entrega permanece sin ejecutar. Guía: [functional-test.md](functional-test.md).

002 permanece aceptado e intacto: EXE SHA256
`42432671B4BE7602C4F2B4060586CFE5B6EB36AE0D382B84312038CBE60BF23A`, ZIP
`4F3387DE83D7331C1D044FF7FA677392B3FF6BCA33DAF3E218012FB42956931A`,
comprobados después del empaquetado. No se reabre su limpieza ni la base/AR-005.
Los artefactos de medición/build actuales siguen siendo necesarios y se conservan.

## Evidencia y aceptación

| Criterio | Resultado y límite |
| --- | --- |
| AC-01 | Ejecutores reproducibles, entorno/configuración/EXE/datos identificados y hashes comparados; CSV/JSON y detalles privados locales |
| AC-02 | 30 aperturas por posición/escenario de base y 003 preparada/vacía; primera apertura de otra sesión Windows pendiente, sin simular disco frío |
| AC-03 | Último CTest IPC correcto y diez escenarios nativos, incluido receptor arrancado. IPC-003-CONCURRENT-01 conserva un fallo previo concurrente intermitente; sin causa ni protocolo modificado. IPC-014-01 sigue cerrado por reporte anterior |
| AC-04 | CPU/peak working set/E/S lógica y GPU por PID; muestras separadas de 15 s de primer uso, n=1 por posición. No certifican reposo habitual sin diagnóstico ni E/S física |
| AC-05 | Costes, tradeoffs, censuras, revisión y candidatos de medición registrados; no se atribuye toda latencia fría a una sola causa |
| AC-06 | Generaciones, gating foreground, cancelación, entrega en UI y snapshots comprobados; recorrido visual/interacciones del propietario pendiente |
| AC-07 | BGRA exacto, variantes independientes, Unicode, invalidación/reemplazo/borrado, corrupción de payload, raíz inválida, LRU con cuota de 4 MiB y locks/conexión/proceso adicionales; sin agotar volumen real |
| AC-08 | Guardado de sesión pendiente, ordenación tardía y borrados conocidos comprobados. Sin migraciones de datos ni nuevas interacciones; aceptación manual global pendiente |
| AC-09 | Candidato limpio/hash del mismo EXE medido y comparación documentados; confirmación DXGI separada de envío de frame, sin certificar 100 ms |

Release correcto; último `ctest --preset release --output-on-failure`: **8/8,
18,27 s**. `loader.startup_cache` incluye 94 checks con WIC/WARP y datos propios.
Recorrido nativo de diez escenarios correcto sobre b0e98f8. El portable normal,
sin variables de diagnóstico, respondió a entradas de Viewer/tira/Waterfall/Board
y cerró con salida 0 en una copia aislada. Eso no certifica calidad visual completa.

Se obtuvieron 120 observaciones comparables de reutilización (10 ciclos por
posición/modo/versión), sin nuevas decodificaciones ni fallos. El intento inicial
de base C no estaba preparado tras 30 s; se conserva y excluye de esa comparación.
Con 60 s quedó preparado antes de los ciclos. No se ocultó como medición caliente.

## Pendiente para cierre

Recoger [aceptación funcional](functional-test.md) de este EXE, coordinar la primera
sesión real y disponer expresamente los límites restantes, incluida la intermitencia
del harness IPC y el coste de Waterfall vacío. No se modificaron fotografías ni
se cambiaron asociaciones; datos/cachés de pruebas permanecen aislados. No declarar
aceptación por CTest, por el portable previo ni por el retorno de Present.

## Historial previo

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

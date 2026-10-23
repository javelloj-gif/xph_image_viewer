# Resultado: 005 Entrega IPC

Fecha: 2026-10-22. Estado: completado en alcance. [Spec](spec.md), [plan](plan.md),
[revisión](review.md). Base ac4a5da, codex/repository-foundation; cambios previos
R1/R2/003/004 preservados, todo el trabajo posterior sigue sin commit.

Seguimiento posterior: [validación real de 003](../003-diagnostico-rendimiento/real-validation.md)
confirma PNG presentado tras IPC, ventanas completas de los flags y diálogo Win32
231/salida 2 reales. El cierre de implementación de abajo conserva sus límites
históricos. --lite tuvo capturas negras iniciales hasta F11 (AR-008, causa pendiente).
La [campaña aislada posterior](../003-diagnostico-rendimiento/ar008-reproduction.md)
mostró JPEG sin F11/resize en seis procesos y constató discrepancia de captura;
no se reprodujo un fallo persistente de --lite. No cambió el producto ni su hash.

## Contrato e implementación

Se escribió primero el resultado esperado ante fallo: ForwardFailed con error,
sin primario alternativo; App retorna fallo específico y main muestra diálogo de
transporte con código Win32 y retorna 2. Forwarded lleva a AlreadyRunning/salida 0,
solo acredita escritura completa. No hay ACK de recepción/aplicación.

Se mantiene el sondeo. SendMessage reintenta apertura ante busy/missing durante
2000 ms, con esperas cortas (Sleep si falta la pipe para evitar bucle activo).
No reintenta WriteFile, ni siquiera tras escritura parcial/fallida. Busy inicial
ya indica receptor; errores distintos de ausencia inicial no crean servidor.
Los contadores conservan semántica diagnóstica por campo, sin snapshot transaccional.

El servidor retiene unique_ptr del payload hasta PostMessage correcto; en fallo
registra código/cuenta y lo destruye localmente. Window conserva el consumo y delete
existentes. No se rediseña propiedad de mensajes ya posteados durante cierre.
SendMessage es compartido con el centinela: este también obtiene reintento missing
dentro de su presupuesto existente de conexión; no cambia el algoritmo de Shutdown.

## Evidencia de ejecución

Windows 10.0.26300 x64, SDK 10.0.22621.0, MSVC/VS 2022 y CMake del checkout.
Receptor Window/App/WIC reales, D2D WARP software sin swapchain ni ventana visible;
procesos emisores reales. Copias JPEG/PNG Unicode, user_data y pipes privadas.

| Criterio | Resultado observado |
| --- | --- |
| AC-01 | Cinco Initialize: exit 0, un handler e imagen/bitmap correctos por intento; dos concurrentes: exit 0 ambos, cada ruta aplicada una vez |
| AC-02 | Pipe creada tras 100 ms: envío completo recuperado; pipe ausente con presupuesto 80 ms: error ERROR_FILE_NOT_FOUND. Pipe saturada: ForwardFailed/ERROR_PIPE_BUSY, sin hilo primario nuevo; App::Initialize real retorna InitResult::ForwardFailed con el mismo error |
| AC-03 | UI no aplica antes de bombear; PNG con bitmap después. Ruta inexistente llega al handler pero conserva imagen; vacía no abre. Tras drenaje extra: received=10, posted=10, handlers=10, postFailures=0 |
| AC-04 | HWND destruido: received=1, posted=0, postFailures=1; transporte puede tener éxito sin post. Propiedad local RAII y liberación revisadas en código; no detector dinámico de fugas |
| AC-05 | Configure/build Release correctos; CTest 4/4 y git diff --check correctos; revisión y guías actualizadas |

Comandos ejecutados desde raíz:

```powershell
cmake --preset windows-x64
cmake --build --preset release
ctest --preset release --output-on-failure
git diff --check
```

CTest final: metadata.tagging, common.user_data_paths (19 checks),
ui.async_previews (98 checks) e ipc.delivery_diagnostics (7,35 s); total 11,48 s.
El nombre histórico IPC se conserva, pero ahora exige éxito del producto en esa matriz.
Logs y copias solo bajo build/windows-x64/test-runs; no se versionan.

SHA-256 de binarios de la validación:

- lenteuv.exe: `400045F207A8775598368A5B5430BED2C33C117C27DC3BB97A722934C544A5C4`
- async_preview_tests.exe: `8F976D1AE8FA01B290F61C8136320BFFC0334C97C5A974536C176651AE9F4C8F`

## Límites y continuidad

AR-003 corregido en transporte/error y payload de post fallido. El diagnóstico
histórico de 003 sigue siendo evidencia previa, no describe el nuevo resultado.
App/main se siguieron en código; la rama de fallo de App tiene ejecución real, pero
diálogo modal y salida 2 de wWinMain solo están comprobados por inspección/compilación.
Los exit codes del harness son propios (1000+Win32 en fallo), no los de wWinMain.

No hay ACK: ruta inexistente o post fallido no se notifican al cliente como rechazo
de aplicación. El presupuesto limita conexión; WriteFile/FlushFileBuffers y join siguen
síncronos sin plazo garantizado. No hay elección atómica: ausencia en el primer sondeo
(incluido un intervalo antes de detectar receptor) todavía puede iniciar otro primario.
Dos clientes con receptor existente pasaron esta ejecución; no es garantía general
de concurrencia. Errores no transitorios del sondeo están cubiertos por código, sin
inducción exhaustiva de permisos. No se provocó escritura parcial ni fallo de WriteFile.

Sin validación de presentación, Explorer real, arranque completo --multiple/--lite,
primera ejecución real o reposo. El caso App fallido no alcanza renderer ni muestra
ventana; no sustituye el arranque completo. No se midió ni se declara mejora de
rendimiento. AR-005/006 y cobertura AR-007 quedan pendientes. Library permanece:
valor antiguo 4 y placeholder/tooltip aún requieren decisión. 003 continúa abierto.

# 005: Entrega IPC y fallo explícito (AR-003)

Fecha: 2026-10-22. Alcance autorizado: corrección local de AR-003 después de 003.

## Objetivo y decisiones previas al contrato

El sondeo consume una conexión; el servidor la cierra y recrea su pipe antes del
reenvío. Mantener el sondeo y reintentar apertura ante ERROR_FILE_NOT_FOUND y
ERROR_PIPE_BUSY durante un presupuesto de 2000 ms. Es la opción mínima frente a
reutilizar la conexión, que obligaría a otro tratamiento del camino ocupado.
El presupuesto limita espera de conexión, no IO síncrono ni cierre.
Nunca reintentar después de empezar WriteFile: podría duplicar una petición.

Initialize distingue Primary, Forwarded y ForwardFailed. Un ERROR_PIPE_BUSY del
sondeo ya detecta receptor: agotar la espera no autoriza otro primario.
Solo ERROR_FILE_NOT_FOUND inicial permite iniciar servidor; otro error del sondeo
retorna ForwardFailed con su código, aunque no se haya detectado receptor.
Forwarded significa escritura completa, no aplicación. Con ruta vacía se conserva
el envío vacío sin apertura. Un fallo de envío retorna ForwardFailed con error
Win32 observable; App interrumpe el arranque y main muestra error específico y
retorna 2. No abrir una ventana alternativa ni tratarlo como AlreadyRunning exitoso.
La ruta inexistente sigue rechazándose en App sin ACK al cliente.

PostMessage exitoso transfiere el payload al handler Window; si falla, el servidor
lo destruye, registra error y contador. Sin ACK, ese fallo no puede comunicarse al
cliente como aplicación fallida. La propiedad tras post exitoso/cierre no se rediseña.

## Aceptación

- AC-01: cinco clientes por Initialize y dos concurrentes con receptor existente
  aplican JPEG/PNG con bitmap; una entrega exacta por petición y sin primarios extras.
- AC-02: apertura con pipe temporalmente ausente se recupera; pipe ocupada hasta
  vencer presupuesto produce ForwardFailed y error, sin servidor nuevo ni handler.
  La traducción App/main diferencia fallo y salida 2 de AlreadyRunning/salida 0.
- AC-03: UI diferida no aplica antes de bombear; ruta inexistente/vacía conserva
  imagen. Conteos finales y drenaje adicional detectan duplicados.
- AC-04: PostMessage con HWND destruido falla y conserva propiedad local RAII;
  contador confirma fallo y ausencia de entrega. No afirmar detección dinámica de fugas.
- AC-05: Release y CTest completo correctos; revisión, guías, evidencia y límites.

## Exclusiones

Sin ACK, elección atómica, cancelación/cierre, migraciones, retirada de Library ni
cambios de gestos/formatos. No validar rendimiento, primera ejecución real, reposo
o presentación visual mediante este harness WARP sin ventana visible.

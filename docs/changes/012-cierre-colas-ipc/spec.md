# 012: Primera porción AR-005 — cierre de colas y servidor IPC

Fecha: 2026-10-22. Autorización: actualizar inventario 004–011, comenzar implementación
de AR-005 y entregar ejecutable temporal. Esta petición autoriza cambiar el cierre en
este alcance, anteriormente excluido. No autoriza ACK, elección atómica o retirar Library.

## Contrato

Loader: Initialize/Shutdown/ProcessPendingTextures pertenecen al hilo propietario UI,
sin llamadas concurrentes/reentrantes entre sí. Enqueue acepta trabajo solo entre
Initialize exitoso y Shutdown. Al cerrar, deja de aceptar, descarta tareas aún en cola,
espera únicamente las ya tomadas por workers y libera resultados/callbacks pendientes
sin ejecutarlos ni subir GPU. Repetir cierre es seguro. No interrumpir WIC en curso.
Inicializar de nuevo es posible tras cierre completo; el dueño debe reconstruir modelos
con solicitudes canceladas. App cierra terminalmente, no reutiliza esos flags.

Durante uso normal permanecen callbacks, tokens 004, layouts 007 y predicado 011.
El predicado UI nunca se evalúa en worker. No añadir cancelación por consumidor durante
navegación en esta primera porción; no convertir caducidad en cancelación general.

IPC: servidor usa ConnectNamedPipe/ReadFile overlapped y evento privado de parada;
Shutdown señala, espera finalización de I/O cancelado y une el hilo, sin enviar centinela.
Mantener bytes UTF-8, endpoint, tamaño/read únicos y semántica 005 de envío, sin ACK.
Aceptar el centinela histórico al recibirlo por compatibilidad, pero el cierre local no
depende de que otro cliente deje libre la pipe. No cambiar envío síncrono del secundario.
HWND debe seguir vivo durante Shutdown; serializado en su hilo dueño. Liberar payloads
ya posteados que queden en la cola de ese HWND al cerrar, sin aplicar nuevas rutas.

App debe parar servicios antes de destruir consumidores también si Initialize falla.
Sin TerminateThread ni detach en producto. CancelIoEx requiere esperar la completion
antes de liberar OVERLAPPED/buffer: no basta solicitar cancelación. Fallos al crear
eventos IPC retornan error explícito sin iniciar servidor. Sin plazos universales:
WIC en curso, otros scanners y envío síncrono mantienen sus límites independientes.

## Aceptación

- AC-01: inventario acumulado 004–011 enlaza alcance/evidencia y conserva pendientes.
- AC-02: prueba determinista con worker ocupado, cola grande y resultados pendientes;
  cero tareas en cola ejecutadas tras stop, rechazo de encolado tardío, liberación sin
  callbacks, espera de tarea activa y repetición/reinicio. Contrastar baseline previo.
- AC-03: procesos propios con plazo externo: cierre IPC vacío, inmediato, cliente que
  conecta sin escribir, desconexión, payload pendiente y cierres repetidos. Probar fin
  de proceso y liberación/reutilización del endpoint; reproducir bloqueo previo aislado.
- AC-04: Release/CTest completo, regresiones 004–011 y smoke nativo 010; revisión de
  ownership/locks/comentarios y límites. Sin afirmar mejora de latencia de apertura.
- AC-05: portable nuevo con runtime/defaults/locales, hashes y protocolo funcional;
  no datos personales, asociaciones, commits ni sustitución de candidatos anteriores.

No cerrar AR-005 globalmente: quedan cancelación por consumidor en navegación,
caracterización de WIC/scanners y envío síncrono. Esta porción elimina drenaje de cola
al salir y dependencia del cliente para detener el servidor.

> Estado vigente 2026-10-23: completado en alcance. Aceptación funcional base reportada por el propietario; sin hash o matriz específica por flags. Evidencia automatizada conservada y cierre en [013](../013-cierre-base/result.md).

# 010: Inicialización secundaria mínima (OPT-005)

Fecha: 2026-10-22. Estado: implementado, en revisión de aceptación visual de usuario.
Origen: OPT-005 de la revisión de fuentes de 003. [Evidencia](result.md).

## Objetivo y evidencia

App::Initialize crea WIC/cuatro workers, lee favoritos y caché XMP y enlaza overlays
antes de resolver IPC. Evidencia de código, no medición del coste global.
Un proceso que reenvía o falla al reenviar debe terminar sin inicializar esos servicios.
Conservar el HWND oculto requerido por la API IPC existente y la política de settings.

## Orden previsto y prerrequisitos

| Fase | Contrato |
| --- | --- |
| Entrada | Mantener DPI, COM, parsing y construcción de App en main. |
| Preflight | Settings e i18n en su orden actual; crear HWND oculto con WindowConfig actual. Sin callbacks que consuman servicios aún no inicializados. |
| IPC | Instalar callback temporal de rutas; si single_instance y no multiple, llamar una vez a Initialize con HWND válido y la misma prioridad archivo/carpeta. Mantener endpoint y sondeo. |
| Salida secundaria | Forwarded → AlreadyRunning/salida 0; ForwardFailed → error Win32 y salida 2. Sin servicios pesados, bandeja, renderer, páginas, sesión ni ventana principal visible. |
| Continuación | Primary o bypass: inicializar FolderDataManager/favoritos, loader de cuatro workers, tags con su WIC factory, opciones y modal, en ese orden. |
| UI completa | Bandeja, callbacks restantes, renderer, Show/fullscreen, páginas, SetPage, sesión, argumento explícito y timer conservan su orden relativo actual. Después instalar handler IPC definitivo y aplicar rutas retenidas. |

Mover el bloque FolderDataManager..TaggingModal detrás de IPC.
No dividir descubrimiento y servidor ni introducir infraestructura de inicialización
diferida. i18n se mantiene temprano por alcance conservador; «mínima» describe los
servicios evitados, no ausencia de toda E/S, HWND, COM o construcción de miembros.

El hilo servidor solo postea; App aplica en UI. No añadir bombeo de mensajes durante
la fase incompleta. Auditar también posible despacho reentrante de llamadas Win32/COM:
la mera ausencia de PeekMessage no prueba seguridad. Una petición temprana debe
conservarse hasta que callbacks, renderer, páginas y servicios estén listos. No aceptar
una implementación que la pierda o ejecute LoadImage/LoadFolder prematuramente.

Ajuste de diseño durante implementación: callback temporal instalado antes de IPC,
propietario de un vector compartido local capturado por valor. Copia rutas entregadas
durante reentrada; Window sigue destruyendo el payload original. Después de contenido
inicial y timer, instalar callback definitivo y aplicar copias en orden. Ante fallo,
el callback conserva propiedad hasta destruir App. Sin referencias a locales, API,
miembro de estado, worker ni servicio diferido nuevos. Prueba con despacho inducido
antes de WIC; no se afirma reentrada espontánea reproducida en uso normal.

## Errores y compatibilidad

Conservar íntegro 005: busy detecta receptor, solo ausencia inicial permite Primary;
reintentos missing/busy de conexión 2000 ms, sin reintentar escritura. Forwarded no
es ACK de aplicación. Ruta vacía/inexistente y propiedad del payload no cambian.
No fallback a otro primario tras ForwardFailed. Fallos de HWND o del loader/renderer
del proceso que continúa siguen retornando Failed/salida 1.

Cambio deliberado de precedencia: la decisión IPC sucede antes de favoritos/WIC/XMP;
un secundario ya no falla por inicializar servicios que no utilizará. HWND fallido
ahora precede también al loader fallido. Un primario puede tener servidor activo si
después falla WIC; verificar destrucción parcial con RAII existente, sin rediseñar
Shutdown ni prometer plazo de cierre. No alterar orden de miembros por este cambio.

Sin ACK, elección atómica, cancelación/cierre nuevos, cambios a enums, datos, formatos,
gestos, Library o decoder 009. --multiple/--lite y single_instance=false continúan
como aplicación completa. No trasladar servicios al primer gesto.

## Aceptación

| ID | Resultado requerido y comprobación |
| --- | --- |
| AC-01 | Secundario exitoso y fallido evitan favoritos, WIC/workers, caché XMP, overlays y UI completa. Verificar estado/efectos reales en harness aislado, no solo orden textual; conservar error exacto y traducción main. |
| AC-02 | Matriz 005: cinco envíos y par concurrente a receptor existente aplican bitmap una vez; timeout, recuperación, vacío/inexistente y post fallido mantienen resultados. |
| AC-03 | Primario, bypass por settings y ambos flags inicializan servicios antes de uso. Probar archivo, carpeta, sesión y sin argumentos; petición durante arranque no se pierde ni se aplica prematuramente. Recorrido nativo explícito para presentación/modos. |
| AC-04 | Fallos parciales de HWND/WIC/renderer preservan Failed y liberación; fallos IPC conservan ForwardFailed y código sin servidor alternativo. Inducción de fallos solo en tests; registrar límites de cierre heredados. |
| AC-05 | Base y candidato aislados con mismo receptor/archivo/settings y logging controlado: tiempo del secundario y aplicación separados, rondas alternadas y hashes. Informar sin umbral temporal frágil ni atribuir mejora de primera apertura/reposo/UX global. |
| AC-06 | Release, CTest completo, revisión de contratos/comentarios y documentos. No cerrar con criterios necesarios pendientes. |

No hay decisiones de producto pendientes para esta implementación. Escenarios
automatizados ejecutados; aceptación visual de usuario de AC-03 pendiente.

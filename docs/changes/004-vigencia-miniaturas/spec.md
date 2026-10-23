# 004: Vigencia de resultados asíncronos de miniaturas

Fecha: 2026-10-22. Origen: AR-002 de la auditoría de 001. Implementación autorizada
por el propietario; se preserva el diff local R1/R2 sin commit sobre `ac4a5da`.

## Objetivo y contrato

Los callbacks indexados de ThumbnailStrip, Waterfall y Library no deben escribir
en otro elemento ni acceder fuera de rango tras mutar las listas. Es un riesgo
confirmado por código; no se declara un crash previamente reproducido.

Cada solicitud lleva un token único ligado a su ruta. Solo se aplica si ese token
y ruta siguen presentes en el modelo actual. Ordenar o desplazar elementos conserva
su identidad y acepta el resultado en su nueva posición. Borrar/reemplazar descarta
el resultado; quitar y volver a añadir la misma ruta no revive la solicitud anterior.
El resultado vigente, exitoso o fallido, termina isLoading y consume el token.
La reconstrucción de la tira conserva solicitudes de elementos retenidos una sola
vez por ocurrencia, junto con texturas/fallos ya cacheados.

## Límites

Sin cancelación del trabajo del loader, cambios de colas/cierre/IPC, GPU en workers,
nuevos formatos, rutas, enums o gestos. No retirar Library ni ampliar su arquitectura.
Los callbacks de hover que ya contrastan rutas quedan fuera de esta entrega indexada.
App mantiene vida de consumidores durante ProcessPendingTextures en hilo UI.

## Aceptación

| ID | Evidencia requerida |
| --- | --- |
| AC-01 | Tira: entrega diferida tras lista vacía/menor, reordenación, ruta reemplazada y retorno a la misma ruta; sin escritura errónea ni isLoading huérfano |
| AC-02 | Previews compartidos: ordenar, desplazar/borrar carpeta o archivo, recrear misma ruta y solicitudes desde ambas páginas; éxito/fallo solo en elemento vigente |
| AC-03 | Repetir una entrega no modifica un resultado posterior; checks activos en Release |
| AC-04 | Compilación Release y CTest pertinente; pruebas ejecutan callbacks productivos sin stubs ni datos personales |
| AC-05 | Comentarios de contratos, revisión acotada y evidencia/límites en result/review; roadmap con siguiente paso |

La ejecución determinista sin ventana no certifica UX, GPU ni rendimiento.

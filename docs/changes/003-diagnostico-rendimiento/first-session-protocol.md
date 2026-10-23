# Protocolo para la primera ejecución real de otra sesión

Estado: preparación; sin medición de primera ejecución real. Fecha: 2026-10-22.
Este documento coordina una observación manual; no crea recordatorio, monitor ni
proceso residente y no autoriza reiniciar/cerrar sesión o cambiar asociaciones.

## Caso conocido y datos que faltan

Reporte del propietario: abre mediante asociación de Windows, con doble clic en
una imagen. Reporta formatos/tamaños múltiples y carpetas tanto de menos de 100 como
de más de 1000 elementos. Falta un caso representativo y si la espera es anterior
a ventana o a imagen. La confirmación
previa de uso de LenteUV en esta sesión impide etiquetar las pruebas actuales como
primera real. No deducir esos datos de las fixtures JPEG/PNG de laboratorio.

La lectura actual de UserChoice .jpg/.png apunta a Applications\lenteuv.exe y al
portable habitual fuera del checkout. Hash del EXE asociado:
`15F90CCD654C7B9ECA4F3C121F8AFDD7A1182195DAD3A52536B9454F6FD03097`.
Build 005 validado: `400045F207A8775598368A5B5430BED2C33C117C27DC3BB97A722934C544A5C4`.
Son binarios distintos; no se conoce la revisión del portable solo por su hash.
No atribuir a la instalación asociada los resultados/correcciones del build 005.
No se sustituirá ni asociará automáticamente el EXE nuevo para fabricar comparación.

## Preparar antes de otra sesión, sin lanzar LenteUV

1. Acordar el caso habitual (formato, tamaño, cantidad, almacenamiento y fase de demora).
2. Elegir una sola instalación/hash para toda la serie. Si se usa el doble clic
   habitual, registrar asociación y hash del portable; si se usa copia aislada del
   build nuevo, identificar la diferencia de entrada/configuración. No mezclarlos.
3. Usar una copia controlada de la imagen, registrar su hash y tamaño, sin versionar
   contenido personal. Conservar settings/sesión/cachés: no vaciarlos para simular frío.
4. Acordar instrumento: captura o grabación manual visible del doble clic y ventana/
   primera imagen. Las capturas del helper tienen límites AR-008: no usarlas solas
   para cronometrar el primer frame. No hay todavía instrumento fiable seleccionado
   para latencia de imagen; un timestamp de proceso/HWND no demuestra presentación.

## Ejecutar cuando el propietario disponga de una sesión nueva

- Antes de abrir cualquier LenteUV, registrar si hubo reinicio o inicio de sesión,
  aplicaciones relevantes, ausencia de procesos del visor y revisión/configuración.
- El propietario abre la copia de imagen mediante el flujo acordado; distinguir
  acción del usuario, inicio de proceso, ventana visible e imagen utilizable.
- Etiquetar como primera real solo si no se había ejecutado LenteUV en esa sesión.
  Si ya ocurrió, conservar la observación como posterior; no reiniciar por iniciativa
  del agente. Registrar qué ocurrió, no inferir frío/caliente por el orden del test.
- Tras cierre confirmado del proceso, repetir cinco aperturas posteriores con la
  misma entrada, imagen, EXE y settings. No borrar cachés entre repeticiones.
- Separar el escenario con receptor ya abierto (IPC) de los arranques sin receptor.
  Registrar PID receptor, imagen final y diferencias; salida 0 no basta para aplicación.

## Registro mínimo y límites

Por observación: fecha/hora America/Bogota, tipo primera/posterior/IPC, hash EXE,
entrada, flags, hash/tamaño de imagen, cantidad de carpeta, condiciones de sesión,
intervalos observables e instrumento/resolución. Resultado no observado = pendiente,
no cero. Preservar evidencia local bajo build; versionar solo resumen sin datos privados.
Informar tamaño de muestra y dispersión; sin comparación causal entre instalaciones.

La medida actual de CPU/memoria con logging activo no sustituye latencia de arranque,
GPU/VRAM ni consumo habitual sin diagnóstico. Una [muestra posterior](explorer-validation.md)
sí recoge CPU/memoria/GPU por PID sin logging, con límites instrumentales. AC-02 y
partes de AC-01 siguen pendientes. Este protocolo preparado no constituye evidencia
de ejecución de un arranque medido.

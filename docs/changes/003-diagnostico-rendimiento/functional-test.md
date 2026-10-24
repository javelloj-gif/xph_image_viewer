# 003: aceptación del candidato asíncrono

Estado: pendiente del propietario sobre `dist/lenteuv_qa_003_20261024_async_cache/lenteuv.exe`.
SHA256 y ZIP en result.md; fuentes b0e98f8.
La aceptación de 002 no se traslada a este binario. No cambiar asociaciones para
estas pruebas: abrir el EXE candidato con el archivo como argumento o arrastrarlo.
Conservar sus datos y los de 002; no borrar fotografías para probar cancelación.

## Recorrido visual y de interacción

1. Abrir JPEG del inicio, centro y final de la carpeta habitual. Confirmar original
   completo, calidad/efectos habituales, zoom y paneo al aparecer. La navegación
   puede habilitarse después de la imagen; nunca debe recorrer otra carpeta.
2. Mostrar la tira. Confirmar selección destino, animación conservada y miniaturas
   correctas; navegar rápidamente hacia adelante/atrás y entre extremos. Una carga
   anterior no debe reemplazar la imagen pedida más recientemente.
3. Entrar en Waterfall, desplazar en ambos sentidos y abrir/cerrar su visor modal.
   Volver a Viewer y Waterfall repetidamente; revisar que no se mezclen miniaturas,
   desaparezcan efectos ni cambie el modo por una respuesta tardía.
4. Cerrar completamente y reabrir las mismas zonas. La caché es automática; anotar
   la diferencia percibida respecto de la primera visita. Una carpeta recién
   visitada todavía necesita generar sus miniaturas.
5. Probar PNG/GIF/otros formatos habituales y animaciones, con controles actuales.
   Revisar Board, selección, zoom y retorno al visor. No editar metadatos ni guardar
   tableros sobre datos originales para comprobar esta optimización.
6. Con receptor candidato abierto, abrir otra imagen mediante ese mismo EXE.
   Confirmar que el visor muestra la nueva. Registrar separadamente aperturas
   rápidas/simultáneas; existe una intermitencia de harness documentada en revisión.
7. Con una copia de sesión/datos, cerrar al iniciar restauración de varias carpetas
   y reabrir: comprobar que no se pierdan registros pendientes. Cerrar durante
   carga/scroll no debe dejar una instancia propia trabajando indefinidamente.

Para cada fallo: hash/candidato, paso, resultado esperado/observado, formato,
tamaño aproximado, cantidad de carpeta y si ocurrió al iniciar, navegar o cerrar.
No publicar fotografías, rutas personales ni logs completos en Git.

## Medición pendiente de otra sesión

La matriz automática se hizo en una sesión ya utilizada. La primera apertura real
requiere coordinar una nueva sesión de Windows y el mismo archivo/configuración.
No reiniciar, cerrar sesión ni vaciar cachés del sistema automáticamente. Seguir
el [protocolo histórico](first-session-protocol.md), sustituyendo la instalación
histórica por el hash del candidato 003. El ejecutor actual añade QPC y correlación
DXGI; un tiempo de envío de frame no equivale por sí solo a imagen visible.

La meta de 100 ms solo se certifica en condiciones con p95 observado <=100 ms,
original completo y evidencia de presentación. Registrar un límite medido mayor
es un resultado válido del análisis; no reducir calidad para declarar éxito.

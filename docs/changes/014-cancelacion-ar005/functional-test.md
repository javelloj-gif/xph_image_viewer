# Prueba funcional del portable 014

Usar el EXE/ZIP **final** enlazado en [resultado](result.md). Los paquetes 010–013
y el candidato preliminar 014 se conservaron; no sustituir el portable personal.
Este binario es posterior al reporte funcional de la base.

1. Abrir el EXE en su directorio extraído, con imágenes/carpetas copiadas para la
   prueba. Incluye runtime, locales y user_data limpio; no requiere instalación.
2. Viewer: abrir imagen, navegar rápido, cambiar carpeta, mostrar/desplazar tira,
   ordenar y borrar únicamente una copia. Las miniaturas deben corresponder a su ruta.
3. Library y Waterfall: cambiar carpeta mientras cargan previews; mover el puntero
   rápidamente entre estáticos/animaciones y fuera del preview, regresar a la misma
   imagen, ordenar y cambiar de modo. No deben reaparecer imágenes de solicitudes viejas.
4. Board: añadir dos copias de la misma ruta, borrar una mientras carga y cambiar
   de tablero/modo. La restante debe completar y conservar las interacciones normales.
5. En una carpeta copiada, iniciar catálogo con Ctrl+L y cerrar/salir del modo mientras
   escanea. El proceso debe finalizar tras la operación actual; un catálogo anterior
   debe seguir completo si se cancela. Reabrir y terminar un catálogo sin cancelar.
6. IPC: con otras instancias personales cerradas, abrir este candidato y después
   otro archivo mediante el mismo EXE sin flags. Debe aplicarlo el receptor. Probar
   --multiple/--lite por separado si se desean ventanas independientes.
7. Cerrar mientras hay cargas o un escaneo; verificar fin del proceso, no solo que
   desaparece la ventana. Ocultar a bandeja mantiene servicios, como antes.

Registrar síntoma concreto, pasos, formato, tamaño/cantidad de archivos y hash del
EXE de result.md. La espera de una llamada WIC/E/S ya iniciada no tiene un máximo
universal; no se fuerza su hilo. La matriz automatizada comprueba sus límites y
cancelación posterior, además de un receptor IPC que no lee. No hace falta simular
ese receptor bloqueado manualmente para la prueba funcional ordinaria.

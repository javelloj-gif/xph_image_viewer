# Pruebas funcionales: retirada de Library (002)

Preparado el 2026-10-23 por petición del propietario. Resultado: pruebas funcionales
exitosas confirmadas por el propietario el mismo día; aceptación de 002 registrada.
El reporte es global, sin matriz individual ni hash aportado por el usuario. Además,
confirma que al abrir una nueva imagen desde Windows el visor se actualiza correctamente.
La identidad local del candidato se volvió a comprobar; véase [resultado](result.md).
Fuentes del candidato: `bff3d92`, rama `codex/repository-foundation`.

## Ejecutable temporal

Desde la raíz del repositorio:

- EXE: `dist/lenteuv_qa_002_20261023_sin_library/lenteuv.exe`.
- ZIP: `dist/lenteuv_qa_002_20261023_sin_library.zip`.
- SHA256 EXE: `42432671B4BE7602C4F2B4060586CFE5B6EB36AE0D382B84312038CBE60BF23A`.
- SHA256 ZIP: `4F3387DE83D7331C1D044FF7FA677392B3FF6BCA33DAF3E218012FB42956931A`.

Carpeta lista para ejecutar, con runtime VC++ x64, locales, licencia, defaults
actualizados y sesión vacía. No separar el EXE de su carpeta. No contiene datos
personales ni el selector descartado. El EXE empaquetado coincide con el Release.
No usar el ZIP `OBSOLETO_con_dialogos` ni los candidatos 010–014 para probar 002.

## Preparación y recorrido

Usar copias de imágenes y una carpeta de pruebas. No probar borrado o escritura de
metadatos sobre originales. Para excluir el caso IPC conocido, arrancar la copia
de forma independiente; desde PowerShell en la raíz, por ejemplo:

```powershell
& './dist/lenteuv_qa_002_20261023_sin_library/lenteuv.exe' --multiple 'C:/ruta/a/copias-de-prueba'
```

1. **Arranque:** la ventana y la imagen/carpeta aparecen sin selector de modos ni
   aviso de migración. Cerrar normalmente y volver a abrir; comprobar restauración
   de la carpeta de pruebas.
2. **Viewer:** NUM1 abre el visor. Comprobar navegación entre imágenes, zoom y tira
   de miniaturas. NUM2 y `2` no cambian de modo.
3. **Waterfall:** NUM3/3 abre cascada; comprobar miniaturas, scroll, apertura de la
   imagen en el modal y regreso a la cuadrícula. NUM2/2 no cambian el modo ni cierran
   el modal por sí solos.
4. **Board:** NUM4/4 abre tablero; comprobar imagen activa, paneo/zoom y entrada de
   imágenes por drop. NUM2/2 no seleccionan otro modo.
5. **Texto:** escribir `2` en una nota del tablero y en búsqueda/entrada de tags.
   El carácter debe aparecer. No es necesario guardar metadatos para comprobarlo.
6. **Radial en fullscreen:** la antigua posición muestra `—` gris y tooltip
   `Unavailable`. Clic/doble clic no activan modo ni acción de fondo. Viewer,
   Waterfall y Board siguen accesibles por sus posiciones.
7. **Opciones y drop:** las preferencias de modo ya no ofrecen Library. Probar
   arrastre de archivo/carpeta fuera de Board con un modo soportado elegido en las
   opciones existentes. Board conserva su propio drop. No deben aparecer diálogos
   añadidos por 002.
8. **Cierre:** cerrar sin bloqueo; repetir una apertura independiente con `--lite`
   si se usa ese flag habitualmente. No usar una apertura por instancia única como
   prueba de aceptación de 002.

Registrar aprobado/fallido/no probado por punto e identificar el EXE/hash. Un fallo
debe incluir gesto y comportamiento observado, sin atribuir causa sin evidencia.
No inferir aprobación de puntos omitidos. El antiguo valor persistido 4 no tiene
tratamiento añadido ni compatibilidad certificada; no introducir uno como arreglo
incidental. IPC-014-01 permanece diferido y excluido del recorrido.

## Después del reporte

La siguiente sesión debe registrar la evidencia en `result.md`, cerrar únicamente
los criterios realmente aceptados y seguir [resume.md](resume.md).
El propietario autoriza eliminar los ejecutables previos **solo si las pruebas
son exitosas**. Antes de ese reporte, conservarlos todos. La limpieza no puede
eliminar datos locales ni el candidato aceptado.

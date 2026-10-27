# Resultado 1.0

Entrega: comportamiento funcional aprobado de 026–030, corrección del orden del
menú y metadatos de versión 1.0.0. CMake 1.0.0; FileVersion/ProductVersion Windows
1.0.0.0. Se mantienen AppId, rutas, formatos, licencia y atribuciones.

## Validación

Release y CTest 10/10 (18,45 s), antes de retirar el build regenerable. Empaquetador
productivo con -SkipBuild -Format All -Version 1.0.0; Inno Setup 6.7.3 compiló el
instalador y el control dumpbin validó imports del EXE y runtime app-local.
Portable e instalador proceden del mismo staging limpio. Se verificaron las versiones
de ambos PE y los hashes del EXE compilado/empaquetado. No se ejecutó una instalación
real ni se repitieron benchmarks por cambiar únicamente VERSIONINFO.

| Artefacto | SHA256 |
| --- | --- |
| image_viewer.exe | 0555E3366485857DABD1AF6D092C83FDBC3E4EB54A60EE63B360B71B02AC57AE |
| image_viewer_portable_1.0.0.zip | CBE29CF879E5410C4BDAAF769C0DCDF5B7A11D76E81CB918537BFCBFEF6E97D7 |
| image_viewer_setup_1.0.0.exe | 7A740CA4D48F56CD8D68EFA4393D9F438E17CFCFD6B6664BEA91579390C56C26 |

## Limpieza y continuación

Retirados aproximadamente 9,5 GiB: build completo (objetos, cachés de tests, scripts
temporales, benchmarks, ETW y checkouts auxiliares), .kilo y todas las distribuciones
anteriores. El worktree auxiliar estaba limpio y se retiró mediante Git; su rama y
los commits originales siguen en el repositorio. Se verificó que los HEAD de los
checkouts y el estado previo e2abe3d existen en el historial preservado.

Antes del borrado se comprobaron rutas absolutas bajo este workspace y ausencia de
reparse points. No se borraron fuentes, fixtures, docs, assets, .agents ni .git.
user_data original se conservó; la copia antigua del respaldo coincidía con sus tres
archivos. Planes locales únicos y configuraciones/sesiones anteriores fuera de cachés
se conservaron en user_data/preserved-release-1.0.0, ignorado por Git.

dist conserva solo el portable 1.0.0 extraído, su ZIP, el instalador, SHA256SUMS.txt
y release-1.0.0.json. Los paquetes no incluyen los datos preservados. Los resúmenes
versionados de pruebas anteriores siguen siendo evidencia histórica; sus rutas
build/dist antiguas se retiraron por autorización expresa del propietario.

Para continuar: regenerar con cmake --preset windows-x64, compilar y ejecutar CTest.
No restaurar una versión de pruebas antigua como build actual.

## Publicación

Referencia de entrega: v1.0.0. Destinos: origin/codex/015-roadmap-design y la rama
predeterminada origin/codex/repository-foundation. Publicar por avance directo,
sin reescribir historia. Verificar referencias con git ls-remote tras el envío;
la etiqueta proporciona la identidad del commit sin insertar un hash autorreferencial.
Los binarios, user_data, logs y estado de herramientas permanecen fuera de Git.

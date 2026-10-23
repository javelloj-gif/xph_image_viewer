# Resultado: 002 Retirada de Library

Fecha: 2026-10-23. Base: `b1681c0`, rama codex/repository-foundation.
Especificación: [spec.md](spec.md). Plan: [plan.md](plan.md).
Estado: completado; aceptación funcional confirmada por el propietario el 2026-10-23.
La evidencia automatizada y sus límites se conservan abajo como registro previo al cierre.

## Cambios

Eliminados LibraryPage (.cpp/.h), su enumerador sin alias, miembro/friend/include,
fuentes CMake e inicialización/update/draw. Retiradas ramas exclusivas de eventos,
sincronización/hover, borrado/tags, routing y opciones/defaults.
NUM2/2 quedan reservados; OnChar y editores permanecen sin cambios.
Posición radial — / Unavailable gris e inactiva, sin propagación de clic/doble clic.
Waterfall mantiene NUM3/3 y su posición. Viewer=2, Board=3 y Waterfall=5 se conservan.
FolderDataManager/modelos continúan en core con los mismos efectos y datos;
solo se ajustaron nombres/logs/comentarios heredados. Sesión/favoritos/KTAG/KBRD y
contratos de hilo/cancelación se conservan. Regresiones existentes adaptadas a los
consumidores restantes, sin reinterpretar evidencia histórica de las dos páginas.

La instrucción final del propietario sustituyó el selector acordado anteriormente:
no quedan diálogos nuevos, opciones nuevas, migración, fallback ni tratamiento
específico del antiguo valor 4. El selector provisional, dependencias y harness
auxiliar fueron retirados. Sus 124 checks nativos no validan el diff final y no se
utilizan como aceptación de esta entrega. El aviso de guardado mostrado durante
esa prueba procedía de una copia marcada solo lectura, no de datos personales.

## Evidencia final

- `cmake --preset windows-x64`: correcto.
- `cmake --build --preset release`: correcto, MSVC/Windows x64.
- `ctest --preset release --output-on-failure`: 7/7, 14.97 s, tras retirar el selector.
- `git diff --check`: sin errores.
- `rg` en src/tests/CMake/defaults: sin referencias al modo Library ni al selector;
  C++ Standard Library en pch es una coincidencia ajena conservada.
- Revisión estática mediante la habilidad: [review.md](review.md), alcance y límites.

| Criterio | Resultado y límite |
| --- | --- |
| AC-01 | Handler global sin selección por NUM2/2; OnChar sin cambios; aceptación funcional global del propietario |
| AC-02 | Gris/inactivo y Waterfall conservados por código; aceptación funcional global del propietario |
| AC-03 | Ramas Library retiradas de arranque/eventos/opciones/drop/IPC; no alias |
| AC-04 | Fuentes exclusivas retiradas, modelo core preservado, Release/CTest correctos |
| AC-05 | Suites compartidas pasan y el propietario confirma pruebas funcionales exitosas; sin matriz individual aportada |
| AC-06 | Sin diálogos/opciones nuevas/migración/fallback; sin aceptación de preferencia antigua 4 |
| AC-07 | Guías, comentarios, spec/plan/review/result y roadmap actualizados |

## Límites y continuidad al preparar la entrega (histórico)

El propietario detuvo la campaña ampliada y pidió limitarse a retirar Library.
No continuar UI/compatibilidad/empaquetado por iniciativa propia. La inspección
visual previa no obtuvo ventana targetable y no demuestra UX final. Una ejecución
ampliada intermedia agotó el watchdog; quedó corregida en el harness provisional,
posteriormente retirado. No se atribuyen sus resultados al código final.

El portable provisional contiene el selector descartado y se renombró
`dist/lenteuv_qa_002_20261023_OBSOLETO_con_dialogos.zip`; no es una entrega válida.
En ese cierre de implementación no se generó paquete final. El EXE compilado está en
`build/windows-x64/bin/Release/lenteuv.exe`, sin selector. Los artefactos de build,
pruebas y paquetes son locales/ignorados, sin datos personales versionados.
La comprobación adicional del portable fue rechazada por el permiso del entorno;
no se ejecutó ni se eludió esa decisión.

IPC-014-01 permanecía diferido. El único cambio en OnIpcOpenPath es retirar la
selección de Library en la rama de carpetas; no se investigó/reprodujo/corrigió el
reporte. CTest aislado no acredita aceptación funcional completa de IPC.

## Candidato temporal solicitado posteriormente

El propietario solicitó explícitamente un ejecutable temporal y un prompt para
continuar en otra sesión; autorizó eliminar los ejecutables previos solo después
de que las pruebas sean exitosas. Esta petición autoriza la preparación del
candidato, sin reabrir diseño de UI ni compatibilidad.

- Fuentes: `bff3d92`. Release reconstruido correctamente el 2026-10-23.
- Comando: `./scripts/create_portable_zip.ps1 -SkipBuild -OutputName lenteuv_qa_002_20261023_sin_library`.
- Candidato: `dist/lenteuv_qa_002_20261023_sin_library/lenteuv.exe`, carpeta extraída
  nueva con runtime, locales, licencia, defaults sin Library y sesión vacía.
- ZIP: `dist/lenteuv_qa_002_20261023_sin_library.zip`.
- SHA256 EXE: `42432671B4BE7602C4F2B4060586CFE5B6EB36AE0D382B84312038CBE60BF23A`.
- SHA256 ZIP: `4F3387DE83D7331C1D044FF7FA677392B3FF6BCA33DAF3E218012FB42956931A`.
- El hash del EXE extraído coincide con el Release. El script comprobó imports
  del EXE/runtime y preparó staging nuevo, sin copiar datos personales.
- No se ejecutó el candidato ni se afirma aceptación funcional. No se repitió
  CTest: sus 7/7 corresponden al mismo código simplificado; la reconstrucción
  posterior no introduce cambios funcionales.
- [Recorrido funcional](functional-test.md) y [prompt de continuidad](resume.md)
  preparados. El reporte posterior y el cierre se registran a continuación.
- Inventario inicial local bajo `build/qa-continuity-002/`, excluyendo .git/.kilo y
  estado de agentes. No se eliminó ningún ejecutable ni paquete en esta preparación.
  Renovar el inventario y verificar ámbito/candidato aceptado antes de la limpieza
  condicional de versiones anteriores en la siguiente sesión.

## Aceptación y cierre posterior, 2026-10-23

El propietario confirma: «Se confirman pruebas funcionales exitosas» y comunica
que IPC opera correctamente: al abrir una nueva imagen desde Windows el visor
se actualiza correctamente. Se acepta 002 sobre el candidato indicado en esta
continuidad. El reporte es global, sin resultados individuales, hash aportado,
formatos, rutas o logs; no se inventa una matriz por escenario ni se certifica
compatibilidad con el antiguo valor 4 o rendimiento con carpetas grandes.

Los hashes locales del EXE y ZIP coinciden con los registrados antes y después
de la limpieza. No se modificaron fuentes ni se generó otro candidato. Release
y CTest 7/7 siguen siendo evidencia previa del mismo código; no se repitieron
para este cierre documental. [IPC-014-01](../014-cancelacion-ar005/user-validation.md)
queda cerrado por reporte de funcionamiento actual, sin causa ni corrección
atribuida a 002 y sin nueva investigación.

Cumplida la condición de éxito, se ejecutó la limpieza autorizada con PowerShell,
rutas absolutas comprobadas y Remove-Item -LiteralPath sobre archivos concretos.
Se retiraron **157 EXE y 8 ZIP**: 150 archivos bajo build y 15 bajo dist, incluidos
instaladores antiguos, copias de diagnóstico, runs, staging y el paquete obsoleto
con selector. Los ZIP retirados se inspeccionaron: contenían lenteuv.exe.
El inventario y la lista concreta permanecen locales en build/qa-continuity-002.

Excepciones conservadas: candidato aceptado completo y ZIP, Release actual y sus
tres EXE de tests, cinco CompilerIdCXX de CMake (uno dentro de la copia independiente),
la copia Git independiente build/clean-checkout completa y dos ZIP de respaldo
sin ejecutables. No se tocaron .git, .kilo, worktrees, fixtures, fuentes ni datos,
logs o cachés. No se cerró ningún proceso ni se eliminó un EXE detectado en uso.
La consulta CIM no estaba disponible; se usaron Get-Process y apertura exclusiva
de cada EXE antes de eliminarlo.

Verificación final: los 165 archivos seleccionados ya no existen; bajo dist solo
quedan el EXE aceptado y su ZIP como ejecutable/paquete. Los hashes EXE/ZIP permanecen
idénticos. La comprobación genérica de archivos preservados se detuvo al detectar
que el propio inventario local había sido actualizado; se verificaron después
directamente los archivos retirados, las excepciones y los hashes del candidato.
No se borraron carpetas completas ni se amplió la búsqueda a todo el disco.

# Resultado: 002 Retirada de Library

Fecha: 2026-10-23. Base: `b1681c0`, rama codex/repository-foundation.
Especificación: [spec.md](spec.md). Plan: [plan.md](plan.md).
Estado: implementación simplificada y verificación automatizada realizadas;
aceptación visual final no completada. No se declara cierre funcional completo.

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
| AC-01 | Handler global sin selección por NUM2/2; OnChar sin cambios. Recorrido visual final no realizado |
| AC-02 | Gris/inactivo y Waterfall conservados por código; observación visual final pendiente |
| AC-03 | Ramas Library retiradas de arranque/eventos/opciones/drop/IPC; no alias |
| AC-04 | Fuentes exclusivas retiradas, modelo core preservado, Release/CTest correctos |
| AC-05 | Suites compartidas pasan; no se certifica UX nativa completa de los tres modos |
| AC-06 | Sin diálogos/opciones nuevas/migración/fallback; sin aceptación de preferencia antigua 4 |
| AC-07 | Guías, comentarios, spec/plan/review/result y roadmap actualizados |

## Límites y continuidad

El propietario detuvo la campaña ampliada y pidió limitarse a retirar Library.
No continuar UI/compatibilidad/empaquetado por iniciativa propia. La inspección
visual previa no obtuvo ventana targetable y no demuestra UX final. Una ejecución
ampliada intermedia agotó el watchdog; quedó corregida en el harness provisional,
posteriormente retirado. No se atribuyen sus resultados al código final.

El portable provisional contiene el selector descartado y se renombró
`dist/lenteuv_qa_002_20261023_OBSOLETO_con_dialogos.zip`; no es una entrega válida.
No se generó paquete final. El EXE final compilado está en
`build/windows-x64/bin/Release/lenteuv.exe`, sin selector. Los artefactos de build,
pruebas y paquetes son locales/ignorados, sin datos personales versionados.
La comprobación adicional del portable fue rechazada por el permiso del entorno;
no se ejecutó ni se eludió esa decisión.

IPC-014-01 permanece diferido. El único cambio en OnIpcOpenPath es retirar la
selección de Library en la rama de carpetas; no se investigó/reprodujo/corrigió el
reporte. CTest aislado no acredita aceptación funcional completa de IPC.

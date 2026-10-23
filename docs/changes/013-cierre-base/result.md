# Resultado 013: cierre de la base

2026-10-23. **Completado en alcance.** [Spec](spec.md), [plan](plan.md),
[revisión](review.md) y [cierre de 001](../001-refactorizacion-base/closure.md).

El reporte del propietario acepta la conservación de funcionalidades base del
ejecutable temporal. Se cierran los pendientes generales de R2/010/012 con esa
evidencia atribuida; no se dispone del hash ni de un protocolo detallado del usuario.

## Cambios y aceptación

| Criterio | Resultado |
| --- | --- |
| AC-01 | Roadmap y 001/010/012 reconciliados; evidencia histórica preservada |
| AC-02 | AR-006: lectura KTAG v1 transaccional. Comprobaciones antes de asignar; fallo conserva entradas/dirty. Rutas, versión, duplicados, vacíos y tolerancia final conservados |
| AC-03 | Control previo falla; candidato pasa 164 truncados × dos estados dirty, firma/versión/cantidades imposibles y compatibilidad/roundtrip |
| AC-03b | App une BoardTagScanner y búsqueda de tags antes del loader/destrucción de dependencias; prueba de trabajo retenido durante destrucción pasa |
| AC-04 | Todos los hallazgos y áreas inventariadas tienen disposición; AR-005 global y deuda documental no se presentan corregidos |
| AC-05 | Lote guardado en Git; prompt vigente en resume.md; artefactos y datos excluidos |

Comandos: `cmake --preset windows-x64`, `cmake --build --preset release`,
`ctest --preset release --output-on-failure`. **Release y 6/6 suites pasaron**,
15,14 s en la última ejecución. 162025 checks async, 294 WIC y nueve escenarios
de cierre conservados. La suite IPC conserva su matriz; no se repitió la campaña
opcional de diez escenarios gráficos nativos porque el cambio no altera arranque/dibujo.

Control anterior a modificar LoadCache: `metadata.tagging` falló con
`KTAG transactional load regression` (CTest informó salida 0xc0000409 después de
la excepción del harness). Tras el fix la suite completa pasa. Logs y datos solo
en build/windows-x64/test-runs y Testing, ignorados; nunca sobre fixtures originales.

## Paquete posterior a la prueba del usuario

`./scripts/create_portable_zip.ps1 -SkipBuild -OutputName lenteuv_qa_013_20261023`
creó ZIP desde staging nuevo con runtime VC++, locales, LICENSE y defaults limpios.

- EXE build: `build/windows-x64/bin/Release/lenteuv.exe`
- SHA256 EXE: `C48ED2BC44F70E2F1F540A7C311BBC905A399A85F4E337A4C74C286EED0118D0`
- ZIP: `dist/lenteuv_qa_013_20261023.zip`
- SHA256 ZIP: `A4EF16387D0366AF7AEC1FE2F721CDC7E4E27689CB407DB67B1A7FC46F736CDC`
- Staging: `build/staging/30c62ad2fba242f9bd54b30a688e8055`

El script validó imports; no se instala ni sustituye asociación/portable personal.
010/011/012 conservados. Este candidato incluye dos correcciones adicionales de
robustez; **no se le atribuye la prueba manual anterior**. Si se desea probarlo,
extraer el ZIP a un directorio nuevo, usar imágenes copiadas y su user_data limpio;
abrir los cuatro modos y cerrar tras cargas/búsqueda. No es requisito que mantenga
la refactorización abierta indefinidamente; las correcciones tienen regresión propia.

## Límites

No nueva UX observada ni mejora global medida. No formato nuevo ni migración de datos.
KTAG válido grande puede requerir memoria proporcional y bloquear lectores durante
carga; no se induce agotamiento real de memoria. SaveCache conserva comportamiento
previo. El join evita destruir dependencias con productores activos, pero Board
espera su escaneo completo; WIC/E/S activa y envío/flush IPC siguen sin plazo universal.
AR-005 global se difiere expresamente a trabajo futuro, no se declara solucionado.
002, campañas 003, OPT-007 y auditoría exhaustiva no pertenecen al cierre base.

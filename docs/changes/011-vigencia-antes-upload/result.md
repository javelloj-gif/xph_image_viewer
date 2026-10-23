# Resultado 011: Vigencia antes del upload

Fecha: 2026-10-22. Estado: completado en el alcance de [spec](spec.md).
[Plan](plan.md), [revisión](review.md), [prompt de continuidad](resume.md).
Rama codex/repository-foundation, base ac4a5da; todo el trabajo previo sin commit
preservado. No se creó commit ni se cambió de rama.

## Cambio y evidencia

LoadAsync transporta un predicado opcional hasta ProcessPendingTextures. Se evalúa
en UI, fuera del mutex y por resultado; false evita upload pero mantiene callback y
resultado CPU original. Tira y previews Library/Waterfall comparan token/ruta antes
del upload, conservando comprobaciones y efectos finales de 004/007. Otros consumidores
y cargas animadas no optan al guard. No cancela decode ni tareas de cierre.

Windows x64 10.0.26300, VS2022, SDK 10.0.22621.0. Snapshot de fuentes previas en
build/optimization-011/before. Validación final:

```powershell
cmake --preset windows-x64
cmake --build --preset release
ctest --preset release --output-on-failure
git diff --check
```

| Criterio | Evidencia |
| --- | --- |
| AC-01 | Pasó: guard ausente/true/false, CPU válido/null, callback una vez con identidad original y en UI. Decode ejecutado en hilo COM separado no evalúa predicado ni callback. WARP crea bitmap para vigente/sin guard y no para false. Callback encola otra carga; un callback invalida otra entrada del mismo lote antes de su upload. |
| AC-02 | Pasó: siete mutaciones por cada uno de tres consumidores, más recreación con nueva solicitud pendiente y ocurrencias duplicadas. Obsoletos y entregas repetidas sin bitmap; retención/reordenación con bitmap. Fallo vigente termina loading; guard no modifica revisión, completion vigente válida la incrementa una vez. |
| AC-03 | Configure y Release correctos; CTest 5/5 en 11,73 s. 162022 checks async (321 nuevos sobre 161701), 294 decoder y suites de metadatos/rutas/IPC conservadas. Diff específico y contratos revisados; sin errores de whitespace. |

Pruebas: decodificación de ruta inexistente por tarea real y sustitución controlada
por píxeles CPU sintéticos 2x1 antes de entregar por la cola productiva. No se mide
rendimiento del decoder ni códecs nuevos. WARP real con dispositivo sin ventana ni
swapchain; fixtures intactos, pruebas de metadatos en staging del harness.
Los tests 004 anteriores siguen comprobando callbacks; los nuevos conservan también
el predicado al manipular completions. No se afirma interacción nativa certificada.

Coste: std::function opcional y captura adicional del token por solicitud, más una
búsqueda lineal al entregar en consumidores optados. Se conserva la búsqueda final
de 004. No hay benchmark temporal, reducción de memoria global ni mejora de latencia
demostrada. Evita creación de bitmaps caducados, no CPU decode, colas o drenaje al cierre.

## Portable temporal

Generado desde staging nuevo con runtime/locales/licencia/defaults/session vacía;
imports comprobados por el script. No copió datos personales ni modificó asociaciones.

```powershell
./scripts/create_portable_zip.ps1 -SkipBuild -Format Portable -OutputName lenteuv_qa_011_20261022
Expand-Archive -LiteralPath dist/lenteuv_qa_011_20261022.zip -DestinationPath dist/lenteuv_qa_011_20261022
```

- EXE: dist/lenteuv_qa_011_20261022/lenteuv.exe.
  SHA256: 503D3B36F7B702EF50FAF850098046AC91E5F6663E85FC34FB2BE110ADDDBC83.
- ZIP: dist/lenteuv_qa_011_20261022.zip.
  SHA256: BAEB408E88EC470FEE70A415B5164614EC22AAC7AB5798F984C459026913B599.
- Harness: A5EC34D03F0C9282A75900C5218931FEC20AAAC1E8D8F31C22A8504BAF4D4248.
- Staging local: build/staging/955dd5f7ee5c48fb89b51c80d30f825d.

Candidato actualizado, sin ejecución visual nueva. Con copias de imágenes, probar
cambios rápidos de carpeta, ordenar y alternar Viewer/Library/Waterfall durante carga;
comprobar que no quedan miniaturas cargando ni se asignan a otra ruta. Para probar IPC,
cerrar otras instancias por endpoint compartido y seguir también protocolo 010.
Los paquetes y hashes de 010 se conservan; su aceptación visual sigue pendiente.

## Pendientes y siguiente paso

011 no cierra el roadmap. Siguiente propuesta: especificar OPT-007 con mapa completo
de invalidación/temporizadores antes de tocar presentación; sin bajar FPS/calidad/efectos.
Masonry y última medida del overlay siguen separados. Mantener primera real/caso
representativo de 003, AR-005/006/007, límites 008/009 y aceptación visual 010.
Library espera decisión sobre valor antiguo 4 y placeholder/tooltip. Sin ACK, elección
atómica ni rediseño de cierre. Regenerar el portable al terminar próximas porciones.

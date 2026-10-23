> Actualización 2026-10-23: aceptación funcional base reportada por el propietario; no se conoce hash del ejecutable probado. 013 corrige además orden de productores; AR-005 global sigue diferido como robustez futura. Véase [cierre 013](../013-cierre-base/result.md).

# Resultado 012 — colas y cierre IPC

Fecha de cierre de la porción: 2026-10-23; iniciada 2026-10-22.
Estado: completado en alcance; AR-005 global sigue abierto.
[Spec](spec.md), [plan](plan.md), [revisión](review.md), [continuidad](resume.md).
Git: codex/repository-foundation, ac4a5da; todos los cambios anteriores preservados,
sin commits ni cambio de rama.

## Implementación

- Loader deja de admitir trabajo al parar y descarta la cola no iniciada. Espera las
  tareas ya tomadas, libera resultados/capturas pendientes y WIC sin ejecutar callbacks.
  Cierre repetido y reinicio tras cierre completo comprobados; no interrumpe WIC activo.
- Servidor IPC usa eventos y conexión/lectura overlapped cancelables. Cierra sin
  enviar centinela ni esperar escritura del cliente. Espera completion antes de liberar
  almacenamiento. Conserva protocolo 005 y centinela recibido de versiones antiguas.
- App para IPC antes de destruir HWND en cierre terminal y para ambos servicios antes
  de destruir consumidores, incluso en fallo de inicialización. Payloads posteados se
  liberan sin abrir rutas; WM_QUIT se conserva. Ocultar a bandeja conserva servicios.
- [Inventario acumulado 004–011](../001-refactorizacion-base/coverage-004-011.md)
  distingue revisiones por porción de cobertura pendiente; histórico 001 intacto.

## Evidencia

Windows x64 10.0.26300, VS2022, SDK 10.0.22621.0. Snapshot previo de fuentes y binarios
en build/shutdown-012/before. Harness nuevo compilado antes de modificar lógica de
producción; solo friends y registro CMake/dispatch añadidos para reproducir baseline.

| Criterio | Resultado |
| --- | --- |
| AC-01 | Inventario acumulado con enlaces de revisión, alcance, evidencia y áreas no auditadas; AR-001/002/003/004 resueltos en sus alcances, AR-005/006/007 pendientes diferenciados. |
| AC-02 | Baseline ejecuta 1001 tareas encoladas tras stop; candidato 0 y una activa completada. Comprueba admisión antes de Initialize/después de stop, completions estáticas/animadas pendientes y publicadas por activo, captura liberada sin callbacks, cero workers, doble cierre y reinicio. |
| AC-03 | Baseline: cliente silencioso y variante repetida agotaron 5 s (hijos propios terminados por harness); payload posteado falló check. Candidato: nueve escenarios terminan con salida 0; uno ejecuta doce ciclos de Initialize/Shutdown del mismo objeto. Incluye cierre inmediato, idle, desconexión, payload sin dispatch, centinela histórico y conservación de WM_QUIT=42. |
| AC-04 | Release y CTest 6/6 en 13,28 s. 162022 checks async y 294 decoder conservados. Dos fallos CreateEventW inducidos retornan error y liberan setup parcial. Diez escenarios nativos de 010 pasan otra vez, con WM_CLOSE real tras post pendiente y hide-to-tray ampliados. Revisión y diff --check correctos. |
| AC-05 | Portable aislado preparado desde staging nuevo; rutas/hashes y protocolo debajo. Aceptación funcional/visual del usuario por realizar. |

CTest lifecycle.shutdown usa procesos propios con watchdog de 5 s por proceso; mide
su terminación, no solo la desaparición de una ventana. Tiempo observado final por
proceso: 46–407 ms, incluyendo arranque, setup y doce ciclos en el más largo. No es
un benchmark de cierre de carpetas reales ni prueba de latencia garantizada. No
TerminateProcess/TerminateThread ni detach en producto; el watchdog solo mata hijos
propios fallidos del harness. Los tests no contactan el endpoint público.

```powershell
cmake --build --preset release
ctest --preset release --output-on-failure
./scripts/run_ipc_diagnostics.ps1 -Executable build/windows-x64/Release/async_preview_tests.exe -Fixtures tests/fixtures -RunRoot build/shutdown-012 -Startup
git diff --check
```

CMake se reconfiguró automáticamente al añadir el sexto CTest. Logs locales:
build/shutdown-012/before/baseline-test.log y build/shutdown-012/final-ctest.log.
Smoke nativo: build/shutdown-012/ipc-4f0ee45f20994730a4e6b18dee83ed86.
El caso directo loader reportó active=1, queued_executed=0, callbacks=0; baseline
queued_executed=1001. No usar su close_ms redondeado a 0 como medición de rendimiento.

## Portable y protocolo

Generado con create_portable_zip.ps1 -SkipBuild -Format Portable -OutputName
lenteuv_qa_012_20261023. Staging nuevo, runtime/locales/licencia/defaults/session
vacía; imports verificados. Sin datos personales ni asociaciones. Paquetes 010/011
preservados. Staging: build/staging/ff52509a838e483b870df94af0f0759e.

EXE: dist/lenteuv_qa_012_20261023/lenteuv.exe.
ZIP: dist/lenteuv_qa_012_20261023.zip. Hashes registrados después de extraer el ZIP.

- SHA256 EXE (coincide con Release): 3EBEAC6724A8BBE74110B11F1D5560978627F4299E75DA66CF90CB5A0A191419.
- SHA256 ZIP: 8A1905C68A29EADC2166FAFE4A6941845C743423A31301F3C33ACE17EDE734E7.
- SHA256 harness validado: 4853FB6989C26F0BC2ACE826A8A4A6B99A970A434168B56640125E39F445C3B7.

Pruebas del usuario, siempre con copias de imágenes:

1. Cerrar otras instancias para que el endpoint compartido no reenvíe al portable anterior.
2. Abrir JPEG/PNG, navegar entre Viewer/Library/Waterfall/Board y reenviar otra apertura.
3. Abrir una carpeta con muchas miniaturas y salir mientras aún cargan. Comprobar fin
   del proceso, no solo ventana; volver a abrir y comprobar carga normal.
4. Repetir alternando carpetas y cerrando con botón de cierre y salida explícita.
5. Si se activa ocultar a bandeja, comprobar que ocultar conserva el proceso y acepta
   aperturas; después salir explícitamente desde la aplicación/bandeja.

Anotar modo, volumen/tamaño/formato de imágenes, gesto de salida y si el proceso
permanece activo. No se necesita alterar asociaciones ni instalar el paquete.

## Límites y continuidad

AR-005 parcialmente atendido: quedan cancelación por consumidor durante navegación,
WIC activo (puede alargar salida), otros escáneres y escritura/flush síncronos del
cliente IPC. El servidor ya no depende de un cliente silencioso para detenerse.
No plazo universal de cierre ni medición de memoria/fugas. Lifecycle serializado UI;
no se soporta Shutdown concurrente/reentrante o desde un worker. Modelos cancelados
deben reconstruirse si se reutiliza un loader; App no reanuda después del cierre.

Sin cambios de ACK, elección atómica, datos/formatos/enums/gestos ni retirada de Library.
010 mantiene aceptación visual pendiente; el smoke automático no la sustituye.
R2 visual, AR-006, cobertura AR-007, 003 primera real/caso representativo, límites
008/009, masonry/última medida y OPT-007 conservan sus pendientes independientes.

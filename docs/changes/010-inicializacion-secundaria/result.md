> Cierre 2026-10-23: completado en alcance; el propietario reporta conservación de todas las funcionalidades base del ejecutable temporal. Se acepta AC-03 a nivel funcional general, sin atribuir hash ni escenarios específicos no descritos. Véase [013](../013-cierre-base/result.md); el resto conserva evidencia histórica.

# Resultado: 010 Inicialización secundaria mínima

Fecha: 2026-10-22. [Spec](spec.md), [plan](plan.md), [revisión](review.md).
Estado: implementado; en revisión, pendiente aceptación visual de usuario (AC-03).
Base ac4a5da, rama codex/repository-foundation; trabajo previo sin commit preservado.

## Implementación

App resuelve IPC después de settings/i18n/HWND oculto y antes de gestor/favoritos,
cuatro workers/WIC, tags/caché y overlays. Mantiene resultados/errores 005 y orden
interno de servicios. No traslada coste al primer gesto.
Callback temporal anterior al servidor retiene copias de rutas despachadas durante
inicialización. Captura shared_ptr por valor, sin referencias a locales que queden
colgando al fallar. Después de páginas/sesión/argumento/timer se instala el handler
definitivo y se aplican las copias en orden. Window sigue destruyendo su payload.
Sin cambios a IPC/Shutdown/main/enums. La retención añade una pequeña asignación
incluso al secundario; no elimina toda E/S ni inicialización del proceso.

## Evidencia de aceptación

Windows x64 10.0.26300, VS2022 y SDK 10.0.22621.0. Snapshot de fuentes y binarios
bajo build/optimization-010/before. Referencia instrumentada compilada antes de
modificar App; no se usó HEAD como producto previo. Artefactos locales ignorados.

| Criterio | Resultado |
| --- | --- |
| AC-01 | Pasó: seis secundarios App y par concurrente; factories WIC/tags nulas, favorito sintético no cargado, renderer no inicializado y HWND no visible. Fallo busy evita factories/UI. Código confirma workers tras factory y overlays dentro del bloque omitido; sin contador directo de lectura de caché. |
| AC-02 | Pasó: matriz 005 de cinco envíos, par concurrente, diferido, vacío/inexistente, recuperación missing, busy y post fallido conservada. Nuevos secundarios App aplican bitmap; el par entrega ambas rutas una vez. |
| AC-03 | Parcial: diez escenarios nativos automatizados cubren primario, --multiple, --lite, single_instance=false, archivo/carpeta/sesión/directorio actual. Siete arranques completos dibujan los cuatro modos con dispositivo real. Dos rutas JPEG/PNG se postean y despachan antes de WIC: no cargan prematuramente y PNG reemplaza al argumento JPEG después. Pendiente revisión visual/interactiva por usuario. |
| AC-04 | Pasó en casos inducidos: fallos CreateWindowExW, CoCreateInstance WIC y D3D11CreateDevice retornan Failed. WIC/renderer con rutas retenidas no las aplican; App se destruye por RAII y el test termina. Busy conserva ForwardFailed/error 231. Sin inducción exhaustiva ni plazo general de cierre certificado. |
| AC-05 | Medición realizada; sin reducción clara de latencia ni mejora global atribuida. |
| AC-06 | Configure/Release correctos; CTest 5/5 en 11,39 s; revisión y diff --check. Smoke nativo separado pasó después del último ajuste de teardown. Cierre final depende de AC-03. |

Reentrada y fallos inducidos mediante imports temporales del EXE de tests restaurados
por scope: sin hooks productivos ni cambios a DLL/registro. No se afirma reentrada
espontánea reproducida. El smoke nativo es explícito, fuera del CTest basado en WARP.
Exige fixtures y user_data junto al EXE aislado; nunca datos personales.

```powershell
cmake --preset windows-x64
cmake --build --preset release
ctest --preset release --output-on-failure
./scripts/run_ipc_diagnostics.ps1 -Executable build/windows-x64/Release/async_preview_tests.exe -Fixtures tests/fixtures -RunRoot build/optimization-010 -Startup
```

Smoke final: build/optimization-010/ipc-ea07c71df7c14d7aabd317b4a7dff431.
CTest conserva 294 checks decoder 009 y 161701 previos; límites APNG/WebP y visuales
siguen vigentes. Compilar/dibujar sin fallar no certifica calidad visual o gestos.

## Medición local

Mismo receptor WARP, PNG, settings por defecto y favorito sintético; seis rondas
alternadas referencia/candidato, primera descartada. Serie final sin otras suites
concurrentes; exploratoria concurrente excluida. Logging de hijos según defaults
(desactivado), receptor diagnóstico con logging. Sin umbrales temporales en tests.

| Mediana de cinco muestras | Referencia | Candidato |
| --- | ---: | ---: |
| Dentro de App::Initialize | 19,2933 ms | 19,3566 ms |
| Inicio del hijo hasta salida observada | 57,7812 ms | 56,6775 ms |
| Bombeo/aplicación después de esperar salida | 21,0377 ms | 22,0553 ms |

Rango Initialize: 18,2067–31,1821 ms frente a 18,6426–21,2512 ms. Variación y pocas
muestras: no se concluye aceleración. El tercer intervalo incluye decode/bitmap y
bombeo deliberadamente diferido; no es latencia interactiva extremo a extremo.
Reducción de trabajo probada por estado/camino omitido, no por estas medianas.
Sin medición de caché XMP grande, primera real, memoria, GPU o reposo.

Serie: build/optimization-010/ipc-f5c64565ca644a0ba737bd1b703a00d7;
resumen comparison-final.log. Hashes SHA256 del harness medido:
- Referencia: C8FE89DF14D242AA4E7F462931D67D4EB0D8FE3BA89AFBD609A0D52E3DB3EA0B.
- Candidato: 214D1AB0FC520C32A839684472C6A1BD525F4672D26089FB19B9C2B8FCCA8176.

App no cambió después de esa serie. Ajustes posteriores: aislamiento/RAII del harness
nativo y documentación. Harness final: FB67E54417804C2D90F843098467C8B56465605C73CB5EA80016E92903A7BC04.

## Portable temporal y continuidad

EXE: dist/lenteuv_qa_010_20261022/lenteuv.exe; ZIP: dist/lenteuv_qa_010_20261022.zip.
Generados con create_portable_zip.ps1 -SkipBuild -Format Portable -OutputName
lenteuv_qa_010_20261022. Staging nuevo, runtime/locales/licencia/defaults y sesión
vacía; sin datos personales. Imports verificados. EXE extraído con el mismo SHA256:
EF8FB503A8F04121023CFC4E06AB16E430D594C44FFB95F2B79476E50587CEFB.

Candidato de 010, no cierre de todas las correcciones. Probar con copias: abrir
JPEG/PNG, reenviar varias aperturas al receptor, alternar modos, flags y restaurar
sesión; observar imagen, foco, gestos y cierre. No sustituir asociaciones.
Siguiente porción: especificar OPT-006 (vigencia antes de upload), preservando 004/007.
Masonry, última medida, scheduler OPT-007, 003/AR-005/006/007 y límites 008 siguen
separados. Library espera decisión del valor antiguo 4 y placeholder/tooltip. Sin commits.

# Continuidad 003 — 2026-10-24

Implementación entregada, aceptación funcional pendiente. Leer Git, AGENTS,
workflow, roadmap y spec/plan/implementation-plan/result/review/performance-result
antes de actuar. No iniciar otro cambio funcional ni modificar workflow por inercia.

## Identidad y estado

- Rama de trabajo: `codex/003-arranque-cache`. Fuentes del candidato: `b0e98f8`.
  Los commits posteriores de documentación/ejecutores no cambian el EXE medido.
- Base instrumentada: `5d1f67f`, referencia `codex/003-instrumented-baseline`.
  Conserva comportamiento base; no usar las series exploratorias de otros hashes
  para atribuir cifras al candidato final.
- EXE: `dist/lenteuv_qa_003_20261024_async_cache/lenteuv.exe`.
  SHA256 `A8C54E3B42B5811EA17CCF347AA91A7FFF4C2808BF19EE45BD4E6C7F0AF04213`.
- ZIP homónimo: SHA256
  `47734AF95ED795C7776A9A39C8407CDF0AC36F77CD182E24A6DA1ECDE9188DB5`.
- Runtime/locales/defaults/sesión vacía desde staging nuevo. La entrega no se
  ejecutó en su carpeta; el smoke normal usó otra copia y datos propios en build.

P1..P7 automatizados completos: Release/CTest 8/8, 94 checks startup/cache, diez
escenarios nativos, empaquetado/imports y normal-run smoke con salida 0. Las series
principales tienen 30 aperturas por posición/escenario/condición, más 120 ciclos
comparables dentro del mismo proceso. Los artefactos y detalles privados son locales.

## Resultado medido y límites

Original completo enviado: medianas 297–300 ms frente a 369–386 ms de base;
100 ms no acreditados. Renderer 218–221 ms de mediana, decodificación solapada.
Waterfall preparado: p95 67–72 ms; vacío **más lento** que la base, p95 1,22–1,95 s.
La tira conserva su animación; destino preparado no equivale a desplazamiento
terminado. Revisar tablas, censuras, CPU/E/S/RAM/GPU y método en performance-result.md.
No se redujo calidad ni se agregó un proceso residente. No presentar CTest, HWND,
WaitForInputIdle o Present1 como certificación visual.

## Siguiente trabajo autorizado

1. Recoger el reporte del propietario sobre este EXE según functional-test.md;
   la aceptación de 002 no se traslada a 003. Documentar fallos comprobados, conservar
   datos y corregir solo los correspondientes al alcance autorizado. Usar la habilidad
   de arquitectura para cambios relevantes y revalidar lo afectado.
2. Coordinar una primera apertura real de otra sesión de Windows con el mismo
   caso/EXE/configuración. No reiniciar automáticamente ni simular disco frío
   vaciando solo la caché de aplicación. Hay protocolo y ejecutores preparados.
3. Registrar aceptación y disposición de límites, incluidos el coste de Waterfall
   vacío y la intermitencia IPC-003-CONCURRENT-01 del harness, sin causa confirmada.
   El reporte manual antiguo IPC-014-01 permanece cerrado; no reabrir su diagnóstico.
4. Actualizar resultado/plan/revisión/roadmap con evidencia real antes del cierre
   global. La meta de 100 ms era a evaluar, sin garantía: conservar el límite medido.

002 y su candidato aceptado siguen intactos, con hashes comprobados al empaquetar.
La base/Library/AR-005 permanecen cerrados. No repetir campañas anteriores, añadir
selector/migración del antiguo modo 4 ni renumerar modos. No borrar artefactos/datos
de pruebas actuales ni el candidato 002 por iniciativa de esta entrega.
No versionar build/dist, cachés, logs, fotografías ni inventarios locales completos.

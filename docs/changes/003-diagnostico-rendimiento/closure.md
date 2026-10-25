# Cierre de 003 — 2026-10-24

**003 completada y aceptada por el propietario en su alcance implementado.**
Tras conocer los tiempos, los límites y la propuesta de cierre, autoriza:
«Basado en los criterios presentados, procede con el cierre de 003» y confirma que,
junto con las tareas anteriores, existe una base sólida para nuevas modificaciones
funcionales. Esta decisión sustituye el bloqueo de cierre por rendimiento inicial;
no convierte en alcanzada la meta de 100 ms ni declara corregidos los seguimientos.

## Entrega final

- Portable: `dist/lenteuv_003_final_20261024.zip`.
- Extracción limpia: `dist/lenteuv_003_final_20261024/lenteuv.exe`.
- EXE SHA256: `874F5783B219E24FE1AD74B330BBF9A344EF2DAB0E3664738CED579CF5FF50D6`.
- ZIP SHA256: `B73B8240C7735B0C3AB9AA7BE0EDEA0452AF5014B0E6695B0D613E76FFFB341A`.
- Mismo EXE comprobado en frío y en regresiones. Código del último lote: diff sobre
  `76dc06c`, rama `codex/003-arranque-cache`; el historial Git identifica su consolidación.
- Staging nuevo, 21 archivos: runtime app-local, locales, ICO, licencia y atribuciones.
  Settings idénticos a packaging/default-settings.json, sesión vacía y directorios
  boards/cache/tags vacíos. Sin logs, cachés, fotografías, fixtures ni trazas.
  Candidatos anteriores de 002 y 003 conservados; no se cambian asociaciones.

Empaquetado con create_portable_zip.ps1 -SkipBuild -Format Portable, sin reconstruir
el EXE medido. Imports estáticos comprobados. En otra extracción bajo build, smoke
del producto sin diagnóstico: --multiple y --lite, tres modos, NUM2 reservado y
cierre normal, salida 0. No se ejecutó el paquete de entrega con datos personales.
Logs locales: build/003-final-package.log y build/003-final-package-smoke.log.

## Criterios y disposición

| Criterio | Evidencia y disposición de cierre |
| --- | --- |
| AC-01, protocolo e identidad | Protocolos QPC/ETW, hashes y condiciones en los informes; corrección explícita del boot no comprobado en la serie comparada |
| AC-02, arranque real/posterior | Series posteriores comparables y aperturas únicas tras boots distintos; no hay distribución fría ni mejora causal exacta certificada. Límite aceptado para este cierre |
| AC-03, IPC/instancias | Regresiones, diez escenarios nativos y reporte funcional OK. Intermitencia concurrente transferida expresamente a seguimiento; no declarada resuelta |
| AC-04, reposo/recursos | Evidencia acotada en explorer-validation.md y performance-result.md; no se certifican presupuestos universales de CPU/GPU/RAM |
| AC-05, hallazgos y propuestas | Informes de causas/opciones y seguimientos delimitados abajo |
| AC-06, hilos y vigencia | CPU asíncrona, UI/GPU principal, latest-wins/cancelación y pruebas; último lote solapa enumeración con renderer sin entregar callbacks antes de estar listo |
| AC-07, caché | Variantes exactas 100/200, invalidación, corrupción, cuota/LRU, concurrencia y fallos prescindibles probados |
| AC-08, datos y controles | Seis áreas funcionales confirmadas por propietario; tests aislados de sesión/cierre. Sin cambios de calidad, efectos, IDs, KTAG/KBRD ni gestos |
| AC-09, candidato/comparación | Portable final limpio con mismo EXE, hashes y smoke independiente; Present1 no se presenta como confirmación física universal |

Release y CTest: 8/8 en 17,34 s. Arranque nativo: diez escenarios correctos.
El último diff conserva los comentarios/contratos de los métodos modificados y fue
revisado localmente con lenteuv-architecture-review; no es revisión independiente.
El smoke final añade validación de distribución, no otra campaña de rendimiento.

## Límites aceptados y seguimiento separado

1. **PERF-003-FOLLOWUP, arranque y generación inicial.** Último frío n=1:
   1995,63 ms invocación→original enviado, 1522,44 ms antes de main.entry y
   473,18 ms internos. Enumeración 277,89 ms solapada ~221 ms con renderer.
   Defender registra FileScan de 1385,61 ms; la atribución causal completa del
   bloqueo corresponde a la captura histórica, no se extrapola a cada milisegundo.
   La meta de 100 ms no se cumple. Waterfall con caché vacía conserva el coste
   documentado frente a la base. Se acepta cerrar 003 con estos límites y continuar
   después mediante un alcance específico: distribución/confianza, creación D3D,
   solapamiento del decode CPU y/o primera generación según prioridad del propietario.
   No se cambia seguridad ni se promete que una firma elimine una demora medida.
2. **IPC-003-CONCURRENT-01.** Intermitencia del harness con dos clientes; una corrida
   falló y la repetición pasó. Causa desconocida, sin atribución al último diff.
   El propietario acepta su seguimiento independiente. Conservar evidencia y test;
   una futura corrección requiere reproducción/diseño propios. No reabrir IPC-014-01.
3. **Cobertura acotada.** Los seis OK no equivalen a matriz universal de códecs,
   animaciones, equipos, agotamiento de memoria o fallos gráficos. Son límites de
   evidencia aceptados, sin tareas de implementación ocultas para cerrar esta base.

Los seguimientos quedan en [problemas conocidos](../../known-issues.md), sin crear
otro chat, automatización o tarea de implementación. No se ejecutan por inercia.

## Estado de la base

003 y los cambios anteriores quedan cerrados en sus alcances aceptados. La base
está disponible para nuevas modificaciones funcionales. El siguiente cambio debe
tener especificación/plan propios; no reiniciar auditorías, campañas de rendimiento
ni limpieza de datos como continuación automática. Se conserva la prohibición de
alterar multimedia de la carpeta de pruebas. Las oportunidades de primera ejecución
ya consumidas mantienen sus guards y no se repiten como arranques fríos.

Informes principales: [frío final](cold-internal-result.md), [serie del último diff](internal-startup-result.md),
[métricas generales](performance-result.md), [ETW causal histórico](etw-result.md),
[aceptación del propietario](user-validation.md) y [continuidad](resume.md).

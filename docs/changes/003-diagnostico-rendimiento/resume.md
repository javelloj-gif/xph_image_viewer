# Continuidad 003 — cerrada 2026-10-24

**003 completada y aceptada por el propietario.** La base y los cambios anteriores
están disponibles para nuevas modificaciones funcionales. No continuar 003 por
inercia. El siguiente cambio requiere su propio alcance y documentación.

## Entrega final

- ZIP: `dist/lenteuv_003_final_20261024.zip`.
- EXE limpio: `dist/lenteuv_003_final_20261024/lenteuv.exe`.
- SHA256 EXE: `874F5783B219E24FE1AD74B330BBF9A344EF2DAB0E3664738CED579CF5FF50D6`.
- SHA256 ZIP: `B73B8240C7735B0C3AB9AA7BE0EDEA0452AF5014B0E6695B0D613E76FFFB341A`.
- Rama `codex/003-arranque-cache`; último lote sobre 76dc06c, consolidación en Git.
  Candidatos previos de 002/003 preservados; no se cambian asociaciones ni datos personales.

## Evidencia aceptada

Release/CTest 8/8 en 17,34 s, diez escenarios nativos y smoke del portable final
--multiple/--lite correctos. Seis áreas funcionales OK por reporte del propietario.
Último frío: boot 17:20:23,500, invocación 17:25:38,403 America/Bogota; original
enviado en 1995,63 ms, 473,18 ms tras main.entry. Enumeración 277,89 ms solapada
~221 ms con renderer. Cero pérdidas ETW/QPC y cero mutaciones del visor observadas
en multimedia. No se certifican pantalla física, p95 frío, mejora causal exacta
entre boots ni 100 ms. Ver [cierre y criterios](closure.md).

## Seguimientos independientes aceptados

- PERF-003-FOLLOWUP: espera previa al inicio, D3D/decode y coste de generación
  inicial/Waterfall vacío. Mantener los tiempos y límites; no declarar resuelto.
- IPC-003-CONCURRENT-01: fallo intermitente de dos clientes del harness, sin causa
  confirmada. Seguimiento aceptado, test conservado. IPC-014-01 permanece cerrado.
- Cobertura no universal de formatos, equipos, animaciones, memoria y fallos GPU.

Quedan en [problemas conocidos](../../known-issues.md), sin tarea/chat/automatización
nuevos ni ejecución automática. No son bloqueos pendientes de esta entrega aceptada.

## Preservación

Nunca alterar multimedia de `F:\Pictures\FotosTEMP`. Medios solo lectura; todas
las pruebas de escritura sobre copias. No publicar build/dist/user_data/logs/trazas.
No borrar los guards ni repetir una apertura consumida como primera real:
último guard `build/startup-etw-003-preparation/attempt-639284772235000000.json`.
Artefactos privados locales descritos en [frío final](cold-internal-result.md),
[serie comparada](internal-startup-result.md) y [ETW histórico](etw-result.md).

# Hoja de ruta

## Entrega 1.0 — 2026-10-26

[031: versión, limpieza y publicación](changes/031-release-1.0/result.md).
Release/CTest 10/10; portable e instalador 1.0.0 desde staging limpio.
Último EXE: Windows 1.0.0.0, con el orden final Settings/Exit del menú.
Se retiraron aproximadamente 9,5 GiB de builds, diagnósticos, copias auxiliares y
distribuciones anteriores. Se conservan fuentes, tests, documentación, historial
Git y datos locales únicos. Las rutas históricas a artefactos anteriores no
representan archivos disponibles tras esta limpieza.

Continuación: usar `v1.0.0` y los presets para regenerar build. Paquetes vigentes en dist.

Validación del propietario, 2026-10-26: pruebas funcionales satisfactorias del lote
026–030, salvo el orden del menú contextual. Ajuste aplicado en 026: Settings
penúltimo y Exit último. La comprobación del ajuste se registra en su resultado.

## Lote 026–030 — 2026-10-26

[Acuerdo, tareas, dependencias y criterios](changes/product-batch-20261026.md).
Implementación completada en el árbol local; sin commit ni publicación remota.

| ID | Resultado / criterios | Evidencia |
| --- | --- | --- |
| 026 | Menú del visor y envío único al tablero sin navegar, AC-01/02 | [Resultado](changes/026-menu-tablero/result.md) |
| 027 | Modal de seis órdenes y filtros combinables, trabajo CPU/cachés/cancelación, AC-03/04 | [Resultado](changes/027-orden-filtro/result.md) |
| 028 | Docks por modo, MSAA y tres propuestas vectoriales, AC-05 | [Resultado](changes/028-docks/result.md) |
| 029 | Movimiento de metadatos con fade posterior, AC-06 | [Resultado](changes/029-metadatos/result.md) |
| 030 | Opciones en ruso/español y selector Español, AC-07 | [Resultado](changes/030-localizacion/result.md) |

Release/CTest final 10/10; 509 comprobaciones nuevas del lote y 144 frames WARP.
Arranque medido después de filtros/docks: 30 pares por escenario, mediana +0,08%
con defaults y -0,82% con selección guardada; p95 dentro de +10%.
Reposo nativo: cero Draw/Present en cinco intervalos estables. [Medidas y límites](changes/027-orden-filtro/performance.md).
Portable limpio: dist/image_viewer_026_030_20261026.zip. Pendiente aceptación manual
del propietario; no se presenta como validación universal de UX, códecs o fotos grandes.


## Sesión 2026-10-26: transparencia y GPU

- [023: transparencia de leyenda del dock](changes/023-transparencia-dock/result.md):
  implementada, Release/CTest 9/9 y snapshot WARP. Pruebas aceptadas por el propietario.
- [024: diagnóstico GPU](changes/024-diagnostico-gpu/review.md): análisis de código
  y log GPU-Z finalizado; máximo 60%, persistencia minimizada reportada. El código
  revisado presentaba en reposo; intervención posterior medida en 025.
  Amplía OPT-007 con escenarios concretos, sin implementar una reducción de FPS/calidad.
- [025: renderizado por cambios](changes/025-renderizado-por-cambios/result.md):
  implementado, Release/CTest 9/9 y smoke nativo 10 escenarios. Referencia ~37
  Present/s en reposo y 144 con dock retenido; final cero frames en tramos estables
  visible/minimizado/oculto. Conserva sondeo CPU. Portable limpio entregado y
  pruebas marcadas satisfactorias por el propietario. [Cierre local 023–025](changes/025-renderizado-por-cambios/closure.md):
  commit/merge autorizados; ajustes finales y publicación remota para próximas sesiones cortas.

## Lote de producto acordado — 2026-10-25

El mapeo 015–022 sustituye la indicación histórica de elegir el próximo cambio.
La base 001–014 sigue cerrada; PERF-003-FOLLOWUP e IPC-003-CONCURRENT-01
conservan seguimiento independiente. No iniciar otra campaña de 003 por inercia.
Acuerdos y validación: [mapa del lote](changes/product-batch-20261025.md).

| ID | Clasificación y tareas originales | Aceptación principal | Dependencias / estado |
| --- | --- | --- | --- |
| 015 | [Preparación: inventario y limpieza](changes/015-preparacion-limpieza/result.md) | Retirar regenerables identificados; preservar datos y dos últimas entregas completas | Completada: 361 archivos / 1,53 GiB; excepciones documentadas |
| 016 | [Diseño: tareas 3 y 6](changes/016-propuestas-visuales/result.md) | Dock compacto, Outline, barra centrada, activa 117 DIP (+30 %), borde cian sin rótulo | Completado y elegido por el propietario el 2026-10-26; sin implementación C++ |
| 017 | [Simplificación funcional: tareas 1 y 2](changes/017-simplificacion-funcional/plan.md) | Zoom animado inicial; retirar opciones/features indicadas; doble clic permanente | Completado en alcance: Release/CTest 9/9 y recorridos afectados. Repetición solicitada con recursos liberados solapa rango histórico, mediana aún mayor; comparabilidad limitada; [evidencia](changes/017-simplificacion-funcional/result.md). Base sin volver a ejecutar/compilar |
| 018 | [Tira y metadatos: tareas 4 y 6](changes/018-tira-metadatos/plan.md) | Base inicial 44/activa 57,2 tras ajuste del propietario; borde cian sin rótulo; barra centrada y multiselección independiente | Implementado sobre 2d19d3d; 9/9 iniciales con 90/117; ajuste 44/57,2: Release y 3/3 suites afectadas, sin nuevas medidas de rendimiento; [evidencia y límites](changes/018-tira-metadatos/result.md). Sin commit; ajuste visual de fuente 13/gap 3 comprobado, empaquetado y aprobado junto con 020 |
| 019 | [Composición compartida: tarea 7](changes/019-composicion-compartida/result.md) | Visor completo en Waterfall, incluida tira; volver al mismo scroll sin duplicar loader ni estado | Implementado local; pruebas y recorridos acotados registrados |
| 020 | [Navegación superior: tarea 3](changes/020-navegacion-superior/result.md) | Dock Outline detallado centrado arriba; retirar radial; Viewer contextual | Corrección visual aprobada expresamente por el propietario el 2026-10-26: Release/CTest 9/9, 12 frames WARP y recorrido nativo; fuente 13 original/gap 3 DIP; ZIP e instalador actualizados; sin commit ni nuevas medidas de rendimiento |
| 021 | [Gestos: tareas 5 y 8](changes/021-gestos/result.md) | Fondo exterior compacta Viewer; hover Waterfall +66,7 | Implementado local; comprobación de franja compacta retomada con la corrección de 020; límites en resultados |
| 022 | [Neutralización de marca](changes/022-neutralizacion-marca/result.md) | Placeholder Image Viewer; código, recursos y empaquetado neutrales con excepciones de compatibilidad | Implementado local, contratos conservados e inventariados; candidato visual vigente y paquetes en 020; no instalación real ni aceptación universal declaradas |

Orden acordado: 015, propuestas y elección en 016, luego planes independientes de
017–022 según dependencias. No implementar una variante por defecto ni presentar
estos criterios como comportamiento actual. Cada cambio funcional tendrá spec,
plan y result propios al desarrollarse.

## Base histórica aceptada

**Estado vigente, 2026-10-24:** 003 cerrada y aceptada por el propietario, junto
con los cambios anteriores. Base lista para nuevas modificaciones funcionales.
[Cierre, portable final y límites aceptados](changes/003-diagnostico-rendimiento/closure.md).
Rendimiento restante e IPC concurrente son seguimientos independientes; no bloquean
esta entrega y no se declaran resueltos. Los relatos de rechazo inferiores son históricos.

Actualización: 2026-10-24. **Refactorización base y AR-005 completados.** El propietario reportó
que probó el ejecutable temporal y que todas las funcionalidades base están intactas.
Se registra como aceptación funcional del usuario; no aportó hash ni una matriz
por escenario. [013](changes/013-cierre-base/result.md) registra la revisión final,
la lectura transaccional KTAG y el orden de cierre de productores.

**Prueba manual histórica del portable 014:** el propietario reportó fallo en el
punto 6 (IPC): al abrir otra imagen mediante el mismo EXE, el visor conserva la
anterior. **Reporte posterior del 2026-10-23:** confirma pruebas funcionales exitosas
de 002 y que el visor se actualiza correctamente al abrir otra imagen desde Windows.
[IPC-014-01](changes/014-cancelacion-ar005/user-validation.md) queda cerrado por
funcionamiento actual reportado, sin diagnóstico ni corrección atribuida a 002.
No se infiere una matriz individual ni cobertura universal de IPC.

014 implementa los cuatro faltantes de AR-005 y entrega un portable comprobado en
sus escenarios automatizados. Se puede continuar con nuevos cambios funcionales
según la decisión del propietario; el fallo IPC histórico conserva su evidencia.
No continuar la refactorización por inercia. Las propuestas opcionales y los límites
de cobertura quedan identificados abajo; no son tareas restantes del lote base ni
defectos declarados corregidos. El próximo cambio de producto se elige por separado.

| ID | Cambio | Estado |
| --- | --- | --- |
| 001 | [Refactorización base y auditoría](changes/001-refactorizacion-base/spec.md) | Completado en alcance: R1/R2, inventario, disposición de AR-001..007 y aceptación funcional reportada |
| 002 | [Retirada de Library](changes/002-retirar-library/spec.md) | Completado y aceptado por el propietario el 2026-10-23; Release/CTest 7/7 previos. Limpieza autorizada realizada: 157 EXE y 8 ZIP; candidato aceptado conservado |
| 003 | [Arranque asíncrono y miniaturas persistentes](changes/003-diagnostico-rendimiento/closure.md) | Completado y aceptado el 2026-10-24. Release/CTest 8/8, diez escenarios nativos y portable final comprobado. Frío n=1: 1996 ms totales / 473 ms internos; 100 ms no alcanzados. Rendimiento e IPC intermitente transferidos a seguimiento aceptado |
| 004 | [Vigencia de miniaturas](changes/004-vigencia-miniaturas/result.md) | Completado: AR-002, regresión previa y 98 checks iniciales |
| 005 | [Entrega IPC y fallo explícito](changes/005-entrega-ipc/result.md) | Completado en alcance: AR-003, transporte/error/propiedad; presentación real posterior en 003 |
| 006 | [Optimizaciones de tira y overlay](changes/006-optimizaciones-locales/result.md) | Completado en alcance: equivalencia y 12 frames WARP; medición local |
| 007 | [Layouts por cambios](changes/007-layouts-por-cambios/result.md) | Completado en alcance: 140 modelos, 24 frames y conservación de 006 |
| 008 | [Formatos DirectWrite](changes/008-formatos-texto/result.md) | Completado en alcance: caché acotada, aislamiento y contraste fresco; variación inicial documentada |
| 009 | [Decoder compartido](changes/009-decoder-compartido/result.md) | Completado en alcance: 294 checks WIC y fallback; límites de códecs explícitos |
| 010 | [Inicialización secundaria mínima](changes/010-inicializacion-secundaria/result.md) | Completado: evidencia automatizada y aceptación funcional base reportada el 2026-10-23 |
| 011 | [Vigencia antes del upload](changes/011-vigencia-antes-upload/result.md) | Completado en alcance: 321 checks nuevos WARP; sin mejora global afirmada |
| 012 | [Cierre de colas y servidor IPC](changes/012-cierre-colas-ipc/result.md) | Completado en alcance y aceptación funcional base reportada; ampliado el orden de productores en 013 |
| 013 | [Cierre de la base](changes/013-cierre-base/spec.md) | Completado: AR-006 y join de productores; Release/CTest 6/6, documentación y continuidad |
| 014 | [Cancelación completa AR-005](changes/014-cancelacion-ar005/result.md) | Implementación completada y pruebas automatizadas conservadas. Fallo IPC histórico cerrado por reporte posterior de funcionamiento actual al aceptar 002; sin diagnóstico causal |

## Disposición final de la base

003: opción 1 (ETW) completada en otra primera apertura, carpeta acordada y multimedia
solo lectura. [Resultado](changes/003-diagnostico-rendimiento/etw-result.md): espera
síncrona del creador de 1426 ms en Defender, FileScan del EXE 1408 ms; traza sin
pérdidas. El último lote solapa enumeración/render y fue medido en frío; 003 está cerrada
por aceptación expresa. Distribución/confianza y trabajo interno restante quedan
en PERF-003-FOLLOWUP. No repetir automáticamente ninguna captura consumida.

| Hallazgo | Resolución o límite explícito |
| --- | --- |
| AR-001 | Resuelto en R2: gestor/modelo compartido en core |
| AR-002 | Resuelto en 004, ampliado en 011: identidad/vigencia; no equivale a cancelar CPU |
| AR-003 | Corrección histórica 005 en transporte/error/propiedad; sin ACK/elección. IPC-014-01 cerrado por reporte posterior: apertura desde Windows actualiza el visor. Sin diagnóstico causal ni certificación universal de IPC |
| AR-004 | Resuelto en R1: resolución común de datos preservando algoritmo |
| AR-005 | Completado en 014 sobre 012/013: cancelación por consumidor, stop cooperativo WIC y caracterización de llamada activa, escáneres y cliente IPC con presupuesto total. Sin faltantes de implementación diferidos; una llamada WIC/E/S activa debe retornar antes del join |
| AR-006 | Resuelto en 013 para lectura KTAG: comprobación de longitudes/lecturas y publicación transaccional |
| AR-007 | Cumplido en porciones modificadas; cobertura documental preexistente restante continúa progresiva al tocar componentes |

[El cierre de 001](changes/001-refactorizacion-base/closure.md) relaciona cada
criterio con evidencia y dispone la cobertura no exhaustiva. AR-005 está completado según el contrato cooperativo y la evidencia de 014; no se certifica todo el código ni un plazo universal de códecs con estas pruebas.

## Trabajo futuro separado, sin ejecución automática

- **Seguimientos tras 003:** PERF-003-FOLLOWUP (arranque y generación inicial) e
  IPC-003-CONCURRENT-01 (harness intermitente) conservados como trabajo independiente
  aceptado al [cerrar 003](changes/003-diagnostico-rendimiento/closure.md), sin declarar
  corregidos sus síntomas. El último lote solapa enumeración/render; frío n=1 de
  1996 ms totales / 473 ms internos. Sin certificación de 100 ms ni mejora causal
  exacta entre boots. La siguiente intervención requiere alcance propio.
- **Optimización:** índice de visibles masonry, última medida del overlay y
  presentación por cambios OPT-007; [revisión de oportunidades](changes/003-diagnostico-rendimiento/source-optimization-review.md).
  No reducir FPS/calidad ni cambiar efectos incidentalmente.
- **Cobertura:** auditoría integral de UI/gestos, dispositivo/pérdida, Board/KBRD,
  escritores de metadatos, Settings/shell/i18n. Inventario y límites en
  [cobertura acumulada](changes/001-refactorizacion-base/coverage-004-011.md).
  No son implementaciones obligatorias para dar por terminada la base.
- **AR-008:** reapertura solo ante síntoma nativo reproducible. Las capturas negras
  no fueron exclusivas de --lite; [contraste](changes/003-diagnostico-rendimiento/ar008-reproduction.md).
  No hay corrección del renderer sustentada por esa evidencia.

## Entrega y continuidad

Entrega final 003: **dist/lenteuv_003_final_20261024/lenteuv.exe** y ZIP
**dist/lenteuv_003_final_20261024.zip**. Mismo EXE del último frío; identidad y
aceptación en [cierre](changes/003-diagnostico-rendimiento/closure.md).

Candidato anterior 003 conservado (histórico): **dist/lenteuv_qa_003_20261024_async_cache/lenteuv.exe** y ZIP
homónimo, fuentes b0e98f8. [Resultado/hash](changes/003-diagnostico-rendimiento/result.md),
[métricas](changes/003-diagnostico-rendimiento/performance-result.md) y
[continuidad](changes/003-diagnostico-rendimiento/resume.md). Código y evidencia
automatizada entregados; seis áreas funcionales confirmadas por el propietario,
pero tiempo inicial rechazado. Captura instrumental inicial única y propuestas
entregadas; falta resolver y aceptar el rendimiento inicial.
La mejora de caché preparada no oculta el coste mayor de generación inicial.

Candidato aceptado de 002:
**dist/lenteuv_qa_002_20261023_sin_library/lenteuv.exe** y ZIP homónimo.
Fuentes `bff3d92`; identidad/evidencia en [resultado 002](changes/002-retirar-library/result.md).
[Pruebas funcionales](changes/002-retirar-library/functional-test.md) exitosas según
confirmación global del propietario. Limpieza posterior realizada: 157 EXE y 8 ZIP
anteriores retirados; candidato, runtime, datos y ZIP conservados. Excepciones y
verificación en resultado 002. [Continuidad](changes/002-retirar-library/resume.md)
actualizada al cierre; no queda trabajo funcional autorizado por esta entrega.

Candidato histórico de 014, retirado en la limpieza: **dist/lenteuv_qa_014_20261023_final/lenteuv.exe** y ZIP homónimo.
Runtime, locales y defaults limpios; SHA256/evidencia en
[resultado 014](changes/014-cancelacion-ar005/result.md). Release y CTest 7/7, diez
escenarios nativos y EXE empaquetado con --multiple/--lite, cuatro modos y cierre 0.
No quedan pendientes de implementación de AR-005; su límite de WIC/E/S activa está
caracterizado y documentado. La [prueba funcional del usuario](changes/014-cancelacion-ar005/functional-test.md)
ya se realizó, con fallo IPC registrado en [user-validation](changes/014-cancelacion-ar005/user-validation.md).
La implementación de cancelación/cierre queda cerrada; el escenario de apertura
IPC visible fue aceptado por reporte posterior de funcionamiento actual en 002,
sin atribuir esa aceptación al binario histórico de 014.

Los candidatos 010–014 se retiraron al cumplirse la condición de limpieza de 002.
No se atribuye al candidato nuevo la aceptación funcional histórica de la base.
[Continuidad histórica 014](changes/014-cancelacion-ar005/resume.md): la base y AR-005
permanecen cerrados; no iniciar otra campaña general.
No publicar datos, build, dist ni estado de herramientas.

Actualización de cierre de sesión, 2026-10-26: el propietario autoriza el commit
del trabajo aprobado y su documentación, incluidos los archivos preservados de
015/016. Este registro sustituye las menciones previas a cambios locales pendientes
de commit. Se conserva la evidencia acotada de 018–022; no se realizan nuevas
mediciones durante el cierre. Próxima etapa acordada: revisar picos de uso de GPU,
todavía sin reproducción, medición ni diagnóstico en esta sesión.

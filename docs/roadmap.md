# Hoja de ruta

Actualización: 2026-10-23. **Refactorización base y AR-005 completados.** El propietario reportó
que probó el ejecutable temporal y que todas las funcionalidades base están intactas.
Se registra como aceptación funcional del usuario; no aportó hash ni una matriz
por escenario. [013](changes/013-cierre-base/result.md) registra la revisión final,
la lectura transaccional KTAG y el orden de cierre de productores.

**Prueba manual posterior del portable 014:** el propietario reporta fallo en el
punto 6 (IPC): al abrir otra imagen mediante el mismo EXE, el visor conserva la
anterior. [IPC-014-01](changes/014-cancelacion-ar005/user-validation.md) queda
**diferido por decisión expresa del propietario**, que lo identifica como error
legado. No investigar ni corregir ahora. No hay aceptación funcional completa de IPC;
no se infiere aprobación individual de los demás puntos del protocolo.

014 implementa los cuatro faltantes de AR-005 y entrega un portable comprobado en
sus escenarios automatizados. Se puede continuar con nuevos cambios funcionales
según la decisión del propietario, conservando la limitación IPC conocida.
No continuar la refactorización por inercia. Las propuestas opcionales y los límites
de cobertura quedan identificados abajo; no son tareas restantes del lote base ni
defectos declarados corregidos. El próximo cambio de producto se elige por separado.

| ID | Cambio | Estado |
| --- | --- | --- |
| 001 | [Refactorización base y auditoría](changes/001-refactorizacion-base/spec.md) | Completado en alcance: R1/R2, inventario, disposición de AR-001..007 y aceptación funcional reportada |
| 002 | [Retirada de Library](changes/002-retirar-library/spec.md) | Inventario/diseño actualizados contra 8625d95; consulta conjunta de valor 4 y placeholder/tooltip pendiente. Sin implementación |
| 003 | [Diagnóstico de arranque, IPC y reposo](changes/003-diagnostico-rendimiento/spec.md) | Investigación independiente en curso; campañas restantes sin prioridad de ejecución asignada |
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
| 014 | [Cancelación completa AR-005](changes/014-cancelacion-ar005/result.md) | Implementación completada y pruebas automatizadas conservadas. Prueba manual realizada: punto 6 IPC fallido (IPC-014-01), diferido por el propietario como legado reportado |

## Disposición final de la base

| Hallazgo | Resolución o límite explícito |
| --- | --- |
| AR-001 | Resuelto en R2: gestor/modelo compartido en core |
| AR-002 | Resuelto en 004, ampliado en 011: identidad/vigencia; no equivale a cancelar CPU |
| AR-003 | Corrección histórica 005 en transporte/error/propiedad; sin ACK/elección. IPC-014-01 reporta apertura visible fallida, diferida y sin diagnóstico; no se declara IPC globalmente resuelto |
| AR-004 | Resuelto en R1: resolución común de datos preservando algoritmo |
| AR-005 | Completado en 014 sobre 012/013: cancelación por consumidor, stop cooperativo WIC y caracterización de llamada activa, escáneres y cliente IPC con presupuesto total. Sin faltantes de implementación diferidos; una llamada WIC/E/S activa debe retornar antes del join |
| AR-006 | Resuelto en 013 para lectura KTAG: comprobación de longitudes/lecturas y publicación transaccional |
| AR-007 | Cumplido en porciones modificadas; cobertura documental preexistente restante continúa progresiva al tocar componentes |

[El cierre de 001](changes/001-refactorizacion-base/closure.md) relaciona cada
criterio con evidencia y dispone la cobertura no exhaustiva. AR-005 está completado según el contrato cooperativo y la evidencia de 014; no se certifica todo el código ni un plazo universal de códecs con estas pruebas.

## Trabajo futuro separado, sin ejecución automática

- **IPC-014-01:** nueva imagen no visible mediante el mismo EXE; conserva la
  anterior. Fallo manual del punto 6, identificado como legado por el propietario.
  Diferido expresamente: sin investigación ni corrección hasta nueva instrucción.
- **002:** decidir qué hace una preferencia antigua con valor 4 y fijar texto de
  placeholder/tooltip antes de retirar Library. Conservar enums, KTAG/KBRD y datos.
  [Inventario y diseño](changes/002-retirar-library/review.md) separan página,
  servicios core, tests y referencias históricas. No repetir extracción de
  FolderDataManager ni correcciones AR-002/AR-005; [plan](changes/002-retirar-library/plan.md)
  y matriz actualizados para continuar tras las dos decisiones.
- **003:** primera apertura real tras iniciar sesión y caso representativo de demora.
  [Explorer/reposo](changes/003-diagnostico-rendimiento/explorer-validation.md) y
  [presentación/errores](changes/003-diagnostico-rendimiento/real-validation.md)
  tienen evidencia acotada; no demuestran una mejora causal global.
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

Candidato vigente: **dist/lenteuv_qa_014_20261023_final/lenteuv.exe** y ZIP homónimo.
Runtime, locales y defaults limpios; SHA256/evidencia en
[resultado 014](changes/014-cancelacion-ar005/result.md). Release y CTest 7/7, diez
escenarios nativos y EXE empaquetado con --multiple/--lite, cuatro modos y cierre 0.
No quedan pendientes de implementación de AR-005; su límite de WIC/E/S activa está
caracterizado y documentado. La [prueba funcional del usuario](changes/014-cancelacion-ar005/functional-test.md)
ya se realizó, con fallo IPC registrado en [user-validation](changes/014-cancelacion-ar005/user-validation.md).
La implementación de cancelación/cierre queda cerrada; la apertura IPC visible no
se da por aceptada y permanece diferida por el propietario.

Los candidatos 010–013 y el preliminar 014 se conservaron. Usar el sufijo **final**.
No se atribuye al candidato nuevo la aceptación funcional histórica de la base.
[Continuidad vigente](changes/014-cancelacion-ar005/resume.md): atender el siguiente
objetivo funcional que indique el propietario, sin iniciar otra campaña general.
No publicar datos, build, dist ni estado de herramientas.

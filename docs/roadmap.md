# Hoja de ruta

Actualización: 2026-10-23. **Refactorización base cerrada.** El propietario reportó
que probó el ejecutable temporal y que todas las funcionalidades base están intactas.
Se registra como aceptación funcional del usuario; no aportó hash ni una matriz
por escenario. [013](changes/013-cierre-base/result.md) registra la revisión final,
la lectura transaccional KTAG y el orden de cierre de productores.

No continuar la refactorización por inercia. Las propuestas opcionales y los límites
de cobertura quedan identificados abajo; no son tareas restantes del lote base ni
defectos declarados corregidos. El próximo cambio de producto se elige por separado.

| ID | Cambio | Estado |
| --- | --- | --- |
| 001 | [Refactorización base y auditoría](changes/001-refactorizacion-base/spec.md) | Completado en alcance: R1/R2, inventario, disposición de AR-001..007 y aceptación funcional reportada |
| 002 | [Retirada de Library](changes/002-retirar-library/spec.md) | En definición; requiere decidir valor persistido 4 y placeholder/tooltip |
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

## Disposición final de la base

| Hallazgo | Resolución o límite explícito |
| --- | --- |
| AR-001 | Resuelto en R2: gestor/modelo compartido en core |
| AR-002 | Resuelto en 004, ampliado en 011: identidad/vigencia; no equivale a cancelar CPU |
| AR-003 | Resuelto en 005 en transporte/error/propiedad; no añade ACK ni elección atómica |
| AR-004 | Resuelto en R1: resolución común de datos preservando algoritmo |
| AR-005 | Cierre de loader/servidor implementado en 012; 013 une Board/búsqueda antes de sus dependencias. Cancelación por consumidor, WIC activo y límite total del cliente síncrono quedan diferidos explícitamente como robustez futura |
| AR-006 | Resuelto en 013 para lectura KTAG: comprobación de longitudes/lecturas y publicación transaccional |
| AR-007 | Cumplido en porciones modificadas; cobertura documental preexistente restante continúa progresiva al tocar componentes |

[El cierre de 001](changes/001-refactorizacion-base/closure.md) relaciona cada
criterio con evidencia y dispone la cobertura no exhaustiva. No se declara AR-005
resuelto globalmente ni se certifica todo el código por la prueba funcional.

## Trabajo futuro separado, sin ejecución automática

- **002:** decidir qué hace una preferencia antigua con valor 4 y fijar texto de
  placeholder/tooltip antes de retirar Library. Conservar enums, KTAG/KBRD y datos.
- **003:** primera apertura real tras iniciar sesión y caso representativo de demora.
  [Explorer/reposo](changes/003-diagnostico-rendimiento/explorer-validation.md) y
  [presentación/errores](changes/003-diagnostico-rendimiento/real-validation.md)
  tienen evidencia acotada; no demuestran una mejora causal global.
- **AR-005:** cancelación cooperativa por consumidor, latencia de códecs/E/S,
  escáneres y escritura/flush del cliente. Board espera su escaneo completo.
  Requiere especificación y regresión propias; sin terminar hilos a la fuerza.
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

El portable probado por el propietario no se identifica por hash en su reporte.
Se conservan los candidatos 010/011/012. El candidato posterior de esta sesión es
**dist/lenteuv_qa_013_20261023.zip**, creado desde staging limpio, con runtime,
locales y defaults propios. Incluye KTAG y orden de cierre adicionales; no se atribuye
a ese binario la prueba manual anterior. Hashes/evidencia en
[resultado 013](changes/013-cierre-base/result.md).

[Prompt para la nueva sesión](changes/013-cierre-base/resume.md).
No publicar datos, build, dist ni estado de herramientas. Los IDs identifican cambios,
no prioridad; seguir [workflow](workflow.md) para el siguiente objetivo.

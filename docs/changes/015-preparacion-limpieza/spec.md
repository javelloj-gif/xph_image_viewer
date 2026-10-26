# 015: Preparación y limpieza

Estado: alcance aprobado el 2026-10-25. [Acuerdos del lote](../product-batch-20261025.md).

## Objetivo y límites

Inventariar artefactos y fijar la identidad de la base antes de limpiar regenerables.
Conservar las dos últimas entregas de dist, sus ZIP, datos locales, fuentes,
fixtures, configuración de herramientas, respaldos y evidencia histórica no
regenerable. No cambiar código funcional ni iniciar benchmarks históricos.

## Criterios de aceptación

| ID | Resultado | Comprobación |
| --- | --- | --- |
| AC-01 | Identidad de Git y dos entregas conservadas antes de limpiar | Revisión y hashes SHA256 |
| AC-02 | Solo regenerables explícitos retirados, dentro del workspace | Inventario de rutas absolutas previo, sin seguir reparse points |
| AC-03 | Datos locales y entregas conservadas intactos | Comparación de hashes antes/después |
| AC-04 | Roadmap, índice y placeholder documentados | Diff y enlaces locales |

Las excepciones justificadas deben quedar en result.md, incluyendo archivos de
evidencia y repositorios anidados que no sean simples productos de compilación.

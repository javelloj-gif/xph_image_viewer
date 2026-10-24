# 003: arranque asíncrono, miniaturas persistentes y métricas reales

**Alcance vigente aprobado, 2026-10-23:** implementar arranque asíncrono completo,
prioridad de miniaturas, caché persistente sin pérdidas y medición real según
[implementation-plan.md](implementation-plan.md). Sustituye las exclusiones de
optimización y carpetas grandes del texto histórico inferior. Meta de 100 ms
para el original utilizable, sin reducir calidad; no es garantía previa.

AC-01..05 se amplían con AC-06: UI/GPU exclusivamente en principal y solicitudes
obsoletas descartadas; AC-07: caché exacta, invalidable, acotada y prescindible;
AC-08: sesión/datos/controles preservados durante cargas y cierre; AC-09: candidato
limpio y comparación reproducible con base identificada, sin certificar pantalla
por la mera llamada a Present. Los criterios necesarios sin evidencia quedan abiertos.

## Especificación histórica del diagnóstico

Fecha: 2026-10-22. Estado: porción diagnóstica IPC implementada y ejecutada;
primera ejecución real pendiente; base acotada CPU/memoria/GPU sin logging obtenida.
Sin optimizaciones aplicadas. [Validación real posterior a 005](real-validation.md).

## Problema y evidencia inicial

**Reporte del propietario:** la primera ejecución de la aplicación durante una sesión
de Windows es lenta; usos posteriores parecen más rápidos. No hay evidencia inmediata
de un proceso residente que explique esa diferencia. Esto no prueba su ausencia ni
la causa de la mejora aparente. Las guías registran un fallo previo de entrega IPC y
caminos síncronos de carga, sin mediciones que expliquen el arranque reportado.

## Objetivo

Obtener una base reproducible y un mapa de problemas confirmados, indicios e hipótesis
sobre arranque, IPC/múltiples instancias y CPU/GPU/memoria en reposo. Crear propuestas
acotadas de corrección con evidencia y prioridad sugerida.

## Fuera del alcance

- Optimizar o rediseñar durante la investigación.
- Carpetas masivas, navegación intensiva y animaciones como campaña completa inicial.
- Atribuir la causa a cachés, códecs, antivirus, GPU o IPC sin comprobarlo.
- Fijar porcentajes de mejora antes de medir la base o instalar herramientas sin necesidad.

## Comportamiento esperado y escenarios

- Primera ejecución real de LenteUV en una sesión de Windows; documentar si se inició
  la sesión tras reinicio o cierre de sesión y qué aplicaciones relevantes estaban abiertas.
- Ejecuciones posteriores con proceso anterior cerrado y mismos datos/configuración;
  comprobar procesos antes/después. Registrar cachés sin borrarlas arbitrariamente.
- Instancia activa más apertura de archivo por segunda invocación y comportamiento
  de `--multiple`; verificar receptor e imagen mostrada, no solo exit code del cliente.
- Reposo con imagen estática y carga asentada; observar CPU, GPU, memoria privada,
  working set y memoria de GPU cuando la herramienta permita atribución fiable.

Identificar inicio del proceso, ventana visible y primera imagen utilizable como
eventos distintos. Registrar ruta de entrada, archivo/carpeta y configuración.
Los instrumentos concretos y duración de observación se fijan antes de medir.

## Restricciones y compatibilidad

Medir Release y una revisión identificada; separar datos si cambia el ejecutable.
Usar copias controladas y datos sin información personal en informes versionados.
No reiniciar ni cerrar la sesión del propietario automáticamente para fabricar
una primera ejecución. Coordinar esa observación o registrarla como pendiente.
Permitir instrumentación diagnóstica acotada si hace falta, registrando su diff,
coste de observación y si entra en el ejecutable medido; no mezclar optimizaciones.

## Criterios de aceptación

| ID | Resultado observable | Comprobación |
| --- | --- | --- |
| AC-01 | Protocolo reproducible, entorno, revisión y datos identificados | Plan y ficha de ejecución |
| AC-02 | Primera ejecución y posteriores comparables con variabilidad | Series de mediciones; primera real etiquetada, no simulada |
| AC-03 | Mapa de entrega IPC y comportamiento de instancias | Pasos, trazas y resultado observado en receptor |
| AC-04 | Base de consumo en reposo con límites del instrumento | Series/ventanas y métricas atribuidas al proceso |
| AC-05 | Hallazgos clasificados, reproducibles y candidatos de corrección | `result.md`, enlaces a evidencia resumida y hoja de ruta |

No se exige una mejora de rendimiento para aceptar una investigación. Un escenario
necesario no medido queda pendiente, aunque las otras partes produzcan resultados.

## Preguntas y decisiones pendientes

La porción IPC se ejecutó con fixtures copiados, procesos reales, pipe exclusiva,
handler Window/App y bitmap software; evidencia y límites en [result.md](result.md).
Esa porción conservó el protocolo; AR-003 se corrigió posteriormente en el alcance de
[005](../005-entrega-ipc/result.md). 003 conserva sus pendientes de arranque/reposo
y presentación real; la matriz actual es regresión de éxito/error exactos de 005.

- Entrada habitual confirmada: doble clic por asociación; formatos/tamaños múltiples,
  carpetas <100 y >1000 elementos. Falta caso representativo y fase de demora.
- Elegir instalación/hash para medir: el portable asociado difiere del build 005;
  [Explorer/Abrir con](explorer-validation.md) preservó el predeterminado.
- Método/herramientas disponibles, ventanas de observación y repeticiones. Propuesta
  inicial: cinco ejecuciones posteriores y primeras ejecuciones en sesiones distintas
  cuando estén disponibles, informando tamaño de muestra y sin inferir de una sola observación.
- Coordinar primera ejecución real sin interferir con la sesión de trabajo.

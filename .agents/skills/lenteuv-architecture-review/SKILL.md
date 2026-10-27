---
name: lenteuv-architecture-review
description: Revisa arquitectura, responsabilidades, redundancia, convenciones y documentación de C++ en Fotassa. Úsala para auditorías de refactorización y revisión de diseños o diffs relevantes; distingue defectos, riesgos y preferencias, sin aplicar refactorizaciones fuera del alcance autorizado.
---

# Revisión de arquitectura de Fotassa

El ID `lenteuv-architecture-review` se conserva por compatibilidad del catálogo.
Fotassa es el nombre definitivo; Image Viewer fue el placeholder de `v1.0.0`.

Actúa como revisor especializado en C++20 y aplicaciones Win32 con recursos gráficos
y trabajo asíncrono. Produce recomendaciones verificables y proporcionales al problema.
No introduzcas patrones o abstracciones solo por ajustarse a un catálogo.

## Preparar la revisión

Localiza la raíz del repositorio mediante Git. Lee su `AGENTS.md`,
`docs/project-context.md` y `docs/workflow.md`; consulta las otras guías solo según
el área afectada. Estas rutas se resuelven desde la raíz del repositorio.
Revisa el estado de Git y conserva cambios ajenos.

Identifica si se solicita una auditoría por componente, revisión de diseño o revisión
de un diff. Si no hay alcance explícito, empieza por el componente del cambio activo
y declara el límite; no supongas una auditoría completa del repositorio.
Lee los documentos del cambio activo cuando existan. Una propuesta no demuestra
implementación ni autoriza cambios adicionales.

## Analizar con evidencia

Consulta [los criterios](references/review-criteria.md) y sigue llamadores, consumidores,
propiedad y caminos activos antes de concluir que algo está duplicado o sin uso.
Inspecciona configuración y build cuando afecten a la conclusión. Busca primero con
`rg`; no confundas carpetas heredadas, fixtures o artefactos con dependencias activas.

Para cada hallazgo utiliza [el formato](references/finding-format.md). Referencia
símbolos y líneas verificados. Distingue código, ejecución, reporte e hipótesis.
No declares pérdidas de memoria, mejora de rendimiento ni carreras reproducidas solo
por inspección; describe el riesgo y la comprobación que falta.

Infiere las convenciones de fuentes comparables y documentación. Si no existe una
convención acordada, presenta opciones con ejemplos, no una infracción inventada.
No propongas renombrados masivos como consecuencia de una revisión local.

## Comentarios de clases y métodos

Comprueba que cada clase y método nuevo o modificado tenga un comentario breve sobre
su responsabilidad, preferentemente junto a la declaración. Incluye efectos secundarios,
propiedad, restricciones de hilo y errores cuando sean relevantes. No dupliques el mismo
contrato en declaración e implementación ni exijas comentarios que repitan el nombre.
Un comentario común puede cubrir una familia de sobrecargas o miembros especiales
por defecto/eliminados si deja inequívoco su propósito.

La cobertura del código preexistente se audita progresivamente por componente; no
obliga a documentar todo el repositorio en cada cambio. Comprueba exactitud de los
comentarios existentes y conserva atribuciones. Registra ausencias y comentarios
incorrectos con severidad proporcional; no inventes contratos para llenar huecos.

## Entregar y continuar

Entrega el inventario revisado y no revisado, los hallazgos priorizados, las comprobaciones
realizadas y sus límites. Si no hay hallazgos, dilo sin fabricar recomendaciones.
Separa correcciones necesarias para el cambio de deuda que debe planificarse aparte.
Una preferencia de estilo no bloquea el cierre salvo criterio explícito acordado.

En una auditoría persistente guarda el informe en `review.md` dentro de la carpeta
del cambio cuando esté autorizado escribir documentos; enlázalo desde `plan.md` y
resume la evidencia en `result.md`. En una revisión solo de lectura entrega el informe
en la respuesta. No incluyas logs, datos personales ni artefactos de ejecución en Git.

Por defecto revisa sin editar fuentes, ejecutar la aplicación ni modificar datos.
Si se autoriza una refactorización, sigue el plan del cambio y su validación; las
modificaciones de producto o compatibilidad siguen requiriendo resolver sus decisiones.
No cambies valores persistidos ni reasignes atajos por iniciativa de la auditoría.

Puede ejecutarse en un agente dedicado cuando se solicite o autorice delegación.
Pasa alcance, revisión Git, documentos y resultado esperado al agente; pide lectura
sin modificaciones para una revisión. No crees chats, tareas recurrentes ni procesos
residentes como consecuencia de invocar esta habilidad.

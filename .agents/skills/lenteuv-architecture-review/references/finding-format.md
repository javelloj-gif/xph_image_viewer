# Formato de entrega

## Cobertura

Identifica revisión Git y modificaciones locales relevantes; objetivo de la revisión,
documentos consultados, archivos/símbolos inspeccionados, búsquedas auxiliares y áreas
sin revisar. En auditorías por etapas, deja un siguiente componente concreto.

## Hallazgos

Asigna un identificador estable como `AR-001`. Para cada hallazgo registra:

- **Ubicación:** archivo, símbolo y línea comprobada.
- **Evidencia:** código, ejecución, reporte o hipótesis; describe lo observado.
- **Clasificación:** defecto, riesgo, deuda o convención.
- **Prioridad:** alta, media o baja según impacto y urgencia, con justificación.
- **Confianza:** confirmada, indicio o hipótesis; especifica la comprobación pendiente.
- **Consecuencia:** escenario y efecto concreto, sin generalizaciones.
- **Propuesta:** opción mínima, alternativas y motivo para preferirla o mantener el diseño.
- **Coste estimado:** pequeño, medio o grande, con alcance y dudas; no inventes plazos.
- **Compatibilidad y validación:** contratos afectados y cómo comprobar preservación.
- **Relación con el cambio:** necesario para su aceptación o candidato a trabajo posterior.

No uses una cifra de prioridad para convertir automáticamente preferencias en bloqueos.
Una ubicación compartida puede agrupar varias ausencias de comentarios; enumera los
símbolos realmente revisados sin fabricar un hallazgo por línea.

## Cierre de la revisión

Resume comprobaciones ejecutadas, las no realizadas, riesgos pendientes y preguntas
de producto. Separa recomendaciones de decisiones aprobadas. Una auditoría terminada
no implica que sus refactorizaciones estén implementadas ni que el repositorio completo
esté certificado. Si no hay hallazgos accionables, declara ese resultado y sus límites.

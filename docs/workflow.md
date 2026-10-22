# Flujo de trabajo por cambios

Base inicial para trabajar con Codex mediante acuerdos escritos, planes ejecutables
y evidencia reproducible. Se ajustará con las especificaciones del propietario.
No requiere instalar herramientas adicionales.

## Responsabilidad de los documentos

| Documento | Responsabilidad |
| --- | --- |
| `../AGENTS.md` | Reglas permanentes y entrada a las guías |
| `project-context.md`, `usage.md`, `development.md` | Comportamiento observado, controles y comandos vigentes |
| `known-issues.md` | Reportes, defectos e hipótesis con su evidencia |
| `validation.md` | Evidencia histórica de la reforma base |
| `roadmap.md` | Orden, dependencias y estado de los cambios |
| `changes/<id>-<nombre>/spec.md` | Resultado deseado, límites y aceptación |
| `changes/<id>-<nombre>/plan.md` | Tareas, decisiones y estado para continuar |
| `changes/<id>-<nombre>/result.md` | Resultado real, evidencia y limitaciones |

Los planes no prueban comportamiento implementado. La evidencia de cambios nuevos
vive con cada cambio; no reescribir la validación histórica para presentarla como actual.

## Tamaño del trabajo

- **Trivial:** una descripción del cambio y su comprobación bastan; no crear una
  carpeta solo para corregir una errata o un enlace.
- **Funcional:** usar los tres documentos de cambio, con detalle proporcional.
- **Transversal o de datos:** ampliar los mismos documentos con compatibilidad,
  cancelación, fallos y recuperación cuando corresponda.

Cada cambio debe poder revisarse y aceptarse por separado. Dividir objetivos amplios
en entregas y registrar sus dependencias; no mezclar rediseños incidentales.

## Ciclo de trabajo

1. **Investigar.** Revisar Git, las guías relevantes y el camino activo del código.
   Separar evidencia de código, ejecución, reporte e hipótesis. Reproducir cuando
   sea posible y anotar lo que todavía no se ha comprobado.
2. **Especificar.** Copiar las [plantillas](changes/README.md) para un cambio real.
   Definir objetivo, exclusiones, restricciones y criterios `AC-01`, `AC-02`, etc.
   Consultar decisiones de producto o información que no pueda obtenerse del repositorio.
3. **Planificar.** Identificar componentes y propietarios del estado. Desglosar
   tareas verificables, asociarlas a criterios y definir la validación necesaria.
   Para trabajo complejo usar el modo de planificación de Codex y guardar el acuerdo
   en los documentos antes de implementar.
4. **Implementar.** Ejecutar el alcance acordado por porciones verificables. Actualizar
   tareas y decisiones. Si cambia el diseño, corregir el plan; si cambia el resultado
   deseado, revisar la especificación con el propietario antes del trabajo dependiente.
5. **Revisar.** Contrastar el diff con la especificación y buscar regresiones.
   Corregir hallazgos y repetir las comprobaciones afectadas. Una revisión en otra
   sesión puede ayudar, pero no sustituye evidencia de ejecución.
6. **Cerrar.** Completar `result.md`, relacionar cada criterio con evidencia y actualizar
   las guías afectadas y la hoja de ruta. Un criterio necesario pendiente o fallido
   impide declarar el cambio completado.

El alcance acordado autoriza sus tareas rutinarias; no pedir confirmación en cada paso.
Preguntar cuando una decisión cambie comportamiento, compatibilidad o alcance de forma
material. Continuar con trabajo independiente mientras se resuelve una pregunta.

## Uso de Codex y continuidad

- `AGENTS.md` dirige al agente a este procedimiento; los documentos de cambio son
  convenciones del repositorio y deben leerse explícitamente al trabajar en ellos.
- Al comenzar o retomar, leer `spec.md`, `plan.md`, `result.md` y el estado de Git.
  El siguiente paso debe poder entenderse sin consultar el historial del chat.
- Actualizar el plan al terminar una porción de trabajo y antes de interrumpirla.
  Registrar decisiones relevantes, no una transcripción de la conversación.
- Preferir una rama `codex/<nombre>` por cambio revisable. No cambiar de rama ni
  trasladar cambios ajenos sin revisar el estado existente.
- Si se usan varios checkouts, aislar build y datos. Los presets actuales comparten
  un árbol de build dentro de cada checkout; no compilar simultáneamente en él.

La base usa instrucciones, plantillas, scripts existentes y la habilidad local
[`lenteuv-architecture-review`](../.agents/skills/lenteuv-architecture-review/SKILL.md).
No hay agentes residentes ni automatizaciones periódicas.

## Revisión de arquitectura y comentarios

Invocar la habilidad para una auditoría por componente, una revisión de diseño o un
diff relevante. Puede usarse en la sesión actual o en un agente dedicado cuando se
autorice delegación. Acotar archivos, revisión Git y resultado esperado; una auditoría
inicial se divide por componentes y conserva inventario de cobertura.

Ejemplo de petición en Codex:

```text
Usa $lenteuv-architecture-review para revisar el componente del cambio activo.
Entrega hallazgos con evidencia y cobertura, sin modificar fuentes.
```

La habilidad no entrena un modelo ni inicia procesos permanentes. Su descubrimiento
depende de la sesión; si no aparece en la sesión actual, abrir una nueva o pedir que
se lea explícitamente su `SKILL.md` por la ruta enlazada.

Cada clase y método nuevo o modificado debe tener un comentario breve sobre su
responsabilidad. Documentar contratos de hilo, propiedad, efectos y errores cuando
sean relevantes, preferentemente en la declaración; no duplicar en la implementación.
Un comentario común puede cubrir una familia de sobrecargas o miembros especiales
por defecto/eliminados si su alcance es inequívoco. La cobertura de código anterior
se revisa por componentes, sin ampliar incidentalmente cualquier cambio pequeño.

Los informes persistentes se guardan como `review.md` dentro del cambio cuando esté
autorizado escribir documentación; el plan los enlaza y el resultado resume su evidencia.
Separar defectos, riesgos, deuda y convenciones; no presentar preferencias como fallos
ni atribuir impacto de rendimiento sin medición. Las refactorizaciones ajenas al alcance
se proponen como trabajo posterior. La revisión no sustituye pruebas ni recorridos UI.

Cada hallazgo persistente conserva un ID (`AR-001`, etc.) dentro de su informe. El plan
del cambio debe indicar si se abordará en una tarea, requiere diagnóstico, se difiere
o se descarta con motivo. No promover automáticamente recomendaciones a requisitos
aprobados ni marcar un hallazgo corregido sin el diff y su validación. Si se crea otro
trabajo para resolverlo, enlazar tanto el hallazgo de origen como la nueva especificación.

## Validación proporcional

Consultar [desarrollo](development.md) para los comandos exactos y
[validación de la base](validation.md) para sus límites.

| Área afectada | Evidencia que debe planificarse |
| --- | --- |
| Documentación | Diff, enlaces y correspondencia con fuentes relevantes |
| Lógica funcional | Compilación y pruebas relevantes; casos de regresión cuando aporten valor |
| UI y gestos | Recorrido visual reproducible en los modos afectados |
| Concurrencia | Cambios rápidos, cancelación y cierre con trabajo pendiente |
| Rendimiento | Base y comparación bajo el mismo entorno y conjunto de datos; acordar umbrales |
| Persistencia y metadatos | Copias aisladas, lectura posterior y preservación/compatibilidad |
| Distribución | Staging nuevo y comprobaciones específicas del paquete |

El harness actual de metadatos no prueba renderizado. Compilar no demuestra UX ni
rendimiento. Registrar comando o procedimiento, entorno, revisión evaluada y resultado;
distinguir fallos del producto de restricciones del entorno sin asumir su causa.
Mantener logs, capturas, cachés y datos de prueba locales fuera de Git según las reglas
del repositorio. Versionar solo el resumen necesario y reproducible de la evidencia.

# Criterios de revisión

Aplica solo los criterios pertinentes al alcance. Las reglas del repositorio son
la referencia; este archivo añade preguntas para investigarlas.

## Responsabilidades y diseño

- Identifica quién posee cada estado y quién puede modificarlo. Evalúa responsabilidades
  mezcladas, invariantes dispersas y duplicación de estado con sus consumidores reales.
- Las páginas comparten estado con `App` mediante `friend`; su mera presencia no es
  un defecto. Justifica cualquier cambio de frontera con un problema concreto.
- Antes de sugerir un patrón, explica el problema, la opción mínima, las alternativas
  y el coste de migración. Considera mantener el diseño si no hay beneficio demostrable.

## Redundancia y dependencias

- Sigue referencias, handlers, configuración y lista de fuentes del build. La ausencia
  de una llamada textual no prueba por sí sola código muerto: revisa callbacks y rutas
  indirectas. Distingue duplicación accidental de semánticas distintas.
- Una carpeta de un modo puede contener servicios compartidos. Para retirar funcionalidad,
  separa lo exclusivo de lo compartido antes de proponer borrar o mover archivos.
- Evalúa nombres que ya no representen responsabilidad, dependencias y consumidores.

## Recursos y concurrencia

- Revisa RAII, propiedad, referencias, capturas y duración de objetos en tareas pendientes.
  Busca invalidación por cambio de carpeta, ordenación, borrado y cierre.
- Recursos D2D/texturas y UI se crean o modifican en el hilo principal. Los callbacks
  del loader se entregan en `ProcessPendingTextures`; no extrapoles a los escáneres.
- Revisa colas, cancelación, joins y duración de bloqueos. Un riesgo estático no equivale
  a una carrera reproducida ni a una medición de latencia.
- Distingue retención intencional de caché y fuga; RAM de CPU y memoria de GPU; recuento
  de texturas y presupuesto en bytes. Recomienda medición para afirmar impacto.

## Compatibilidad y datos

- Revisa consumidores de enums persistidos, settings, KTAG/KBRD y rutas de datos. Los
  cambios de formato requieren decisiones explícitas y validación sobre copias.
- Comprueba rutas de arranque, IPC y entrada por Explorer/drag-and-drop cuando se
  retiren modos. Detectar un valor obsoleto exige definir su tratamiento, no inventarlo.
- No realices pruebas de escritura de metadatos o borrado sobre originales.

## Convenciones y comentarios

- Identifica convenciones existentes de miembros, locales, tipos, unidades y funciones.
  Separa inconsistencia comprobada de preferencia personal; justifica mejoras de claridad.
- Revisa comentarios de clases y métodos, incluidos los privados. Prioriza contratos
  de hilo, propiedad y efectos laterales sobre descripciones mecánicas.
- Señala comentarios contradictorios o nombres provisionales; no reemplaces atribuciones.
  Evita duplicar documentación o escribir comentarios especulativos.

## Validación

- Vincula cada refactorización propuesta a invariantes y comprobaciones de regresión.
  No pidas tests que solo reproduzcan el texto o la estructura de la implementación.
- El harness de metadatos usa un stub de renderizado. CTest no certifica UI/GPU.
  Compilar no demuestra UX, rendimiento ni recepción de IPC.
- Distingue fallo del producto, restricción del entorno y comprobación pendiente.
  No declares revisado un archivo que solo apareció en una búsqueda.

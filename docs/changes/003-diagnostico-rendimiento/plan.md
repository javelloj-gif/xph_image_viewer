# Plan: 003 Diagnóstico de rendimiento

Especificación: [spec.md](spec.md). Última actualización: 2026-10-22.
Estado: protocolo preliminar; sin mediciones ejecutadas.

## Componentes y diseño

Seguir `main`/`App::Initialize`, inicialización de renderer/loader, restauración de
sesión, primera carga, IPC y bucle de reposo. Usar los hallazgos de 001 como preguntas,
sin tratarlos como causa probada. Desglosar inicio, ventana e imagen utilizable.

Entradas de la [auditoría de 001](../001-refactorizacion-base/review.md): AR-003 para
IPC; AR-005 en su parte IPC/cierre, dejando la campaña de loader para etapa posterior;
AR-006 como pregunta de coste de carga de caché, sin introducir endurecimiento en esta
investigación. El informe añade preguntas sobre restauración de sesión, carga síncrona
y reposo. No ampliar por ello el alcance inicial a toda navegación o animaciones.

## Tareas y comprobaciones

- [ ] T-01: Acordar caso reportado, datos y ruta de entrada; registrar hardware,
  Windows, almacenamiento, códecs relevantes, revisión, Release y settings. AC-01.
- [ ] T-02: Seleccionar instrumentos y definir fases, repeticiones y observación. AC-01.
- [ ] T-03: Medir arranque inicial real y posteriores; registrar procesos/cachés
  observables sin alterarlos arbitrariamente. AC-02.
- [ ] T-04: Ejecutar matriz instancia existente/segunda apertura/`--multiple`;
  registrar recepción, proceso activo, imagen resultante y fallos. AC-03.
- [ ] T-05: Medir reposo con imagen estática y condiciones estables; separar CPU,
  GPU, working set/memoria privada y métricas no disponibles. AC-04.
- [ ] T-06: Publicar resumen y hallazgos con impacto, confianza, reproducción,
  prioridad sugerida y próxima comprobación/corrección. AC-05.

## Validación

Conservar mismos datos y configuración entre ejecuciones comparables. Registrar
cada observación y dispersión; no mezclar primeras ejecuciones reales con posteriores.
No considerar el cliente IPC exitoso como demostración de recepción. La instrumentación
y su revisión deben figurar en las mediciones. Logs/perfiles quedan locales e ignorados;
versionar el resumen suficiente para reproducir. No afirmar fuga por un pico de memoria.

## Riesgos y recuperación

No interferir con procesos/datos personales. La primera ejecución real requiere
coordinación con el propietario, no reinicio automático. Si falta un instrumento,
registrar su límite y continuar mediciones independientes.

## Decisiones y ajustes

2026-10-22: se limita la primera etapa a arranque, IPC y reposo. Refactorización
es la prioridad; si una porción de 001 afecta estos escenarios, adelantar solo su base
antes de cambiar el código para conservar comparación.

## Estado para continuar

- Completado: alcance y significado de primera ejecución documentados.
- En curso: ninguna medición.
- Preguntas: caso habitual de apertura, protocolo operativo y oportunidad de primera ejecución.
- Siguiente paso: seleccionar el caso y los instrumentos disponibles sin instalar ni ejecutar nada todavía.

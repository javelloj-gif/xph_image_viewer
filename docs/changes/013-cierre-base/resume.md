# Prompt para continuar en una nueva sesión

```text
Trabaja en E:\TEMPORAL\Proyectos\Sergkrumas\image_viewer, rama
codex/repository-foundation. Revisa Git antes de editar y conserva cambios locales.
La refactorización base está cerrada: no reanudes una campaña general de limpieza.
El propietario reportó prueba funcional del ejecutable temporal con funcionalidades
base intactas y pidió cerrar la base por consumo de recursos superior al esperado.

Lee AGENTS.md, docs/roadmap.md, docs/workflow.md, docs/project-context.md,
docs/changes/001-refactorizacion-base/closure.md y spec/plan/result/review de
docs/changes/013-cierre-base. Usa la habilidad lenteuv-architecture-review solo
si el siguiente trabajo requiere revisión de arquitectura o un diff relevante.

El lote R1/R2 y 004–012 está guardado en Git junto con 013. Consulta git log;
ac4a5da es una base histórica, NO el producto funcional actual. 013 añade lectura
transaccional KTAG v1 (AR-006) y une BoardTagScanner/búsqueda antes del loader y de
destruir dependencias. Release y CTest 6/6 pasaron; 164 truncados × dos dirty,
compatibilidad KTAG, 162025 checks async y 294 WIC. Datos y artefactos ignorados.

Portable nuevo dist/lenteuv_qa_013_20261023.zip; hashes en result.md. Incluye las
correcciones 013, posteriores a la prueba manual reportada; no atribuirles esa
prueba ni reabrir por ello todo el lote base. Paquetes anteriores conservados.

Todos los pendientes de ejecución de la base tienen cierre/disposición explícita.
AR-005 global NO está corregido: cancelación por consumidor, WIC/E/S activa,
latencia de scanners y WriteFile/FlushFileBuffers síncronos son robustez futura.
AR-007 preexistente sigue progresivo. No afirmar auditoría universal ni mejora global.
002 (retirar Library) sigue en definición: decidir valor persistido 4 y placeholder.
003 primera real/caso representativo, masonry/última medida y OPT-007 son trabajo
separado sin ejecución automática. No resolver decisiones de producto por iniciativa.

El siguiente objetivo debe ser el que indique el propietario en esta sesión.
Si aún no dio uno, informa brevemente que la base está guardada y solicita el nuevo
objetivo; no empieces refactorización, benchmarking ni retirada de Library.
Si hay un objetivo concreto, impleméntalo de forma acotada según workflow, con
comentarios de responsabilidad, validación proporcional y sin tocar datos personales.
No publicar, instalar, cambiar asociaciones, forzar hilos o renumerar enums/formatos.
```

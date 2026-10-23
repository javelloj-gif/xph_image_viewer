# Prompt de continuidad para una nueva sesión

**Estado actualizado, 2026-10-23:** 002 cerrado tras confirmación del propietario
de pruebas funcionales exitosas. IPC-014-01 cerrado por reporte de funcionamiento
actual: abrir una nueva imagen desde Windows actualiza el visor. Limpieza autorizada
completada: 157 EXE y 8 ZIP retirados, con candidato aceptado y datos conservados;
excepciones y evidencia en [result.md](result.md). No repetir la limpieza ni pedir
el reporte ya recibido. No hay nueva tarea funcional autorizada. El texto siguiente
se conserva como prompt histórico de preparación, sustituido por este estado.

Copia el texto siguiente como petición en la nueva conversación:

```text
Continúa la tarea 002: retirada de Library, en:
E:\TEMPORAL\Proyectos\Sergkrumas\image_viewer

Revisa Git antes de actuar y preserva cualquier cambio posterior. La rama de
preparación es codex/repository-foundation; la implementación está en bff3d92.
Puede haber un commit documental posterior con este prompt y el candidato.
Lee AGENTS.md, docs/workflow.md, docs/roadmap.md y spec.md, plan.md, result.md,
review.md y functional-test.md de docs/changes/002-retirar-library/.
Lee también docs/changes/014-cancelacion-ar005/user-validation.md.

Estado y alcance vigente:
- Library y sus referencias funcionales fueron retirados. Viewer, Waterfall,
  Board y FolderDataManager/modelos en core se conservan; no renumerar sus IDs.
- NUM2/2 están reservados, conservando el carácter 2 en editores. La posición
  radial — / Unavailable es gris e inactiva.
- El propietario canceló expresamente el selector que se había propuesto.
  No crear diálogos, opciones nuevas, fallback ni migración del antiguo valor 4.
  Limitar cualquier corrección a un fallo comprobado de la retirada solicitada.
- Base y AR-005 cerrados. No repetir extracciones ni AR-002 ni abrir otra campaña.
- IPC-014-01 sigue diferido por el propietario: no investigar, reproducir ni
  corregir. CTest no certifica aceptación funcional completa de IPC.

Candidato temporal para las pruebas del propietario:
dist/lenteuv_qa_002_20261023_sin_library/lenteuv.exe
ZIP: dist/lenteuv_qa_002_20261023_sin_library.zip
SHA256 EXE: 42432671B4BE7602C4F2B4060586CFE5B6EB36AE0D382B84312038CBE60BF23A
SHA256 ZIP: 4F3387DE83D7331C1D044FF7FA677392B3FF6BCA33DAF3E218012FB42956931A
La carpeta incluye runtime/locales/user_data limpios. No es el ZIP obsoleto con
diálogos. Build Release correcto; CTest 7/7 del diff simplificado ya registrado.
El empaquetado y sus imports se comprobaron; eso no demuestra aceptación visual.
El harness provisional con selector fue retirado: sus 124 checks no acreditan
el candidato actual.

Trabajo pendiente:
1. Recoger el reporte del propietario según functional-test.md. Si todavía no
   hay reporte, pedirlo y esperar; no inferir éxito ni borrar ejecutables.
2. Si hay fallos, documentarlos y corregir solo los que correspondan a 002,
   conservando datos. Usar lenteuv-architecture-review para cambios relevantes.
   Revalidar lo afectado y generar otro candidato solo si hace falta. No convertir
   un fallo o una ambigüedad en autorización para diseñar interacciones adicionales.
3. Si las pruebas necesarias son exitosas, registrar la aceptación y límites,
   actualizar result/plan/review/roadmap y cerrar 002 con evidencia real.
4. Solo tras ese éxito, ejecutar la limpieza que el propietario autorizó:
   eliminar todos los ejecutables previos de este proyecto identificados en disco.
   Conservar el candidato aceptado, su runtime, sus datos y su ZIP.

Alcance y método de limpieza autorizada:
- Eliminar versiones anteriores de LenteUV, instaladores antiguos y copias de
  ejecutables de pruebas/diagnóstico generadas por este repositorio bajo build/
  y dist/, incluidos staging, extracciones y runs anteriores. Retirar asimismo
  paquetes antiguos que contengan esos ejecutables para no conservarlos en ZIP.
- Hay un inventario local inicial en
  build/qa-continuity-002/executables-before-candidate.txt. Es orientativo: volver
  a inventariar después de las pruebas y verificar cada archivo y su pertenencia.
- No borrar por extensión sobre todo el disco ni eliminar software ajeno,
  herramientas instaladas, fixtures, fuentes, documentación, .git, .kilo o
  worktrees ni datos personales/user_data/logs/cachés para limpiar ejecutables.
  Si aparecen copias fuera de las ubicaciones conocidas y no está claro si
  pertenecen a esta entrega, precisar esas rutas con el propietario antes de borrar.
- Usar PowerShell de extremo a extremo, rutas absolutas verificadas y operaciones
  sobre archivos concretos con -LiteralPath. No borrar carpetas completas como
  atajo. Verificar que cada ruta esté dentro del ámbito autorizado y que no sea
  el candidato aceptado. No borrar un EXE en uso; cerrar solo procesos propios.
- Los ejecutables del build/test actual que aún sean necesarios no son versiones
  antiguas; preservar lo necesario para continuar desarrollo y validación.
- Registrar cantidades/ubicaciones generales retiradas, excepciones y verificación
  final. No versionar el inventario local completo, artefactos ni datos de prueba.

Guarda el trabajo y deja Git y la continuidad claros. No iniciar otra tarea
funcional ni modificar el workflow por iniciativa propia.
```

# Continuidad tras AR-005

```text
Trabaja en E:\TEMPORAL\Proyectos\Sergkrumas\image_viewer,
rama codex/repository-foundation. Revisa Git y preserva cambios locales.
Lee AGENTS.md, docs/roadmap.md y spec/plan/result/review de
docs/changes/014-cancelacion-ar005. El objetivo AR-005 está implementado y cerrado:
no reabrirlo ni continuar una campaña de refactorización por inercia.

014 implementa cancelación por consumidor en tira/previews/hover/Board, poda real
de cola, stop cooperativo WIC entre etapas/frames/filas, cancelación de ambos
escáneres con publicación segura y cliente IPC overlapped bajo presupuesto total.
App une productores antes de destruir HWND/dependencias. Los límites de una llamada
WIC/E/S que ya ejecuta están caracterizados; no se fuerza ni detacha su hilo.

Release y CTest 7/7, 42 checks nuevos de consumidores/Board, 314 WIC, 162025 async,
13 escenarios de cierre por procesos y diez nativos. El producto extraído arrancó
con --multiple/--lite, recorrió cuatro modos por mensajes de su propio proceso y
cerró con código 0. Esta es evidencia automatizada histórica, sin mejora global afirmada.

Actualización del propietario 2026-10-23: pruebas funcionales del portable 014
realizadas. Falló el punto 6: abrir otra imagen mediante el mismo ejecutable mantiene
la imagen anterior. Lee user-validation.md (IPC-014-01). El propietario lo identifica
como error legado y ordena no tratarlo todavía: no investigar, reproducir ni corregir
IPC sin una instrucción posterior que lo autorice. No asumir aprobación individual
de los demás puntos ni aceptación completa de IPC; no reabrir AR-005 automáticamente.

EXE para pruebas funcionales:
dist/lenteuv_qa_014_20261023_final/lenteuv.exe
ZIP homónimo; hashes y evidencia en result.md, protocolo en functional-test.md.
Usar el sufijo final; no sustituir el portable personal ni confundirlo con el
candidato preliminar 014 o 010–013, que se conservaron. No publicar dist/build/datos.

La aplicación queda preparada para el próximo cambio funcional que indique el
propietario. Atiende su objetivo concreto sin iniciar optimizaciones opcionales,
retirada de Library ni auditoría universal. 002 requiere decidir valor persistido
4 y placeholder si se elige esa tarea; 003/OPT-007 son trabajos independientes.
Si el usuario trae más resultados de 014, registra la evidencia y respeta las
decisiones de aplazamiento; IPC-014-01 continúa diferido hasta nueva instrucción.
Para el siguiente cambio autorizado, usa workflow y validación
proporcional; conserva formatos, enums, gestos y atribuciones.
```

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
cerró con código 0. Sin certificación visual/manual ni mejora global afirmada.

EXE para pruebas funcionales:
dist/lenteuv_qa_014_20261023_final/lenteuv.exe
ZIP homónimo; hashes y evidencia en result.md, protocolo en functional-test.md.
Usar el sufijo final; no sustituir el portable personal ni confundirlo con el
candidato preliminar 014 o 010–013, que se conservaron. No publicar dist/build/datos.

La aplicación queda preparada para el próximo cambio funcional que indique el
propietario. Atiende su objetivo concreto sin iniciar optimizaciones opcionales,
retirada de Library ni auditoría universal. 002 requiere decidir valor persistido
4 y placeholder si se elige esa tarea; 003/OPT-007 son trabajos independientes.
Si el usuario trae resultados funcionales de 014, registra pasos/hash y corrige
regresiones concretas antes del siguiente cambio. Usa workflow y validación
proporcional; conserva formatos, enums, gestos y atribuciones.
```

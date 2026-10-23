> Prompt histórico sustituido por [continuidad 013](../013-cierre-base/resume.md). No ejecutar la siguiente porción AR-005 por inercia: la base quedó cerrada y sus límites diferidos explícitamente.

# Prompt para retomar

```text
Continúa en E:\TEMPORAL\Proyectos\Sergkrumas\image_viewer.
Rama codex/repository-foundation, base ac4a5da. Revisa Git y preserva todo el trabajo
sin commit; no uses HEAD como baseline del producto actual.
Usa lenteuv-architecture-review. Lee AGENTS, workflow, project-context, roadmap,
coverage-004-011.md de 001 y spec/plan/result/review de 012-cierre-colas-ipc.

012 completó la primera porción autorizada de AR-005: loader rechaza nuevas tareas
al parar, descarta cola y une activas sin callbacks; servidor IPC con stop event e
I/O overlapped cancelable, sin centinela local; App para IPC antes de destruir HWND
y ambos servicios antes de consumidores. Se conservan callbacks normales 004/007/011,
protocolo 005 y retención temprana 010. Prueba WM_QUIT/payload y hide-to-tray incluidas.
Release/CTest 6/6, nueve escenarios de cierre en procesos propios (uno con doce ciclos),
162022 checks async, 294 decoder y diez escenarios nativos pasados. Baseline reprodujo
1001 tareas drenadas y dos timeouts IPC; candidato 0 encoladas ejecutadas y sin timeouts.
No mejora de latencia de apertura ni plazo universal de cierre afirmados.

Portable dist/lenteuv_qa_012_20261023/lenteuv.exe y ZIP homónimo; hashes/protocolo en
result.md. Paquetes 010/011 preservados. Aceptación visual/funcional del usuario pendiente.

AR-005 sigue abierto: especificar siguiente porción de cancelación por consumidor y
caracterización WIC/scanners/cliente síncrono antes de implementar. No ampliar a ACK,
elección atómica o retirada de Library. AR-006/007, R2 visual, 010 visual, 003 primera
real/caso representativo, límites 008/009, masonry/última medida y OPT-007 separados.
Actualizar evidencia y continuidad y regenerar portable limpio tras nuevas porciones.
```

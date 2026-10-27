# Plan 024

- [x] Revisar guías, habilidad local, Git y antecedente OPT-007 de 003.
- [x] Seguir Run/RenderFrame, Dock, Renderer, Window, animaciones y uploads.
- [x] Contrastar contratos DXGI con documentación primaria de Microsoft.
- [x] Analizar CSV GPU-Z original sin modificarlo y registrar límites.
- [x] Comparar segundo log con horas reportadas de apertura/minimización/cierre.
- [x] Documentar hallazgos, tradeoffs y protocolo en review.md.
- [x] Relacionar resultado con problemas conocidos y hoja de ruta.

AR-001/002/003/005: candidatos a futura planificación de frames, requieren
especificación y medición nativa. AR-004: correlacionar bytes/tiempo de uploads
antes de introducir cuotas. Ninguno se implementa incidentalmente en 023.

Continuidad posterior: 025 implementa y mide la primera corrección con atribución
por PID y contadores Draw/Present. El propietario acepta las pruebas de la sesión
el 2026-10-26; véase [cierre](../025-renderizado-por-cambios/closure.md).
Una nueva campaña requiere alcance propio, EXE/datos aislados y ausencia de
builds/tests simultáneos; no reutilizar por inercia el protocolo de arranque 003.

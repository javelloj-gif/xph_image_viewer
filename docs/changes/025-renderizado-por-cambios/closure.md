# Cierre aceptado 023–025 — 2026-10-26

El propietario solicita marcar las pruebas como satisfactorias y realizar commit
y merge. Se acepta la entrega de la sesión: transparencia de la leyenda del dock,
análisis de los dos logs GPU-Z y corrección de dibujo redundante/invisible.

Evidencia conservada: Release/CTest 9/9, 14 checks nuevos, smoke nativo de diez
escenarios y comparación por proceso de Draw/Present y contadores GPU. EXE y ZIP
identificados en [result.md](result.md). La aceptación no añade mediciones ni
elimina límites, especialmente IPC-003-CONCURRENT-01 y el sondeo CPU de 16 ms.

Flujo Git del cierre: rama `codex/025-gpu-idle-approved` desde la integración
actual `codex/015-roadmap-design`, commit del código/documentación de 023–025
y merge local explícito de vuelta a la integración. `master` es la base histórica
del upstream original; no es el destino de este cierre ni se cambia su tracking.

No publicar al remoto ni crear PR en este cierre. El propietario reserva los
ajustes finales antes de subir al repositorio maestro para próximas sesiones
cortas. No se instalan paquetes ni se limpian datos/artefactos locales.

Al retomar: revisar Git y este cierre; definir el ajuste concreto de la sesión.
No repetir campañas de rendimiento ni reabrir automáticamente la base aceptada.

# Resultado 023 — 2026-10-26

Implementado sobre 6e66535, con árbol inicialmente limpio. La leyenda usa
`Black(0.6f * alpha)` y borde `White(0.15f * alpha)`, como el fondo/borde de
metadatos. Conserva tipografía y geometría anteriores.

- AC-01: comprobado en diff y snapshot productivo WARP. Se inspeccionó el PNG
  `build/session-023/dock-801-hover.png`: las dos leyendas tienen fondo/borde
  similares. El fondo sintético uniforme no certifica legibilidad sobre toda foto.
- AC-02: `./scripts/build.ps1 -Configuration Release -Test`, correcto, **9/9**.
  `async_preview_tests.exe --dock-snapshot build/session-023/dock.bgra`, correcto:
  **12 frames WARP** (320/801 px, 96/144 DPI, tres estados).

EXE: `build/windows-x64/bin/Release/image_viewer.exe`.
SHA256: `0D1C062912ABB14E32CF0A7F476E9B00ADB1F125535FDEF1B7E9B4F4CC3C79CD`.
Log: `build/session-023-build.log`. Artefactos locales ignorados.

En la validación inicial no se ejecutó recorrido nativo ni se crearon paquetes/commit.
No se atribuye al ajuste una mejora de consumo GPU.

## Aceptación y cierre — 2026-10-26

El propietario solicita «Marca las pruebas como satisfactorias» y autoriza commit
y merge local del trabajo de la sesión. Se registra aceptación de la entrega,
incluido este fix incorporado al portable final de 025. No se infiere un recorrido
visual detallado ni se añaden mediciones distintas de las documentadas.

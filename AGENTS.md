# Fotassa: entrada para agentes

Nombre definitivo: **Fotassa**; proyecto/EXE `fotassa` / `fotassa.exe`.
Image Viewer fue el placeholder de la entrega original `v1.0.0`.
Leer [032](docs/changes/032-nombre-fotassa/result.md) para la corrección posterior;
conservar nombres históricos y contratos internos de compatibilidad.

Visor de imágenes nativo de Windows en C++20. Tres modos: Viewer, Waterfall
y Board. Conserva la atribución a Sergei Krumas y xPhang y la licencia
del repositorio. No confundir carpetas/muestras heredadas con dependencias activas.

## Leer según la tarea

- [Contexto técnico](docs/project-context.md): arquitectura, flujos, hilos, formatos y mapa de intervención.
- [Uso](docs/usage.md): controles actuales y diferencias entre teclas numéricas.
- [Desarrollo](docs/development.md): compilación, tests y paquetes.
- [Flujo de trabajo](docs/workflow.md): especificaciones, planes y evidencia por cambio.
- [Hoja de ruta](docs/roadmap.md): prioridades, dependencias y estado del trabajo.
- [Revisión de arquitectura](.agents/skills/lenteuv-architecture-review/SKILL.md): habilidad para auditorías y revisión de diseños/diffs relevantes.
- [Problemas conocidos](docs/known-issues.md): hallazgos, reportes e hipótesis.
- [Validación de la base](docs/validation.md): evidencia y límites de esta reforma.

## Comandos desde la raíz

```powershell
cmake --preset windows-x64
cmake --build --preset release
ctest --preset release
./scripts/build.ps1 -Configuration Debug -Test
./scripts/build_release.ps1 -SkipIcons
```

Windows x64, Visual Studio 2022 C++/Windows SDK, CMake 3.21+ y PowerShell.
No se necesita Python si se conserva el ICO. El instalador requiere Inno Setup 6.

## Reglas de mantenimiento

- Revisa Git antes de trabajar. Preserva cambios ajenos y los datos locales.
- `src/` contiene la aplicación; `tests/fixtures/` son muestras, no archivos para modificar en tests.
- Crea recursos D2D/texturas y modifica UI en el hilo principal. Los callbacks del loader
  se entregan en `ProcessPendingTextures`; los escáneres tienen otro contrato.
- No cambies enums persistidos, formatos KTAG/KBRD, rutas de datos o gestos como efecto
  secundario de una reorganización. Las páginas comparten estado con `App` vía `friend`.
- Ejecuta metadatos/borrado sobre copias. CTest prepara su propio EXE, fixtures y `user_data`.
- No publiques `user_data`, logs, cachés, `build/`, `dist/` ni estado de herramientas.
  Los paquetes se crean desde staging nuevo, nunca desde datos personales.
- Distingue evidencia de código, ejecución, reporte e hipótesis. Una compilación no demuestra
  UX ni rendimiento con carpetas grandes. Actualiza la guía afectada al cambiar un contrato.
- Para cambios funcionales usa `docs/changes/<id>-<nombre>/` según `docs/workflow.md`.
  Mantén especificación, plan y resultado al día; al retomar, lee esos archivos y Git.
  Los cambios triviales solo requieren descripción y comprobación proporcional.
- Cada clase y método nuevo o modificado debe tener un comentario breve de su
  responsabilidad; añade contratos de hilo, propiedad y efectos cuando correspondan.
  Prefiere la declaración y evita duplicar el comentario en la implementación.
  La documentación del código preexistente se revisa progresivamente por componente.

# LenteUV: entrada para agentes

Visor de imágenes nativo de Windows en C++20. Cuatro modos: Viewer, Library,
Waterfall y Board. Conserva la atribución a Sergei Krumas y xPhang y la licencia
del repositorio. No confundir carpetas/muestras heredadas con dependencias activas.

## Leer según la tarea

- [Contexto técnico](docs/project-context.md): arquitectura, flujos, hilos, formatos y mapa de intervención.
- [Uso](docs/usage.md): controles actuales y diferencias entre teclas numéricas.
- [Desarrollo](docs/development.md): compilación, tests, paquetes y futura adopción de Spec Kit.
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
- `.specify/` y `specs/` están reservados para la futura instalación de Spec Kit;
  no hay comandos o constitución generados todavía.

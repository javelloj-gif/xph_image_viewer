# Image Viewer (placeholder)

Versión **1.0** (`1.0.0`). Entrega Windows x64: portable
`image_viewer_portable_1.0.0.zip` e instalador `image_viewer_setup_1.0.0.exe`.
Fuentes: etiqueta `v1.0.0`. [Resultado de entrega](docs/changes/031-release-1.0/result.md).

Visor de imágenes para Windows inspirado en Picasa, derivado del
[image_viewer de Sergei Krumas](https://github.com/sergkrumas/image_viewer),
migrado a C++ y extendido por xPhang con visor, cascada, tablero y metadatos XMP.

El visor usa Win32, Direct3D 11, Direct2D, DirectWrite, DirectComposition y WIC.
Requiere Windows 10 1903+ x64 y una GPU compatible; los formatos adicionales dependen
de los códecs WIC instalados. No se ha certificado aún el rendimiento con decenas
de miles de imágenes.

## Compilar y probar

Instala Visual Studio 2022 con desarrollo de escritorio C++ y Windows SDK,
CMake 3.21+ y PowerShell. Desde esta carpeta:

```powershell
cmake --preset windows-x64
cmake --build --preset release
ctest --preset release
./build/windows-x64/bin/Release/image_viewer.exe ./tests/fixtures/linux.png
```

También: `./scripts/build.ps1 -Configuration Release -Test`.
Para crear ZIP e instalador con Inno Setup 6: `./scripts/build_release.ps1 -SkipIcons`.
Los resultados se escriben en `build/` y `dist/`, fuera del seguimiento de Git.

## Documentación

- [Guía de uso](docs/usage.md)
- [Compilación y distribución](docs/development.md)
- [Contexto para agentes](docs/project-context.md) y [entrada breve](AGENTS.md)
- [Problemas conocidos](docs/known-issues.md) y [validación](docs/validation.md)
- [Flujo de trabajo](docs/workflow.md), [hoja de ruta](docs/roadmap.md) y [plantillas de cambios](docs/changes/README.md)

`src/` conserva los subsistemas nativos; `locales/` contiene traducciones;
`tests/fixtures/` conserva las muestras. Los datos personales permanecen locales.
Los cambios funcionales se organizan mediante especificaciones, planes y evidencia
versionados en `docs/changes/`, siguiendo el flujo de trabajo del repositorio.

Licencia: consultar [LICENSE](LICENSE), GNU GPL versión 3. Se preservan el historial
y las atribuciones del proyecto de origen.

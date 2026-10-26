# Excepciones de compatibilidad 022

Image Viewer / image_viewer son placeholders. Se conserva deliberadamente:

| Contrato | Motivo |
| --- | --- |
| user_data, resolución de ascendientes, nombres de settings/session/cache/boards/tags | Datos existentes; sin migración ni ruta nueva |
| Page y formatos KTAG/KBRD/XMP | Datos persistidos y atribuciones intactos |
| pipe `\\.\pipe\lenteuv` | Interoperabilidad con instancias previas; mismo protocolo |
| clase HWND `LenteUVWindow`, identidad manifiesto `xPhang.LenteUV` | Integración Windows existente |
| valor Run `LenteUV`, claves shell `LenteUV`, AppId Inno y directorio instalado `{autopf}\LenteUV` | No duplicar registros/instalaciones por renombrado visual; sin migración silenciosa |
| variables `LENTEUV_*` de instrumentación | Alias de lectura de IMAGE_VIEWER_*; el nombre neutral tiene precedencia. No preferencias nuevas |
| scripts de diagnóstico dedicados a 003 y rutas a dist históricas | Reproducibilidad de evidencia conservada; no ejecutar por esta neutralización |
| ID de habilidad local `lenteuv-architecture-review` | Catálogo existente del agente; no alterar su instalación por el renombrado del producto |

El EXE nuevo se llama image_viewer.exe. No se borra un EXE viejo instalado ni se reescriben accesos directos/Run de una instalación previa durante las pruebas. La actualización/desinstalación real de instalaciones antiguas no se ejecuta en la máquina del propietario; requiere validación en entorno aislado. El instalador conserva AppId y directorio para esa compatibilidad.

Historial 001–021, dist y LICENSE no se renombra. Assets retirados preservados en history/; no los consume el build nuevo. No se declara cambiado el nombre definitivo del producto.

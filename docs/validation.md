# Validación de la reforma base

Fecha de auditoría: 2026-10-21. No interpretar una entrada pendiente como una prueba pasada.

## Base anterior

- **Ejecución:** compilación Release satisfactoria de app y harness con la estructura anterior.
- **Ejecución:** pruebas JPEG/PNG, cache, cancelación, metadatos extendidos y palabras clave
  satisfactorias en una carpeta temporal fuera del sandbox.
- **Ejecución:** el primer intento en el sandbox falló en escritura JPEG; la repetición
  fuera pasó. No se atribuyó ese fallo a un defecto funcional del proyecto.
- **Código/Ejecución:** el proyecto anterior no registraba pruebas en CTest.

## Base reorganizada

| Comprobación | Estado |
| --- | --- |
| Configuración y compilación Release desde cero | Pasó |
| CTest Release: `metadata.tagging` | Pasó |
| Configuración y compilación Debug | Pasó fuera del sandbox; el primer intento dio C1902 en MSVC/PDB |
| CTest Debug | Pasó |
| Paquete portable e imports PE | Pasó; EXE y DLLs app-local inspeccionados; ZIP sin datos personales |
| Compilación del instalador | Pasó con Inno Setup 6.7.3 |
| Instalación, reinstalación y desinstalación QA | Pasó con AppId independiente, CURRENTUSER, sin iconos/asociaciones; preservó sesión existente |
| Checkout limpio y documentación | Pendiente |
| Prueba visual de modos/compacto/opciones | Pasó el recorrido descrito abajo, sobre el portable extraído fuera del repositorio |
| IPC | Fallo previo reproducido también con el binario anterior; ver abajo |
| Datos persistidos sobre copias | Sesión de cuatro líneas, settings y guardado/recarga KBRD comprobados; sin favoritos/tableros históricos disponibles |

## Recorridos y evidencia

- Windows 11, MSVC 19.43.34809, SDK 10.0.22621.0; generador Visual Studio 17 2022 x64.
- Release/Debug: compilar app y harness, ejecutar `metadata.tagging` con código 0.
  Un único test CTest ejecuta el conjunto de casos del harness, no solo una assertion.
- Portable temporal: restauró carpeta mediante sesión; mostró PNG con transparencia,
  biblioteca, miniaturas y preview de hover; envió la imagen al tablero y guardó KBRD.
  Al volver al tablero cargó el guardado e incorporó la imagen activa.
- Cascada: cuadrícula, clic en JPEG, visor modal y clic externo para regresar.
  F11 alternó fullscreen; opciones se abrieron y el botón RU cargó textos rusos.
  NUM1 regresó a Viewer; arriba y rueda cambiaron el zoom y apareció la tira inferior.
- No se probó gráficamente la región Ctrl+arrastre, toda la edición de nodos/enlaces,
  todos los menús ni todos los gestos; sus contratos se documentaron mediante código.
- IPC: con la instancia activa se abrió un cliente temporal con BMP y luego PNG;
  ambos clientes terminaron en 0, pero la imagen no cambió. La misma prueba con el
  EXE conservado del build previo tampoco entregó el JPEG a su instancia inicial PNG.
  No se cambió lógica IPC para resolverlo. El exit code del cliente no prueba entrega.
- Instalador QA: compilado con `MyAppId=LenteUV-Foundation-QA-20261021`, instalado en
  TEMP con `/CURRENTUSER /NOICONS /TASKS=""`, reinstalado tras escribir una sesión
  marcadora y desinstalado. La marca sobrevivió; el EXE se retiró. No se instaló el
  AppId productivo sobre una instalación real.
- ZIP revisado: cinco catálogos `.po`, runtime x64, EXE/ICO/LICENSE/README y defaults
  sin BOM; ninguna sesión personal, caché, log, favorito, tablero o muestra incorporada.
- Las fuentes trasladadas coinciden por hash Git con el commit de preservación,
  excepto el comentario de threading corregido en `image_loader.h`.

## Preservación

El commit `e2abe3d` conserva los cambios locales anteriores sin incluir datos de
ejecución. El respaldo recuperable local está en `build/backups/pre-foundation/`,
ignorado por Git: HEAD anterior, árbol preservado, patch, datos y planes locales.
La primera copia TEMP del sandbox no persistió; el respaldo de workspace se reconstruyó
desde esos commits y los datos intactos, y se verificó la copia de datos por hash.
No se borraron los worktrees ni el estado local de `.kilo`.

## Límites

No se ha certificado rendimiento con miles de archivos, todos los códecs, todos los
monitores/DPI, pérdida de GPU ni instalación en versiones antiguas de Windows.
Las pruebas del harness no verifican rendering. La validación de imports PE no
equivale a iniciar el programa ni a instalar/desinstalar el paquete.

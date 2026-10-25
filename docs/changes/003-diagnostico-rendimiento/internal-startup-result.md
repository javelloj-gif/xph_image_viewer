# Comprobación del solapamiento interno — 2026-10-24

**Resultado: sin mejora acreditada en aperturas posteriores. 003 permanece abierta.**

## Corrección de contexto tras el reporte del propietario

El propietario indicó que Windows acababa de reiniciarse. La consulta posterior
confirmó LastBootUpTime 2026-10-24 17:10:30, zona SA Pacific Standard Time.
El harness nativo se ejecutó hacia 17:16; después, el primer CSV anterior se escribió
a 17:16:40 y el nuevo a 17:16:42. **No se comprobó el boot antes de iniciar las pruebas**;
el harness ya había utilizado D3D/WIC. Esta comprobación no conservó una primera
ejecución limpia del producto nuevo. La captura ETW histórica pertenece a otro boot.

El par de acondicionamiento, excluido de las medianas, muestra diferencias grandes:

| Primer par de esta serie (ms) | Anterior, ejecutado primero | Nuevo, ejecutado después |
| --- | ---: | ---: |
| Invocación → envío del original | 1588,16 | 1263,18 |
| Entrada → envío del original | 554,59 | 309,91 |
| Enumeración completa no cancelada | 186,41 | 10,01 |

El anterior también tuvo un escaneo cancelado de 21,35 ms. El nuevo pudo beneficiarse
de las lecturas previas del anterior y ambos tuvieron costes previos a main.entry
cercanos a un segundo, sin atribución causal ETW en esta serie. **La diferencia de
este par no se puede atribuir al cambio de código.** Las ocho parejas posteriores
siguen describiendo aperturas preparadas; no prueban ausencia de beneficio en frío.
Hace falta otra sesión para medir primero el candidato nuevo sin harness previo;
una comparación fría anterior/nuevo requiere sesiones separadas comparables.

## Identidad y método

Fuentes locales sobre `codex/003-arranque-cache`, HEAD `76dc06c`, sin commit nuevo.
Anterior: SHA256 `A8C54E3B42B5811EA17CCF347AA91A7FFF4C2808BF19EE45BD4E6C7F0AF04213`.
Nuevo: SHA256 `874F5783B219E24FE1AD74B330BBF9A344EF2DAB0E3664738CED579CF5FF50D6`.
El candidato anterior de dist no se reemplazó; tampoco se creó un paquete nuevo.

`scripts/compare_startup_folder.ps1`: ocho pares, orden alternado anterior/nuevo,
más un par de acondicionamiento excluido. Procesos cerrados entre aperturas,
`--multiple`, entrada de carpeta explícita, fullscreen, mismos defaults, sesión con
esa carpeta y logging desactivado. Cada versión usa su propio staging, user_data y
caché nuevos bajo build; no se purgó caché del SO. Entrada solicitada por el propietario
en solo lectura; ninguna acción de edición, borrado o escritura de metadatos sobre
multimedia. No se hizo una auditoría ETW ni hash completo de originales en esta serie.

Artefactos privados locales: `build/startup-internal-003-comparison/`, CSV por
proceso y `results.json`; resumen en `build/startup-internal-003-comparison.log`.
Dieciocho procesos terminaron con código 0 y sin eventos QPC perdidos.
La presentación significa envío del original completo tras Present1 correcto;
no certifica aparición física en pantalla. Son aperturas posteriores, no primera
ejecución tras reinicio ni repetición de la captura única anterior.

## Resultado medido

| Métrica (ms) | Anterior | Nuevo |
| --- | ---: | ---: |
| Entrada de aplicación → envío del original, mediana | 327,98 | 334,69 |
| Mismo intervalo, mínimo–máximo | 319,14–351,66 | 317,00–362,31 |
| Invocación → envío del original, mediana | 338,73 | 347,08 |
| Inicialización del renderer, mediana | 204,04 | 204,22 |
| Decodificación original, mediana | 56,55 | 57,40 |
| Primera subida GPU, mediana | 17,02 | 17,30 |

El nuevo tiene una mediana 6,72 ms mayor (+2,05 %). Con n=8 y rangos solapados
no se atribuye una regresión estadísticamente demostrada ni una mejora.
Se confirma por trazas un único escaneo nuevo frente a dos inicios anteriores
(uno cancelado). La enumeración completa preparada ronda 9–12 ms: se solapa con
el renderer, pero su coste en este entorno es pequeño frente a las otras fases.
No usar el primer escaneo cancelado de ~0,3 ms como duración de enumeración completa.

La nueva instrumentación ubica **199,03 ms de mediana en D3D11CreateDevice**, dentro
de los 204,22 ms del renderer (~97,5 %). D2D ronda 2,6–2,8 ms, DWrite <0,1 ms,
swap chain 1,5–1,7 ms y composición 0,7–0,9 ms. Esto localiza el coste en la llamada
de creación del dispositivo; no determina su causa interna en driver/Windows.
La decodificación de la imagen seleccionada sigue comenzando después del renderer:
el snapshot se publica al drenar en UI. El solapamiento aplicado no cubre ese trabajo.

## Regresiones y revisión

- Release y CTest: 8/8 correctos, 17,34 s, log
  `build/startup-internal-003-build.log` (ejecutados antes de esta comprobación).
- Arranque nativo: diez escenarios correctos, salida 0, log
  `build/startup-internal-003-native.log`. Incluye archivo/carpeta/sesión/cwd,
  bypass, IPC durante inicio, fallos WIC/HWND/D3D y cierre con trabajo pendiente.
  Los resultados Failed de las inyecciones son los esperados y pasan sus asserts.
- La primera invocación del script de arranque apuntó a un nombre de EXE inexistente:
  falló al copiar, sin iniciar aplicación. Se corrigió a `async_preview_tests.exe`.
- Revisión local del diff: App::Initialize, LoadSession, contratos de entrega y
  teardown, marcadores de renderer y regresiones. Sin defecto bloqueante confirmado
  en ese alcance; no equivale a revisión independiente ni certificación visual universal.

## Estado y siguiente intervención

Se verificó el cambio, pero no se acepta como solución del rendimiento inicial.
La ganancia potencial en frío del escaneo solapado sigue sin medir. Los 789,46 ms
son una resta hipotética de ETW, no se han eliminado ni se deben comparar directamente
con las medianas de esta serie preparada. Defender no se modificó.

El siguiente diseño debe estudiar la creación D3D y el solapamiento de la
decodificación CPU seleccionada, conservando recursos GPU/estado UI en su hilo y
latest-wins/cancelación. Aún no se implementó esa intervención. Confirmar el arranque
real del nuevo candidato tras reinicio seguirá requiriendo otra sesión de Windows.

# Resultado: 008 Formatos de texto

Seguimiento independiente: [009](../009-decoder-compartido/result.md) implementa
decoder compartido para carga del visor. La evidencia/cierre de 008 se mantiene como
registro histórico; no se elimina su limitación de baseline ni se cambia Renderer.

[Spec](spec.md), [plan](plan.md), [revisión](review.md). Fecha: 2026-10-22.
Estado: completado en alcance. Base ac4a5da más trabajo previo preservado sin commit
en codex/repository-foundation. Contrato/aceptación y recuperación especificados antes
de modificar producción; protocolo visual ampliado con controles tras discrepancia.

## Implementación

DrawText y MeasureText usan GetInternalTextFormat privado: familia propia exacta,
tamaño, alineaciones efectivas y factory. Colección null, peso regular, estilo/stretch
normal, en-us y demás defaults permanecen constantes; si se amplía API privada,
ampliar clave. API pública CreateTextFormat sigue creando objetos mutables nuevos,
incluidas sus variantes weight/style. No se publica ningún formato interno.

Máximo 32 entradas FIFO, nombres de familia retenidos de hasta 128 wchar_t, sin truncar
otros inputs. Tamaño inválido/nombre demasiado largo no se cachean; fallos tampoco.
Objetos internos no se mutan después de publicar. ComPtr local mantiene uso válido
aunque se expulse otra entrada. No se cachean layouts, texto ni medidas.

ClearTextFormatCache libera formatos/identidad y reinicia FIFO antes de crear recursos
y al descartarlos. Lookup también invalida si se reemplaza factory; una referencia
mantiene identidad viva evitando confusión por dirección reutilizada. Vector conserva
su capacidad fija al limpiar; se destruye con Renderer. Todo UI/render, sin locks.

## Evidencia de aceptación

Windows x64, Visual Studio 2022, Release, DirectWrite/WARP del sistema. Datos sintéticos
y artefactos ignorados bajo build/optimization-008; sin ventana visible, swapchain,
datos personales ni cambios de asociaciones. Snapshot de tres fuentes previo al cambio.

| Criterio | Resultado |
| --- | --- |
| AC-01 | Pasó: 108 combinaciones de familia/tamaño/alineación, equivalencia de propiedades (familia, locale, peso/estilo/stretch, wrapping, dirección, tab, spacing, trimming y colección), hits y separación de claves. 48 medidas con Unicode/espacios/líneas/tab comparadas bit a bit. |
| AC-01 propiedad/límites | Pasó: formatos públicos distintos y mutación aislada; weight/style públicos intactos; enum desconocido conserva defaults; FIFO/capacidad bajo churn; nombres 128 retenidos y 129 no insertados sin truncar; tamaños inválidos con resultado original y sin inserción; factory aislada reemplazada y Discard limpia estado. Null-family revisado en código, no prueba dinámica de ese input inválido. |
| AC-02 | Pasó en contraste controlado: 12 frames de texto, 3 familias/9 alineaciones, tamaños fraccionarios, Unicode/CJK/emoji, espacios/líneas, alfa, DPI 96/144, anchos 320/801 y churn de 40 tamaños. Cero bytes distintos contra oráculo fresco y renderer.cpp original recompilado. Discrepancia inicial registrada debajo. Los 12/24 frames de 006/007 conservan hashes. |
| AC-03 | Pasó: cmake --build --preset release; ctest --preset release 4/4 en 10,91 s, 161701 checks totales del harness ampliado. Diff específico/revisión/comentarios y git diff --check correctos. |
| AC-04 | Pasó: comparación local de obtención/configuración y MeasureText completo, debajo. Retención CPU acotada declarada, sin ahorro global atribuido. |

### Control de la discrepancia inicial

Antes de modificar producción, before.bgra dio SHA256
`3DF622F0A510CF9D42176781E91681E18A7D54534912F2D63A83CEDA9F4CA524`.
La primera candidata difirió en 48 bytes de 27 píxeles repartidos en cuatro frames
del caso Unicode de Segoe UI (dos por DPI); diferencia máxima de canal 79.
No se aceptó por tolerancia.

Un oráculo de dibujo/medida con formato público fresco dio exactamente los bytes de
la candidata. Se recompiló renderer.cpp previo desde snapshot, manteniendo el header
actualizado del harness/candidata, y el control sin caché dio también ese hash. Se
confirmó recompilación efectiva del .cpp, y se restauró candidata por hash en finally.
El contraste final completamente fresco y la candidata coinciden. No se confirma
causa de la variación inicial; no se atribuye automáticamente a fuente/caché/driver.
No se cambió tipografía, geometría ni antialiasing para hacer pasar la comparación.

SHA256 de reference-final.bgra, final.bgra y original-control.bgra:
`4F3CEA37ECED09397EB7D62C4E8DA6AD1DEC766F241BDAEFBE29DAF17B87D6DE`.
006 preservado: `64E006D5427E0F17A04BDA1CAED169C845F5AC985457C9E1D2D3EFC12207D88B`.
007 preservado: `0890DFBA56E4AB5CF488D85B023E8BFA2AA4DBD5C409189F19A66463E8B86652`.
EXE Release final: `AC5B43789AEBAD29D0AA65F646C45D04514104782E8E4FB37594352DFE118E51`.
Los artefactos iniciales y controles se conservan localmente, no se versionan.

## Medición local

`--text-benchmark`, mismo proceso Release, familia Segoe UI, conjunto caliente de
cuatro tamaños. Seis rondas alternando orden, una descartada, 1000 llamadas por lado
y mediana de las cinco restantes. Consumo de resultados; sin umbral temporal de test.
Formato fresco incluye alineación como antes; MeasureText fresco conserva oráculo
previo y checks de HRESULT. No dibujo/Present/IO ni arranque completos en la medición.

| Operación | Fresco µs/llamada | Reutilizado µs/llamada |
| --- | ---: | ---: |
| Obtener/configurar formato | 0,6590 | 0,0157 |
| MeasureText completo | 16,3569 | 15,7902 |

Se evita crear formatos repetidos en ese conjunto. MeasureText sigue creando layouts
y copiando texto, por eso el ahorro total allí es menor. No demuestra reducción de
CPU/GPU/reposo/arranque completos, working set o memoria privada. Retener hasta 32
formatos y una identidad de factory consume memoria CPU adicional acotada; no se
cuantifican bytes internos de DirectWrite ni picos. Churn/misses no son ese hot set.

## Límites y continuidad

Sin criterios necesarios pendientes dentro del alcance controlado. Igualdad visual
probada en casos ejecutados, no toda UX, compositor/driver físico ni recuperación real
de dispositivo. Ciclo real de recreación revisado por código; reemplazo de factory y
discard ejercitados por tests. La discrepancia inicial de captura se conserva como
limitación del baseline, sin defecto persistente exclusivo de caché demostrado.

Actualizados roadmap, índice, desarrollo, contexto y seguimiento 003/007. No se cierran
primera ejecución/caso representativo de 003, AR-005/006/007, AR-008 ni decisiones Library.
Siguiente recomendado: especificar decoder único OPT-004; última medida del overlay,
índice masonry, inicialización secundaria, uploads y scheduler siguen pendientes.
No ACK/elección atómica/cierre ni cambios de formato persistido. Sin commits.

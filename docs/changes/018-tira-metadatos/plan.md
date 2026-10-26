# Plan: 018 — Tira y metadatos

Especificación: [spec.md](spec.md). Preparado el 2026-10-26.
Estado: T-01–T-08 completadas en alcance el 2026-10-26; [evidencia y límites](result.md).
La petición «procede secuencialmente» autorizó confirmar 017 y preparar este plan;
el «Procede» posterior autorizó implementar 018.
No hay código funcional de 018 mezclado con el commit 017.

## Base y componentes

Rama actual codex/015-roadmap-design; HEAD 2d19d3d (017). Permanecen cambios locales
de documentación 015/016/lote, índice, contexto y roadmap, así como debug_ipc.txt
eliminado. Preservarlos sin limpieza, reset ni cambios de rama incidentales.

ThumbnailStrip posee previews, selección y scroll. App posee imagen/índice y el
orden de entradas. ViewerPage dibuja metadatos con GetDisplayZoom. Reutilizar estas
responsabilidades y friend. Los helpers de geometría residirán en ThumbnailStrip;
ViewerPage consulta su envolvente al posicionar la barra. No duplicar el modelo.

Propuesta técnica: cálculo determinista de rectángulos a partir de tamaño base,
índice actual, ancho/alto cliente y scroll. Solo una miniatura cambia de ancho,
por lo que sus posiciones pueden calcularse sin recorrer todas las anteriores.
Conservar búsqueda acotada de visibles sobre rectángulos monótonos y probarla
contra recorrido completo. La misma geometría debe resolver índice/posición de
inserción y límites; eliminar divisiones por ancho uniforme de los consumidores.
No capturar punteros a entradas de previews ni sustituir sus tokens/leases.

## Tareas

- [x] T-01 / investigación: leer 016/lote/017, Settings, barra activa de Viewer,
  tira, consumidores y regresiones. Identificados HEIGHT fijo, ampliación basada en
  m_selected, índices por división y barra basada en constante/opacidad.
- [x] T-02 / AC-01,03,04: trazar unidades desde renderer/resize/eventos/DPI. Definir
  una geometría común (slots, activa, envolvente, clip, barra) y documentar propiedad/
  hilo en declaraciones. Probar 320/480/736/1024 y tamaños cliente reales, sin
  introducir una conversión global de toda la UI como efecto secundario.
- [x] T-03 / AC-01: cambiar thumbnail_width inicial a 44 (ajuste posterior del propietario) en Settings y packaging;
  alinear fallback THUMBNAIL_WIDTH. Ampliar settings.preferences para ausencia,
  archivo sin clave, preferencias existentes (incluidas 50/73/100), round trip y
  defaults nuevos. No modificar el parser ni imponer migración del valor guardado.
- [x] T-04 / AC-02: distinguir activa de m_selected/hover al dibujar; 130 % de base,
  borde cian 2 DIP, opacidad plena, sin label. Check de selección separado, con
  intersecciones/drag conservados. No deducir imagen actual solo del primer selected.
  Mantener crop/reflejos/placeholders y reutilizar exactamente las texturas actuales.
- [x] T-05 / AC-03: conectar GetThumbnailRect/GetVisibleRange y autohide/clip con la
  geometría nueva; adaptar SyncSelection, hover, down/up/wheel/doble clic, selección
  de caja, arrastre, inserción, centrado y límites. Verificar extremos inclusive,
  imagen activa fuera de pantalla, primera/última, listas vacías y duplicados.
  No permitir que ampliar selección consuma un doble clic destinado al visor fuera
  de la superficie visible. Mantener orden de overlays/radial/tira/páginas de 017.
- [x] T-06 / AC-04,05: posicionar ViewerPage::DrawOverlayInfo con envolvente real de
  tira visible y margen mínimo 12 DIP; acotar texto/posición y preservar sufijos
  de dimensiones/zoom/frame/índice. Acceso al nombre completo mediante tooltip
  acotado a cliente; evitar nombres duplicados. Mantener show_metadata/M, fade,
  editor T y composición actual Waterfall sin reservar tira inexistente de 019.
- [x] T-07 / AC-01..05: adaptar regresiones de visibilidad y snapshots solo por
  cambios deliberados de geometría; añadir asserts Release de tamaño/estado/entradas,
  scrollbar/centrado e intervalos diferenciales con fracciones y listas grandes.
  Preservar identidad, cancelación, prioridades y checks de carga/bitmap existentes.
- [x] T-08 / AC-05,06: revisar diff con habilidad local, Release/CTest, snapshots y
  recorridos nativos; medir solo candidato de forma proporcional. Corregir hallazgos,
  actualizar spec/plan/result, usage/project-context/development y roadmap con
  evidencia exacta. No cerrar un criterio necesario pendiente ni generar dist.

## Validación y evidencia

`cmake --preset windows-x64`, `cmake --build --preset release`,
`ctest --preset release --output-on-failure`. No compilar la base histórica ni
repetir sus medidas: se conserva la instrucción expresa del propietario. Datos y
staging nuevos en build/session-018; fixtures copiados y EXE/user_data propios,
resolución comprobada antes de escribir. No pruebas sobre preferencias personales.

Matriz geométrica: base 30/44/50/73/90/100 con activa 130 %, índices 0/medio/último y
sin activa, listas 0/1/varios/miles, scroll entero/fraccional, bordes exactos y
fallos de tamaño inválido con fallback seguro. Comparar candidatos visibles contra
rectángulos completos, no solo el texto de la implementación. Cambiar hover o
selección no cambia identidad/área de la activa. Rango de destino coherente con
scroll actual y nuevas alturas durante animación; sin decode adicional por estética.

Recorridos nativos: compacto/fullscreen a DPI disponible y otro DPI cuando sea
posible, nombre largo/Unicode, navegación rápida/Inicio/Fin, primera/última, scroll
manual, selección de caja, drag/reordenación, clic/doble clic en activa ampliada,
overlays de opciones/T, M, zoom/rotación, PNG/JPEG/GIF y cierre con carga pendiente.
Modal Waterfall debe seguir funcionando sin anticipar 019. Registrar exactamente
DPI/ventanas/escenarios observados y los que no se pudieron cubrir; WARP/snapshots
no demuestran percepción física ni todos los frames.

Referencia nueva conservada de 017: SHA256
3021B349F6BFA5223C3C7E89D506A88DD97041EA9CD04ECF17F89ABDCA207FEB,
serie solicitada tras liberar recursos, n=8, 356,13 ms, rango 344,40–375,32,
[fuente y condiciones](../017-simplificacion-funcional/result.md).
No mezclar con las series previas bajo otra carga de fondo. La referencia final
003, hash 874F5783…FF50D6 y sus informes enlazados en 017, se conserva como
histórica; no lanzar otra vez ninguno de esos binarios para medir una base.

Para AC-06 usar solo ejecutor individual del candidato, mismos inputs/settings y
protocolo cuando estén disponibles; registrar diferencias de configuración (mediciones iniciales 90
frente a 50; ajuste posterior a 44 sin nuevas medidas) en vez de atribuirles causalidad. Para tira, mantener
series de funcionamiento/carga/layout proporcionadas y separar cache preparada de
primera generación. Si no existe serie de rendimiento de 017 para una métrica,
registrar ese límite; no rellenarlo con cifras de otro hash ni reejecutar la base.
No medir con builds/tests simultáneos. Empeoramiento repetido fuera de variación
conservada bloquea; confirmar solo el caso del candidato afectado, sin tolerancias
porcentuales inventadas ni más campañas generales por inercia.

## Riesgos y recuperación

Los índices de navegación pueden anticipar la entrega del original; seguir ese
camino antes de elegir la identidad visual para no marcar una imagen todavía no
presentada como si ya estuviera abierta. Preferir la identidad actual disponible
sin alterar generaciones/latest-wins. Duplicados requieren índice de ocurrencia
coherente, no búsqueda que marque todas las rutas iguales.

Mayor geometría puede ampliar número de previews visibles y recorte sin cambiar
su resolución; verificar cancelación de fuera de vista y prioridades de destino.
Evitar una caché de layout persistente adicional si un cálculo local O(1) basta.
Las adaptaciones DPI deben permanecer acotadas al componente y conservar contratos
existentes de mouse/renderer; una conversión global requiere otro alcance.

Recuperación: revertir exclusivamente el diff futuro de 018. No borrar user_data,
fixtures, staging histórico ni documentos previos. No cambia formatos ni claves;
thumbnail_width existente continúa cargándose como antes. Se conservan licencia y
atribuciones. No hay limpieza ni empaquetado implícitos.

## Estado para continuar

Default vigente ajustado a 44/57,2; ver ampliación final. La validación inicial
siguiente corresponde al candidato anterior de 90/117, no al ajuste posterior.

Implementación, Release/CTest 9/9, 3251 regresiones nuevas de tira, preferencias,
12 frames WARP, diez escenarios nativos de arranque y recorridos afectados
en los casos descritos en result. Arranque candidato n=8: 355,33 ms (343,51–367,34)
frente a referencia conservada de 017: 356,13 ms (344,40–375,32). No se ejecutó ni
compiló ninguna base. Métricas de tira nuevas sin referencia comparable no se
presentan como mejora ni certificación universal.

Ajustes de implementación: renderer/entradas conservan píxeles; conversión DIP local
por DPI de ventana. La identidad presentada se deriva en SyncSelection/Update;
ApplyLoadedImage sincroniza también su retorno temprano animado. Slots monótonos,
6 DIP entre vecinos, rectángulos O(1), búsquedas binarias y misma envolvente para
entrada/clip/barra. Reordenación por ocurrencias conserva duplicados. Nombre largo
con tooltip envuelto y scroll si no cabe; barra usa dos líneas cuando lo requiere
el cliente. El hover en tira/barra/tooltip evita desplazar el control durante lectura;
el fade continúa fuera de esas áreas. [Revisión local](review.md).

No quedan tareas de código 018 pendientes en este alcance. El diff está sin commit;
017 permanece en 2d19d3d. No iniciar 019–022 ni otra campaña de 003 automáticamente.
No se generó paquete ni se tocaron preferencias personales; 015/016 preservados.

## Ajuste de defaults tras prueba del propietario — 2026-10-26

Autorizado aproximar la captura: base 44 DIP y activa 57,2 (slider 20 % de
30–100). Cambiar solo Settings, packaging y fallback; adaptar expectativas de
default y añadir 44 a las matrices existentes, conservando 90 como preferencia
guardada válida. Actualizar guías/spec/result/roadmap; no modificar geometría,
proporción ni preferencias personales. Validación proporcional: build candidato y
suites afectadas de preferencias, tira y startup/cache. Sin benchmarks nuevos por
indicación expresa del propietario. La evidencia previa 90/117 conserva su fecha/hash.

Ajuste completado: Release correcto y 3/3 suites afectadas (4,28 s),
48 checks de preferencias, 3899 checks de presentación dentro de 96109 de previews,
y 100 de startup/cache. Evidencia en result. Sin nuevas medidas de rendimiento.

Ajuste visual posterior: fuente original 13 y gap 3 DIP implementados y comprobados junto con [020](../020-navegacion-superior/result.md); no nueva campaña de rendimiento.

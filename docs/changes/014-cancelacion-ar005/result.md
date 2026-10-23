# Resultado 014: AR-005 completado

Fecha: 2026-10-23. **Completado.** Base 6f84dc5, implementación y pruebas en
codex/repository-foundation. [Spec](spec.md), [plan](plan.md), [revisión](review.md).
La entrega implementa los cuatro faltantes; no los sustituye por una nueva
disposición documental. Lista para los siguientes cambios funcionales del usuario,
con el reporte IPC histórico y su actualización de estado registrados a continuación.

## Resultado manual histórico — 2026-10-23

El propietario realizó pruebas del portable 014 y reportó fallo en el punto 6:
abrir otra imagen mediante el mismo ejecutable mantiene la imagen anterior.
**IPC-014-01**, [reporte y decisión](user-validation.md), queda diferido por
instrucción expresa del propietario, que lo identifica como error legado.
Sin reproducción adicional ni causa confirmada; no se modifica código ni paquete.
No se declara aceptación funcional completa de IPC ni se infieren resultados de
los demás puntos. El cierre de implementación AR-005 y las pruebas históricas
inferiores conservan su alcance; no invalidan ni resuelven este reporte manual.

**Actualización posterior del mismo día:** al aceptar el candidato de 002, el
propietario confirma pruebas funcionales exitosas e IPC operativo: abrir una nueva
imagen desde Windows actualiza correctamente el visor. IPC-014-01 se cierra por
ese reporte actual, sin reproducción, diagnóstico causal ni corrección en esta
sesión. No se atribuye el éxito al binario histórico de 014 ni se certifican todos
los escenarios IPC. Detalle y límites en [user-validation.md](user-validation.md).

## Matriz de cierre

| Criterio | Evidencia inspeccionada |
| --- | --- |
| AC-01 | Leases débiles en loader y todos los consumidores; poda de cola sin decode. 42 checks específicos: retirada/reintento, retención independiente, previews, carpeta/modo, hover real y fallback, sort/delete, Board con rutas duplicadas y entregas ya pendientes. Las 162025 comprobaciones anteriores se conservan |
| AC-02 | Stop global/por lease antes/después de etapas WIC y entre frames/filas; no resultados parciales. Proxy retiene un decoder abierto real: Shutdown espera 47 ms y no pide el siguiente frame. Cancelación por consumidor dentro de open/GetFrame y liberación de decoder comprobadas. BMP 4096×4096 real: 67108864 bytes, 32 ms en esta ejecución. GIF de 100 frames, WebP pequeño y fallos/corrupto/ausente conservados; 314 checks WIC |
| AC-03 | Board con progreso atómico y cancelación por entrada/archivo/tag/chunk; catálogo en temporal CREATE_NEW y reemplazo completo. Pruebas conservan catálogo anterior al cancelar, reinician, rechazan publicación tras stop y conservan bytes ante reemplazo fallido, sin temporales residuales. Búsqueda se cancela con llamada WIC retenida sin match ni entrada nueva de caché; callback anidado y cache-hit solicitan cancelación sin deadlock. finished una vez y join antes de HWND/dependencias |
| AC-04 | Cliente overlapped con presupuesto compartido y sin FlushFileBuffers. 13 hijos con watchdog, incluidos cuatro nuevos: no lector, desconexión durante write, lector tardío y conexión ocupada 200 ms seguida de escritura bajo 300 ms totales. Errores/entrega/concurrencia 005 conservados |
| AC-05 | Configure y Release completos; CTest final **7/7**, 14,11 s. Diez escenarios nativos pasaron con cuatro modos/fallos parciales/hide-to-tray/cierre. Revisión de contratos, comentarios y diff sin hallazgos bloqueantes |
| AC-06 | ZIP final desde staging nuevo, EXE extraído y hashes/defaults comprobados. Copia del producto empaquetado arrancó con --multiple y --lite, recibió cambios de los cuatro modos y cerró normalmente con código 0. Documentación/roadmap y cambios guardados en Git |

Reproducción automatizada desde raíz:

```powershell
cmake --preset windows-x64
cmake --build --preset release
ctest --preset release --output-on-failure
./scripts/run_ipc_diagnostics.ps1 -Executable build/windows-x64/Release/async_preview_tests.exe -Fixtures tests/fixtures -RunRoot build/cancellation-014 -Startup
```

CTest añade lifecycle.cancellation mediante wrapper que copia EXE y PNG a un root
único con user_data propio. lifecycle.shutdown pasa sus trece hijos con fin real de
proceso; el watchdog solo termina hijos propios si fallan. En ejecución directa de
los casos de cliente: no lector, 157 ms/ERROR_TIMEOUT; conexión+write, 312 ms/ERROR_TIMEOUT
con 300 ms solicitados (resolución/scheduling y completion de cancelación). El gate
del segundo es <450 ms; no se concede un segundo presupuesto completo al escribir.

El control de lectura de metadatos inicialmente solo cubría filename y no alcanzaba
el bloqueo esperado: se corrigió el proxy para stream. La regresión sort con App
sin enlazar detectó una preparación incompleta del harness; el test específico
ahora inicializa el gestor con su App real. Ambos fallos quedaron resueltos, sin
omitir casos ni relajar condiciones para obtener el pase.

## Ejecutable temporal final

- EXE listo: `dist/lenteuv_qa_014_20261023_final/lenteuv.exe`
- ZIP: `dist/lenteuv_qa_014_20261023_final.zip`
- SHA256 EXE: `CAD27F77B398B12661D5291B3C803A8C553975FEC0B08D3E8C6DAA92BD501CA7`
- SHA256 ZIP: `FFCA519E2AB863C11191B722AEA68F19CDD8916D7D8360DC660E689A04D01416`
- Staging: `build/staging/bff78b8bdcbb43b385f7589609fd1bbd`
- Smoke sobre copia extraída: `build/cancellation-014/package-final-75c553fefc42490a89a113daf1d371a2`
- Smoke nativo final: `build/cancellation-014/ipc-7a9eceae18bd42099f98aa85e18296d4`

Paquete creado con create_portable_zip.ps1 -SkipBuild -OutputName
lenteuv_qa_014_20261023_final. Contiene runtime VC++ x64 (incluido atomic_wait),
locales, LICENSE/atribuciones y defaults; imports validados por el script.
El settings extraído coincide byte a byte con packaging/default-settings.json
(SHA256 CC1E8C0B4C2C2FC2EFB013CC4F92EB04834BF915198E1F64310A66143CFEE820),
session.txt vacío, sin logs/caché personal. El hash del EXE coincide con el build.
El smoke usa otra copia y desactiva fullscreen solo en sus settings para pruebas;
el candidato final conserva los defaults. No se instala ni cambia asociaciones.
010–013 y el candidato preliminar 014 se preservan; usar exclusivamente el sufijo
**final** para esta entrega. Artefactos/datos permanecen ignorados y fuera del commit.

[Protocolo funcional](functional-test.md) para el propietario y
[continuidad](resume.md). La prueba manual posterior está registrada en
[user-validation.md](user-validation.md), con excepción IPC diferida; no se le
atribuye equivalencia visual universal ni aprobación de puntos no detallados.

## Límites del contrato completado

La cancelación se comprueba entre operaciones: una llamada WIC/E/S en ejecución
debe retornar antes de unir su hilo. No hay plazo universal para un códec o driver
atascado, no se fuerza/detacha ningún hilo y no se libera su almacenamiento en uso.
Las cargas síncronas del visor siguen bloqueando al llamador. Estos límites se
caracterizaron; no son faltantes de implementación diferidos de AR-005.

CancelSearch mantiene una notificación finished, incluso cancelado; un callback ya
admitido puede completar antes del retorno del join. El dueño no destruye/reinicia
scanner desde callbacks. Cancelar un escaneo conserva las entradas de caché válidas
ya obtenidas; no publica un catálogo parcial. No se alteraron KTAG/KBRD, rutas,
enums, atajos, calidad ni elección/ACK IPC. No se afirma mejora global de rendimiento.
APNG del entorno sigue exponiendo un frame y WebP de 108 MB estimados conserva el
límite de prueba previo; no se certifican códecs universales con esta entrega.

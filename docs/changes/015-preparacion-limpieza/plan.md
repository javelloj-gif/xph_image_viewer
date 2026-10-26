# Plan: 015

Especificación: [spec.md](spec.md). Actualización: 2026-10-25.

- [x] Revisar Git, guías, dist y naturaleza de build; identificar repositorios anidados.
- [x] Registrar mapeo, criterios y compatibilidad provisional de marca.
- [x] Registrar inventario absoluto y hashes locales de datos y entregas.
- [x] Retirar intermediarios regenerables, ZIP antiguo y contenedor dist vacío.
- [x] Verificar preservación, revisar diff y completar result.md.

Preservar EXE/DLL de referencia, trazas, informes, scripts locales de diagnóstico,
backups, árboles de datos y checkouts con .git. Eliminar solo intermediarios de
compilación conocidos y rutas enumeradas; abortar ante enlaces o rutas fuera de raíz.
No ejecutar limpiezas generales de Git ni borrar build completo.

Estado: completado; manifest local ignorado en build/session-015. Siguiente paso:
elección visual de 016, luego planes funcionales según el roadmap.

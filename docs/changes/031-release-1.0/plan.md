# Plan de entrega 1.0

- [x] Inventario Git, entregas, datos locales y repositorios anidados; origin admite avance directo.
- [x] Unificar versión CMake/Windows sin modificar identidades persistidas.
- [x] Compilar Release y ejecutar CTest: 10/10.
- [x] Crear portable/instalador 1.0.0 y verificar hashes/versiones/imports.
- [x] Preservar datos únicos, retirar worktree limpio por Git y limpiar rutas absolutas comprobadas.
- [x] Preparar documentación y entrega versionada para commit/etiqueta v1.0.0.

Publicación: avanzar las ramas codex/015-roadmap-design y codex/repository-foundation
de origin junto con v1.0.0, sin force. La verificación remota se realiza tras el
commit con git ls-remote; la etiqueta identifica la entrega y la respuesta de cierre
registra el commit confirmado. Si falla el envío, no declarar publicada la versión.

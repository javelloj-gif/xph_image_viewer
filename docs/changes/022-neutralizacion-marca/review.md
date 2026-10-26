# Revisión 022

Habilidad local lenteuv-architecture-review. Cobertura: títulos/About/errores, CMake/recursos/manifiesto, icono neutral/generador, scripts genéricos y packaging Inno/ZIP; aliases de PerformanceTrace, inventario de compatibilidad, guards de harness y atribuciones. No se migra ni borra user_data; staging procede de defaults limpios.

AR-022-001 (defecto medio, reproducido por regresión): alias podía activarse para valor neutral vacío por GetLastError heredado. Se limpia el error antes de GetEnvironmentVariableW y solo se usa alias si falta la variable. Prueba diferenciada de ausencia, prioridad neutral y vacío explícito.

Conservados explícitamente pipe, clase HWND, identidad de manifiesto, claves Run/shell, AppId y directorio instalado; bibliotecas del sistema, enums y formatos intactos. Símbolos nuevos neutrales. Referencias restantes están inventariadas, incluidas herramientas históricas 003 e ID de habilidad instalado. Se retiraron enlaces oleacc redundantes durante revisión.

Sin hallazgos accionables restantes en el alcance; no certifica actualización/desinstalación sobre instalaciones reales previas ni auditoría integral de subsistemas no modificados. Las comprobaciones de paquete y recursos no equivalen a UX/rendimiento: ver result.md.

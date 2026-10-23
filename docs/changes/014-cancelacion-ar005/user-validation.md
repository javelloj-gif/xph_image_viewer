# Validación funcional reportada del portable 014

Fecha del reporte: 2026-10-23. Fuente: propietario, pruebas manuales del portable
014. Esta actualización solo registra evidencia y decisión; no reproduce ni
investiga el fallo, no modifica fuentes y no genera otro ejecutable.

## IPC-014-01: abrir otra imagen mantiene la anterior

**Estado vigente, reporte posterior del 2026-10-23:** cerrado por confirmación del
propietario de funcionamiento actual. Informa que IPC opera correctamente y que
el visor se actualiza al abrir una nueva imagen desde Windows, al confirmar las
pruebas funcionales exitosas del candidato de [002](../002-retirar-library/result.md).
El reporte no aporta matriz, hash, rutas ni logs. No se investigó, reprodujo ni
corrigió en esta sesión; no se conoce la causa del fallo anterior ni se atribuye
su desaparición a la retirada de Library. Esta aceptación cubre el escenario
reportado, sin certificar todos los casos IPC ni reinterpretar el portable 014.

### Reporte original y decisión histórica

- **Escenario:** punto 6 del [protocolo funcional](functional-test.md): abrir una
  nueva imagen mediante el mismo ejecutable con el visor ya abierto.
- **Esperado:** el visor muestra la nueva imagen.
- **Observado por el propietario:** continúa mostrando la imagen anterior; la
  nueva no se visualiza.
- **Clasificación:** fallo funcional reportado. El propietario lo identifica como
  error legado de la base anterior; no se ha contrastado aquí contra un binario
  anterior ni se ha establecido su causa o etapa de fallo.
- **Estado original:** diferido expresamente por el propietario. No investigar, reproducir
  ni corregir hasta que lo solicite; no bloquea por su decisión el siguiente trabajo
  funcional ni reabre automáticamente la entrega de cancelación AR-005.

El reporte identifica el portable 014, pero no aporta hash, formatos, rutas,
parámetros de lanzamiento ni logs. El candidato final y sus hashes siguen
documentados en [resultado](result.md); no se afirma haber verificado la identidad
exacta del binario probado por el usuario. No se solicitan datos adicionales ahora
porque se ha pospuesto el tratamiento del hallazgo.

Se han realizado pruebas funcionales, con fallo explícito en el punto 6. No se
dispone de resultados individuales de los demás puntos y no se marcan como
aprobados por inferencia. En aquel reporte no había aceptación funcional completa
de IPC; el estado actual del escenario consta al principio de este documento.

## Relación con la evidencia anterior

Se conservan Release/CTest 7/7, matrices aisladas y smoke del paquete de 014 como
evidencia histórica de sus escenarios. No certifican el escenario manual fallido.
El smoke del producto con --multiple/--lite omite la ruta de instancia única; sus
cuatro modos y cierre correctos no prueban reenvío IPC entre instancias.
Las aplicaciones controladas de 005/003 tampoco descartan este reporte.

AR-003 conserva la corrección histórica de transporte/error/propiedad en 005;
IPC-014-01 queda registrado como fallo de apertura visible pendiente de diagnóstico,
sin atribuirlo al transporte, al callback o al renderer. AR-005 conserva su cierre
de implementación de cancelación/cierre y presupuesto IPC; no equivale a declarar
libre de errores toda la funcionalidad IPC.

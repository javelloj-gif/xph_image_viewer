# 013: Cierre de la refactorización base

Fecha: 2026-10-23. Petición: cerrar los pendientes de la base, guardar el trabajo
y preparar continuidad. El propietario reporta que probó el ejecutable temporal
y que todas las funcionalidades base permanecen intactas; no identifica hash ni
matriz detallada. Esa evidencia es aceptación funcional del usuario, no profiling.

## Alcance y aceptación

- AC-01: reconciliar 001 y 004–012, cerrar aceptación funcional R2/010/012 con
  atribución al reporte del usuario y conservar evidencia histórica.
- AC-02: resolver AR-006: leer KTAG v1 completamente antes de publicar; comprobar
  firma, versión, lecturas y longitudes contra bytes restantes antes de reservar.
  Un fallo devuelve false sin alterar caché ni dirty. Mantener rutas, formato,
  normalización, cadenas vacías, duplicados (último gana) y bytes finales tolerados.
  Sin límites arbitrarios nuevos a archivos válidos ni cambios al escritor.
- AC-03: probar válido/roundtrip, cada truncado y cantidades imposibles con datos
  sintéticos aislados; compilar Release y ejecutar CTest completo.
- AC-03b: unir productores Board/búsqueda antes de destruir sus dependencias y antes
  de apagar WIC en App, tanto al salir de Run como en destrucción parcial. Probar
  trabajos retenidos a través de la destrucción. No cambiar política de escaneo.
- AC-04: cada hallazgo y área de cobertura tiene disposición final explícita;
  mantener límites de WIC/escáneres/cliente y mejoras opcionales como trabajo futuro,
  sin declararlos corregidos ni exigir una auditoría universal para cerrar la base.
- AC-05: guardar el lote en Git sin datos locales; dejar un prompt que impida
  reiniciar la campaña de refactorización por inercia.

La base termina con las porciones implementadas y aceptadas. No incluye retirada
de Library, scheduler, cancelación forzada de códecs, ACK/elección de IPC ni una
auditoría exhaustiva del repositorio. Los límites existentes no adquieren por ello
garantías nuevas. La conservación funcional reportada no prueba archivos corruptos.

# Plan 011

[Spec](spec.md), [resultado](result.md), [revisión](review.md).

- [x] Revisar Git, guías, habilidad, 010 y contratos 004/007; snapshot en
  build/optimization-011/before, preservando todo el trabajo sin commit.
- [x] Especificar predicado opcional UI que omite únicamente upload.
- [x] Implementar loader y consumidores de tokens; conservar completion final.
- [x] Probar cola real, WARP, obsoletos/vigentes/fallos/reordenación y revisión 007.
- [x] Compilar Release, ejecutar CTest y revisar diff acotado/comentarios.
- [x] Actualizar evidencia, roadmap y [prompt de continuidad](resume.md).
- [x] Generar portable 011 aislado, conservando candidato 010 y aceptación pendiente.

Estado: completado en alcance. Release/CTest 5/5; 321 checks nuevos. Riesgos UP-001..003
tratados según [revisión](review.md). Sin mejora de latencia certificada. Continuar con
especificación OPT-007 como propuesta; demás pendientes conservados en resultado.

Sin delegación. No tocar los restantes pendientes. Recuperación solo del diff 011
contra snapshot, nunca reset global. Artefactos bajo build/dist ignorados.

# Resultado 024 — 2026-10-26

Continuidad posterior autorizada: [025](../025-renderizado-por-cambios/result.md)
implementa y mide la primera corrección. Este documento conserva el diagnóstico
anterior a esa intervención; sus menciones de scheduler sin cambios son históricas.

Revisión y análisis del log finalizados: AC-01/02 en [review.md](review.md).
Base 6e66535; modificación funcional simultánea limitada a transparencia 023.

El primer log registra 368 muestras, máximo 60%, p95 48% y media 18,73% del dispositivo.
Entre 16:11 y 16:13 no hay muestras de carga cero; no es solo un pico inicial.
Ese primer log no tenía tiempos de acciones ni atribución por PID.

Segundo log recibido con apertura 17:52:30, minimización 17:53:30 y cierre
17:54:15 reportados por el propietario: 154 muestras a intervalos de 1 s.
Media antes 4,38%, visible 29,17%, minimizado 9,27%, cerrado 0,43%.
Excluyendo cinco segundos iniciales de minimización quedan 40 muestras con
media 9,20%, máximo 18% y mínimo 1%. El máximo de 45% de todo el tramo
minimizado corresponde al segundo exacto del evento, no a carga sostenida.
Visible conserva media 31,09% al excluir cinco segundos iniciales; no es solo
una ráfaga de apertura. Tras cierre hay siete muestras, seis a 0% y una a 3%.

Hay relación temporal más fuerte con el visor y actividad residual minimizada
compatible con la ausencia de suspensión explícita. No se reproducen >50%
minimizado en esta captura ni se dispone de atribución por PID/motor.
Identidad, segmentación y límites completos en review.md.

Causa candidata principal: dibujo/presentación incondicional en reposo. Dock
retenido y animación completa sin pausa agravan otros escenarios. Uploads por
lotes pueden producir ráfagas de carga, sin explicar solos reposo indefinido.

Recomendación: medir por PID/motor y después diseñar presentación por cambios
con vencimientos/notificaciones, conservando aceleración y calidad. No se cambió
el scheduler ni se declara corregido el síntoma. Release/CTest 9/9 de 023 no
certifica consumo GPU. No se ejecutó una reproducción nativa nueva.

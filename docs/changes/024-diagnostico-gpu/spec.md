# 024: Análisis de picos GPU

Autorizado el 2026-10-26: analizar posibles causas, soluciones y tradeoffs.
Reporte: oscilaciones y picos superiores al 50% mientras el visor está abierto,
incluso minimizado. Hardware reportado: RTX 4070 Ti SUPER 16 GB y LG27GL850 144 Hz.
Se aporta GPU-Z Sensor Log.txt, analizado de solo lectura; no contiene eventos
del visor ni atribución por PID.

Actualización: segundo log con horas de acciones reportadas por el propietario
(apertura 17:52:30, minimización 17:53:30 y cierre 17:54:15), analizado por estado.
No equivale a eventos instrumentados del proceso ni amplía el alcance a implementación.

- AC-01: revisar bucle principal, presentación, animaciones y uploads; priorizar
  hallazgos y distinguir código, reporte, datos del log e hipótesis causal.
- AC-02: comparar soluciones conservando aceleración/calidad y proponer medición
  que permita atribución y valide futuras correcciones.

Entregable: review.md persistente, plan y resultado. No implementar aquí scheduler,
reducciones de FPS/calidad ni ejecutar la campaña ETW histórica de arranque.
La revisión puede completarse dejando abierta la atribución causal de los picos.

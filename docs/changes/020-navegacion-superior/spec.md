# 020 — Navegación superior

Autorizado por el propietario el 2026-10-26, sobre 019.

- AC-01: dock compacto Outline solo en Viewer y modal Waterfall, seis acciones: Waterfall, Board, Rotate 90°, Tags (T), Sort & Filter deshabilitado, Settings.
- AC-02: objetivos mínimos 44 DIP, adaptación a ancho/DPI; dock centrado arriba y activación superior de 48 DIP sobre su ancho, retención hover/foco/overlay, espera 900 ms y fade 180 ms con effects. Oculto no intercepta. Tooltip con nombre; navegación de teclado.
- AC-03: radial retirado de todos los modos; menús contextuales Waterfall/Board ofrecen Viewer y NUM1 se conserva.
- AC-04: precedencia de overlays/dock/tira antes de imagen/exterior, sin modificar loader/caché/formatos.

Glifos originales mediante primitivas nativas: dirección Outline elegida en 016, sin assets/dependencias externas. No introducir gestos 021 ni marca 022 aquí.


## Corrección solicitada tras pruebas — 2026-10-26

La captura Outline del propietario es la referencia visual vigente: columnas con
fotografías y divisiones para Waterfall; tres fotografías libres para Board;
flecha curva y cuadro para Rotate; dos etiquetas con agujeros; líneas/triángulo
de orden más embudo; engranaje continuo de seis dientes. Contornos redondeados,
iconos claros, hover/activo cian y filtro gris, divisores finos entre botones.
No basta sustituirlos por símbolos genéricos. Dock centrado en la parte superior,
también en compacto y modal; conservar acceso por teclado/MSAA y autoocultamiento.
Los glifos detallados ocupan 28 DIP dentro de objetivos de 44 DIP.

Ajustes asociados en 018: fuente flotante original 13 unidades cliente (confirmada
en HEAD anterior a 018; sin multiplicar por DPI) y gap de tira 3 DIP, mitad de 6.
Conservar thumbnail_width guardado y default 44/activa 57,2; geometría común de
visibles/hits/scroll/reordenación. Sin cambios de loader, formatos o gestos.

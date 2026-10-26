# 021 — Gestos

Autorizado por el propietario, sobre 019/020.

- AC-01: clic izquierdo simple en fondo transparente exterior a la imagen de Viewer fullscreen sale a ventana compacta mediante el ajuste existente. Consume solo el primer gesto; sin enviar eventos a otra aplicación. Un segundo clic fuera de la ventana resultante sigue el enrutamiento normal de Windows.
- AC-02: ya compacto no alterna fullscreen por pulsar/doble pulsar fondo exterior; excluye controles, menús, overlays y gestos activos. Transparencia interna del bitmap cuenta como imagen. Conserva doble clic sobre imagen y comportamiento modal Waterfall.
- AC-03: hover Waterfall aumenta ancho 66,6667 (antes 40), preservando centro/proporción y usando el mismo helper en dibujo/hits.

Sin cambio de formatos, loader, caché, prioridades ni preferencias persistidas. Sin reenvío manual de ratón.

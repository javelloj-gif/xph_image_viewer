# Resultado 022

Código neutral implementado en el árbol local sobre HEAD 2d19d3d, rama
codex/015-roadmap-design. Proyecto/EXE image_viewer, títulos y About Image Viewer,
recursos y herramientas/empaquetado actuales neutrales. Se conservan licencia,
Sergei Krumas y xPhang. Contratos y excepciones en [compatibility.md](compatibility.md).
No migraciones de datos/IPC/Windows ni cambios de formatos o enums.

La sesión anterior dejó pendiente el recorrido final al detectar Escape físico.
La corrección visual posterior de 020 retomó el recorrido y produjo el candidato
vigente A975803A72C4424F7DD8ECE301640689AD8F55DCD139BFF8926290497A6CCFC1:
Release/CTest 9/9, 15,52 s; 12 frames WARP y recorrido nativo acotado, MSAA,
compacto/fullscreen y modal. ZIP extraído con --multiple/--lite, tres modos y
salida normal 0. Recursos de marca/aliases e inventario siguen los revisados en
[review.md](review.md). La regresión de alias neutral vacío pasa.

ZIP vigente: dist/image_viewer_020_visual_20261026.zip. Instalador regenerado:
dist/image_viewer_setup_0.90.0.exe. Ambos de staging limpio; no se ha instalado
ni probado upgrade/desinstalación sobre una instalación previa de la máquina.
No se publican staging/datos/logs ni se hacen commits. 015/016 preservados.

La evidencia detallada y los límites de captura/monitores y rendimiento están en
[resultado 020](../020-navegacion-superior/result.md). Sin nuevas medidas para la
corrección visual y sin recompilar/ejecutar base. La serie histórica del candidato
86FFDF10… (347,20 ms, n=8) pertenece solo a ese hash; no al vigente. No se declara
una mejora global, aceptación posterior del propietario ni estabilidad de cada
frame o lector de pantalla por una compilación.

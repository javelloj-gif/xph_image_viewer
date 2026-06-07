"""
generate_icons.py — LenteUV Icon Generator
===========================================
Genera el archivo lenteuv.ico multibanda para Windows desde la imagen de referencia
de alta resolución o, en su defecto, desde el SVG vectorial (requiere cairosvg).

Estrategia de renderizado (en orden de prioridad):
  1. PNG de referencia de alta resolución (lenteuv PNG.png) — Fidelidad 1:1 garantizada
  2. cairosvg — Renderizado vectorial perfecto desde el SVG
  3. Error explícito — Se informa al usuario que falta la fuente de imagen

Uso:
  python native/generate_icons.py
"""

import os
import sys
from pathlib import Path

# ─── Rutas ─────────────────────────────────────────────────────────────────────
SCRIPT_DIR    = Path(__file__).parent
PROJECT_ROOT  = SCRIPT_DIR.parent
SVG_PATH      = SCRIPT_DIR / "lente_uv.svg"
PNG_REF_PATH  = PROJECT_ROOT / "lenteuv PNG.png"
OUTPUT_ICO    = SCRIPT_DIR / "src" / "resources" / "lenteuv.ico"
ICO_SIZES     = [16, 32, 48, 256]

def banner(msg):
    print(f"\n{'=' * 60}")
    print(f"  {msg}")
    print(f"{'=' * 60}")

def load_source_image():
    """
    Carga la imagen fuente de alta resolución para el ícono.
    Prioridad: PNG de referencia > cairosvg (SVG) > Error
    """
    from PIL import Image

    # ── Prioridad 1: PNG de referencia (lenteuv PNG.png) ─────────────────────
    if PNG_REF_PATH.exists():
        print(f"\n[Source] Usando PNG de referencia de alta resolucion:")
        print(f"         {PNG_REF_PATH}")
        img = Image.open(PNG_REF_PATH).convert("RGBA")
        print(f"         Tamaño original: {img.width}x{img.height} px, modo: {img.mode}")
        return img

    # ── Prioridad 2: cairosvg → SVG a PNG de alta resolución ─────────────────
    try:
        import cairosvg
        import io
        print(f"\n[Source] cairosvg disponible — renderizando desde SVG:")
        print(f"         {SVG_PATH}")
        png_bytes = cairosvg.svg2png(
            url=str(SVG_PATH),
            output_width=512,
            output_height=512,
        )
        img = Image.open(io.BytesIO(png_bytes)).convert("RGBA")
        print(f"         Renderizado: {img.width}x{img.height} px")
        return img
    except ImportError:
        pass

    # ── Sin fuente disponible → Error informativo ─────────────────────────────
    print("\n[ERROR] No se encontró ninguna fuente de imagen para el ícono.")
    print(f"  Falta: {PNG_REF_PATH}")
    print(f"  Falta: cairosvg (instala con 'pip install cairosvg')")
    print()
    print("  Soluciones:")
    print("    1. Coloca 'lenteuv PNG.png' en la raíz del proyecto (recomendado)")
    print("    2. Instala cairosvg: pip install cairosvg")
    sys.exit(1)


def make_ico_band(source_img, size):
    """
    Genera una banda del ícono al tamaño indicado con calidad máxima.
    Usa redimensionado Lanczos (bicúbico de 8 lóbulos) para máxima nitidez.
    Para 256px se preserva como PNG dentro del ICO (compresión sin pérdida).
    """
    from PIL import Image

    # El tamaño de origen puede no ser cuadrado — hacemos fit cuadrado con padding
    w, h = source_img.size
    if w != h:
        side = max(w, h)
        square = Image.new("RGBA", (side, side), (0, 0, 0, 0))
        offset_x = (side - w) // 2
        offset_y = (side - h) // 2
        square.paste(source_img, (offset_x, offset_y), source_img)
        source_img = square

    return source_img.resize((size, size), Image.LANCZOS)


def write_ico(bands, output_path):
    """
    Escribe el archivo .ico multibanda. Inno Setup y Windows esperan las bandas
    en orden de mayor a menor tamaño para la correcta representación en el
    Explorador de archivos, la barra de tareas y la lista de programas.
    """
    output_path.parent.mkdir(parents=True, exist_ok=True)

    # Ordenar de mayor a menor (256 → 16)
    sizes_desc = sorted(bands.keys(), reverse=True)
    imgs_in_order = [bands[s] for s in sizes_desc]

    # Pillow necesita que la lista sea [img_grande, ..., img_pequeña]
    # y el primer elemento es el "principal"
    imgs_in_order[0].save(
        str(output_path),
        format="ICO",
        sizes=[(img.width, img.height) for img in imgs_in_order],
        append_images=imgs_in_order[1:],
    )


def main():
    banner("LenteUV Image Viewer -- Icon Generator")
    print(f"\n  SVG source  : {SVG_PATH}")
    print(f"  PNG ref     : {PNG_REF_PATH}")
    print(f"  Output ICO  : {OUTPUT_ICO}")
    print(f"  Sizes       : {ICO_SIZES}")

    # Verificar que Pillow está instalado
    try:
        from PIL import Image
    except ImportError:
        print("\n[ERROR] Pillow no está instalado.")
        print("  Instala con: pip install Pillow")
        sys.exit(1)

    # Cargar imagen fuente
    source = load_source_image()

    # Generar cada banda
    print("\n[Generando bandas ICO]")
    bands = {}
    for size in ICO_SIZES:
        band = make_ico_band(source, size)
        bands[size] = band
        print(f"  [{size:>3}x{size:<3}] OK")

    # Escribir el ICO
    print(f"\n[Escribiendo] {OUTPUT_ICO}")
    write_ico(bands, OUTPUT_ICO)

    # Verificar resultado
    from PIL import Image as PILImage
    result = PILImage.open(str(OUTPUT_ICO))
    file_size = OUTPUT_ICO.stat().st_size

    print(f"\n[OK] ICO generado exitosamente:")
    print(f"     Ruta      : {OUTPUT_ICO}")
    print(f"     Tamaño    : {file_size:,} bytes")
    print(f"     Formato   : {result.format}")
    print(f"     Modo      : {result.mode}")
    print(f"     Dimensión : {result.size[0]}x{result.size[1]} px (banda activa)")
    print()
    print("  Siguiente paso: recompilar lenteuv.exe")
    print('  $cmake = "C:\\Program Files (x86)\\...\\cmake.exe"')
    print("  & $cmake --build native/build --config Release")
    print()


if __name__ == "__main__":
    main()

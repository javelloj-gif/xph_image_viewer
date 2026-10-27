"""Generate Fotassa's photo-frame icon as a multi-band Windows ICO.

Uses only Python's standard library. Geometry mirrors assets/branding/image_viewer.svg;
no historical bitmap or third-party artwork is imported. Run from any working directory.
"""
from pathlib import Path
import struct

ROOT = Path(__file__).resolve().parent.parent
OUTPUT = ROOT / "src/resources/image_viewer.ico"
SIZES = (16, 32, 48, 256)


def inside_triangle(x, y, vertices):
    """Test a sample against a filled triangle in the icon's original 64-unit coordinates."""
    sides = []
    for a, b in zip(vertices, vertices[1:] + vertices[:1]):
        sides.append((x-a[0])*(b[1]-a[1]) - (y-a[1])*(b[0]-a[0]))
    return all(v >= 0 for v in sides) or all(v <= 0 for v in sides)


def sample(x, y):
    """Return RGBA for one original frame/mountain/sun sample; outside is transparent."""
    if not (4 <= x <= 60 and 4 <= y <= 60):
        return (0, 0, 0, 0)
    # Rounded dark tile; maintain the same geometry at every ICO resolution.
    cx, cy = min(52, max(12, x)), min(52, max(12, y))
    if (x-cx)**2 + (y-cy)**2 > 64:
        return (0, 0, 0, 0)
    color = (21, 28, 36, 255)
    if 10.5 <= x <= 53.5 and 12.5 <= y <= 51.5 and not (13.5 < x < 50.5 and 15.5 < y < 48.5):
        color = (0, 232, 240, 255)
    if inside_triangle(x, y, [(15,46),(27,29),(40,46)]) or inside_triangle(x, y, [(31,46),(42,33),(50,46)]):
        color = (230, 237, 243, 255)
    if (x-43)**2 + (y-24)**2 <= 16:
        color = (0, 232, 240, 255)
    return color


def make_band(size):
    """Encode one 32-bit bottom-up DIB plus AND mask, with four-sample edge antialiasing."""
    pixels = bytearray()
    for row in reversed(range(size)):
        for col in range(size):
            colors = [sample((col+dx)*64/size, (row+dy)*64/size) for dx,dy in ((.25,.25),(.75,.25),(.25,.75),(.75,.75))]
            alpha = sum(c[3] for c in colors)
            rgb = [round(sum(c[ch]*c[3] for c in colors)/alpha) if alpha else 0 for ch in range(3)]
            pixels.extend((rgb[2],rgb[1],rgb[0],round(alpha/4)))
    mask = bytes(((size+31)//32)*4*size)
    header = struct.pack('<IiiHHIIiiII',40,size,size*2,1,32,0,len(pixels),0,0,0,0)
    return header + pixels + mask


def main():
    """Write all four bands deterministically; only the neutral application ICO is replaced."""
    bands = [make_band(size) for size in SIZES]
    offset = 6 + 16*len(bands)
    directory = bytearray(struct.pack('<HHH',0,1,len(bands)))
    for size, band in zip(SIZES,bands):
        directory.extend(struct.pack('<BBBBHHII',size%256,size%256,0,0,1,32,len(band),offset))
        offset += len(band)
    OUTPUT.write_bytes(directory + b''.join(bands))
    print(f"Generated {OUTPUT} ({', '.join(map(str,SIZES))} px)")


if __name__ == '__main__':
    main()

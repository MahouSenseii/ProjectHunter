"""Generate the System-window panel textures.

Plain Python with Pillow - this does not need Unreal. It writes the two PNGs
next to their .uasset files; reimport them in the editor afterwards, which
PolishSystemMenu.py does for you.

Two rules this file exists to enforce:

1. **Power-of-two dimensions.** The originals were 160x160. Unreal cannot
   mip a non-power-of-two texture and handles it as a special case, which is
   what distorted the frame. 256x256 removes the problem at the source rather
   than compensating for it in brush settings.

2. **No corner accents baked in.** The floating squares come from the authored
   M_SquareFlicker material, which animates them and can be reused by any menu.
   A square painted into the texture cannot flicker and cannot be reused.

Transparency is baked into the Fill texture rather than applied as a brush
tint alpha. A single tint alpha would fade the white border along with the
body; baking it keeps the border opaque over a translucent body, which is what
makes the window read as a frame at all.
"""

import math
from pathlib import Path

from PIL import Image, ImageDraw

SIZE = 256          # power of two, per rule 1 above
SUPERSAMPLE = 4     # rendered at 1024 then reduced, for clean chamfer diagonals
CHAMFER = 40.0      # 45-degree corner cut, in final pixels

# Ring geometry, as inset distances from the panel edge in final pixels.
# outer line | gap | inner line | body
OUTER_LINE = (2.0, 5.0)
INNER_LINE = (11.0, 16.0)
BODY_INSET = 16.0

WHITE = (255, 255, 255, 255)

# How much of the world shows through the window body. The white double line is
# always fully opaque regardless of this, which is the whole reason the body's
# transparency lives in the texture instead of in a brush tint alpha.
# 0.0 would be a frame with no tint at all; raise it toward 1.0 for a solid panel.
BODY_ALPHA = 0.35
AZURE_BODY = (46, 155, 224, round(255 * BODY_ALPHA))    # #2E9BE0 - PHUIStyle::Azure

OUT_DIR = Path(__file__).resolve().parents[2] / "Content/ProjectHunter/UI/Widgets/Menus/System"


def octagon(inset):
    """Points of the octagon inset by `inset`, as a true parallel offset.

    Naively insetting the bounding box while holding the chamfer length fixed
    moves the 45-degree edges inward by inset*sqrt(2) instead of inset, so the
    border would render noticeably thicker on the diagonals than on the straight
    runs. Shrinking the chamfer by inset*(2-sqrt(2)) cancels that exactly.
    """
    scale = SUPERSAMPLE
    a = inset * scale
    b = (SIZE - inset) * scale
    c = (CHAMFER - inset * (2.0 - math.sqrt(2.0))) * scale
    return [(a + c, a), (b - c, a), (b, a + c), (b, b - c),
            (b - c, b), (a + c, b), (a, b - c), (a, a + c)]


def mask_shape(inset):
    """Filled coverage mask for the octagon at `inset`."""
    image = Image.new("L", (SIZE * SUPERSAMPLE,) * 2, 0)
    ImageDraw.Draw(image).polygon(octagon(inset), fill=255)
    return image


def ring(outer_inset, inner_inset):
    """Coverage for the band between two insets."""
    band = mask_shape(outer_inset).copy()
    hole = mask_shape(inner_inset)
    band.paste(0, (0, 0), hole)
    return band


def compose(layers):
    """Paint (coverage, rgba) pairs back to front, then downsample."""
    canvas = Image.new("RGBA", (SIZE * SUPERSAMPLE,) * 2, (0, 0, 0, 0))
    for coverage, rgba in layers:
        canvas.paste(Image.new("RGBA", canvas.size, rgba), (0, 0), coverage)
    return canvas.resize((SIZE, SIZE), Image.LANCZOS)


def main():
    border = [(ring(*OUTER_LINE), WHITE), (ring(*INNER_LINE), WHITE)]

    # Fill: translucent body first, then the opaque double line over it.
    fill = compose([(mask_shape(BODY_INSET), AZURE_BODY)] + border)
    # Frame: the line language alone, for reuse on any other panel.
    frame = compose(border)

    # Mask: the panel's silhouette in white on black, for materials that need to
    # confine themselves to the window shape (M_SquareFlicker's MaskValue). Opaque
    # everywhere so a material samples a clean 1/0, not an alpha edge.
    mask = compose([(mask_shape(BODY_INSET), (255, 255, 255, 255))])
    mask = mask.convert("RGB")

    OUT_DIR.mkdir(parents=True, exist_ok=True)
    for name, image in (("T_SystemPanel_Fill", fill), ("T_SystemPanel_Frame", frame),
                        ("T_SystemPanel_Mask", mask)):
        path = OUT_DIR / (name + ".png")
        image.save(path)
        print("%-24s %dx%d  %s" % (name, image.width, image.height, path))

    # The 9-slice margin every consumer must use: the corner block has to
    # contain the whole chamfer, or stretching will shear it.
    corner = CHAMFER + 8.0
    print("\nSlateBrush BOX margin for these textures: %.4f (%d px of %d)"
          % (corner / SIZE, corner, SIZE))


if __name__ == "__main__":
    main()

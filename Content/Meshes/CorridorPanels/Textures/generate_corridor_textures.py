from PIL import Image, ImageDraw, ImageFilter, ImageEnhance, ImageChops, ImageOps
import numpy as np
from pathlib import Path

OUT = Path(r"H:\Projects\SpaceshipCrew\Content\Meshes\CorridorPanels\Textures\Source")
OUT.mkdir(parents=True, exist_ok=True)
SIZE = 1024


def save(img, name):
    path = OUT / name
    img.save(path, compress_level=3)
    print("saved", path, img.size, img.mode)


def noise(size, seed=0):
    local = np.random.default_rng(seed)
    h = local.random((size // 8, size // 8))
    img = Image.fromarray((h * 255).astype(np.uint8), mode="L")
    return img.resize((size, size), Image.BICUBIC)


def tileable_blur(img, radius=2):
    w, h = img.size
    canvas = Image.new(img.mode, (w * 3, h * 3))
    for y in range(3):
        for x in range(3):
            canvas.paste(img, (x * w, y * h))
    canvas = canvas.filter(ImageFilter.GaussianBlur(radius=radius))
    return canvas.crop((w, h, 2 * w, 2 * h))


def panel_lines(size, step=128, thickness=3, color=40):
    img = Image.new("L", (size, size), 0)
    d = ImageDraw.Draw(img)
    for i in range(0, size, step):
        d.line([(i, 0), (i, size)], fill=color, width=thickness)
        d.line([(0, i), (size, i)], fill=color, width=thickness)
    inset = step // 8
    for y in range(0, size, step):
        for x in range(0, size, step):
            d.rectangle(
                [x + inset, y + inset, x + step - inset, y + step - inset],
                outline=max(color // 2, 1),
                width=2,
            )
    return img


def rivets(size, step=128, radius=4):
    img = Image.new("L", (size, size), 0)
    d = ImageDraw.Draw(img)
    margin = step // 6
    for y in range(0, size, step):
        for x in range(0, size, step):
            for dx, dy in [
                (margin, margin),
                (step - margin, margin),
                (margin, step - margin),
                (step - margin, step - margin),
            ]:
                cx, cy = x + dx, y + dy
                d.ellipse([cx - radius, cy - radius, cx + radius, cy + radius], fill=180)
                d.ellipse(
                    [cx - radius // 2, cy - radius // 2, cx + radius // 2, cy + radius // 2],
                    fill=90,
                )
    return img


def height_to_normal(height_img, strength=2.5):
    h = np.asarray(height_img, dtype=np.float32) / 255.0
    dx = np.roll(h, -1, axis=1) - np.roll(h, 1, axis=1)
    dy = np.roll(h, -1, axis=0) - np.roll(h, 1, axis=0)
    nx = -dx * strength
    ny = -dy * strength
    nz = np.ones_like(h)
    length = np.sqrt(nx * nx + ny * ny + nz * nz)
    nx, ny, nz = nx / length, ny / length, nz / length
    n = np.stack([(nx + 1) * 0.5, (ny + 1) * 0.5, (nz + 1) * 0.5], axis=-1)
    return Image.fromarray((n * 255).astype(np.uint8), mode="RGB")


# ---------- HULL ----------
base = np.zeros((SIZE, SIZE, 3), dtype=np.float32)
base[:] = (48, 58, 68)
nimg = np.asarray(noise(SIZE, seed=1), dtype=np.float32) / 255.0
nimg2 = np.asarray(noise(SIZE, seed=2), dtype=np.float32) / 255.0
base += (nimg[..., None] - 0.5) * 18
base += (nimg2[..., None] - 0.5) * 8

lines = panel_lines(SIZE, step=128, thickness=4, color=255)
riv = rivets(SIZE, step=128, radius=5)
groove = np.asarray(tileable_blur(lines, 1), dtype=np.float32) / 255.0
base *= 1.0 - groove[..., None] * 0.35
rv = np.asarray(riv, dtype=np.float32) / 255.0
base = base * (1.0 - rv[..., None] * 0.15) + np.array([160, 175, 190])[None, None, :] * rv[..., None] * 0.55
edge = panel_lines(SIZE, step=128, thickness=1, color=255)
edge = np.asarray(tileable_blur(edge, 0.5), dtype=np.float32) / 255.0
base += edge[..., None] * np.array([8, 20, 28])
hull_d = Image.fromarray(np.clip(base, 0, 255).astype(np.uint8), mode="RGB")
save(hull_d, "T_CorridorHull_D.png")

height = Image.new("L", (SIZE, SIZE), 140)
hd = ImageDraw.Draw(height)
for y in range(0, SIZE, 128):
    for x in range(0, SIZE, 128):
        hd.rectangle([x + 10, y + 10, x + 118, y + 118], fill=170)
height = ImageChops.subtract(height, ImageOps.autocontrast(lines).point(lambda p: int(p * 0.45)))
height = ImageChops.add(height, riv.point(lambda p: int(p * 0.35)))
height = tileable_blur(height, 1)
noise_h = noise(SIZE, seed=3).point(lambda p: int((p - 128) * 0.08 + 128))
height = ImageChops.add(height, noise_h)
hull_n = height_to_normal(height, strength=3.0)
save(hull_n, "T_CorridorHull_N.png")

ao = ImageChops.invert(tileable_blur(lines, 2))
ao = ImageEnhance.Contrast(ao).enhance(0.7)
ao = ImageChops.multiply(ao, Image.new("L", (SIZE, SIZE), 220))
rough = Image.new("L", (SIZE, SIZE), 110)
rough = ImageChops.add(rough, noise(SIZE, seed=4).point(lambda p: int((p - 128) * 0.2)))
rough = ImageChops.add(rough, lines.point(lambda p: int(p * 0.15)))
metal = Image.new("L", (SIZE, SIZE), 200)
metal = ImageChops.subtract(metal, lines.point(lambda p: int(p * 0.25)))
orm = Image.merge("RGB", [ao.convert("L"), rough.convert("L"), metal.convert("L")])
save(orm, "T_CorridorHull_ORM.png")

# ---------- FLOOR ----------
floor = np.zeros((SIZE, SIZE, 3), dtype=np.float32)
floor[:] = (28, 30, 34)
fn = np.asarray(noise(SIZE, seed=5), dtype=np.float32) / 255.0
floor += (fn[..., None] - 0.5) * 12
dplate = Image.new("L", (SIZE, SIZE), 0)
dd = ImageDraw.Draw(dplate)
for y in range(0, SIZE, 32):
    for x in range(0, SIZE, 32):
        dd.polygon([(x + 16, y + 4), (x + 28, y + 16), (x + 16, y + 28), (x + 4, y + 16)], fill=60)
dplate = tileable_blur(dplate, 0.8)
dp = np.asarray(dplate, dtype=np.float32) / 255.0
floor += dp[..., None] * np.array([18, 18, 20])
strip = Image.new("L", (SIZE, SIZE), 0)
sd = ImageDraw.Draw(strip)
sd.rectangle([SIZE // 2 - 90, 0, SIZE // 2 + 90, SIZE], fill=90)
sd.rectangle([SIZE // 2 - 70, 0, SIZE // 2 + 70, SIZE], fill=130)
for y in range(0, SIZE, 64):
    sd.rectangle([SIZE // 2 - 12, y + 10, SIZE // 2 + 12, y + 40], fill=200)
sp = np.asarray(strip, dtype=np.float32) / 255.0
floor = floor * (1.0 - sp[..., None] * 0.25) + np.array([55, 58, 62])[None, None, :] * sp[..., None]
guides = Image.new("L", (SIZE, SIZE), 0)
gd = ImageDraw.Draw(guides)
gd.rectangle([0, 0, SIZE, 24], fill=180)
gd.rectangle([0, SIZE - 24, SIZE, SIZE], fill=180)
gp = np.asarray(guides, dtype=np.float32) / 255.0
floor = floor * (1.0 - gp[..., None] * 0.2) + np.array([70, 90, 100])[None, None, :] * gp[..., None] * 0.5
floor_d = Image.fromarray(np.clip(floor, 0, 255).astype(np.uint8), mode="RGB")
save(floor_d, "T_CorridorFloor_D.png")

fheight = Image.new("L", (SIZE, SIZE), 120)
fheight = ImageChops.add(fheight, dplate.point(lambda p: int(p * 0.5)))
fheight = ImageChops.add(fheight, strip.point(lambda p: int(p * 0.25)))
fheight = tileable_blur(fheight, 1)
floor_n = height_to_normal(fheight, strength=2.2)
save(floor_n, "T_CorridorFloor_N.png")

form = Image.merge(
    "RGB",
    [
        Image.new("L", (SIZE, SIZE), 210),
        ImageChops.add(Image.new("L", (SIZE, SIZE), 150), noise(SIZE, seed=6).point(lambda p: int((p - 128) * 0.15))).convert("L"),
        Image.new("L", (SIZE, SIZE), 160),
    ],
)
save(form, "T_CorridorFloor_ORM.png")

# ---------- TRIM ----------
trim = np.zeros((SIZE, SIZE, 3), dtype=np.float32)
trim[:] = (140, 155, 165)
tn = np.asarray(noise(SIZE, seed=7), dtype=np.float32) / 255.0
trim += (tn[..., None] - 0.5) * 20
tlines = panel_lines(SIZE, step=64, thickness=2, color=255)
tg = np.asarray(tileable_blur(tlines, 1), dtype=np.float32) / 255.0
trim *= 1.0 - tg[..., None] * 0.25
trim_d = Image.fromarray(np.clip(trim, 0, 255).astype(np.uint8), mode="RGB")
save(trim_d, "T_CorridorTrim_D.png")
theight = Image.new("L", (SIZE, SIZE), 150)
theight = ImageChops.subtract(theight, tlines.point(lambda p: int(p * 0.35)))
trim_n = height_to_normal(tileable_blur(theight, 1), strength=2.0)
save(trim_n, "T_CorridorTrim_N.png")
torm = Image.merge(
    "RGB",
    [
        Image.new("L", (SIZE, SIZE), 230),
        Image.new("L", (SIZE, SIZE), 70),
        Image.new("L", (SIZE, SIZE), 230),
    ],
)
save(torm, "T_CorridorTrim_ORM.png")

# ---------- GLOW ----------
glow = Image.new("RGB", (SIZE, SIZE), (0, 0, 0))
gdraw = ImageDraw.Draw(glow)
for y in range(32, SIZE, 128):
    gdraw.rectangle([0, y - 4, SIZE, y + 4], fill=(40, 180, 255))
    gdraw.rectangle([0, y - 1, SIZE, y + 1], fill=(180, 240, 255))
for x in range(32, SIZE, 128):
    gdraw.rectangle([x - 2, 0, x + 2, SIZE], fill=(20, 120, 180))
glow = tileable_blur(glow, 1.2)
arr = np.asarray(glow, dtype=np.float32) * 1.4
glow = Image.fromarray(np.clip(arr, 0, 255).astype(np.uint8), mode="RGB")
save(glow, "T_CorridorGlow_E.png")
mask = ImageEnhance.Contrast(glow.convert("L")).enhance(2.0)
save(mask, "T_CorridorGlow_M.png")

print("ALL DONE")
for p in sorted(OUT.glob("*.png")):
    print(p.name)

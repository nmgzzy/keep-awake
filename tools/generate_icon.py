#!/usr/bin/env python3
# 生成 src/app.ico（16x16 + 32x32，32bpp BGRA + AND mask），无第三方依赖。
# 图案：琥珀色实心圆（“保持清醒”的太阳意象），圆外透明。
import struct, os

def make_image(size):
    cx = cy = (size - 1) / 2.0
    r = size * 0.46
    pixels = []  # 顶到底，每行 [(B,G,R,A), ...]
    for y in range(size):
        row = []
        for x in range(size):
            dx, dy = x - cx, y - cy
            d = (dx * dx + dy * dy) ** 0.5
            if d <= r:
                if d < r * 0.5:
                    R, G, B = 255, 196, 90   # 亮中心
                else:
                    R, G, B = 255, 168, 40   # 琥珀
                row.append((B, G, R, 255))
            else:
                row.append((0, 0, 0, 0))     # 透明
        pixels.append(row)

    # XOR 像素数据，自下而上
    xor = bytearray()
    for y in range(size - 1, -1, -1):
        for (B, G, R, A) in pixels[y]:
            xor += bytes((B, G, R, A))

    # AND 掩码：1bit/像素，1=透明；每行按 32bit 对齐
    rowbytes = ((size + 31) // 32) * 4
    andmask = bytearray()
    for y in range(size - 1, -1, -1):
        bits = bytearray(rowbytes)
        for x in range(size):
            if pixels[y][x][3] == 0:
                bits[x // 8] |= (0x80 >> (x % 8))
        andmask += bits

    bih = struct.pack('<IiiHHIIiiII',
                      40, size, size * 2, 1, 32, 0,
                      len(xor) + len(andmask), 0, 0, 0, 0)
    return bih + bytes(xor) + bytes(andmask)

sizes = [16, 32]
images = [make_image(s) for s in sizes]

out = bytearray()
out += struct.pack('<HHH', 0, 1, len(images))      # ICONDIR
offset = 6 + 16 * len(images)
for s, d in zip(sizes, images):                    # ICONDIRENTRY
    w = s if s < 256 else 0
    out += struct.pack('<BBBBHHII', w, w, 0, 0, 1, 32, len(d), offset)
    offset += len(d)
for d in images:
    out += d

dst = os.path.join(os.path.dirname(__file__), '..', 'src', 'app.ico')
dst = os.path.abspath(dst)
with open(dst, 'wb') as f:
    f.write(out)
print('wrote', dst, len(out), 'bytes')

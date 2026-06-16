#!/usr/bin/env python3
# 生成两态托盘图标（各含 16x16 + 32x32，32bpp BGRA + AND mask），无第三方依赖。
#   icon_idle.ico  —— 蓝色（空闲）
#   icon_awake.ico —— 橙色（防休眠中）
# 图案：实心圆，圆外透明。
import struct, os

def make_image(size, outer_rgb, inner_rgb):
    cx = cy = (size - 1) / 2.0
    r = size * 0.46
    oR, oG, oB = outer_rgb
    iR, iG, iB = inner_rgb
    pixels = []  # 顶到底，每行 [(B,G,R,A), ...]
    for y in range(size):
        row = []
        for x in range(size):
            dx, dy = x - cx, y - cy
            d = (dx * dx + dy * dy) ** 0.5
            if d <= r:
                if d < r * 0.5:
                    R, G, B = iR, iG, iB   # 亮中心
                else:
                    R, G, B = oR, oG, oB   # 主色
                row.append((B, G, R, 255))
            else:
                row.append((0, 0, 0, 0))     # 透明
        pixels.append(row)

    xor = bytearray()
    for y in range(size - 1, -1, -1):
        for (B, G, R, A) in pixels[y]:
            xor += bytes((B, G, R, A))

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


def write_ico(path, outer_rgb, inner_rgb):
    sizes = [16, 32]
    images = [make_image(s, outer_rgb, inner_rgb) for s in sizes]
    out = bytearray()
    out += struct.pack('<HHH', 0, 1, len(images))      # ICONDIR
    offset = 6 + 16 * len(images)
    for s, d in zip(sizes, images):                    # ICONDIRENTRY
        w = s if s < 256 else 0
        out += struct.pack('<BBBBHHII', w, w, 0, 0, 1, 32, len(d), offset)
        offset += len(d)
    for d in images:
        out += d
    with open(path, 'wb') as f:
        f.write(out)
    print('wrote', path, len(out), 'bytes')


here = os.path.dirname(__file__)
src = os.path.abspath(os.path.join(here, '..', 'src'))
write_ico(os.path.join(src, 'icon_idle.ico'),  (40, 120, 220), (90, 165, 255))   # 蓝
write_ico(os.path.join(src, 'icon_awake.ico'), (255, 140, 30), (255, 180, 80))   # 橙

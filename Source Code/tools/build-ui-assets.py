"""Pack native-size Platinum bitmap faces and the procedural SINO pointer.

Jarrah / Kurrajong / Mallee artwork: Ben Letchford, SIL OFL 1.1.
Pinned upstream sources and license are in tools/fonts/platinum.
No systemless emulator implementation is compiled or imported.
"""
from pathlib import Path
import re
import struct
import gzip
import unicodedata
from PIL import Image, ImageDraw, ImageFilter

ROOT = Path(__file__).resolve().parents[1]
FONT_DIR = ROOT / "tools/fonts/platinum"


def atlas(filename, size, width, height, baseline):
    source = (FONT_DIR / filename).read_text(encoding="utf-8")
    source = re.sub(r"//[^\n]*", "", source)
    glyphs = re.findall(r"g!\(\s*(\d+)\s*,\s*\(\s*(-?\d+)\s*,\s*(-?\d+)\s*\)\s*,(.*?)\)", source, re.S)
    assert len(glyphs) == 95, (filename, len(glyphs))
    glyphs.append(glyphs[ord('?') - 32])
    advances, pixels = bytearray(), bytearray()
    for advance, origin_x, origin_y, art in glyphs:
        advances.append(int(advance))
        bitmap = bytearray(width * height)
        for y, line in enumerate(re.findall(r'"([.#]+)"', art)):
            for x, value in enumerate(line):
                if value == '#':
                    xx, yy = x + int(origin_x), y + baseline + int(origin_y)
                    # An ink-bearing can be negative in the upstream face.
                    # A one-pixel atlas side bearing preserves it without scaling.
                    xx += 1
                    assert 0 <= xx < width and 0 <= yy < height, (filename, xx, yy)
                    bitmap[yy * width + xx] = 15
        for i in range(0, len(bitmap), 2):
            pixels.append((bitmap[i] << 4) | bitmap[i + 1])
    return struct.pack("<4sHHHHI", b"SNF1", width, height, 96, size, 0) + advances + pixels


def pointer():
    scale = 4
    image = Image.new("RGBA", (28 * scale, 34 * scale))
    points = [(x * scale, y * scale) for x, y in [(1, 1), (1, 17), (5, 13), (9, 21), (12, 19), (8, 12), (15, 12)]]
    shadow = Image.new("RGBA", image.size)
    ImageDraw.Draw(shadow).polygon([(x + scale, y + 2 * scale) for x, y in points], fill=(0, 0, 0, 115))
    image.alpha_composite(shadow.filter(ImageFilter.GaussianBlur(scale * .9)))
    ImageDraw.Draw(image).polygon(points, fill=(25, 27, 32, 255), outline=(246, 248, 251, 255), width=scale)
    return image.resize((28, 34), Image.Resampling.LANCZOS).tobytes()


def chinese():
    glyphs = []
    with gzip.open(ROOT / 'tools/fonts/unifont/unifont-16.0.04.hex.gz', 'rt') as source:
        for line in source:
            key, bitmap = line.strip().split(':')
            cp = int(key, 16)
            if 0x4e00 <= cp <= 0x9fff or 0x3000 <= cp <= 0x303f or 0xff00 <= cp <= 0xffef or 0x2000 <= cp <= 0x206f:
                data = bytes.fromhex(bitmap)
                if len(data) == 16:
                    data = b''.join(bytes((b, 0)) for b in data)
                assert len(data) == 32
                glyphs.append(struct.pack('<H', cp) + data)
    return struct.pack('<I', len(glyphs)) + b''.join(glyphs)


def pinyin():
    readings = {}
    for line in (ROOT / 'tools/pinyin/pinyin.txt').read_text(encoding='utf-8').splitlines():
        if not line.startswith('U+'):
            continue
        key, values = line.split(':', 1)
        cp = int(key[2:], 16)
        if not 0x4e00 <= cp <= 0x9fff:
            continue
        for value in values.split('#')[0].strip().split(','):
            value = value.replace('ü', 'v')
            decomposed = unicodedata.normalize('NFD', value)
            # Preserve umlaut through accented ü (lǜ -> lv).
            value = ''.join(c for c in decomposed if not unicodedata.combining(c)).replace('u:', 'v')
            if '\u0308' in decomposed:
                value = value.replace('u', 'v')
            if value.isascii() and value.isalpha() and len(value) < 8:
                chars = readings.setdefault(value, [])
                if cp not in chars:
                    chars.append(cp)
    # SINO's small, hand-written common-character priority list. Other candidates
    # retain upstream Unicode order; this is a character IME, not a phrase model.
    common = '的我你他她它是了在有和人这中大为上个国到说们地也子时道出而要于就下得可生自之年过发后作里用来行所然家种事成方多经么去法学如都同现当没动面起看定天分还进好小部其些主样理心本前开但因只从想实日军者意无力它与长把机十民第公此已工使情明性知全三又关点正业外将两高间由问很最重并物手应战向头文体政美相见被利什二等产或新己制身果加西斯月话合回特代内信表化老给世位次度门任常先海通教儿原东声提立及比员解水名真论处走义各入几口认条平系气她题活尔更别打女变四神总何电数安少报才结反受目太量再感建务做接必场件计管期市直德资命山金指克许统区保至队形社便空决治展马科司五基眼书非则听白却界达光放强即像难且权思王象完设式色路记南品住告类求据程北边死张该交规万取拉格望觉术领共确传师观清今切院让识候带导争运笑飞风步改收根干造言联持组每济车亲极林服快办议往元英士证近失转夫令准布始怎呢存未远叫台单影具罗字爱击流备兵连调深商算质团集百需价花党华城石级整府离况亚请技际约示复病息究线似官火断精满支视消越器容照须九增研写称企八功吗包片史委乎查轻易早曾除农找装广显吧阿李标谈吃图念六引历首医局突专费号尽另周较注语仅考落青随选列武红响虽推势参希古众构房半节土投某案黑维革划敌致陈律足态护七兴派孩验责营星章音跟志底站严巴例防族供效续施留讲型料终答紧黄绝奇察母京段依批群项故按河米围江织害斗双境客纪采举杀攻父苏密低朝友诉止细愿千值仍男钱破网热助倒育属坐帝限船脸职速刻乐否刚威毛状率甚独球般普怕弹校苦创假久错承印晚兰试股拿脑预谁益阳若哪微尼继送急血惊伤素药适波夜省初喜卫源食险待述陆习置居劳财环排福纳欢雷警获模充负云停木游龙树疑层冷洲冲射略范竟句室异激汉村哈策演简卡罪判担州静退既衣您宗积余痛检差富灵协角占配征修皮挥胜降阶审沉坚善妈刘读啊超免压银买皇养怀执副乱抗犯追帮宣佛岁航优怪香著田铁控税航懂桌盘鼠删贴粘撤销拼输存储夹剪属性移废纸篓'
    rank = {ord(c): i for i, c in reversed(list(enumerate(common)))}
    records, data = [], bytearray()
    count = len(readings)
    for key, chars in sorted(readings.items()):
        chars.sort(key=lambda cp: rank.get(cp, 100000 + cp))
        records.append(struct.pack('<8sIHH', key.encode(), 4 + count * 16 + len(data), len(chars), 0))
        data.extend(struct.pack('<' + 'H' * len(chars), *chars))
    return struct.pack('<I', count) + b''.join(records) + data


def main():
    parts = [atlas("geneva12.rs", 12, 20, 20, 14),
             atlas("geneva18.rs", 18, 24, 28, 21),
             atlas("monaco12.rs", 12, 10, 20, 14),
             atlas("chicago12.rs", 12, 20, 20, 14), pointer(), chinese(), pinyin()]
    header = bytearray(64)
    header[:4] = b"SNUI"
    struct.pack_into("<I", header, 4, 2)
    offset = 64
    for i, part in enumerate(parts):
        struct.pack_into("<I", header, [8, 12, 16, 20, 24, 28, 44][i], offset)
        offset += len(part)
    struct.pack_into("<III", header, 32, 4, 28, 34)
    output = ROOT / "disk32/UIASSET.BIN"
    output.write_bytes(header + b"".join(parts))
    assert output.stat().st_size <= 0x100000, 'UI resources exceed mapped asset memory'
    (ROOT / "disk32/FONT_OFL.TXT").write_bytes((FONT_DIR / "OFL.txt").read_bytes())
    cjk_dir = ROOT / 'tools/fonts/unifont'
    (ROOT / 'disk32/CJK_OFL.TXT').write_bytes(
        b'GNU Unifont 16.0.04; SINO CJK/punctuation bitmap subset.\n'
        b'SINO uses the SIL OFL 1.1 option. Upstream notices follow.\n\n'
        + (cjk_dir / 'OFL-1.1.txt').read_bytes() + b'\n\n'
        + (cjk_dir / 'COPYING').read_bytes())
    (ROOT / 'disk32/PINYIN.TXT').write_bytes((ROOT / 'tools/pinyin/LICENSE.txt').read_bytes())
    print(f"Built {output.name}: {output.stat().st_size} bytes; 12px body/UI/terminal, 18px headings")


if __name__ == "__main__":
    main()

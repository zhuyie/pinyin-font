# 拼音组件缺失与字形回退

本文说明源 TrueType 字体缺少拼音声调符号或预组拼音字符时，合成器如何复用源字形、生成内部声调组件，以及如何为带声调的 `i` 派生 dotless `i`。布局算法本身见 [拼音布局算法](pinyin-layout.md)。

相关实现主要位于：

- `src/pinyin/pinyin_db.cpp`：将预组拼音字符规范化为 base + combining mark。
- `src/synthesis/pinyin_font_builder.cpp`：预组字符替换、拼音簇合成、失败分类。
- `src/synthesis/pinyin_components.cpp`：声调解析、轮廓生成、dotless `i` 派生与缓存。
- `src/opentype/ot_font.cpp`：追加内部简单字形并更新字体元数据。
- `tests/pinyin_components_test.cpp`：组件生成与 dotless `i` 的单元测试。

## 问题边界

拼音数据库中的读音会先转换成由普通字母和组合符号组成的序列。例如：

```text
mā  → m a U+0304 COMBINING MACRON
nǚ  → n u U+0308 COMBINING DIAERESIS U+030C COMBINING CARON
yī  → y i U+0304 COMBINING MACRON
ḿ   → m U+0301 COMBINING ACUTE ACCENT
```

源字体可能存在以下任意组合：

- 完整的预组字符，如 `ā`、`ǚ`、`ī`；
- combining mark，如 `U+0301`；
- 仅有 spacing 形式的兼容符号，如 `U+00B4 ACUTE ACCENT`；
- 只有普通 Latin base，没有任何可用声调符号；
- 普通 `i` 存在，但带声调的 `ī/í/ǐ/ì` 不存在。

回退机制的目标是尽量保持源字体的 Latin 风格，同时只补充合成所需的最小内部组件。它不会补齐源字体缺少的汉字或普通 Latin base，也不支持 CFF/CFF2 轮廓；当前字体解析和组件生成以 TrueType `glyf`/`loca` 为前提。

## 解析流程

```text
规范化读音为 base + mark
          ↓
源字体中有对应预组字符？ ──是──→ 使用源预组字形
          │ 否
          ↓
base 是带 mark 的 i？ ──是──→ 尝试复用/派生 dotless i
          │ 否                    │
          └──────────────┬────────┘
                         ↓
             解析每个剩余 mark
        combining → spacing → 内部生成
                         ↓
              组装 TrueType 复合字形
                         ↓
       任一步失败：分类统计并保留源 cmap 映射
```

这个流程是惰性的：只有某个读音实际需要缺失组件时才创建内部字形，后续读音复用已创建的 glyph ID。

## 读音规范化

`PinyinDB::__normalize` 将支持的预组字符统一展开，以便所有字体走同一套组件解析逻辑。当前覆盖：

- `ā/á/ǎ/à`
- `ē/é/ě/è`
- `ī/í/ǐ/ì`
- `ō/ó/ǒ/ò`
- `ū/ú/ǔ/ù`
- `ü/ǖ/ǘ/ǚ/ǜ`
- `ḿ`
- `ń/ň/ǹ`

规范化不会强制最终使用分解字形。合成阶段仍会查询源字体：如果对应预组字符存在，就重新合并并直接复用它。

## 源字形优先级

### 预组字符

构建字体时，`__buildSubstitutions` 为当前源字体实际存在的预组字符建立替换表。例如：

```text
i + U+0304 → ī
u + U+0308 → ü
ü + U+030C → ǚ
```

只有 `CharToGlyphIndex(precomposed) != 0` 时才注册替换，所以不会因为规则表中列出了 `ī` 就无条件替换到缺失字形。一个簇最多包含两个 mark，替换会连续执行；因此 `u + diaeresis + tone` 可以先变成 `ü + tone`，再在源字体允许时变成 `ǖ/ǘ/ǚ/ǜ`。

源预组字符一旦命中，已经被它吸收的 mark 不再作为独立组件生成。这是保持源字体设计风格的最高优先级路径。

### 独立声调符号

未被预组字符吸收的 mark 交给 `PinyinComponents::ResolveMark`，按以下顺序查找：

| Combining mark | 含义 | spacing 回退 |
|---|---|---|
| `U+0304` | macron（一声） | `U+00AF` |
| `U+0301` | acute（二声） | `U+00B4` |
| `U+030C` | caron（三声） | `U+02C7` |
| `U+0300` | grave（四声） | `U+0060` |
| `U+0308` | diaeresis（ü） | `U+00A8` |

首先查询 combining code point，然后查询 spacing 兼容符号。两者都缺失时，才生成内部声调字形。

当前“可用”判断是 cmap 中存在非零 glyph ID，随后能成功读取该 glyph；不会进一步检查源 mark 是否为空轮廓或视觉质量是否合适。

## 字体相对的几何参数

内部声调不是从外部组件字体复制的，而是直接使用目标字体的设计单位生成。这样无需处理字体许可、UPM 换算或外部字体风格不匹配。

### x-height 解析

`PinyinComponents` 创建时只解析一次 `xHeight`：

1. 如果 `OS/2.sxHeight` 位于 `20%..90% UnitsPerEm`，直接使用。
2. 否则测量 `a/c/e/m/n/o/r/s/u/v/w/x/z` 的可用源字形，取 `YMax - max(0, YMin)`；至少取得三个样本时，排序后取中位数。
3. 样本不足时使用 `UnitsPerEm / 2`，并限制在 `1/3..2/3 UnitsPerEm`。

这个值是组件尺寸的内部几何基准，不是对外配置项。

### mark 间距

声调与 base、双 mark 之间共用 `markGap`：

```text
markGap = round(xHeight * 0.14)
markGap ∈ [50/1000 UPM, 120/1000 UPM]
```

下限至少为一个设计单位。间距看起来比普通 accent gap 偏大，是因为拼音组件最终还会缩放到约 `35%`；过小的源空间间距在生成字形中几乎不可见。

## 内部声调字形生成

### 公共尺寸

基础笔画宽度为：

```text
stroke = round(xHeight * 0.08)
stroke ∈ [25/1000 UPM, 90/1000 UPM]
```

基础宽、高为：

```text
width  = max(2 * stroke, round(xHeight * 0.42))
height = max(2 * stroke, round(xHeight * 0.20))
```

所有生成点都是 on-curve 点，最终通过点坐标重新计算 bbox 和 contour 数。

### 各声调轮廓

- **macron**：以原点为水平中心的矩形，高度为 `stroke`。
- **acute / grave**：互为镜像的倾斜四边形；对角笔画至少为 `16% xHeight`，宽度至少为 `30% xHeight`，高度至少为 `32% xHeight`。
- **caron**：两条有轻微重叠的倾斜四边形，重叠量至少为一个单位或 `stroke / 3`，避免尖点缩放后断开。
- **diaeresis**：两个对称的八边形点；直径至少为 `14% xHeight`，点间距至少为 `12% xHeight`。

生成字形使用稳定的内部名称：

```text
pinyin.mark.macron
pinyin.mark.acute
pinyin.mark.caron
pinyin.mark.grave
pinyin.mark.diaeresis
```

其 `AdvanceWidth` 是轮廓宽度（最小为 `1`），`LSB` 等于 `XMin`。这个 advance 主要用于保持内部字形度量完整；mark 在拼音簇中不会推进逻辑游标。

### 缓存与 cmap

`generatedMarks_` 以 combining code point 为键缓存 glyph ID。首次缺失时生成一次，以后所有读音复用同一个内部字形。

这些 glyph 通过 `OpenType_Font::AddGlyph` 追加到字体，但不会加入 cmap。它们只能被生成的复合字形引用，因此不会改变源字符映射，也不会覆盖源字体中不存在的 Unicode 声调字符。

`AddGlyph` 会同步维护：

- `maxp.NumGlyphs`、`MaxPoints`、`MaxContours`
- `head` 全局边界
- `hhea` advance、side bearing 和 extent 极值
- `hmtx` 与 glyph name 数组

## 声调的簇内定位

无论 mark 来自源字体还是内部生成，定位规则相同。base 的 advance cell 中心为：

```text
baseCenter = baseOffsetX + baseAdvanceWidth / 2
```

mark 的水平偏移为：

```text
markOffsetX = baseCenter - (markXMin + markXMax) / 2
```

第一个 mark 的下边界放在：

```text
markY = baseYMax + markGap
```

若存在第二个 mark：

```text
nextMarkY = markY + firstMarkHeight + markGap
```

因此 `ü` 的 diaeresis 和其声调符号不会占用额外的横向 advance，也不会互相重叠。布局阶段随后将 base 和 mark 一起纳入完整墨迹并集，使用同一 X 缩放和居中变换。

## 带声调 i 与 dotless i

直接在普通 `i` 上叠加声调会保留原点，形成错误的双点结构。因此，当簇仍是 `i + mark` 时——意味着源字体没有命中对应的 `ī/í/ǐ/ì`——合成器必须先取得 dotless `i`。

`ResolveDotlessI` 是一个三态惰性解析器：

```text
Unknown → Ready
Unknown → Unavailable
```

成功或失败都会缓存，避免对每个读音重复分析源轮廓。

### 输入限制

派生只接受具有至少两个 contour 的简单 TrueType `i`：

- cmap 中必须有普通 `i`；
- 必须能读取轮廓；
- `NumberOfContours > 0`；
- contour endpoints 和 point 数组必须一致。

复合 `i`、单个连接轮廓、损坏的 endpoints 或无法明确分离点部的造型都会失败关闭，不尝试猜测。

### contour 分组

算法先计算每个 contour 的 bbox 和有符号面积。bbox 相交的 contour 通过并查集合并为视觉组，这使空心圆点的外轮廓与内轮廓能作为一个整体处理。

主体组按以下顺序选出：

1. `YMin` 最低；
2. 若 `YMin` 相同，绝对面积更大。

其他组只有同时满足下面的启发式条件，才是候选点部：

- 与主体顶部至少相隔 `2% xHeight`；
- 水平中心接近主体中心；允许偏差为 `max(30% xHeight, 75% bodyWidth)`；
- 宽度不超过 `55% xHeight`；
- 高度不超过 `40% xHeight`；
- 若主体面积有效，候选面积不超过主体面积的 `65%`。

必须恰好得到一个候选组。没有候选或存在多个候选都返回失败，防止装饰性字体中误删轮廓。

### 生成派生字形

确认点部后，算法复制其余 contour 和 points，并重新建立 `EndPtsOfContours`。复制点时只保留 `OnCurve` 标记，因为源点的坐标压缩 flags 描述原始 delta 编码，轮廓删除后不能复用。

派生字形还会：

- 重新计算 bbox 和 contour 数；
- 清空 TrueType instructions，避免 hinting 引用已经变化的点索引；
- 保留源 `i` 的 `AdvanceWidth`；
- 将 `LSB` 更新为派生轮廓的 `XMin`；
- 使用内部名称 `pinyin.dotless_i`；
- 不添加 cmap 映射。

一个字体最多生成一个 dotless `i`，后续读音复用它。

## 失败隔离与源 cmap 保留

构建开始时，`__retainSourceCmap` 先复制所有有效源映射。生成字形只有成功完成后才替换对应汉字的映射，所以组件回退失败不会留下半成品映射。

失败按以下类别记录：

- `SourceHanMissing`：源字体中没有目标汉字。
- `ComponentFailed`：普通 base、mark 或相关度量不可用。
- `DotlessIFailed`：无法可靠派生或读取 dotless `i`。
- `OtherFailed`：簇结构、布局或其他合成步骤失败。

默认读音失败时，目标汉字继续指向源字形。多音字的备选读音失败时，只增加 `AlternateSynthesisOmissions` 并省略该备选字形／选择规则；已成功的默认读音仍然有效。

## 诊断统计

CLI 构建完成后会输出两层统计：

- `SourceOnlyGenerated`：完全使用源组件生成的默认读音。
- `ToneFallbackGenerated`：使用了内部声调，但未使用 dotless `i`。
- `DotlessIGenerated`：使用了派生 dotless `i`；该读音也可能同时使用内部声调。
- `ComponentFailed`、`DotlessIFailed`、`OtherFailed`：默认读音失败原因。
- `InternalComponents`：macron、acute、caron、grave、diaeresis 和 dotless-i 的实际使用次数。

注意，`InternalComponents` 中的 mark 数字是引用次数，不是生成 glyph 数。一个内部 acute glyph 被一千个读音复用时，acute use count 为一千，但字体中只追加一个 acute glyph。

`ToneFallbackGenerated` 和 `DotlessIGenerated` 按成功生成的默认汉字计数；备选读音的成功与省略由 `AlternateGlyphsGenerated` 和 `AlternateSynthesisOmissions` 单独统计。

## 测试覆盖

`pinyin_components_test` 验证：

- 源 mark 优先于内部生成；
- 缺失 acute 时创建内部字形，并在重复解析时复用；
- 内部 mark 不会意外出现在 cmap；
- 生成 acute 的尺寸在 `35%` 缩放后仍足够可见；
- 普通分离点和空心点 `i` 可以派生；
- 两个候选点、连接轮廓和复合 `i` 会保守失败；
- dotless `i` 清空 instructions、保留源 advance；
- 序列化再解析后，轮廓、度量及 `maxp` 数据保持一致。

`pinyin_synthesis_test` 进一步验证：

- 使用回退组件的读音能完成实际汉字合成；
- diaeresis 与 tone 的垂直堆叠不重叠；
- 源预组拼音字符仍被优先使用；
- 回退组件参与统一的水平适配；
- 备选读音失败不会破坏默认映射和已生成的选择规则。

修改组件生成、dotless `i` 启发式或 OpenType 元数据维护后，应运行：

```bash
cmake --build build
ctest --test-dir build --output-on-failure
```

轮廓几何变化还应使用 `tools/preview.html` 在多个字号下检查 serif、sans 和装饰字体。自动测试能验证结构和边界，但不能完全判断生成声调与源字体风格是否协调。

## 当前取舍

- 优先保证源字体风格：只在源组件缺失时生成轮廓。
- 优先保证轮廓安全：dotless `i` 不明确时放弃合成，不进行猜测性删点。
- 生成组件不保留 hinting；错误的旧点索引比小字号清晰度下降更危险。
- glyph ID 由实际使用顺序决定，是内部实现细节，测试不应依赖固定编号。
- 声调比例和 dotless `i` 阈值是实现常量，目前不是 CLI 配置。
- 生成字体必须重新构建才能获得新增的内部组件；拼音数据库和命令行格式无需迁移。

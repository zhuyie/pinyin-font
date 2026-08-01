# 拼音布局算法

本文说明合成字形中汉字本体与拼音标注的布局过程，重点覆盖垂直布局、水平布局、TrueType 复合字形编码和失败边界。对应实现主要位于：

- `src/synthesis/pinyin_layout.cpp`：字体级垂直排版带选择、单个读音的水平适配。
- `src/synthesis/pinyin_font_builder.cpp`：拼音簇组装、声调定位、复合字形生成。
- `tests/pinyin_synthesis_test.cpp`：算法级和生成字体级回归测试。

## 设计目标

生成字形沿用源汉字的 `hmtx.AdvanceWidth`，汉字本体缩放到 `65%`，拼音的垂直尺寸缩放到 `35%`。布局需要同时满足以下约束：

1. 同一字体内所有拼音使用稳定的公共基线，不随单个汉字轮廓高低跳动。
2. 拼音字母间距尊重源字体的 Latin 横向度量。
3. 短拼音保持 `35%` 的原始宽高比；长拼音只压缩 X 轴，并且完整墨迹不能越出汉字 advance cell。
4. 汉字轮廓不对称、字体中存在无关的极端字形时，拼音位置不受影响。
5. 参与一个读音的字母、预组字符、组合声调和生成的回退部件共享同一套水平变换。

这里区分两个容易混淆的概念：

- **advance cell**：从 `0` 到源汉字 `AdvanceWidth` 的排版单元，是水平对齐和溢出判断的依据。
- **ink bounds**：字形轮廓实际覆盖的包围盒。它用于计算拼音真实占用宽度，但不用于决定对齐中心。

## 总体流程

```text
读取字体度量
  ├─ 选择字体级垂直排版带 → 计算 baseDY、pinyinDY
  └─ 读取目标汉字 AdvanceWidth
             ↓
将读音拆成 base + 最多两个 mark 的簇
             ↓
按各 base 的 hmtx.AdvanceWidth 排列簇，定位 mark
             ↓
求全部拼音部件平移后的 X 墨迹并集
             ↓
计算公共 ScaleX，并在汉字 advance cell 中居中
             ↓
写入拼音部件（ScaleX × 35%）和汉字本体（65% × 65%）
```

垂直参数每次构建字体时只解析一次；水平参数则针对每个汉字的每个读音单独计算。

## 垂直布局

### 选择排版带

`SelectPinyinVerticalBand` 按以下顺序选出 `[layoutYMin, layoutYMax]`：

1. `OS/2.sTypoDescender .. OS/2.sTypoAscender`
2. `hhea.Descender .. hhea.Ascender`
3. `head.YMin .. head.YMax`

前两组度量只有同时满足下面的条件才有效：

```text
yMax > 0
yMin <= 0
yMax > yMin
```

`OS/2` 和 `hhea` 描述字体预期的排版空间；`head` 则是全字体所有轮廓的极值，只作为兼容性回退。`sTypoLineGap` 和 `hhea.LineGap` 描述行间距，不参与单个复合字形内部布局。

采用这个优先级的直接原因是：一些 CJK 字体的 `head.YMin/YMax` 会被无关的极端轮廓拉大。若用它们放置拼音，普通汉字和拼音之间会产生异常空隙，并且修改一个完全无关的字形也可能改变所有合成字形的位置。

### 计算汉字与拼音基线

当前比例为：

```text
baseRatio   = 0.65
pinyinRatio = 0.35
```

汉字本体的 Y 偏移为：

```text
baseDY = layoutYMin < 0
    ? layoutYMin * (1 - baseRatio)
    : 0
```

拼音公共偏移为：

```text
pinyinDY = baseDY
          + layoutYMax * baseRatio
          - pinyinCharYMin * pinyinRatio
```

其中 `pinyinCharYMin` 是源字体 `f/g/j/p/q/y` 六个字形中最小的 `YMin`；缺失字符会跳过，初值为 `0`。这一步为常见的 Latin 下伸部预留空间，使不同读音的基准位置保持一致。

实现中各乘积会分别转换为 `int16_t`，因此实际结果以字体设计单位截断，而不是在完整公式计算后统一四舍五入。

### 簇内垂直关系

一个拼音簇由一个 base 和最多两个 combining mark 构成，例如：

```text
u
u + ◌̄
u + ◌̈ + ◌̄
```

base 的局部 `OffsetY` 为 `0`。第一个 mark 放在 base 墨迹顶部加 `MarkGap()` 的位置；有第二个 mark 时，再增加第一个 mark 的高度和一个 `MarkGap()`。写入复合字形时，每个拼音部件的最终 Y 偏移是：

```text
dy = pinyinDY + localOffsetY * pinyinRatio
```

因此所有读音共享 `pinyinDY`，但下伸部、分音符和声调仍保留各自的局部关系。汉字本体和缩放标点共同使用 `baseDY`，避免两类字形的主体基线不一致。

## 水平布局

水平布局分为“逻辑组装”和“墨迹适配”两阶段。前者使用排版度量建立簇之间的关系，后者使用轮廓边界保证最终结果确实放得下。

### 1. 按源字体 advance 组装

逻辑游标 `cursor` 从 `0` 开始。每遇到一个 base：

1. 将 base 的 `OffsetX` 设为当前 `cursor`。
2. 从源字体读取该字形的 `hmtx.AdvanceWidth`。
3. 以该 advance cell 的中心定位 mark：

   ```text
   markOffsetX = cursor + baseAdvance / 2
                 - (markXMin + markXMax) / 2
   ```

4. `cursor += baseAdvance`。

mark 不推进游标。预组拼音字形和由 base + mark 组合出的拼音簇都遵循同一个规则。若 base 没有有效横向度量、advance 为 `0`，或者偏移不能安全表示，当前读音合成失败。

旧算法使用“轮廓宽度 + 全字体 `head` X 跨度的 10%”作为字间距。`head` X 跨度会被任意无关宽字形影响，也无法表达源 Latin 字体设计好的 side bearing；新算法因此完全移除了这项全局 tracking。

### 2. 求完整墨迹并集

逻辑组装完成后，每个部件提供：

```text
XMin, XMax, OffsetX
```

未缩放的拼音墨迹并集为：

```text
inkMin   = min(component.XMin + component.OffsetX)
inkMax   = max(component.XMax + component.OffsetX)
inkWidth = inkMax - inkMin
```

集合包含所有 base 和 mark。这样即使声调符号比 base 更宽，或生成的回退部件带有不同 side bearing，也不会漏算溢出。

### 3. 计算每个读音的 X 缩放

设汉字 advance 为 `A`，配置的拼音缩放上限为 `R = 0.35`，理想缩放为：

```text
scaleCap = min(R, A / inkWidth)
```

算法将它向下转换为 TrueType F2DOT14：

```text
scaleX = floor(scaleCap * 16384)
scaleY = floor(R * 16384)   // 当前 R 恰由强制转换编码
```

因此：

- `inkWidth * R <= A` 时，`scaleX == scaleY`，短拼音保持等比缩放。
- 否则只有 `scaleX` 变小，长拼音被水平压缩，垂直高度和公共基线不变。

所有拼音部件共享同一个 `scaleX/scaleY`。这保证声调不会相对字母单独变形，也避免不同簇得到不同压缩率。

### 4. 使用编码后的几何结果校验并居中

浮点公式只能给出候选值。最终字体使用 F2DOT14 和整数偏移，量化后可能多出一个设计单位，因此实现会按实际编码方式重新计算：

```text
scaledOffset[i] = round(OffsetX[i] * scaleX / 16384)

scaledMin = min(XMin[i] * scaleX / 16384 + scaledOffset[i])
scaledMax = max(XMax[i] * scaleX / 16384 + scaledOffset[i])
```

若 `scaledMax - scaledMin > A`，则将 F2DOT14 的 `scaleX` 减 `1` 后重试，直到结果装入 advance cell 或缩放归零。

适配成功后，目标左边界和整体平移为：

```text
targetMin = (A - (scaledMax - scaledMin)) / 2
shift     = targetMin - scaledMin
finalOffset[i] = scaledOffset[i] + shift
```

整数除法会让奇数余量自然落在右侧，所以中心和可能相差一个设计单位。最终保证的是：

```text
0 <= finalInkMin
finalInkMax <= A
finalInkMin + finalInkMax ≈ A
```

居中基于汉字 advance cell，而不是汉字 `XMin/XMax` 的中心。两个 advance 相同但 side bearing 或轮廓形状不同的汉字，会得到相同的拼音水平布局。

## TrueType 复合字形编码

每个拼音部件通过 `OpenType_GlyphComponent` 写入：

- X、Y 缩放相同时使用 `WE_HAVE_A_SCALE`。
- X、Y 缩放不同时使用 `WE_HAVE_AN_X_AND_Y_SCALE`。
- 偏移使用 `UNSCALED_COMPONENT_OFFSET`，即 `Arg1/Arg2` 不再随组件变换二次缩放。
- 超出单字节范围的偏移使用 word 参数。

复合字形自身的包围盒使用与序列化组件相同的 F2DOT14 变换重新计算。汉字本体始终使用 `65% × 65%`，拼音使用每个读音解析出的 `scaleX × 35%`。

## 失败与回退

水平解析在以下情况下返回失败：

- 没有拼音部件；
- 汉字 advance 为 `0`；
- 缩放上限不为正数；
- 墨迹宽度不为正数；
- 部件 advance 缺失或为 `0`；
- 中间或最终组件偏移超出 `int16_t`；
- F2DOT14 缩放降到 `0` 仍无法得到可编码布局。

失败会沿既有的合成失败路径上报。默认读音无法合成时保留源汉字映射；某个多音字的备选读音失败时，只省略对应备选字形或选择规则，不破坏已经成功的默认映射。

垂直排版带本身不会作为裁剪框。即使 mark 超过 ascender，复合字形的实际包围盒仍按所有变换后的组件扩展。

## 不变量与测试覆盖

`pinyin_synthesis_test` 覆盖了这些核心不变量：

- 垂直带严格按 `OS/2 → hhea → head` 回退。
- 短读音保留 `35%` 等比缩放并居中。
- 长读音仅降低 X 缩放；宽 mark 也参与适配。
- 拼音墨迹左右均不越出 advance cell。
- 同一读音的所有拼音组件共享变换。
- 汉字轮廓不对称不会改变拼音中心。
- 相邻汉字共享拼音基线，双 mark 保持堆叠关系。
- 添加未映射的极端 X/Y 轮廓不会改变已生成组件的 X/Y 布局。
- 汉字本体与缩放标点共享垂直偏移。

修改布局代码时，至少应运行：

```bash
cmake --build build
ctest --test-dir build --output-on-failure
```

如果改动触及 F2DOT14 舍入、复合组件 flags 或边界计算，还应检查生成字体中拼音组件的实际变换与包围盒，而不能只比较浮点公式。

## 当前有意保留的取舍

- 不应用 kerning 或 OpenType shaping；簇间距仅来自各 base 的源 `AdvanceWidth`。
- 不为 advance cell 预留额外左右内边距，以减少长拼音的压缩程度。
- 不根据单个汉字 `YMax` 单独贴顶，否则连续文本中的拼音基线会跳动。
- 不修改源字体的行度量，也不改变生成字形的 advance。
- 字体需要重新生成才能获得新布局；CLI 和拼音数据库格式无需迁移。

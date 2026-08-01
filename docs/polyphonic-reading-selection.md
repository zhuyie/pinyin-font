# 多音字生成与读音选择

本文说明生成字体如何保存一个汉字的多个拼音读音，以及文本中的 `汉字@序号` 如何通过 OpenType GSUB 选择指定读音。拼音组件缺失时的回退见 [拼音组件缺失与字形回退](pinyin-component-fallback.md)，字形内部的位置计算见 [拼音布局算法](pinyin-layout.md)。

相关实现主要位于：

- `src/pinyin/pinyin_db.cpp`：读取、规范化并保存候选读音。
- `src/synthesis/pinyin_font_builder.cpp`：生成默认／备选合成字形与选择规则。
- `src/opentype/ot_font.cpp`：内存中的 ligature substitution 模型。
- `src/opentype/ot_font_writer.cpp`：序列化 GSUB `liga` feature。
- `src/opentype/ot_font_parser.cpp`：解析生成字体中的 ligature substitution。
- `tests/pinyin_synthesis_test.cpp`：候选生成、GSUB 结构与失败隔离测试。

## 用户协议

多音字选择语法为：

```text
汉字@序号
```

序号是拼音数据库中候选读音的**一基索引**，不是声调数字。当前支持 `@1` 到 `@4`。

例如数据库记录为：

```text
藏	85CF	cáng,zàng
```

则：

| 输入文本 | 可见结果 |
|---|---|
| `藏` | 默认显示第一项 `cáng` |
| `藏@1` | 显式选择第一项 `cáng`，隐藏 `@1` |
| `藏@2` | 选择第二项 `zàng`，隐藏 `@2` |
| `藏@4` | 没有对应规则，保留默认 `藏` 和可见的 `@4` |

候选顺序来自数据库，并且会直接影响已有文本的含义。因此同一个汉字的读音顺序属于兼容性约定，更新数据库时不应随意重排。

## 为什么使用 GSUB ligature

选择器需要满足三个条件：

1. 可以直接写在普通 Unicode 文本中；
2. 在支持的渲染器中整段消失，只留下指定读音的合成汉字；
3. 在不支持的渲染器中安全降级为可见文本，不破坏字符映射。

生成字体因此使用 OpenType GSUB Lookup Type 4（Ligature Substitution），把 cmap 映射后的三个 glyph 原子替换为一个 glyph：

```text
默认读音汉字 glyph + @ glyph + ASCII 数字 glyph
    → 指定读音汉字 glyph
```

规则挂在标准 `liga` feature 下，并同时暴露给 `DFLT` 和 `hani` script 的默认 language system。大多数正常启用标准连字的 shaping engine 可以直接处理，无需应用程序预处理字符串。

底层文本不会改变。复制、搜索、辅助功能和文本索引仍能看到原始的 `藏@2`；只有字形序列在排版阶段被替换。

## 数据库读取

`PinyinRecord` 固定保存四个读音槽位：

```cpp
std::wstring pinyin[4];
```

数据库每行的第三列使用逗号分隔。`PinyinDB::__extractPinyins` 按原顺序读取最多四项，超过四项的候选被忽略；每项随后执行预组拼音字符规范化。第一项必须非空，否则整条记录不进入数据库。

合成阶段从 `pinyin[0]` 开始连续计数，遇到第一个空槽即停止。因此有效候选应连续排列，数据库中不要写空的中间项，例如 `cáng,,zàng`。

数据库记录最终会按汉字 code point 排序，但单条记录内部的候选顺序保持不变。

## 字形生成流程

### 1. 保留源 cmap

构建开始时，`__retainSourceCmap` 复制源字体的全部有效字符映射。后续只有成功生成的默认拼音汉字会覆盖对应汉字映射。

这一步是失败安全的基础：备选读音没有 cmap 映射，默认读音或选择规则失败也不会删除源字符、`@` 或数字的原始映射。

### 2. 生成默认读音

对每条数据库记录，首先调用：

```text
__addPinyinGlyph(charcode, pinyin[0], readingIndex=0, mapped=true)
```

成功后得到 `defaultGlyphIndex`，并将目标汉字的 cmap 映射改到这个合成字形。这个字形包含第一项拼音和缩放后的源汉字本体。

默认读音失败时，该记录到此结束：

- 源汉字缺失时增加 `SourceHanMissing`；
- 拼音组件、dotless `i` 或布局失败时进入对应失败统计；
- 不生成任何备选字形和 GSUB 规则；
- 源 cmap 中的汉字映射保持不变。

### 3. 判断是否为多音字

只有连续的非空读音数至少为两个时，才进入选择器生成。单读音汉字即使写成 `汉字@1` 也没有规则，`@1` 会保持可见。

### 4. 检查选择器输入 glyph

每个候选索引都会检查：

```text
CharToGlyphIndex('@')
CharToGlyphIndex('1' + readingIndex)
```

缺少 `@` 时所有候选规则都无法生成；缺少某个数字时只省略对应序号。字体构建继续，并增加 `SelectorMissingInputOmissions`。

选择器复用源字体已有的 `@` 和 ASCII 数字 glyph，不生成替代字符，也不改变它们的 cmap 映射。

### 5. 生成备选读音

候选循环包含第一项：

- `readingIndex == 0`：直接复用 `defaultGlyphIndex`，不重复生成字形。
- `readingIndex > 0`：调用 `__addPinyinGlyph(..., mapped=false)` 生成内部备选合成字形。

备选字形与默认字形使用同一套组件回退和布局算法，但 `mapped=false`，因此不会获得 Unicode cmap 映射，只能由 GSUB 引用。名称中的 reading index 使用零基内部编号，例如默认字形后缀为 `py00`，第二候选为 `py01`；这与用户看到的一基 `@1/@2` 不同。

备选合成失败时，只增加 `AlternateSynthesisOmissions` 并跳过当前规则。默认读音、其他成功候选和整个字体构建不受影响。

### 6. 注册 ligature 规则

每个成功候选最终形成：

```text
Components = [defaultGlyphIndex, atGlyphIndex, digitGlyphIndex]
LigatureGlyph = selectedGlyphIndex
```

`@1` 的 `selectedGlyphIndex` 就是 `defaultGlyphIndex`。这条规则看似替换为自身，但会把后面的 `@` 和 `1` 一并消费，从而让显式第一候选与无标记文本显示一致。

`@2` 到 `@4` 指向各自成功生成的内部备选字形。

## cmap 与 GSUB 的执行顺序

理解选择器时，最重要的是区分字符和 glyph：

```text
字符序列：藏    @    2
             │    │    │
          cmap  cmap  cmap
             ↓    ↓    ↓
glyph 序列：默认拼音藏  @glyph  2glyph
             └───────────┬───────────┘
                         │ GSUB liga
                         ↓
                    zàng 合成 glyph
```

GSUB 输入的第一个组件不是源字体原来的“藏”glyph，而是 cmap 已经映射到的默认 `cáng` 合成 glyph。这让无选择器文本自然使用第一读音，同时让所有选择规则共享同一个覆盖入口。

备选 glyph 没有独立 Unicode 编码，也不应该通过 cmap 访问。它是生成字体中的内部排版结果。

## 内存规则模型

`OpenType_Font` 只保存当前项目需要的紧凑模型：

```text
OpenType_LigatureSubstitution {
    Components: vector<glyph ID>
    LigatureGlyph: replacement glyph ID
}
```

`AddLigatureSubstitution` 会拒绝：

- 少于两个或超过 `65535` 个组件；
- replacement 为 `.notdef`、越界或尚未加入字体；
- 任一输入组件为 `.notdef` 或越界；
- 输入组件序列与已有规则完全重复。

规则保持确定性排序：先按首 glyph ID，首 glyph 相同时长规则优先，再按完整组件序列排序。虽然当前选择器始终为三个组件，这个顺序也为以后可能重叠的 ligature 保留了最长匹配优先级。

## GSUB 序列化

只有至少一条 ligature rule 时，writer 才向 SFNT 表目录写入 `GSUB`。当前生成的是最小化的 GSUB 1.0 结构：

```text
GSUB 1.0
├─ ScriptList
│  ├─ DFLT ─┐
│  └─ hani ─┴─ DefaultLangSys → feature index 0
├─ FeatureList
│  └─ liga → lookup index 0
└─ LookupList
   └─ Lookup Type 4
      └─ LigatureSubst Format 1
         ├─ Coverage Format 1：各默认拼音汉字 glyph
         └─ LigatureSet：对应的 @1..@4 规则
```

writer 按首 glyph 分组。coverage glyph 和 ligature set 使用同一组升序键，从而保证索引一一对应；每条 Ligature 记录只序列化首 glyph 之后的 `@` 和数字，因为首 glyph 已由 coverage 表示。

所有 GSUB 内部相对偏移在写入 `uint16_t` 前检查不超过 `0xFFFF`，最终表大小也必须不超过 `65535` 字节。超过范围会返回写入错误，而不是生成截断或回绕的表。GSUB 会参与正常的表目录排序、padding 和 SFNT checksum 计算。

## 源字体 GSUB 的处理范围

字体构建时解析器显式跳过源 `GSUB`，writer 根据 `LigatureSubstitutions` 重新生成目标表。因此当前输出字体只包含项目生成的选择器 ligature，不会合并或保留源字体任意的 GSUB feature。

这是当前字体重建模型的既有限制，不应把多音字功能描述为通用 GSUB 保留或编辑器。如果未来需要保留源 shaping 行为，需要新增结构化合并和冲突处理，而不是简单拼接表字节。

## 渲染与降级行为

有效选择需要渲染器完成以下两步：

1. 正常 cmap 映射；
2. 对 `DFLT` 或 `hani` 启用标准 `liga` feature。

若渲染器不执行 OpenType shaping、显式关闭 `liga`，或在字符间引入 shaping run 边界，则不会发生替换。结果仍然安全：显示默认读音汉字和字面量 `@序号`。

以下情况也保持字面量选择器：

- 序号超过该汉字的候选数；
- 汉字只有一个读音；
- `@` 或数字没有紧跟在符合规则的汉字后面；
- 对应备选读音合成失败；
- 字体缺少所需的 `@` 或数字 glyph；
- 当前应用未启用标准 ligature。

普通位置上的 `@` 和数字完全不受影响。规则不会删除底层字符，也不会把 `@2` 解释成声调二声。

## 失败隔离

各层失败的影响范围如下：

| 失败点 | 结果 |
|---|---|
| 默认读音合成失败 | 保留源汉字 cmap；该汉字不生成任何选择规则 |
| 缺少 `@` 或某个数字 | 只省略受影响规则；继续构建 |
| 某个备选读音合成失败 | 保留默认读音；省略该备选及规则 |
| ligature 规则校验失败 | 不加入该规则；增加备选省略统计 |
| GSUB 偏移或总长度溢出 | writer 返回错误，不写出畸形 GSUB |
| 渲染器不应用 GSUB | 显示默认读音与字面量选择器 |

需要注意：备选字形在 ligature 注册之前生成。正常选择器输入不会触发注册失败，但如果内存模型拒绝规则，已追加的内部备选 glyph 可能不被引用；字体仍保持结构有效，只是该选择器不可用。

## 诊断统计

CLI 构建结果包含：

- `AlternateGlyphsGenerated`：成功生成的非默认读音字形数。
- `SelectorLigaturesGenerated`：成功注册的 `@1`–`@4` 规则数，包括复用默认字形的 `@1`。
- `SelectorMissingInputOmissions`：因缺少 `@` 或对应 ASCII 数字而省略的规则数。
- `AlternateSynthesisOmissions`：备选合成失败、ligature 注册失败或相关替换无法完成的次数。

`tools/font_tool` 的完整性检查还会解析 GSUB 并报告：

```text
ParsedLigatures
SelectorRules
OtherLigatures
InvalidReferences
LigatureReferencesOK
```

它用于确认所有输入和 replacement glyph ID 均为非零且落在生成字体范围内。

## 测试覆盖

`pinyin_synthesis_test` 使用包含多音字的合成字体夹具验证：

- `藏 cáng,zàng` 无标记时映射到第一读音；
- `藏@1` 指回默认 glyph 并消费选择器；
- `藏@2` 指向不同且有效的内部备选 glyph；
- `藏@4` 没有规则；
- 单读音汉字不生成 `@1`；
- `@` 和数字的 cmap 映射与源字体一致；
- 缺少选择器输入 glyph 时只计数并省略规则；
- 某个备选读音无法合成时，默认映射和其他规则仍然有效；
- `DFLT`、`hani`、`liga`、Type 4 lookup、coverage、ligature set 和相对偏移结构正确；
- GSUB 序列化后可重新解析为等价的内存规则；
- 重复输入序列被拒绝，规则按最长优先排序。

手工验证可打开 `tools/preview.html`，加载生成字体后比较：

```text
藏
藏@1
藏@2
藏@4
```

自动测试验证表结构和 glyph 引用，浏览器预览则验证实际 shaping engine 是否启用并执行 `liga`。

修改多音字生成或 GSUB writer 后，应运行：

```bash
cmake --build build
ctest --test-dir build --output-on-failure
```

如果更改 selector 语法、候选上限或 feature/script 注册，还应同步更新拼音数据库兼容约定、预览样例和 GSUB 字节级断言。

## 当前取舍

- 不根据词语上下文自动消歧；选择完全由作者显式指定。
- 候选索引按数据库顺序，不按声调或常用度重新排序。
- 最多保存和选择四个读音。
- 没有关闭备选字形生成的 CLI 选项。
- 依赖标准 `liga`，不保证在禁用 shaping 的环境中隐藏选择器。
- 底层文本始终保留 `@序号`，这是可降级协议的一部分。
- 当前不保留或合并源 GSUB。

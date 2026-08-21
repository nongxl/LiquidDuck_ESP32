# 纯色像素与渐变像素流体模式：需求与技术细节规格书

本规范书总结了在流体模拟硬件项目中“2: 纯色像素”与“3: 渐变像素”两个核心模式的产品需求与技术实现细节，以便于在其他项目（如前端 Web 页面、游戏引擎、单片机或着色器 Shader 中）进行快速移植。

---

## 一、 产品需求与定位 (Requirements)

这两种模式旨在将**连续的流体物理模拟**转化为**复古、科技感十足的“点阵 LED”或“像素艺术”视觉风格**。它们不仅提供了差异化的美学体验，还在技术上平衡了高负载解算与低端设备的渲染性能。

### 1. 模式 2：纯色像素 (Solid Color Pixels)
*   **视觉特征**：去除了流体的“厚度”和“密度”细节。流体粒子聚集的区域以**完全统一的亮色**方块或圆角矩形显示。
*   **产品体验**：模拟老式单色或多色 LED 像素屏、掌上游戏机或霓虹像素管。液滴只要存在，像素点即被“点亮”，边缘极为分明，具有极强的数字化节奏感和波普艺术风格。
*   **适用场景**：极简复古风、低分辨率显示硬件、需要极高反差和轮廓清晰度的界面。

### 2. 模式 3：渐变像素 (Gradient Pixels)
*   **视觉特征**：像素大小保持不变，但**像素的颜色深度和亮度会根据该区域的流体浓度（密度）进行动态渲染**。
*   **产品体验**：融合了“复古点阵”的形式美与“现代渐变”的丝滑感。在流体撞击、飞溅、汇聚时，像素的色彩会在冷暖、明暗之间闪烁过渡，可以更直观地体现出流体解算中的质量积压与动能消散。
*   **适用场景**：赛博朋克风、科幻 HUD 终端界面、需要表现“流体能量感”的交互视觉设计。

### 3. 多种主题调色盘 (Themes)
为了增强可玩性，两个模式均需要支持**多套预设的色块方案（Palette）**供用户切换：
1.  **黑客帝国绿 (Matrix Hacker)**: 极暗绿背景 + 荧光绿像素
2.  **霓虹赛博 (Cyberpunk)**: 深紫背景 + 霓虹粉与青色像素
3.  **经典琥珀 (Amber Terminal)**: 深褐背景 + 琥珀橙像素
4.  **复古黑白 (Classic Mono)**: 深灰背景 + 银灰与纯白像素
5.  **猩红警戒 (Blood Red)**: 暗红背景 + 鲜红与粉红像素
6.  **深海蔚蓝 (Deep Sea)**: 深渊黑蓝 + 冰蓝与青色像素
7.  **生化毒液 (Toxic Venom)**: 暗紫背景 + 荧光黄绿像素
8.  **奢华至臻 (Luxury Gold)**: 暗金背景 + 白金与纯金像素

---

## 二、 核心技术细节 (Technical Specification)

在实现层面上，像素模式并不是在普通的精细流体画面上套用一层像素滤镜（Mosaic Shader），而是**直接在流体物理解算层、网格初始化层以及渲染层进行了全方位的深度改造**。

### 1. 网格降阶与动态重构 (Resolution Reduction)
为了呈现完美的“颗粒感”，需要降低物理网格的水平和垂直分辨率。

*   **常规模式分辨率**：如 $24 \times 42$（画面精细，液滴连贯）。
*   **像素模式分辨率**：降阶为 $14 \times 24$（网格变粗，使每个像素在屏幕中占据显著的物理面积）。
*   **实现逻辑**：
    *   在切换到模式 2 或 3 时，**必须销毁原流体实例，并以低分辨率重新初始化流体解算器**（例如 C++ 中调用 `reinitFluid(PIXEL_WIDTH, PIXEL_HEIGHT)`）。
    *   网格分辨率降低不仅实现了像素美学，还将需要计算的网格总数从 1008 大幅缩减至 336，**将流理解算与渲染开销减少了近 67%**，使低功耗单片机（如 ESP32）能够腾出大量 CPU 算力来提升帧率。

### 2. 物理与解算器参数调优 (Physics Customization)
为了配合大颗粒像素带来的“沉重感”和“撞击感”，像素模式下的流体参数需要进行特殊调整：

*   **重力倍率调大 (PIXEL_GRAVITY = 24.0)**：
    常规流体重力为 `12.0`。像素模式下将重力拉高一倍，使粗颗粒像素的下落速度极快，撞击感和喷涌感更像实体“沙粒”或“水银”，能迅速对设备晃动作出激烈的视觉响应。
*   **压力迭代次数翻倍 (solver_quality iters = 16)**：
    常规解算迭代次数为 `8`。在高重力下，流体极易穿透边界或在狭小空间内发生解算崩溃。因此，像素模式下需要将压力求解器的内部迭代次数提升至 `16`，以维持在高重力冲击下的流体不可压缩性，避免“穿墙”和“乱飞”现象。
*   **飞溅比例调高 (FLIP_RATIO = 0.92)**：
    使用极高的 FLIP 比例，保持像素颗粒的“破碎感”与“飞溅性”，在撞击边界时更容易散落为单颗像素液滴。

### 3. 色彩映射与计算公式 (Color Mapping)

假设当前调色盘数组为 `p`，大小为 `sz`（一般为 7 个颜色，索引 `0` 为背景色，`1 ~ sz-1` 为流体颜色，渐变度由暗到明）。

#### 背景判定：
对于任一像素格 $(x, y)$，获取解算出的密度 `density`：
$$\text{If } density < 0.1 \implies \text{Color} = p[0] \quad (\text{背景色})$$

#### 模式 2（纯色像素）映射：
流体只要达到阈值，就以极其饱满的固定高亮色显示：
$$\text{Color} = p[sz - 2] \quad (\text{通常选择倒数第二个高亮色，例如 p[5]})$$

#### 模式 3（渐变像素）映射：
根据密度大小，将流体浓度线性映射到对应的颜色渐变索引中：
$$\text{factor} = \frac{density}{20.0}$$
$$\text{idx} = \text{clamp}\left( \lfloor factor \times (sz - 1) \rfloor, \ 1, \ sz - 1 \right)$$
$$\text{Color} = p[\text{idx}]$$
*   *注：密度范围上限常数设定为 20.0，若密度超过该值，则强制截断在调色盘的最大亮色上。*

### 4. 像素颗粒的“带间隙圆角”渲染 (Rounded Rect Grid Rendering)
像素艺术的灵魂在于**“网格缝隙”**与**“边缘防生硬”**。

```
常规模式 (RLE合并填充):         像素模式 (独立带间隙圆角):
+─────────────────────+       +─────────────────────+
|█████████████████████|       | ┌──┐ ┌──┐ ┌──┐ ┌──┐ |
|█████████████████████|  ───> | │██│ │██│ │██│ │██│ |
|█████████████████████|       | └──┘ └──┘ └──┘ └──┘ |
+─────────────────────+       +─────────────────────+
```

*   **去除 RLE 优化，独立绘制**：普通流体为了帧率，会将整列的相同颜色像素合并成一个大矩形进行快速绘制（行/列行程编码，RLE）。但像素模式**必须打破这种优化，逐个网格进行独立绘制**，以确保缝隙的完整性。
*   **计算边界并收缩间隙**：
    假设屏幕上某个网格单元 $(x, y)$ 的原始物理边界为：
    *   起始 X：`xs[x]`，结束 X：`xs[x+1]`，宽度：`bw = xs[x+1] - xs[x]`
    *   起始 Y：`ys[y]`，结束 Y：`ys[y+1]`，高度：`bh = ys[y+1] - ys[y]`
    
    实际渲染时的圆角矩形边界应向内收缩 $1$ 像素，即：
    *   **绘制起点**：`(xs[x] + 1, ys[y] + 1)`
    *   **绘制尺寸**：`(bw - 2, bh - 2)`
*   **圆角半径设定**：
    圆角半径推荐设为固定值 `2`（在小屏幕如 $135\times240$ 上）。微小的圆角可以在保留像素风方块骨架的同时，消除尖锐边缘，营造出宛如高端 LED 像素看板的“玻璃拟物感”。

---

## 三、 跨平台移植参考伪代码 (Porting Reference)

这里以一段标准的面向对象/多端通用结构，给出如何在您自己的其他项目中（例如 JS Canvas / Unity C# / Custom Shader）复刻此逻辑的代码参考：

```typescript
// 1. 定义调色盘结构
interface Palette {
    background: string;     // 背景色 (对应 p[0])
    shades: string[];       // 渐变色阶 (对应 p[1] ~ p[sz-1])
}

const CYBER_PALETTE: Palette = {
    background: "#050005",
    shades: ["#330033", "#660066", "#990099", "#ff00ff", "#00ffff", "#ffffff"]
};

// 2. 核心颜色计算函数
function getPixelColor(density: number, mode: number, palette: Palette): string {
    if (density < 0.1) {
        return palette.background; // 没水，画背景色
    }
    
    if (mode === 2) {
        // 模式 2: 纯色像素。返回稳定的高亮主色 (例如倒数第二个高亮色)
        return palette.shades[palette.shades.length - 2];
    } else {
        // 模式 3: 渐变像素。根据密度计算色彩阶段
        const maxDensity = 20.0;
        let factor = density / maxDensity;
        if (factor > 1.0) factor = 1.0;
        
        const maxIdx = palette.shades.length - 1;
        let idx = Math.floor(factor * maxIdx);
        
        // 约束在合法色阶内，不包含背景色
        if (idx < 0) idx = 0;
        if (idx > maxIdx) idx = maxIdx;
        
        return palette.shades[idx];
    }
}

// 3. 渲染主循环中的像素颗粒绘制
function drawPixelFluid(
    ctx: CanvasRenderingContext2D, 
    gridData: number[][], // gridData[col][row] 保存密度值
    cols: number, 
    rows: number, 
    screenWidth: number, 
    screenHeight: number,
    mode: number,
    palette: Palette
) {
    const cellW = screenWidth / cols;
    const cellH = screenHeight / rows;
    const borderRadius = 2; // 圆角半径
    
    // 清屏绘制背景
    ctx.fillStyle = palette.background;
    ctx.fillRect(0, 0, screenWidth, screenHeight);

    for (let x = 0; x < cols; x++) {
        for (let y = 0; y < rows; y++) {
            const density = gridData[x][y];
            
            // 只有存在流体时才进行渲染绘制，跳过背景格减少性能损耗
            if (density >= 0.1) {
                const color = getPixelColor(density, mode, palette);
                
                // 计算带间隙的绘制位置（向内收缩 1 像素，间隙为 2 像素）
                const rx = x * cellW + 1;
                const ry = y * cellH + 1;
                const rw = cellW - 2;
                const rh = cellH - 2;
                
                // 绘制带圆角像素块
                ctx.fillStyle = color;
                ctx.beginPath();
                ctx.roundRect(rx, ry, rw, rh, borderRadius);
                ctx.fill();
            }
        }
    }
}
```

---

## 四、 移植注意事项 (Gotchas)

1.  **网格宽高比的一致性**：
    降低分辨率后，一定要维持流体物理模拟的“虚拟世界宽高比”与屏幕真实的“物理宽高比”一致，否则流体粒子受力在像素点阵上会发生长宽方向的不均等拉伸。
2.  **帧率补偿**：
    由于像素物理的重力极高 (`24.0`)，如果物理更新的 `delta_time` 发生波动，流体会发生剧烈抖动。在移植到其他游戏引擎或前端时，建议使用**固定物理时间步长（Fixed Update）**进行物理计算，而**非固定渲染时间步长**。
3.  **圆角性能**：
    在某些极低端嵌入式设备（如没有硬加速的 GFX 库）上，绘制 `fillRoundRect` 可能会比 `fillRect` 慢数倍。如果移植后发现帧率下降，可考虑降级为普通的 `fillRect`（去掉圆角），仅保留 $1$ 像素缝隙，同样能获得极佳的点阵玻璃质感。

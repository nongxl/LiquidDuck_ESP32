#pragma once
#include <Arduino.h>

// ─────────────────────────────────────────────────────────────
//  LiquidDuck 核心配置文件 (保姆级说明版)
// ─────────────────────────────────────────────────────────────

/**
 * [1. 物理动力学常量]
 * 采用高性能物理模拟模型，解决了粘滞感与贴边问题。
 */
static constexpr float  GRAVITY_STRENGTH   = 800.0f;// 【重力加速度】 下落动力。范围 400~800，越大粒子下落加速越迅猛
static constexpr float  DUCK_GRAVITY_SCALE = 0.50f; // 【鸭子浮力因子】 范围 0.2~0.8。越小鸭子感觉越轻，容易被托浮在粒子堆上方
static constexpr float  DUCK_FOLLOW_ALPHA  = 0.88f; // 【鸭子随动权重】 范围 0.0~1.0。越大鸭子越容易被粒子流“卷走”，增加交互动态感
static constexpr float  DUCK_ROT_STIFFNESS = 0.6f;  // 【姿态回归系数】 范围 0.1~2.0。越大鸭子归位越快
static constexpr float  DUCK_ROT_DAMPING   = 0.32f; // 【姿态回归阻尼】 范围 0.05~0.4。越大摆动越不晃动，增加沉稳感


/**
 * [2. 空间网格优化参数 - 高性能碰撞解算核心 (FLIP)]
 */
static constexpr int    FLIP_GRID_W        = 22;    // 【FLIP 水平解算网格】调低此值 -> 网格变粗 -> 海洋球体积变大！
static constexpr int    FLIP_GRID_H        = 12;    // 【FLIP 垂直解算网格】调低此值 -> 网格变粗 -> 海洋球体积变大！
static constexpr float  FLUID_FILL_RATIO   = 0.68f;  // 【流体填充比例】 配合大球，适当调低比例
static constexpr int    FLIP_PUSH_ITERS    = 1;     // 【FLIP 粒子推离迭代】 调低为 1，让粒子在撞击时允许瞬间挤压，从而像水花一样喷射
static constexpr int    FLIP_PRESSURE_ITERS= 40;    // 【FLIP 压力迭代】 决定流体的“不可压缩性”。值越大流体越“硬”且体积保持越好，但更耗 CPU；值越低流体越像“海绵”会被重力压缩。12 是兼顾帧率与物理体积的推荐值。
static constexpr float  FLIP_RATIO         = 0.90f; // 【FLIP 混合因子】 过高(如0.98)会导致局部涡流(漩涡)，过低会粘稠。0.90 是极佳平衡点。

/**
 * [3. 视觉美学配置 - 晶莹霓虹晶体系统]
 * 采用多层折射纹理模拟实现的昂贵视觉感。
 */
static constexpr uint32_t COLOR_BG_BLUE    = 0x82C0E7; // 【背景色】 天空蓝
static constexpr uint32_t COLOR_BG_ORANGE  = 0xFF9F43; // 【背景色】 活力橙
static constexpr uint32_t COLOR_BG_GREEN   = 0x1DD1A1; // 【背景色】 海草绿
static constexpr uint32_t COLOR_BG_CYAN    = 0x48DBFB; // 【背景色】 晴空蔚蓝
static constexpr uint32_t COLOR_BG_PINK    = 0xFDA7DF; // 【背景色】 梦幻粉紫
static constexpr uint32_t COLOR_BG_MINT    = 0x55EFC4; // 【背景色】 清新薄荷
static constexpr uint32_t LONG_PRESS_MS    = 500;      // 【长按阈值】 毫秒
static constexpr float  SPEED_GLINT_THRESHOLD = 240.0f;// 【闪烁阈值】 限制切换到高光状态的速度门限。越小越灵敏，越大越沉稳。

// ─────────────────────────────────────────────────────────────
//  海洋球多主题系统配置 (支持蔚蓝水流配色与多种晶莹主题)
// ─────────────────────────────────────────────────────────────
struct BallTheme {
    const char* name;
    uint32_t    defaultBg;          // 推荐明亮背景色
    uint32_t    palette[8];         // 8 种小球阶梯/多色调色板
    bool        speedHighlightWhite;// 超速时是否使用纯白水花高光
};

// 霓虹晶体调色板 (极高亮度版)
static const uint32_t NEON_PALETTE[8] = {
    0x00FFFF, // 极光蓝 (Cyan)
    0x00FFAA, // 翡翠绿 (Jade)
    0x55AAFF, // 晶体蓝 (Sky)
    0xD400FF, // 灵动紫 (Purple)
    0xFF00FF, // 霓虹粉 (Magenta)
    0xFF5500, // 熔岩橙 (Orange)
    0xFFCC00, // 璀璨金 (Gold)
    0x00FF88  // 全息青 (Mint)
};

// 预设主题列表
static const BallTheme BALL_THEMES[] = {
    {
        "Ocean Water", // 【蔚蓝海洋水体】：深蓝到浪花白的连续梯度水色
        COLOR_BG_CYAN,
        {
            0x0A2DA5, // 深海静水
            0x216ADA, // 深蓝水流
            0x3989ED, // 蔚蓝水体
            0x63A8F3, // 清澈天蓝
            0x86C2F8, // 浅蓝波浪
            0xAED8FB, // 晶亮浪花
            0xD8EDFD, // 泛白水沫
            0xFFFFFF  // 纯白飞溅
        },
        true // 速度超限时激起纯白水花
    },

    {
        "Neon Crystal", // 【极光霓虹晶体】：多彩高饱和度晶莹小球
        COLOR_BG_BLUE,
        {
            0x00FFFF, 0x00FFAA, 0x55AAFF, 0xD400FF, 
            0xFF00FF, 0xFF5500, 0xFFCC00, 0x00FF88
        },
        false
    },
    {
        "Pastel Candy", // 【马卡龙糖果】：柔和梦幻多色糖果球
        COLOR_BG_PINK,
        {
            0xFF9AA2, 0xFFB7B2, 0xFFDAC1, 0xE2F0CB, 
            0xB5EAD7, 0xC7CEEA, 0xFFDFD3, 0xFFFFD8
        },
        false
    },
    {
        "Cyber Matrix", // 【赛博生化】：黑客绿与毒液青紫
        COLOR_BG_MINT,
        {
            0x00FF66, 0x39FF14, 0x00FFAA, 0x00E5FF, 
            0x76FF03, 0xCCFF00, 0x00FFCC, 0x1DE9B6
        },
        true
    },
    {
        "Sunlit Citrus", // 【阳光夏日】：活力金黄、甜橙与蜜桃明亮果汁色
        COLOR_BG_ORANGE,
        {
            0xFF8A5C, // 蜜桃甜橙
            0xFFA502, // 鲜榨暖橙
            0xFFC048, // 芒果黄
            0xFFD32A, // 阳光金黄
            0xFFE066, // 柠檬亮黄
            0xFFEE99, // 奶油香蕉
            0xFFF5CC, // 浅晶金光
            0xFFFFFF  // 耀眼白光
        },
        true // 速度超限时激起耀眼金白光芒
    }
};
static constexpr int BALL_THEME_COUNT = sizeof(BALL_THEMES) / sizeof(BALL_THEMES[0]);




/**
 * [4. 硬件响应与防抖参数]
 */
static constexpr float  IMU_LPF_ALPHA      = 0.92f; // 【IMU 灵敏度】 越接近 1.0 响应越快；越小感觉重心移动越平滑。
static constexpr float  IMU_DEADZONE       = 0.26f; // 【IMU 死区过滤】 消除设备静止时的轻微抖动，防沸腾。
static constexpr float  FIXED_DT           = 1.0f / 60.0f; // 【物理刷新率】 物理模拟的步长，建议锁定为 60FPS。
static constexpr int    SUB_STEPS          = 12;     // 【计算子步】 每一帧内重复解算的倍数。值越大，大规模堆叠时越稳定、越硬。

// 休眠机制配置
static constexpr float    SLEEP_IMU_THRESHOLD  = 0.05f;  // 【休眠静止阈值】加速度向量变化的判断阈值。低于此值视为静止
static constexpr float    WAKE_IMU_THRESHOLD   = 3.0f;   // 【唤醒晃动阈值】加速度向量变化的判断阈值。高于此值触发唤醒
static constexpr uint32_t SLEEP_TIMEOUT_MS     = 60000;  // 【休眠超时时间】静止多长时间后进入休眠（毫秒）

// 硬件定义
static constexpr int SCREEN_W = 240;
static constexpr int SCREEN_H = 135;


/**
 * [5. 交互与音效配置 - 注入物理交互之魂]
 */
static constexpr uint8_t SYSTEM_VOLUME     = 40;   // 【系统音量】 0~255
static constexpr uint8_t SYSTEM_BRIGHTNESS   = 60;   // 【屏幕亮度】 0~255
static constexpr float   EXPLOSION_FORCE   = 800.0f;// 【爆炸强度】
static constexpr float   EXPLOSION_RADIUS  = 100.0f; // 【爆炸半径】
//static constexpr float   COLLISION_THRESHOLD = 2.4f;// 【碰撞音阈值】
//static constexpr int     COLLISION_FREQ_BASE = 3000;// 【碰撞音频率】

/**
 * [6. 触觉反馈配置 - Hat Vibrator 震动模块 (仅 StickS3 有效)]
 * 采用"碰撞能量累积 + 低通滤波 + PWM 输出"三段式触觉引擎。
 * 效果：粒子碰撞越剧烈，震动越强烈；静止时平滑衰减至零。
 */
static constexpr int    VIBR_PIN           = 0;     // 【震动引脚】 Hat Vibrator Motor 控制线，对应 StickS3 Bus 的 G0
static constexpr int    VIBR_PWM_CHANNEL   = 2;     // 【PWM 通道】 ledc 通道号，避免与其他外设冲突
static constexpr int    VIBR_PWM_FREQ      = 10000; // 【PWM 频率】 Hz，建议 5000~20000
static constexpr int    VIBR_PWM_BITS      = 8;     // 【PWM 位深】 8 位 = 0~255

static constexpr float  VIBR_PARTICLE_W   = 1.0f;  // 【粒子碰撞权重】 粒子间碰撞对总能量的贡献倍率
static constexpr float  VIBR_BOUNDARY_W   = 2.2f;  // 【边界碰撞权重】 边界碰撞通常更剧烈，给予更高权重
static constexpr float  VIBR_ENERGY_LIMIT  = 400.0f; // 【能量上限】 调低此值能显著增加多个小球撞击时的“段落感”
static constexpr float  VIBR_V_THRESHOLD   = 60.0f;  // 【速度触发门限】 过滤平躺堆积时的微小压力，只有速度超过此值才算“撞击”

static constexpr float  VIBR_ENERGY_SCALE = 5.0f;    // 【核心修正】让强度能拉开差距 (sqrt(800)*8.5 ≈ 240)
static constexpr float  VIBR_LPF_ALPHA    = 0.85f;  // 【低通滤波系数】 保持高响应
static constexpr float  VIBR_DECAY        = 0.72f;  // 【冷却衰减系数】 保持高段落感
static constexpr float  VIBR_THRESHOLD    = 30.0f;  // 【最小触发阈值】 配合新量纲调高
static constexpr float  VIBR_MIN_PWM      = 20.0f;  // 【最小启动 PWM】 降低起始点，换取更大动态
static constexpr float  VIBR_MAX          = 80.0f;// 【最大震动强度】 限制 PWM 上限，保护电机 (0~255)
static constexpr uint32_t VIBR_UPDATE_MS  = 10;    // 【更新间隔】 ms，约 100Hz 更新一次 PWM 输出

// ─────────────────────────────────────────────────────────────
//  Bottle Physics Config (FloatingBottle)
// ─────────────────────────────────────────────────────────────
static constexpr int    FLUID_WIDTH_BOTTLE = 27;  
// 【模拟网格宽度】影响流体解算的水平分辨率。
// 建议范围：15 ~ 40。值越大水体越细腻，但会显著增加 CPU 压力和内存占用。
static constexpr int    FLUID_HEIGHT_BOTTLE = 48;  
// 【模拟网格高度】影响流体解算的垂直分辨率。
// 建议范围：30 ~ 60。StickS3 屏幕较长，建议高度设为宽度的 1.5 ~ 2 倍。
static constexpr float  FLUID_FILL_RATIO_BOTTLE = 0.45f; 
// 【初始填充比例】水箱中水的初始占比。
// 范围：0.0 (空) ~ 1.0 (满)。0.4 ~ 0.6 效果最像“半瓶水”的动态感。

static constexpr float  FLUID_FILL_RATIO_PIXEL = 0.60f; 
// 【像素模式初始填充比例】针对无分摊点累加机制，适当拉高至 0.60f，确保整体大水体水汪汪、极度饱满充沛，同时飞散的个体依然极其干爽！
static constexpr float  FLUID_GRAVITY_BOTTLE = 16.0f; 
// 【重力响应强度】模拟重力对水体的拉力。
// 建议范围：5.0 ~ 25.0。值越大水流越快、撞击感越强；值越小水体表现越粘稠、轻盈。

static constexpr float  FLIP_RATIO_BOTTLE = 0.94f; 
// 【海洋瓶物理混合因子】越高水流保留的惯性动能越多，浪花越澎湃。默认 0.94f。

static constexpr float  FLIP_RATIO_PIXEL  = 0.988f; 
// 【像素模式物理混合因子】针对 14x24 粗网格特意大幅拉高至 0.988f，能彻底打散因粗网格插值产生的厚重数字粘滞，使小水滴在晃动时呈现像珍珠或沙粒般独立喷射、漫天溅射然后完美聚拢的极客物理效果！

static constexpr float  PIXEL_SPARKLE_THRESHOLD = 0.38f; 
// 【像素水花凝聚绘制阈值】阻断双线性插值周围低比重分摊。
// 设定在 0.35f ~ 0.42f。较低(如0.1)会让单独飞散的单颗水珠在屏幕上画成4个格子(马赛克大团)；设定为 0.38f 可确保飞溅水滴以极具极客颗粒感的“单格子”或“双格子”完美清爽呈现！

static const uint32_t OCEAN_PALETTE[] = {
    0x010308, 0x001A33, 0x003366, 0x004D99, 0x0066CC, 0x0080FF, 0x4DCCFF
};
static constexpr int OCEAN_PALETTE_SIZE = sizeof(OCEAN_PALETTE) / sizeof(uint32_t);


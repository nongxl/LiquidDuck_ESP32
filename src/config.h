#pragma once
#include <Arduino.h>

// ─────────────────────────────────────────────────────────────
//  LiquidDuck 核心配置文件 (保姆级说明版)
// ─────────────────────────────────────────────────────────────

/**
 * [1. 物理动力学常量 - PBD 模型调优]
 * 采用 Position Based Dynamics 模型，解决了粘滞感与贴边问题。
 */
static constexpr int    NUM_PARTICLES      = 150;   // 【粒子数量】 建议 120~200。数量越少 FPS 越高，视觉上更像奢侈的独立晶体
static constexpr float  PARTICLE_RADIUS    = 4.0f;  // 【粒子半径】 碰撞球体大小。减小会使粒子显得更细碎
static constexpr float  DUCK_RADIUS        = 20.0f; // 【鸭子半径】 对应 40x40 资产，建议不要改动以保持碰撞对齐
static constexpr float  GRAVITY_STRENGTH   = 800.0f;// 【重力加速度】 下落动力。范围 400~800，越大粒子下落加速越迅猛
static constexpr float  DAMPING            = 0.992f;// 【运动阻尼】 PBD 模式极其敏感。0.998 为接近真空，0.90 为浓稠糖浆
static constexpr float  BOUNCE             = 0.98f; // 【反弹弹性】 0.0~1.0。粒子撞击边缘后的反弹力度，越高粒子的活跃跳跃感越强
static constexpr float  REPULSE_STIFFNESS  = 0.10f; // 【排斥刚度】 粒子抵抗重叠的强度。1.0 为刚体，0.5 具有弹性形感
static constexpr float  DUCK_GRAVITY_SCALE = 0.40f; // 【鸭子浮力因子】 范围 0.2~0.8。越小鸭子感觉越轻，容易被托浮在粒子堆上方
static constexpr float  DUCK_FOLLOW_ALPHA  = 0.88f; // 【鸭子随动权重】 范围 0.0~1.0。越大鸭子越容易被粒子流“卷走”，增加交互动态感
static constexpr float  DUCK_ROT_STIFFNESS = 0.4f;  // 【姿态回归系数】 范围 0.1~2.0。越大鸭子归位越快
static constexpr float  DUCK_ROT_DAMPING   = 0.32f; // 【姿态回归阻尼】 范围 0.05~0.4。越大摆动越不晃动，增加沉稳感

/**
 * [2. 空间网格优化参数 - 高性能碰撞解算核心]
 * 通过将屏幕划分为 2D 网格，仅对相邻格子的粒子进行碰撞检测，实现 O(N) 复杂度的极速运算。
 */
static constexpr float  CELL_SIZE          = 9.6f; // 【网格尺寸】 建议设为粒子直径的 1.2 倍。越小解算越精细但开销越大
static constexpr int    GRID_W             = 25;    // 【水平网格数】 算法公式：SCREEN_W / CELL_SIZE
static constexpr int    GRID_H             = 14;    // 【垂直网格数】 算法公式：SCREEN_H / CELL_SIZE
static constexpr int    MAX_PER_CELL       = 16;    // 【单格容量】 单个网格内最多容纳的粒子数。若频繁漏掉碰撞，请调大此值。

/**
 * [3. 视觉美学配置 - 晶莹霓虹晶体系统]
 * 采用多层折射纹理模拟实现的昂贵视觉感。
 */
static constexpr uint32_t COLOR_BG         = 0x00060E; // 【背景色】 极夜蓝，用于反衬晶体粒子的高饱和发光。
static constexpr float  SPEED_GLINT_THRESHOLD = 160.0f;// 【闪烁阈值】 限制切换到高光状态的速度门限。越小越灵敏，越大越沉稳。

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

/**
 * [4. 硬件响应与防抖参数]
 */
static constexpr float  IMU_LPF_ALPHA      = 0.92f; // 【IMU 灵敏度】 越接近 1.0 响应越快；越小感觉重心移动越平滑。
static constexpr float  FIXED_DT           = 1.0f / 60.0f; // 【物理刷新率】 物理模拟的步长，建议锁定为 60FPS。
static constexpr int    SUB_STEPS          = 4;     // 【计算子步】 每一帧内重复解算的倍数。值越大，大规模堆叠时越稳定、越硬。

// 硬件定义
static constexpr int SCREEN_W = 240;
static constexpr int SCREEN_H = 135;

/**
 * [5. 交互与音效配置 - 注入物理交互之魂]
 */
static constexpr uint8_t SYSTEM_VOLUME     = 255;   // 【系统音量】 0~255
static constexpr uint8_t SYSTEM_BRIGHTNESS   = 100;   // 【屏幕亮度】 0~255
static constexpr float   EXPLOSION_FORCE   = 600.0f;// 【爆炸强度】
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
static constexpr float  VIBR_BOUNDARY_W   = 2.5f;  // 【边界碰撞权重】 边界碰撞通常更剧烈，给予更高权重
static constexpr float  VIBR_ENERGY_LIMIT  = 800.0f; // 【能量上限】 调低此值能显著增加多个小球撞击时的“段落感”
static constexpr float  VIBR_V_THRESHOLD   = 40.0f;  // 【速度触发门限】 过滤平躺堆积时的微小压力，只有速度超过此值才算“撞击”

static constexpr float  VIBR_ENERGY_SCALE = 5.0f;    // 【核心修正】让强度能拉开差距 (sqrt(800)*8.5 ≈ 240)
static constexpr float  VIBR_LPF_ALPHA    = 0.85f;  // 【低通滤波系数】 保持高响应
static constexpr float  VIBR_DECAY        = 0.72f;  // 【冷却衰减系数】 保持高段落感
static constexpr float  VIBR_THRESHOLD    = 30.0f;  // 【最小触发阈值】 配合新量纲调高
static constexpr float  VIBR_MIN_PWM      = 20.0f;  // 【最小启动 PWM】 降低起始点，换取更大动态
static constexpr float  VIBR_MAX          = 180.0f;// 【最大震动强度】 限制 PWM 上限，保护电机 (0~255)
static constexpr uint32_t VIBR_UPDATE_MS  = 16;    // 【更新间隔】 ms，约 60Hz 更新一次 PWM 输出

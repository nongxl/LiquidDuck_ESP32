/**
 * LiquidDuck — ESP32-S3 (M5Cardputer)
 * 旋转互动版：实现鸭子不倒翁动态效果
 */

#include <M5Cardputer.h>
#include <vector>
#include <cmath>
#include "config.h"      
#include "character_assets.h"
#include "flip.h"


// ─────────────────────────────────────────────────────────────
//  物理模式与数据结构
// ─────────────────────────────────────────────────────────────
enum PhysicsMode { MODE_FLIP, MODE_BOTTLE };
static PhysicsMode currentMode = MODE_FLIP;
static int currentCharacterIndex = 0; // 当前角色索引
static uint32_t currentBgColor = COLOR_BG_BLUE; // 当前背景色


// ─────────────────────────────────────────────────────────────
//  数据结构
// ─────────────────────────────────────────────────────────────
struct Duck {
    float x, y;
    float lastX, lastY;
    float vx, vy;
    float angle;    // 当前角度 (度)
    float aVel;     // 角速度
};

static FlipFluid* fluid = nullptr;
struct Cloud {
    float x, y;
    float vx, vy;
    float size;
};
static Cloud clouds[2];
static Duck  duck;
static float imuGX = 0.0f, imuGY = 0.0f;

static inline uint16_t rgb32to16(uint32_t c) {
    uint8_t r = (c >> 16) & 0xFF, g = (c >> 8) & 0xFF, b = (c) & 0xFF;
    return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
}

void initClouds() {
    for (int i = 0; i < 2; i++) {
        clouds[i].x = random(SCREEN_W);
        clouds[i].y = random(SCREEN_H);
        clouds[i].vx = 0;
        clouds[i].vy = 0;
        // 分化云朵大小：第一朵较大，第二朵更小
        if (i == 0) clouds[i].size = 18.0f + random(7); // 大云 (18-25)
        else        clouds[i].size = 12.0f + random(4);  // 小云 (12-16)
    }
}

void updateClouds(float dt) {
    // 与重力方向 (imuGX, imuGY) 相反的力
    float ax = -imuGX * 0.15f; 
    float ay = -imuGY * 0.15f;
    
    for (int i = 0; i < 2; i++) {
        clouds[i].vx += ax * dt;
        clouds[i].vy += ay * dt;
        clouds[i].vx *= 0.98f; // 阻尼
        clouds[i].vy *= 0.98f;
        
        clouds[i].x += clouds[i].vx * dt;
        clouds[i].y += clouds[i].vy * dt;
        
        // 边界环绕
        float margin = clouds[i].size * 2.0f;
        int cw = (currentMode == MODE_BOTTLE) ? 135 : 240;
        int ch = (currentMode == MODE_BOTTLE) ? 240 : 135;
        
        // 彻底取消所有轴的环绕逻辑，改为全反弹，确保横竖屏下都不会“穿越”
        if (clouds[i].x < margin / 2.0f) {
            clouds[i].x = margin / 2.0f;
            clouds[i].vx *= -0.5f;
        } else if (clouds[i].x > cw - margin / 2.0f) {
            clouds[i].x = cw - margin / 2.0f;
            clouds[i].vx *= -0.5f;
        }

        if (clouds[i].y < margin / 2.0f) {
            clouds[i].y = margin / 2.0f;
            clouds[i].vy *= -0.5f; 
        } else if (clouds[i].y > ch - margin / 2.0f) {
            clouds[i].y = ch - margin / 2.0f;
            clouds[i].vy *= -0.5f;
        }
    }

    // 云朵间的碰撞处理（防止重叠）
    float dx = clouds[1].x - clouds[0].x;
    float dy = clouds[1].y - clouds[0].y;
    float d2 = dx*dx + dy*dy;
    float minDist = (clouds[0].size + clouds[1].size) * 1.2f; 
    if (d2 < minDist * minDist) {
        float d = sqrtf(d2); if (d < 0.1f) d = 0.1f;
        float overlap = (minDist - d) * 0.5f;
        float nx = dx / d, ny = dy / d;
        clouds[0].x -= nx * overlap; clouds[0].y -= ny * overlap;
        clouds[1].x += nx * overlap; clouds[1].y += ny * overlap;
        
        // 简单的弹性碰撞：交换部分动量
        float rvx = clouds[1].vx - clouds[0].vx;
        float rvy = clouds[1].vy - clouds[0].vy;
        float velNormal = rvx * nx + rvy * ny;
        if (velNormal < 0) {
            float bounce = velNormal * 0.5f;
            clouds[0].vx += nx * bounce; clouds[0].vy += ny * bounce;
            clouds[1].vx -= nx * bounce; clouds[1].vy -= ny * bounce;
        }
    }
}

void drawClouds(M5Canvas* cv) {
    float rad = duck.angle * (M_PI / 180.0f);
    float s_r = sinf(rad);
    float c_r = cosf(rad);

    for (int i = 0; i < 2; i++) {
        uint16_t color = rgb32to16(0xFFFFFF); // 纯白色，在浅蓝背景下更明亮
        float s = clouds[i].size;
        float cx = clouds[i].x;
        float cy = clouds[i].y;

        // 主圆
        cv->fillCircle(cx, cy, s, color);

        // 旋转两个侧翼圆的偏移量
        // 原始偏移: 左(-s*0.7, s*0.2), 右(s*0.7, s*0.2)
        float offX1 = -s * 0.7f, offY1 = s * 0.2f;
        float offX2 =  s * 0.7f, offY2 = s * 0.2f;

        cv->fillCircle(cx + (offX1 * c_r - offY1 * s_r), cy + (offX1 * s_r + offY1 * c_r), s * 0.8f, color);
        cv->fillCircle(cx + (offX2 * c_r - offY2 * s_r), cy + (offX2 * s_r + offY2 * c_r), s * 0.8f, color);
    }
}

static float flip_h = 0.0f;
static float flip_tank_w = 0.0f;
static float flip_tank_h = 0.0f;
static float flip_scale_x = 1.0f;
static float flip_scale_y = 1.0f;

static inline float sim_to_screen_x(float sim_x) { return (sim_x - flip_h) * flip_scale_x; }
static inline float sim_to_screen_y(float sim_y) { return (sim_y - flip_h) * flip_scale_y; }
static inline float screen_to_sim_x(float sc_x) { return (sc_x / flip_scale_x) + flip_h; }
static inline float screen_to_sim_y(float sc_y) { return (sc_y / flip_scale_y) + flip_h; }



static FlipFluid* bottleFluid = nullptr;
static float* bottleGrid = nullptr;

static uint32_t getFluidColor(float density) {
    if (density < 0.1f) return OCEAN_PALETTE[0];
    float factor = density / 20.0f;
    int idx = (int)(factor * (OCEAN_PALETTE_SIZE - 1));
    if (idx < 1) idx = 1;
    if (idx >= OCEAN_PALETTE_SIZE) idx = OCEAN_PALETTE_SIZE - 1;
    return OCEAN_PALETTE[idx];
}

static std::vector<uint8_t> particleColors;

// ─── 震动引擎全局状态 ───────────────────────────────────────────
static float    frameEnergy = 0.0f;  // 当前帧累积的碰撞能量
static float    vibrLevel   = 0.0f;  // 经低通滤波后的平滑震动强度输出

static M5Canvas canvas(&M5Cardputer.Display);
static M5Canvas duckSprite(&canvas);
static M5Canvas ballSprites[2][8];

// 更新角色精灵图
void updateCharacterSprite() {
    const CharacterAsset& asset = CHARACTER_REGISTRY[currentCharacterIndex];
    duckSprite.deleteSprite();
    duckSprite.createSprite(asset.width, asset.height);
    duckSprite.setPivot(asset.width / 2, asset.height / 2);
    duckSprite.fillSprite(0x0001); // 透明色索引
    for (int i = 0; i < (asset.width * asset.height); i++) {
        if (asset.pixels[i] != 0x000000) {
            duckSprite.drawPixel(i % asset.width, i / asset.width, asset.pixels[i]);
        }
    }
}

// 随机切换主题（背景色与角色同时随机变换）
void switchTheme() {
    // 1. 随机背景色 (确保变化)
    static const uint32_t bgColors[] = {COLOR_BG_BLUE, COLOR_BG_ORANGE, COLOR_BG_GREEN, COLOR_BG_DARKBLUE};
    uint32_t oldColor = currentBgColor;
    while (currentBgColor == oldColor) {
        currentBgColor = bgColors[random(4)];
    }
    
    // 2. 随机角色 (确保变化)
    int oldIndex = currentCharacterIndex;
    while (currentCharacterIndex == oldIndex) {
        currentCharacterIndex = random(CHARACTER_COUNT);
    }
    updateCharacterSprite();
    
    Serial.printf("Theme Switched: Color=0x%06X, Character=%s\n", 
                  currentBgColor, CHARACTER_REGISTRY[currentCharacterIndex].name);
}

static uint32_t fpsLastMs = 0;
static int      fpsCount = 0, fpsDisplay = 0;

static const std::vector<int> C_MAJOR_SCALE = {60, 62, 64, 65, 67, 69, 71};

// ─────────────────────────────────────────────────────────────
//  辅助渲染工具
// ─────────────────────────────────────────────────────────────

static uint16_t getBrightened16(uint32_t c, float f) {
    uint8_t r = (uint8_t)min(255.0f, ((c >> 16) & 0xFF) * f);
    uint8_t g = (uint8_t)min(255.0f, ((c >> 8) & 0xFF) * f);
    uint8_t b = (uint8_t)min(255.0f, (c & 0xFF) * f);
    return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
}

// ─────────────────────────────────────────────────────────────
//  音效生成逻辑
// ─────────────────────────────────────────────────────────────
static void playKeyTone() {
    if (SYSTEM_VOLUME == 0) return;
    int midi = C_MAJOR_SCALE[random(C_MAJOR_SCALE.size())] + 12; 
    double freq = 440.0 * pow(2.0, (midi - 69) / 12.0);
    float durationSec = 0.04f; 
    int sampleRate = 44100;
    int sampleCount = (int)(sampleRate * durationSec);
    std::vector<int16_t> buffer(sampleCount * 2);
    float amplitude = 32767.0f * 0.85f; 
    for (int i = 0; i < sampleCount; i++) {
        float amp = amplitude;
        if (i > sampleCount - 200) amp *= (float)(sampleCount - i) / 200.0f;
        int16_t val = (int16_t)(amp * sin(2.0 * M_PI * freq * i / sampleRate));
        buffer[i * 2] = val; buffer[i * 2 + 1] = val;
    }
    M5.Speaker.playRaw(buffer.data(), buffer.size(), sampleRate, true, 1, 0);
}

static void playCollisionSound(float energy) {
    if (SYSTEM_VOLUME == 0) return;
    // 根据能量动态决定频率 (1200Hz ~ 2200Hz)
    float ratio = fminf(1.0f, sqrtf(energy) / sqrtf(VIBR_ENERGY_LIMIT));
    double freq = 1200.0 + (ratio * 1000.0);
    float durationSec = 0.015f; 
    int sampleRate = 44100;
    int sampleCount = (int)(sampleRate * durationSec);
    
    static int16_t colBuffer[800 * 2]; // 静态缓冲区减少分配开销
    int realCount = min(sampleCount, 800);
    float amplitude = 32767.0f * (0.3f + ratio * 0.5f); // 撞得越重越响
    
    for (int i = 0; i < realCount; i++) {
        // 极快的线性衰减包络
        float env = (float)(realCount - i) / realCount;
        int16_t val = (int16_t)(amplitude * env * sin(2.0 * M_PI * freq * i / sampleRate));
        colBuffer[i * 2] = val; colBuffer[i * 2 + 1] = val;
    }
    // 播放到通道 1
    M5.Speaker.playRaw(colBuffer, realCount * 2, sampleRate, true, 1, 1);
}

// ─────────────────────────────────────────────────────────────
//  物理核心
// ─────────────────────────────────────────────────────────────
inline float distSq(float dx, float dy) { return dx * dx + dy * dy; }




static void physicsStepBOTTLE(float dt) {
    if (!bottleFluid) return;
    float ax, ay, az;
    M5.Imu.getAccel(&ax, &ay, &az);
    float gx = -ay;
    float gy = -ax;
    if (fabsf(gx) < IMU_DEADZONE) gx = 0;
    if (fabsf(gy) < IMU_DEADZONE) gy = 0;

    float sub_dt = dt * 0.5f;
    flip_step(bottleFluid, sub_dt, gx, gy);
    flip_step(bottleFluid, sub_dt, gx, gy);
    flip_get_led_grid(bottleFluid, bottleGrid, FLUID_WIDTH_BOTTLE, FLUID_HEIGHT_BOTTLE);
    
    float accelMag = sqrtf(ax*ax + ay*ay + az*az);
    float currentShake = fabsf(accelMag - 1.0f);
    if (currentShake > 0.5f) {
        frameEnergy = fmaxf(frameEnergy, currentShake * 100.0f);
    }
}

static void renderFrameBOTTLE() {
    canvas.fillSprite(rgb32to16(0x00060E)); // 恢复海水模式的深色背景
    float cellW = 135.0f / FLUID_WIDTH_BOTTLE;
    float cellH = 240.0f / FLUID_HEIGHT_BOTTLE;
    for (int x = 0; x < FLUID_WIDTH_BOTTLE; x++) {
        for (int y = 0; y < FLUID_HEIGHT_BOTTLE; y++) {
            float density = bottleGrid[x * FLUID_HEIGHT_BOTTLE + y];
            if (density > 0.1f) {
                uint32_t color = getFluidColor(density);
                canvas.fillRect(lrintf(x * cellW), lrintf(y * cellH), (int)ceilf(cellW), (int)ceilf(cellH), rgb32to16(color));
            }
        }
    }
    canvas.pushSprite(0, 0);
}

static void triggerExplosionFLIP() {
    if (!fluid) return;
    int num; float *pos, *vel;
    flip_get_particles(fluid, &num, &pos, &vel);
    if (num <= 0) return;
    int target = random(num);
    float ex = pos[2*target+0]; float ey = pos[2*target+1];
    float rSq = EXPLOSION_RADIUS * EXPLOSION_RADIUS;
    for (int i = 0; i < num; i++) {
        float dx = pos[2*i+0] - ex; float dy = pos[2*i+1] - ey;
        float d2 = dx*dx + dy*dy;
        if (d2 < rSq && d2 > 0.01f) {
            float d = sqrtf(d2); float f = (EXPLOSION_RADIUS - d) / EXPLOSION_RADIUS * EXPLOSION_FORCE;
            vel[2*i+0] += (dx/d)*f; vel[2*i+1] += (dy/d)*f;
        }
    }
    duck.vx += (duck.x - ex) * 2.0f; duck.vy += (duck.y - ey) * 2.0f; 
    duck.aVel += (random(60) - 30); // 爆炸时给一个随机角速度
    playKeyTone();
}

static void renderFrameFLIP() {
    canvas.fillSprite(rgb32to16(currentBgColor));
    drawClouds(&canvas); // 恢复海洋球模式下的云朵
    const float glintIdxSq = SPEED_GLINT_THRESHOLD * SPEED_GLINT_THRESHOLD;
    
    if (fluid) {
        int num; float *pos, *vel;
        flip_get_particles(fluid, &num, &pos, &vel);
        float pRadius = flip_get_particle_radius(fluid);
        for (int i = 0; i < num; i++) {
            float vx = vel[2*i+0], vy = vel[2*i+1];
            float v2 = vx*vx + vy*vy;
            uint8_t cIdx = (i < particleColors.size()) ? particleColors[i] : 0;
            float px = sim_to_screen_x(pos[2*i+0]);
            float py = sim_to_screen_y(pos[2*i+1]);
            ballSprites[(v2 > glintIdxSq) ? 1 : 0][cIdx].pushSprite(&canvas, (int)(px - pRadius), (int)(py - pRadius), 0x0001);
        }
    }
    // 渲染旋转的鸭子 (使用 pushRotateZoom，以 x,y 为旋转中心)
    float dx = sim_to_screen_x(duck.x);
    float dy = sim_to_screen_y(duck.y);
    duckSprite.pushRotateZoom(&canvas, dx, dy, duck.angle, 1.0f, 1.0f, 0x0001);
    
    //canvas.setTextColor(TFT_DARKGREY); canvas.setCursor(SCREEN_W - 18, 2); canvas.printf("%d", fpsDisplay);
    canvas.pushSprite(0, 0);
}

extern float flip_boundary_energy;

static void physicsStepFLIP(float dt) {
    if (!fluid) return;

    flip_step(fluid, dt, imuGX, imuGY);

    if (flip_boundary_energy > VIBR_V_THRESHOLD) {
        frameEnergy = fmaxf(frameEnergy, flip_boundary_energy * flip_boundary_energy * VIBR_BOUNDARY_W);
    }
    flip_boundary_energy = 0.0f;

    duck.vx += imuGX * DUCK_GRAVITY_SCALE * dt;
    duck.vy += imuGY * DUCK_GRAVITY_SCALE * dt;
    duck.lastX = duck.x; duck.lastY = duck.y;
    duck.x += duck.vx * dt; duck.y += duck.vy * dt;

    int num; float *pos, *vel;
    flip_get_particles(fluid, &num, &pos, &vel);
    float pRadius = flip_get_particle_radius(fluid);

    const float charRadius = CHARACTER_REGISTRY[currentCharacterIndex].radius;
    const float duckMinDist = charRadius + pRadius;
    const float duckMinDistSq = duckMinDist * duckMinDist;
    
    for (int i = 0; i < num; i++) {
        float dx = pos[2*i+0] - duck.x, dy = pos[2*i+1] - duck.y;
        float d2 = distSq(dx, dy);
        if (d2 < duckMinDistSq) {
            float d = sqrtf(d2); if (d < 0.01f) d = 0.1f;
            float overlap = (duckMinDist - d);
            float nx = dx / d, ny = dy / d;
            pos[2*i+0] += nx * overlap * 0.95f;
            pos[2*i+1] += ny * overlap * 0.95f;
            
            // ✨ 核心注入：当鸭子排斥海洋球时，附加一个弹射速度，彻底激活“撞击溅射感”！
            vel[2*i+0] += nx * overlap * 25.0f;
            vel[2*i+1] += ny * overlap * 25.0f;
            
            duck.x -= nx * overlap * 0.05f;
            duck.y -= ny * overlap * 0.05f;
            if (overlap > 0.1f) {
                frameEnergy += overlap * overlap * VIBR_PARTICLE_W;
            }
        }
    }

    // 鸭子的物理边界必须和 FLIP 流体的物理边界保持一致，否则会“卡”在流体底部的 padding 中无法上浮。
    // 在 FLIP 中，流体被限制在距离边缘 h (也就是 pRadius / 0.35) 的范围内。
    float duckMinX = flip_h + charRadius;
    float duckMaxX = flip_tank_w - charRadius;
    float duckMinY = flip_h + charRadius;
    float duckMaxY = flip_tank_h - charRadius;

    if (duck.x < duckMinX) { duck.x = duckMinX; duck.vx *= -0.5f; }
    else if (duck.x > duckMaxX) { duck.x = duckMaxX; duck.vx *= -0.5f; }
    if (duck.y < duckMinY) { duck.y = duckMinY; duck.vy *= -0.5f; }
    else if (duck.y > duckMaxY) { duck.y = duckMaxY; duck.vy *= -0.5f; }
    


    float invDt = 1.0f / dt;
    float dnvx = (duck.x - duck.lastX) * invDt;
    float dnvy = (duck.y - duck.lastY) * invDt;
    duck.vx = dnvx * 0.98f; 
    duck.vy = dnvy * 0.98f;
}

// ─────────────────────────────────────────────────────────────
//  震动引擎
// ─────────────────────────────────────────────────────────────
static void initVibrator() {
    // 关键：先强制拉低引脚，防止 GPIO 浮空导致上电即震动
    pinMode(VIBR_PIN, OUTPUT);
    digitalWrite(VIBR_PIN, LOW);
    // 再挂载 PWM 通道
    ledcSetup(VIBR_PWM_CHANNEL, VIBR_PWM_FREQ, VIBR_PWM_BITS);
    ledcAttachPin(VIBR_PIN, VIBR_PWM_CHANNEL);
    ledcWrite(VIBR_PWM_CHANNEL, 0); // 确保 PWM 输出为零
}

static void updateVibrator() {
    static uint32_t lastVibrMs = 0;
    uint32_t now = millis();
    if (now - lastVibrMs < VIBR_UPDATE_MS) return;
    lastVibrMs = now;

    float target = 0.0f;
    float currentImpact = 0.0f;
    if (frameEnergy > 0.0f) {
        // 应用单帧能量上限，防止大量粒子堆叠导致震动过载
        currentImpact = fminf(frameEnergy, VIBR_ENERGY_LIMIT);
        float limitedEnergy = currentImpact; // 保持对后续计算的兼容
        
        // 非线性映射：sqrt 压缩
        target = sqrtf(limitedEnergy) * VIBR_ENERGY_SCALE;
        target = fminf(target, VIBR_MAX);
        // 低通滤波平滑
        vibrLevel = vibrLevel * (1.0f - VIBR_LPF_ALPHA) + target * VIBR_LPF_ALPHA;
    } else {
        // 无碰撞时冷却衰减
        vibrLevel *= VIBR_DECAY;
    }

    // 阈值过滤 + 最小启动 PWM：确保电机能开始转动
    uint8_t pwmOut = 0;
    if (vibrLevel > VIBR_THRESHOLD) {
        pwmOut = (uint8_t)fminf(fmaxf(vibrLevel, VIBR_MIN_PWM), VIBR_MAX);
        // [新增] 只有在这一帧确实有物理碰撞时才触发音效
        if (currentImpact > 0) playCollisionSound(currentImpact);
    }
    ledcWrite(VIBR_PWM_CHANNEL, pwmOut);

    frameEnergy = 0.0f; // 采集完成，清零等待下一帧
}

static void initParticlesFLIP() {
    if (fluid) flip_destroy(fluid);
    fluid = flip_create(SCREEN_W, SCREEN_H, FLIP_GRID_W, FLIP_GRID_H, FLUID_FILL_RATIO);
    if (fluid) {
        flip_set_gravity_scale(fluid, 1.0f);
        flip_set_solver_quality(fluid, FLIP_PUSH_ITERS, FLIP_PRESSURE_ITERS, FLIP_RATIO);
        
        int num;
        flip_get_particles(fluid, &num, nullptr, nullptr);
        particleColors.resize(num);
        for(int i=0; i<num; i++) {
            particleColors[i] = (uint8_t)random(8);
        }
    }
    
    int sim_num_x_d = FLIP_GRID_W + 2;
    int sim_num_y_d = FLIP_GRID_H + 2;
    float hx_d = (float)SCREEN_W / (sim_num_x_d - 1);
    float hy_d = (float)SCREEN_H / (sim_num_y_d - 1);
    flip_h = fminf(hx_d, hy_d);
    flip_tank_w = flip_h * (sim_num_x_d - 1);
    flip_tank_h = flip_h * (sim_num_y_d - 1);
    flip_scale_x = (float)SCREEN_W / (flip_tank_w - flip_h);
    flip_scale_y = (float)SCREEN_H / (flip_tank_h - flip_h);

    float startRadius = CHARACTER_REGISTRY[0].radius;
    duck.x = duck.lastX = screen_to_sim_x(SCREEN_W / 2.0f); 
    duck.y = duck.lastY = screen_to_sim_y(startRadius + 5.0f); 
    duck.vx = duck.vy = 0.0f; duck.angle = duck.aVel = 0.0f;
}

void setup() {
    // ★ 首要任务：在一切初始化之前，将 Hat2 总线所有可能的控制引脚全部强制拉低
    // 防止 GPIO 浮空导致振动模块上电即启动
    for (int pin : {0, 1, 7, 8}) {
        pinMode(pin, OUTPUT);
        digitalWrite(pin, LOW);
    }

    auto cfg = M5.config(); M5Cardputer.begin(cfg, true);
    M5Cardputer.Display.setRotation(1); M5Cardputer.Display.setBrightness(SYSTEM_BRIGHTNESS);
    M5.Speaker.begin(); M5.Speaker.setVolume(SYSTEM_VOLUME);
    canvas.createSprite(SCREEN_W, SCREEN_H); M5.Imu.init();
    
    // 初始化随机数种子，增加随机性
    randomSeed(millis() + M5.Imu.getAccel(&imuGX, &imuGY, &imuGX)); 
    
    updateCharacterSprite();
    Serial.printf("System Initialized. Character Count: %d\n", CHARACTER_COUNT);
    
    initClouds();
    initParticlesFLIP();
    float simW = 1.0f;
    float simH = simW * ((float)FLUID_HEIGHT_BOTTLE / (float)FLUID_WIDTH_BOTTLE);
    bottleFluid = flip_create(simW, simH, FLUID_WIDTH_BOTTLE, FLUID_HEIGHT_BOTTLE, FLUID_FILL_RATIO_BOTTLE);
    if (bottleFluid) {
        flip_set_gravity_scale(bottleFluid, FLUID_GRAVITY_BOTTLE);
        flip_set_solver_quality(bottleFluid, 1, 12, 0.9f);
    }
    bottleGrid = (float*)malloc(sizeof(float) * FLUID_WIDTH_BOTTLE * FLUID_HEIGHT_BOTTLE);

    
    float pRadius = flip_get_particle_radius(fluid);
    int dia = (int)(pRadius * 2.4f);
    if (dia < 2) dia = 2;

    for (int i = 0; i < 8; i++) {
        uint32_t c = NEON_PALETTE[i];
        for (int b = 0; b < 2; b++) {
            ballSprites[b][i].createSprite(dia, dia); ballSprites[b][i].fillSprite(0x0001);
            float f = (b == 0) ? 1.0f : 1.6f; 
            ballSprites[b][i].fillCircle(dia/2, dia/2, dia/2, getBrightened16(c, f * 0.7f));
            ballSprites[b][i].fillCircle(dia/2, dia/2, dia/2 - 1, getBrightened16(c, f * 1.15f));
            ballSprites[b][i].fillCircle(dia/2 - 1, dia/2 - 1, (b == 0 ? 1 : 2), TFT_WHITE);
        }
    }
    initVibrator();
    fpsLastMs = millis();
}

void loop() {
    M5.update(); // 统一使用 M5Unified 的更新逻辑
    static bool lastKbdPressed = false;
    
    // 兼容判定：Cardputer 键盘按下 OR 任何设备的 BtnA (StickS3 正面按键) 按下
    bool currentKbdPressed = M5Cardputer.Keyboard.isPressed();
    bool btnAPressed = M5.BtnA.wasPressed();
    
    if ((currentKbdPressed && !lastKbdPressed) || btnAPressed) {
        switch (currentMode) {
            case MODE_FLIP: triggerExplosionFLIP(); break;
            default: break;
        }
    }
    lastKbdPressed = currentKbdPressed;

    static uint32_t lastLoopMs = millis();
    uint32_t nowLoop = millis();
    float dt = (nowLoop - lastLoopMs) / 1000.0f;
    if (dt > 0.033f) dt = 0.033f; // 限制最大步长
    if (dt <= 0.0f) dt = 0.001f;
    lastLoopMs = nowLoop;

    // ─────────────────────────────────────────────────────────────
    //  按键交互逻辑 (BtnB: 短按切主题，长按切模式)
    // ─────────────────────────────────────────────────────────────
    static bool btnBLongPressed = false;
    
    // 处理按下过程中的长按判定
    if (M5.BtnB.isPressed()) {
        if (!btnBLongPressed && M5.BtnB.pressedFor(LONG_PRESS_MS)) {
            btnBLongPressed = true;
            // 长按：切换模式
            currentMode = (currentMode == MODE_FLIP) ? MODE_BOTTLE : MODE_FLIP;
            if (currentMode == MODE_BOTTLE) {
                M5Cardputer.Display.setRotation(0);
                canvas.deleteSprite();
                canvas.createSprite(135, 240);
            } else {
                M5Cardputer.Display.setRotation(1);
                if (canvas.width() != 240) {
                    canvas.deleteSprite();
                    canvas.createSprite(240, 135);
                }
            }
            duck.x = duck.lastX = screen_to_sim_x(240 / 2.0f);
            duck.y = duck.lastY = screen_to_sim_y(135 / 2.0f);
            duck.vx = duck.vy = 0.0f;
            frameEnergy = 0.0f;
            vibrLevel = 0.0f;
            playKeyTone(); // 给个声音反馈
            Serial.println("Mode Switched via Long Press");
        }
    }
    
    // 处理释放时的逻辑
    if (M5.BtnB.wasReleased()) {
        if (!btnBLongPressed) {
            // 短按：仅在海洋球模式下切换主题
            if (currentMode == MODE_FLIP) {
                switchTheme();
                playKeyTone();
            }
        }
        btnBLongPressed = false; // 重置长按标志
    }

    if (M5.Imu.isEnabled()) {
        M5.Imu.update(); float ax, ay, az; M5.Imu.getAccel(&ax, &ay, &az);
        
        // ── 休眠逻辑 ──────────────────────────────────────────────
        static bool isSleeping = false;
        static float lastAx = 0.0f, lastAy = 0.0f, lastAz = 0.0f;
        static uint32_t lastMotionMs = millis();

        float deltaA = sqrtf(powf(ax - lastAx, 2) + powf(ay - lastAy, 2) + powf(az - lastAz, 2));
        lastAx = ax; lastAy = ay; lastAz = az;

        bool isCharging = M5.Power.isCharging();
        bool hasInput = M5Cardputer.Keyboard.isPressed() || M5.BtnA.wasPressed() || M5.BtnB.wasPressed();

        if (isSleeping) {
            if (deltaA > WAKE_IMU_THRESHOLD || hasInput) {
                isSleeping = false;
                M5Cardputer.Display.wakeup();
                M5Cardputer.Display.setBrightness(SYSTEM_BRIGHTNESS);
                lastMotionMs = millis();
            } else {
                delay(50);
                return;
            }
        } else {
            if (deltaA > SLEEP_IMU_THRESHOLD || hasInput || isCharging) {
                lastMotionMs = millis();
            }

            if (millis() - lastMotionMs > SLEEP_TIMEOUT_MS) {
                isSleeping = true;
                M5Cardputer.Display.setBrightness(0);
                M5Cardputer.Display.sleep();
                return;
            }
        }

        float rawX = -ax;
        float rawY = ay;
        
        // 【死区过滤】完全复刻原项目的静止防沸腾机制
        if (fabsf(rawX) < IMU_DEADZONE) rawX = 0.0f;
        if (fabsf(rawY) < IMU_DEADZONE) rawY = 0.0f;
        
        float targetGX = rawX * GRAVITY_STRENGTH;
        float targetGY = rawY * GRAVITY_STRENGTH;
        
        // 【低通滤波】恢复传感器的平滑过滤，防止微小噪点引发的持续抖动
        if (abs(targetGX - imuGX) > 1.5f) imuGX = imuGX * (1.0f - IMU_LPF_ALPHA) + targetGX * IMU_LPF_ALPHA;
        if (abs(targetGY - imuGY) > 1.5f) imuGY = imuGY * (1.0f - IMU_LPF_ALPHA) + targetGY * IMU_LPF_ALPHA;

        // 统一更新旋转角度，供小黄鸭和云朵同步使用
        float targetAngle = 0.0f;
        float mSq = imuGX * imuGX + imuGY * imuGY;
        if (mSq > 100.0f) { 
            targetAngle = atan2f(-imuGX, imuGY + 0.0001f) * (180.0f / M_PI);
        }
        
        float angleDiff = targetAngle - duck.angle;
        while (angleDiff > 180.0f) angleDiff -= 360.0f;
        while (angleDiff < -180.0f) angleDiff += 360.0f;

        float angleAcc = angleDiff * DUCK_ROT_STIFFNESS;
        duck.aVel += angleAcc;
        duck.aVel *= (1.0f - DUCK_ROT_DAMPING);
        duck.angle += duck.aVel;
    }
    
    updateClouds(dt);
    // 使用真实时间步长，确保不会因为降帧而变成“慢动作”
    if (currentMode == MODE_FLIP) {
        float flipDt = dt * 0.5f;
        physicsStepFLIP(flipDt);
        physicsStepFLIP(flipDt);
    } else {
        physicsStepBOTTLE(dt);
    }

    if (currentMode == MODE_FLIP) renderFrameFLIP();
    else renderFrameBOTTLE();
    updateVibrator(); // 震动引擎：每帧采集能量并更新 PWM 输出
    fpsCount++; uint32_t now = millis();
    if (now - fpsLastMs >= 1000) { fpsDisplay = fpsCount; fpsCount = 0; fpsLastMs = now; }
}

/**
 * LiquidDuck — ESP32-S3 (M5Cardputer)
 * 旋转互动版：实现鸭子不倒翁动态效果
 */

#include <M5Cardputer.h>
#include <vector>
#include <cmath>
#include "config.h"      
#include "duck_asset.h"

// ─────────────────────────────────────────────────────────────
//  数据结构
// ─────────────────────────────────────────────────────────────
struct Particle {
    float x, y;
    float lastX, lastY;
    float vx, vy;
    uint8_t colorIdx;
};

struct Duck {
    float x, y;
    float lastX, lastY;
    float vx, vy;
    float angle;    // 当前角度 (度)
    float aVel;     // 角速度
};

static Particle particles[NUM_PARTICLES];
static Duck     duck;
static int16_t  grid[GRID_W][GRID_H][MAX_PER_CELL];
static uint8_t  gridCount[GRID_W][GRID_H];
static float    imuGX = 0.0f, imuGY = 0.0f;

static M5Canvas canvas(&M5Cardputer.Display);
static M5Canvas duckSprite(&canvas);
static M5Canvas ballSprites[2][8];

static uint32_t fpsLastMs = 0;
static int      fpsCount = 0, fpsDisplay = 0;

static const std::vector<int> C_MAJOR_SCALE = {60, 62, 64, 65, 67, 69, 71};

// ─────────────────────────────────────────────────────────────
//  辅助渲染工具
// ─────────────────────────────────────────────────────────────
static inline uint16_t rgb32to16(uint32_t c) {
    uint8_t r = (c >> 16) & 0xFF, g = (c >> 8) & 0xFF, b = (c) & 0xFF;
    return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
}

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
    float amplitude = 32767.0f * 0.2f; 
    for (int i = 0; i < sampleCount; i++) {
        float amp = amplitude;
        if (i > sampleCount - 200) amp *= (float)(sampleCount - i) / 200.0f;
        int16_t val = (int16_t)(amp * sin(2.0 * M_PI * freq * i / sampleRate));
        buffer[i * 2] = val; buffer[i * 2 + 1] = val;
    }
    M5.Speaker.playRaw(buffer.data(), buffer.size(), sampleRate, true, 1, 0);
}

// ─────────────────────────────────────────────────────────────
//  物理核心
// ─────────────────────────────────────────────────────────────
inline float distSq(float dx, float dy) { return dx * dx + dy * dy; }
inline void gridClear() { memset(gridCount, 0, sizeof(gridCount)); }

static void physicsStep(float dt) {
    for (int i = 0; i < NUM_PARTICLES; i++) {
        particles[i].vx += imuGX * dt;
        particles[i].vy += imuGY * dt;
        particles[i].lastX = particles[i].x;
        particles[i].lastY = particles[i].y;
        particles[i].x += particles[i].vx * dt;
        particles[i].y += particles[i].vy * dt;
    }
    duck.vx += imuGX * DUCK_GRAVITY_SCALE * dt;
    duck.vy += imuGY * DUCK_GRAVITY_SCALE * dt;
    duck.lastX = duck.x; duck.lastY = duck.y;
    duck.x += duck.vx * dt; duck.y += duck.vy * dt;

    // 鸭子旋转物理 (全向浮标模式：底座始终指向真实重力)
    // 增加死区判定：防止平放设备时角度由于传感器噪点乱跳
    float targetAngle = 0.0f;
    float magSq = imuGX * imuGX + imuGY * imuGY;
    if (magSq > 20000.0f) { // 约 0.25G 的水平分量阈值
        targetAngle = atan2f(-imuGX, imuGY + 0.0001f) * (180.0f / M_PI);
    }
    
    // 计算最短旋转路径，防止 180 度附近的疯狂打圈
    float angleDiff = targetAngle - duck.angle;
    while (angleDiff > 180.0f) angleDiff -= 360.0f;
    while (angleDiff < -180.0f) angleDiff += 360.0f;

    float angleAcc = angleDiff * DUCK_ROT_STIFFNESS;
    duck.aVel += angleAcc;
    duck.aVel *= (1.0f - DUCK_ROT_DAMPING);
    duck.angle += duck.aVel;

    gridClear();
    for (int i = 0; i < NUM_PARTICLES; i++) {
        int cx = (int)(particles[i].x / CELL_SIZE), cy = (int)(particles[i].y / CELL_SIZE);
        if (cx >= 0 && cx < GRID_W && cy >= 0 && cy < GRID_H) {
            if (gridCount[cx][cy] < MAX_PER_CELL) grid[cx][cy][gridCount[cx][cy]++] = (int16_t)i;
        }
    }

    const float minDist = PARTICLE_RADIUS * 2.0f;
    const float minDistSq = minDist * minDist;
    for (int i = 0; i < NUM_PARTICLES; i++) {
        float px = particles[i].x, py = particles[i].y;
        int cx0 = max(0, (int)(px / CELL_SIZE) - 1);
        int cy0 = max(0, (int)(py / CELL_SIZE) - 1);
        int cx1 = min(GRID_W - 1, cx0 + 2);
        int cy1 = min(GRID_H - 1, cy0 + 2);

        for (int cx = cx0; cx <= cx1; cx++) {
            for (int cy = cy0; cy <= cy1; cy++) {
                for (int k = 0; k < gridCount[cx][cy]; k++) {
                    int j = grid[cx][cy][k];
                    if (j <= i) continue;
                    float dx = particles[j].x - px, dy = particles[j].y - py;
                    float d2 = distSq(dx, dy);
                    if (d2 < minDistSq) {
                        float d = sqrtf(d2); if (d < 0.01f) d = 0.1f;
                        float overlap = (minDist - d);
                        float nx = dx / d, ny = dy / d;
                        particles[i].x -= nx * overlap * 0.5f;
                        particles[i].y -= ny * overlap * 0.5f;
                        particles[j].x += nx * overlap * 0.5f;
                        particles[j].y += ny * overlap * 0.5f;
                    }
                }
            }
        }
    }

    const float duckMinDist = DUCK_RADIUS + PARTICLE_RADIUS;
    const float duckMinDistSq = duckMinDist * duckMinDist;
    for (int i = 0; i < NUM_PARTICLES; i++) {
        float dx = particles[i].x - duck.x, dy = particles[i].y - duck.y;
        float d2 = distSq(dx, dy);
        if (d2 < duckMinDistSq) {
            float d = sqrtf(d2); if (d < 0.01f) d = 0.1f;
            float overlap = (duckMinDist - d);
            float nx = dx / d, ny = dy / d;
            particles[i].x += nx * overlap * 0.95f;
            particles[i].y += ny * overlap * 0.95f;
            duck.x -= nx * overlap * 0.05f;
            duck.y -= ny * overlap * 0.05f;
        }
    }

    float invDt = 1.0f / dt;
    for (int i = 0; i < NUM_PARTICLES; i++) {
        if (particles[i].x < PARTICLE_RADIUS) particles[i].x = PARTICLE_RADIUS;
        else if (particles[i].x > SCREEN_W - PARTICLE_RADIUS) particles[i].x = SCREEN_W - PARTICLE_RADIUS;
        if (particles[i].y < PARTICLE_RADIUS) particles[i].y = PARTICLE_RADIUS;
        else if (particles[i].y > SCREEN_H - PARTICLE_RADIUS) particles[i].y = SCREEN_H - PARTICLE_RADIUS;

        float nvx = (particles[i].x - particles[i].lastX) * invDt;
        float nvy = (particles[i].y - particles[i].lastY) * invDt;
        float v2 = nvx*nvx + nvy*nvy;
        if (v2 < 0.2f) { nvx = 0; nvy = 0; } else { nvx *= DAMPING; nvy *= DAMPING; }
        particles[i].vx = nvx; particles[i].vy = nvy;
    }

    if (duck.x < DUCK_RADIUS) duck.x = DUCK_RADIUS; else if (duck.x > SCREEN_W - DUCK_RADIUS) duck.x = SCREEN_W - DUCK_RADIUS;
    if (duck.y < DUCK_RADIUS) duck.y = DUCK_RADIUS; else if (duck.y > SCREEN_H - DUCK_RADIUS) duck.y = SCREEN_H - DUCK_RADIUS;
    
    float dnvx = (duck.x - duck.lastX) * invDt * DAMPING;
    float dnvy = (duck.y - duck.lastY) * invDt * DAMPING;
    duck.vx = dnvx; duck.vy = dnvy;
}

static void triggerExplosion() {
    int target = random(NUM_PARTICLES);
    float ex = particles[target].x; float ey = particles[target].y;
    float rSq = EXPLOSION_RADIUS * EXPLOSION_RADIUS;
    for (int i = 0; i < NUM_PARTICLES; i++) {
        float dx = particles[i].x - ex; float dy = particles[i].y - ey;
        float d2 = dx*dx + dy*dy;
        if (d2 < rSq && d2 > 0.01f) {
            float d = sqrtf(d2); float f = (EXPLOSION_RADIUS - d) / EXPLOSION_RADIUS * EXPLOSION_FORCE;
            particles[i].vx += (dx/d)*f; particles[i].vy += (dy/d)*f;
        }
    }
    duck.vx += (duck.x - ex) * 2.0f; duck.vy += (duck.y - ey) * 2.0f; 
    duck.aVel += (random(60) - 30); // 爆炸时给一个随机角速度
    playKeyTone();
}

static void renderFrame() {
    canvas.fillSprite(rgb32to16(COLOR_BG));
    const float glintIdxSq = SPEED_GLINT_THRESHOLD * SPEED_GLINT_THRESHOLD;
    for (int i = 0; i < NUM_PARTICLES; i++) {
        float v2 = particles[i].vx * particles[i].vx + particles[i].vy * particles[i].vy;
        ballSprites[(v2 > glintIdxSq) ? 1 : 0][particles[i].colorIdx].pushSprite(&canvas, (int)particles[i].x - (int)PARTICLE_RADIUS, (int)particles[i].y - (int)PARTICLE_RADIUS, 0x0001);
    }
    // 渲染旋转的鸭子 (使用 pushRotateZoom，以 x,y 为旋转中心)
    duckSprite.pushRotateZoom(&canvas, duck.x, duck.y, duck.angle, 1.0f, 1.0f, 0x0001);
    
    //canvas.setTextColor(TFT_DARKGREY); canvas.setCursor(SCREEN_W - 18, 2); canvas.printf("%d", fpsDisplay);
    canvas.pushSprite(0, 0);
}

static void initParticles() {
    int cols = (int)(SCREEN_W / (PARTICLE_RADIUS * 2.5f));
    for (int i = 0; i < NUM_PARTICLES; i++) {
        particles[i].x = particles[i].lastX = PARTICLE_RADIUS * 1.5f + (i % cols) * (PARTICLE_RADIUS * 2.5f);
        particles[i].y = particles[i].lastY = PARTICLE_RADIUS * 1.5f + (i / cols) * (PARTICLE_RADIUS * 2.5f);
        particles[i].vx = (random(10) - 5) * 5.0f; particles[i].vy = 0.0f; particles[i].colorIdx = (uint8_t)random(8);
    }
    duck.x = duck.lastX = SCREEN_W / 2.0f; duck.y = duck.lastY = DUCK_RADIUS + 5.0f; 
    duck.vx = duck.vy = 0.0f; duck.angle = duck.aVel = 0.0f;
}

void setup() {
    auto cfg = M5.config(); M5Cardputer.begin(cfg, true);
    M5Cardputer.Display.setRotation(1); M5Cardputer.Display.setBrightness(SYSTEM_BRIGHTNESS);
    M5.Speaker.begin(); M5.Speaker.setVolume(SYSTEM_VOLUME);
    canvas.createSprite(SCREEN_W, SCREEN_H); M5.Imu.init();
    duckSprite.createSprite(DUCK_WIDTH, DUCK_HEIGHT); duckSprite.setPivot(DUCK_WIDTH/2, DUCK_HEIGHT/2); // 设置旋转轴心
    duckSprite.fillSprite(0x0001);
    for (int i = 0; i < (DUCK_WIDTH * DUCK_HEIGHT); i++) if (duck_pixels[i] != 0x000000) duckSprite.drawPixel(i % DUCK_WIDTH, i / DUCK_WIDTH, duck_pixels[i]);
    
    int dia = (int)(PARTICLE_RADIUS * 2.0f);
    for (int i = 0; i < 8; i++) {
        uint32_t c = NEON_PALETTE[i];
        for (int b = 0; b < 2; b++) {
            ballSprites[b][i].createSprite(dia, dia); ballSprites[b][i].fillSprite(0x0001);
            float f = (b == 0) ? 1.0f : 1.6f; 
            ballSprites[b][i].fillCircle(dia/2, dia/2, dia/2, getBrightened16(c, f * 0.7f));
            ballSprites[b][i].fillCircle(dia/2, dia/2, dia/2 - 1, getBrightened16(c, f * 1.15f));
            ballSprites[b][i].fillCircle(dia/2 - 2, dia/2 - 2, (b == 0 ? 1 : 2), TFT_WHITE);
        }
    }
    initParticles(); fpsLastMs = millis();
}

void loop() {
    M5Cardputer.update();
    static bool lastPressed = false;
    bool currentPressed = M5Cardputer.Keyboard.isPressed();
    if (currentPressed && !lastPressed) triggerExplosion();
    lastPressed = currentPressed;

    if (M5.Imu.isEnabled()) {
        M5.Imu.update(); float ax, ay, az; M5.Imu.getAccel(&ax, &ay, &az);
        float targetGX = (-ax) * GRAVITY_STRENGTH, targetGY = ay * GRAVITY_STRENGTH;
        if (abs(targetGX - imuGX) > 1.5f) imuGX = imuGX * (1.0f - IMU_LPF_ALPHA) + targetGX * IMU_LPF_ALPHA;
        if (abs(targetGY - imuGY) > 1.5f) imuGY = imuGY * (1.0f - IMU_LPF_ALPHA) + targetGY * IMU_LPF_ALPHA;
    }
    
    float subDt = FIXED_DT / (float)SUB_STEPS;
    for (int s = 0; s < SUB_STEPS; s++) physicsStep(subDt);

    renderFrame();
    fpsCount++; uint32_t now = millis();
    if (now - fpsLastMs >= 1000) { fpsDisplay = fpsCount; fpsCount = 0; fpsLastMs = now; }
}

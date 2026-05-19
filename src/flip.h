#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct FlipFluid FlipFluid;

// 创建 FLIP 流体模拟器。
// - sim_w/sim_h: 模拟“水箱”物理尺寸（任意单位）
// - visible_w/visible_h: 需要输出到 LED 的可见网格分辨率（不含边界 padding）
// 注意：内部使用方格（统一 spacing），若 sim_w/sim_h 与 visible_w/visible_h 的比例不一致，
// 会在保证方格的前提下，将有效 tank 尺寸裁剪到可用范围。
FlipFluid* flip_create(float sim_w, float sim_h, int visible_w, int visible_h, float fill_ratio);
void flip_destroy(FlipFluid* f);

void flip_step(FlipFluid* f, float dt, float gx, float gy);

// 获取可见网格（布局为 out_grid[x * visible_h + y]）
void flip_get_led_grid(const FlipFluid* f, float* out_grid, int visible_w, int visible_h);

// 根据潮位因子动态调整内部粒子数量：
// - tide_level: 0.0 = 最低潮, 1.0 = 最高潮
// - min_fill_ratio / max_fill_ratio: 相对于“基础水量”的下限/上限比例
//   （基础水量 = 初始创建时的粒子数）
void flip_set_tide_level(FlipFluid* f, float tide_level,
                         float min_fill_ratio, float max_fill_ratio);

void flip_set_gravity_scale(FlipFluid* f, float gravity_scale);
void flip_set_solver_quality(FlipFluid* f, int push_iters, int pressure_iters, float flip_ratio);

// 获取粒子数据
void flip_get_particles(const FlipFluid* f, int* out_num, float** out_pos, float** out_vel);

// 获取粒子的碰撞半径
float flip_get_particle_radius(const FlipFluid* f);

#ifdef __cplusplus
}
#endif

// tests/test_collision.cpp
// 單元測試：CollisionSystem 的三個幾何函式
// 不依賴 OpenGL/SDL2，純數學 CPU 測試

#include "systems/CollisionSystem.h"
#include <cassert>
#include <cstdio>
#include <cmath>

// 輔助：兩個 float 是否近似相等（容差 0.001）
static bool approx(float a, float b) {
    return std::abs(a - b) < 0.001f;
}

// ─────────────────────────────────────────
// Circle vs Circle
// ─────────────────────────────────────────

void test_circle_no_collision() {
    float dx, dy, depth;
    bool hit = duck::circleVsCircle(0,0,10, 100,0,10, dx, dy, depth);
    assert(!hit);
    std::printf("  [PASS] test_circle_no_collision\n");
}

void test_circle_collision_center_overlap() {
    float dx, dy, depth;
    bool hit = duck::circleVsCircle(0,0,10, 5,0,10, dx, dy, depth);
    assert(hit);
    assert(approx(depth, 15.0f));
    assert(approx(dx, 1.0f));
    assert(approx(dy, 0.0f));
    std::printf("  [PASS] test_circle_collision_center_overlap\n");
}

void test_circle_exactly_touching() {
    float dx, dy, depth;
    bool hit = duck::circleVsCircle(0,0,10, 20,0,10, dx, dy, depth);
    assert(!hit);
    std::printf("  [PASS] test_circle_exactly_touching\n");
}

// ─────────────────────────────────────────
// AABB vs AABB
// ─────────────────────────────────────────

void test_aabb_no_collision() {
    float dx, dy, depth;
    bool hit = duck::aabbVsAabb(0,0,10,10, 30,0,10,10, dx, dy, depth);
    assert(!hit);
    std::printf("  [PASS] test_aabb_no_collision\n");
}

void test_aabb_x_overlap() {
    float dx, dy, depth;
    bool hit = duck::aabbVsAabb(0,0,10,10, 15,0,10,10, dx, dy, depth);
    assert(hit);
    assert(approx(depth, 5.0f));
    assert(approx(dx, 1.0f));
    assert(approx(dy, 0.0f));
    std::printf("  [PASS] test_aabb_x_overlap\n");
}

void test_aabb_y_overlap() {
    float dx, dy, depth;
    bool hit = duck::aabbVsAabb(0,0,10,10, 0,15,10,10, dx, dy, depth);
    assert(hit);
    assert(approx(depth, 5.0f));
    assert(approx(dx, 0.0f));
    assert(approx(dy, 1.0f));
    std::printf("  [PASS] test_aabb_y_overlap\n");
}

// ─────────────────────────────────────────
// Circle vs AABB
// ─────────────────────────────────────────

void test_circle_aabb_no_collision() {
    float dx, dy, depth;
    bool hit = duck::circleVsAabb(0,0,10, 50,0,10,10, dx, dy, depth);
    assert(!hit);
    std::printf("  [PASS] test_circle_aabb_no_collision\n");
}

void test_circle_aabb_face_collision() {
    float dx, dy, depth;
    // 圓心 (0,0) r=15; AABB 中心 (20,0) hw=hh=10
    // 最近點 = (10,0), 距離=10, 穿透=5
    bool hit = duck::circleVsAabb(0,0,15, 20,0,10,10, dx, dy, depth);
    assert(hit);
    assert(approx(depth, 5.0f));
    std::printf("  [PASS] test_circle_aabb_face_collision\n");
}

void test_circle_aabb_corner_miss() {
    float dx, dy, depth;
    // 圓心 (0,0) r=5; AABB 中心 (10,10) hw=hh=5
    // 最近點 = (5,5), 距離=sqrt(50)≈7.07 > 5 → 不碰
    bool hit = duck::circleVsAabb(0,0,5, 10,10,5,5, dx, dy, depth);
    assert(!hit);
    std::printf("  [PASS] test_circle_aabb_corner_miss\n");
}

// ─────────────────────────────────────────
// main
// ─────────────────────────────────────────

int main() {
    std::printf("=== Collision Geometry Tests ===\n");

    std::printf("--- Circle vs Circle ---\n");
    test_circle_no_collision();
    test_circle_collision_center_overlap();
    test_circle_exactly_touching();

    std::printf("--- AABB vs AABB ---\n");
    test_aabb_no_collision();
    test_aabb_x_overlap();
    test_aabb_y_overlap();

    std::printf("--- Circle vs AABB ---\n");
    test_circle_aabb_no_collision();
    test_circle_aabb_face_collision();
    test_circle_aabb_corner_miss();

    std::printf("\n=== All tests passed! ===\n");
    return 0;
}

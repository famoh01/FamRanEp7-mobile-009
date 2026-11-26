// PR-A: Shader + Pipeline Bootstrap - CPU-side matrix math unit tests.
// No GPU/GLES context required. Runtime shader compile is Android/device-verified.

#include "../Engine/Renderer/RenderMath.h"
#include <cassert>
#include <cmath>
#include <iostream>

using namespace RANMobile;
static bool near(float a, float b, float eps = 1e-4f) { return std::fabs(a-b) <= eps; }

static void test_identity() {
    Mat4 I = Mat4Identity();
    for (int c=0;c<4;++c) for (int r=0;r<4;++r) assert(near(I.at(c,r), c==r?1.f:0.f));
    std::cout << "[PASS] Mat4Identity\n";
}

static void test_multiply_identity() {
    Mat4 I = Mat4Identity();
    Mat4 T = Mat4Translate(1,2,3);
    Mat4 R = Mat4Multiply(I, T);
    for (int i=0;i<16;++i) assert(near(R.m[i], T.m[i]));
    std::cout << "[PASS] Mat4Multiply identity\n";
}

static void test_translate() {
    Mat4 T = Mat4Translate(10, -2, 5);
    Vec3 p = Mat4TransformPoint(T, {1,2,3});
    assert(near(p.x,11) && near(p.y,0) && near(p.z,8));
    std::cout << "[PASS] Mat4Translate\n";
}

static void test_scale() {
    Mat4 S = Mat4Scale(2,3,4);
    Vec3 p = Mat4TransformPoint(S, {1,1,1});
    assert(near(p.x,2) && near(p.y,3) && near(p.z,4));
    std::cout << "[PASS] Mat4Scale\n";
}

static void test_rotate_y() {
    Mat4 R = Mat4RotateY(3.14159265f / 2.0f); // 90 deg
    Vec3 p = Mat4TransformPoint(R, {1,0,0});
    assert(near(p.x,0,1e-3f) && near(p.y,0,1e-3f) && near(p.z,-1,1e-3f));
    std::cout << "[PASS] Mat4RotateY\n";
}

static void test_perspective() {
    Mat4 P = Mat4Perspective(60.0f * 3.14159265f/180.0f, 16.f/9.f, 0.1f, 100.f);
    // A point on near plane center should project to NDC-ish reasonable values
    Vec3 p = Mat4TransformPoint(P, {0,0,-1.0f});
    assert(std::fabs(p.x) < 0.1f && std::fabs(p.y) < 0.1f);
    std::cout << "[PASS] Mat4Perspective\n";
}

static void test_look_at() {
    Mat4 V = Mat4LookAt({0,0,5}, {0,0,0}, {0,1,0});
    Vec3 p = Mat4TransformPoint(V, {0,0,0}); // center should be on -Z in view
    assert(near(p.x,0) && near(p.y,0) && near(p.z,-5));
    std::cout << "[PASS] Mat4LookAt\n";
}

static void test_mvp_compose() {
    Mat4 P = Mat4Perspective(60.0f * 3.14159265f/180.0f, 1.0f, 0.1f, 100.f);
    Mat4 V = Mat4LookAt({0,0,3}, {0,0,0}, {0,1,0});
    Mat4 M = Mat4Identity();
    Mat4 MVP = Mat4Multiply(P, Mat4Multiply(V, M));
    Vec3 p = Mat4TransformPoint(MVP, {0,0,0});
    assert(std::isfinite(p.x) && std::isfinite(p.y) && std::isfinite(p.z));
    std::cout << "[PASS] MVP compose\n";
}

int main() {
    std::cout << "=== PR-A Renderer Math (CPU) ===\n";
    test_identity();
    test_multiply_identity();
    test_translate();
    test_scale();
    test_rotate_y();
    test_perspective();
    test_look_at();
    test_mvp_compose();
    std::cout << "ALL RENDERER MATH TESTS PASSED!\n";
    return 0;
}

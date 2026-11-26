#pragma once
#include <array>
#include <cmath>
#include <cstdint>

namespace RANMobile {

// Column-major 4x4 matrix (OpenGL/GLES convention).
struct Mat4 {
    std::array<float, 16> m{}; // column-major: m[col*4 + row]
    float& at(int col, int row) { return m[col * 4 + row]; }
    float at(int col, int row) const { return m[col * 4 + row]; }
};

struct Vec3 { float x=0, y=0, z=0; };

Mat4 Mat4Identity();
Mat4 Mat4Multiply(const Mat4& a, const Mat4& b); // a*b
Mat4 Mat4Perspective(float fovYRadians, float aspect, float zNear, float zFar);
Mat4 Mat4Ortho(float left, float right, float bottom, float top, float zNear, float zFar);
Mat4 Mat4LookAt(const Vec3& eye, const Vec3& center, const Vec3& up);
Mat4 Mat4Translate(float x, float y, float z);
Mat4 Mat4Scale(float x, float y, float z);
Mat4 Mat4RotateY(float radians);

// Apply Mat4 to vec4(x,y,z,w), return Vec3 divided by w.
Vec3 Mat4TransformPoint(const Mat4& m, const Vec3& v);

} // namespace RANMobile

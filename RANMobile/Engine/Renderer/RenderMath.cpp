#include "RenderMath.h"
#include <cassert>

namespace RANMobile {

Mat4 Mat4Identity() {
    Mat4 r{};
    r.at(0,0)=1; r.at(1,1)=1; r.at(2,2)=1; r.at(3,3)=1;
    return r;
}

Mat4 Mat4Multiply(const Mat4& a, const Mat4& b) {
    Mat4 r{};
    for (int col = 0; col < 4; ++col) {
        for (int row = 0; row < 4; ++row) {
            float sum = 0.0f;
            for (int k = 0; k < 4; ++k) sum += a.at(k, row) * b.at(col, k);
            r.at(col, row) = sum;
        }
    }
    return r;
}

Mat4 Mat4Perspective(float fovYRadians, float aspect, float zNear, float zFar) {
    Mat4 r{};
    const float f = 1.0f / std::tan(fovYRadians * 0.5f);
    r.at(0,0) = f / aspect;
    r.at(1,1) = f;
    r.at(2,2) = (zFar + zNear) / (zNear - zFar);
    r.at(2,3) = -1.0f;
    r.at(3,2) = (2.0f * zFar * zNear) / (zNear - zFar);
    return r;
}

Mat4 Mat4Ortho(float l, float rgt, float b, float t, float n, float f) {
    Mat4 r = Mat4Identity();
    r.at(0,0) = 2.0f / (rgt - l);
    r.at(1,1) = 2.0f / (t - b);
    r.at(2,2) = -2.0f / (f - n);
    r.at(3,0) = -(rgt + l) / (rgt - l);
    r.at(3,1) = -(t + b) / (t - b);
    r.at(3,2) = -(f + n) / (f - n);
    return r;
}

static Vec3 sub(Vec3 a, Vec3 b) { return {a.x-b.x, a.y-b.y, a.z-b.z}; }
static Vec3 norm(Vec3 v) { float l = std::sqrt(v.x*v.x+v.y*v.y+v.z*v.z); return l>0?Vec3{v.x/l,v.y/l,v.z/l}:v; }
static Vec3 cross(Vec3 a, Vec3 b) { return {a.y*b.z-a.z*b.y, a.z*b.x-a.x*b.z, a.x*b.y-a.y*b.x}; }
static float dot(Vec3 a, Vec3 b) { return a.x*b.x+a.y*b.y+a.z*b.z; }

Mat4 Mat4LookAt(const Vec3& eye, const Vec3& center, const Vec3& up) {
    Vec3 f = norm(sub(center, eye));
    Vec3 s = norm(cross(f, up));
    Vec3 u = cross(s, f);
    Mat4 r = Mat4Identity();
    r.at(0,0)=s.x; r.at(1,0)=s.y; r.at(2,0)=s.z;
    r.at(0,1)=u.x; r.at(1,1)=u.y; r.at(2,1)=u.z;
    r.at(0,2)=-f.x; r.at(1,2)=-f.y; r.at(2,2)=-f.z;
    r.at(3,0)=-dot(s, eye);
    r.at(3,1)=-dot(u, eye);
    r.at(3,2)= dot(f, eye);
    return r;
}

Mat4 Mat4Translate(float x, float y, float z) {
    Mat4 r = Mat4Identity();
    r.at(3,0)=x; r.at(3,1)=y; r.at(3,2)=z;
    return r;
}

Mat4 Mat4Scale(float x, float y, float z) {
    Mat4 r{};
    r.at(0,0)=x; r.at(1,1)=y; r.at(2,2)=z; r.at(3,3)=1;
    return r;
}

Mat4 Mat4RotateY(float radians) {
    Mat4 r = Mat4Identity();
    float c = std::cos(radians), s = std::sin(radians);
    r.at(0,0)=c;  r.at(2,0)=s;
    r.at(0,2)=-s; r.at(2,2)=c;
    return r;
}

Vec3 Mat4TransformPoint(const Mat4& mtx, const Vec3& v) {
    float x = mtx.at(0,0)*v.x + mtx.at(1,0)*v.y + mtx.at(2,0)*v.z + mtx.at(3,0);
    float y = mtx.at(0,1)*v.x + mtx.at(1,1)*v.y + mtx.at(2,1)*v.z + mtx.at(3,1);
    float z = mtx.at(0,2)*v.x + mtx.at(1,2)*v.y + mtx.at(2,2)*v.z + mtx.at(3,2);
    float w = mtx.at(0,3)*v.x + mtx.at(1,3)*v.y + mtx.at(2,3)*v.z + mtx.at(3,3);
    if (std::fabs(w) > 1e-6f) { x/=w; y/=w; z/=w; }
    return {x,y,z};
}

} // namespace RANMobile

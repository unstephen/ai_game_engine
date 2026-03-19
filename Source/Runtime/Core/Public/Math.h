// =============================================================================
// Math.h - 数学库
// =============================================================================

#pragma once

#include "Core.h"

// MSVC 下使用 math.h 而不是 cmath（解决 MSVC cmath 兼容性问题）
#ifdef _MSC_VER
#include <math.h>
#else
#include <cmath>
#endif

namespace Engine
{

// 常量
constexpr float PI         = 3.14159265358979323846f;
constexpr float DEG_TO_RAD = PI / 180.0f;
constexpr float RAD_TO_DEG = 180.0f / PI;

// ============================================================================
// Vector2
// ============================================================================

struct Vector2
{
    float x = 0.0f;
    float y = 0.0f;

    Vector2() = default;
    Vector2(float x, float y) : x(x), y(y) {}

    Vector2 operator+(const Vector2& rhs) const { return {x + rhs.x, y + rhs.y}; }
    Vector2 operator-(const Vector2& rhs) const { return {x - rhs.x, y - rhs.y}; }
    Vector2 operator*(float s) const { return {x * s, y * s}; }
};

// ============================================================================
// Vector3
// ============================================================================

struct Vector3
{
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;

    Vector3() = default;
    Vector3(float x, float y, float z) : x(x), y(y), z(z) {}

    Vector3 operator+(const Vector3& rhs) const { return {x + rhs.x, y + rhs.y, z + rhs.z}; }
    Vector3 operator-(const Vector3& rhs) const { return {x - rhs.x, y - rhs.y, z - rhs.z}; }
    Vector3 operator*(float s) const { return {x * s, y * s, z * s}; }

    float Length() const { return sqrtf(x * x + y * y + z * z); }
    float LengthSq() const { return x * x + y * y + z * z; }

    Vector3 Normalized() const
    {
        float len = Length();
        if (len > 0.0f)
        {
            return {x / len, y / len, z / len};
        }
        return {};
    }

    static Vector3 Cross(const Vector3& a, const Vector3& b)
    {
        return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
    }

    static float Dot(const Vector3& a, const Vector3& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
};

// ============================================================================
// Vector4
// ============================================================================

struct Vector4
{
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float w = 0.0f;

    Vector4() = default;
    Vector4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}
    Vector4(const Vector3& v, float w) : x(v.x), y(v.y), z(v.z), w(w) {}
};

// ============================================================================
// Matrix4x4
// ============================================================================

struct Matrix4x4
{
    float m[4][4] = {};

    Matrix4x4()
    {
        // 单位矩阵
        m[0][0] = m[1][1] = m[2][2] = m[3][3] = 1.0f;
    }

    static Matrix4x4 Identity() { return Matrix4x4(); }

    static Matrix4x4 Translation(float x, float y, float z)
    {
        Matrix4x4 result;
        result.m[3][0] = x;
        result.m[3][1] = y;
        result.m[3][2] = z;
        return result;
    }

    static Matrix4x4 Scaling(float x, float y, float z)
    {
        Matrix4x4 result;
        result.m[0][0] = x;
        result.m[1][1] = y;
        result.m[2][2] = z;
        return result;
    }

    static Matrix4x4 RotationX(float angle);
    static Matrix4x4 RotationY(float angle);
    static Matrix4x4 RotationZ(float angle);
    static Matrix4x4 Perspective(float fov, float aspect, float nearZ, float farZ);
    static Matrix4x4 LookAt(const Vector3& eye, const Vector3& target, const Vector3& up);

    Matrix4x4 operator*(const Matrix4x4& rhs) const;
};

} // namespace Engine
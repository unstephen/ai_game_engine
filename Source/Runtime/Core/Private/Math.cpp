// =============================================================================
// Math.cpp - 数学库实现
// =============================================================================

#include "Math.h"

namespace Engine
{

Matrix4x4 Matrix4x4::RotationX(float angle)
{
    float     c = std::cos(angle);
    float     s = std::sin(angle);
    Matrix4x4 result;
    result.m[1][1] = c;
    result.m[1][2] = s;
    result.m[2][1] = -s;
    result.m[2][2] = c;
    return result;
}

Matrix4x4 Matrix4x4::RotationY(float angle)
{
    float     c = std::cos(angle);
    float     s = std::sin(angle);
    Matrix4x4 result;
    result.m[0][0] = c;
    result.m[0][2] = -s;
    result.m[2][0] = s;
    result.m[2][2] = c;
    return result;
}

Matrix4x4 Matrix4x4::RotationZ(float angle)
{
    float     c = std::cos(angle);
    float     s = std::sin(angle);
    Matrix4x4 result;
    result.m[0][0] = c;
    result.m[0][1] = s;
    result.m[1][0] = -s;
    result.m[1][1] = c;
    return result;
}

Matrix4x4 Matrix4x4::Perspective(float fov, float aspect, float nearZ, float farZ)
{
    float     tanHalfFov = std::tan(fov * 0.5f);
    Matrix4x4 result;
    result.m[0][0] = 1.0f / (aspect * tanHalfFov);
    result.m[1][1] = 1.0f / tanHalfFov;
    result.m[2][2] = farZ / (nearZ - farZ);
    result.m[2][3] = -1.0f;
    result.m[3][2] = (nearZ * farZ) / (nearZ - farZ);
    result.m[3][3] = 0.0f;
    return result;
}

Matrix4x4 Matrix4x4::LookAt(const Vector3& eye, const Vector3& target, const Vector3& up)
{
    Vector3 zAxis = (eye - target).Normalized();
    Vector3 xAxis = Vector3::Cross(up, zAxis).Normalized();
    Vector3 yAxis = Vector3::Cross(zAxis, xAxis);

    Matrix4x4 result;
    result.m[0][0] = xAxis.x;
    result.m[0][1] = yAxis.x;
    result.m[0][2] = zAxis.x;
    result.m[0][3] = 0.0f;
    result.m[1][0] = xAxis.y;
    result.m[1][1] = yAxis.y;
    result.m[1][2] = zAxis.y;
    result.m[1][3] = 0.0f;
    result.m[2][0] = xAxis.z;
    result.m[2][1] = yAxis.z;
    result.m[2][2] = zAxis.z;
    result.m[2][3] = 0.0f;
    result.m[3][0] = -Vector3::Dot(xAxis, eye);
    result.m[3][1] = -Vector3::Dot(yAxis, eye);
    result.m[3][2] = -Vector3::Dot(zAxis, eye);
    result.m[3][3] = 1.0f;
    return result;
}

Matrix4x4 Matrix4x4::operator*(const Matrix4x4& rhs) const
{
    Matrix4x4 result;
    for (int i = 0; i < 4; ++i)
    {
        for (int j = 0; j < 4; ++j)
        {
            result.m[i][j] = 0.0f;
            for (int k = 0; k < 4; ++k)
            {
                result.m[i][j] += m[i][k] * rhs.m[k][j];
            }
        }
    }
    return result;
}

} // namespace Engine
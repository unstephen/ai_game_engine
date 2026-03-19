// =============================================================================
// Math.h - 数学库（基于 DirectXMath）
// ============================================================================

#pragma once

#include "Core.h"

// DirectXMath - 微软官方数学库，跨平台支持
#include <DirectXMath.h>
#include <DirectXPackedVector.h>

namespace Engine
{

// 导入 DirectXMath 命名空间
using namespace DirectX;

// 类型别名
using Vector2 = XMFLOAT2;
using Vector3 = XMFLOAT3;
using Vector4 = XMFLOAT4;
using Matrix4x4 = XMFLOAT4X4;
using Quaternion = XMFLOAT4;

// 常量
constexpr float PI = 3.14159265358979323846f;
constexpr float DEG_TO_RAD = PI / 180.0f;
constexpr float RAD_TO_DEG = 180.0f / PI;

// ============================================================================
// 向量运算辅助函数
// ============================================================================

inline XMVECTOR Load(const Vector3& v) { return XMLoadFloat3(&v); }
inline void Store(Vector3& out, FXMVECTOR v) { XMStoreFloat3(&out, v); }

inline float Length(const Vector3& v)
{
    XMVECTOR vec = XMLoadFloat3(&v);
    return XMVectorGetX(XMVector3Length(vec));
}

inline Vector3 Normalize(const Vector3& v)
{
    XMVECTOR vec = XMLoadFloat3(&v);
    XMVECTOR normalized = XMVector3Normalize(vec);
    Vector3 result;
    XMStoreFloat3(&result, normalized);
    return result;
}

inline Vector3 Cross(const Vector3& a, const Vector3& b)
{
    XMVECTOR va = XMLoadFloat3(&a);
    XMVECTOR vb = XMLoadFloat3(&b);
    XMVECTOR cross = XMVector3Cross(va, vb);
    Vector3 result;
    XMStoreFloat3(&result, cross);
    return result;
}

inline float Dot(const Vector3& a, const Vector3& b)
{
    XMVECTOR va = XMLoadFloat3(&a);
    XMVECTOR vb = XMLoadFloat3(&b);
    return XMVectorGetX(XMVector3Dot(va, vb));
}

// ============================================================================
// 矩阵运算辅助函数
// ============================================================================

inline XMMATRIX Load(const Matrix4x4& m) { return XMLoadFloat4x4(&m); }
inline void Store(Matrix4x4& out, FXMMATRIX m) { XMStoreFloat4x4(&out, m); }

inline Matrix4x4 IdentityMatrix()
{
    Matrix4x4 result;
    XMStoreFloat4x4(&result, XMMatrixIdentity());
    return result;
}

inline Matrix4x4 CreatePerspectiveMatrix(float fov, float aspect, float nearZ, float farZ)
{
    Matrix4x4 result;
    XMStoreFloat4x4(&result, XMMatrixPerspectiveFovLH(fov, aspect, nearZ, farZ));
    return result;
}

inline Matrix4x4 CreateLookAtMatrix(const Vector3& eye, const Vector3& target, const Vector3& up)
{
    XMVECTOR eyeVec = XMLoadFloat3(&eye);
    XMVECTOR targetVec = XMLoadFloat3(&target);
    XMVECTOR upVec = XMLoadFloat3(&up);
    Matrix4x4 result;
    XMStoreFloat4x4(&result, XMMatrixLookAtLH(eyeVec, targetVec, upVec));
    return result;
}

inline Matrix4x4 CreateTranslationMatrix(const Vector3& translation)
{
    Matrix4x4 result;
    XMStoreFloat4x4(&result, XMMatrixTranslation(translation.x, translation.y, translation.z));
    return result;
}

inline Matrix4x4 CreateScaleMatrix(const Vector3& scale)
{
    Matrix4x4 result;
    XMStoreFloat4x4(&result, XMMatrixScaling(scale.x, scale.y, scale.z));
    return result;
}

inline Matrix4x4 CreateRotationMatrixX(float angle)
{
    Matrix4x4 result;
    XMStoreFloat4x4(&result, XMMatrixRotationX(angle));
    return result;
}

inline Matrix4x4 CreateRotationMatrixY(float angle)
{
    Matrix4x4 result;
    XMStoreFloat4x4(&result, XMMatrixRotationY(angle));
    return result;
}

inline Matrix4x4 CreateRotationMatrixZ(float angle)
{
    Matrix4x4 result;
    XMStoreFloat4x4(&result, XMMatrixRotationZ(angle));
    return result;
}

inline Matrix4x4 Multiply(const Matrix4x4& a, const Matrix4x4& b)
{
    XMMATRIX ma = XMLoadFloat4x4(&a);
    XMMATRIX mb = XMLoadFloat4x4(&b);
    Matrix4x4 result;
    XMStoreFloat4x4(&result, ma * mb);
    return result;
}

} // namespace Engine
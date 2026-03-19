// =============================================================================
// Math.h - 数学库（基于 GLM）
// =============================================================================

#pragma once

#include "Core.h"

// MSVC 兼容性：在 GLM 之前包含 C 标准库头文件
#ifdef _MSC_VER
#include <math.h>
#include <cstdlib>
#endif

// GLM 配置
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_LEFT_HANDED
#define GLM_ENABLE_EXPERIMENTAL

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

namespace Engine
{

// 导入 GLM 类型
using Vector2 = glm::vec2;
using Vector3 = glm::vec3;
using Vector4 = glm::vec4;
using Matrix4x4 = glm::mat4;
using Quaternion = glm::quat;

// 常量
constexpr float PI = 3.14159265358979323846f;
constexpr float DEG_TO_RAD = PI / 180.0f;
constexpr float RAD_TO_DEG = 180.0f / PI;

// ============================================================================
// 数学工具函数
// ============================================================================

inline Matrix4x4 CreatePerspectiveMatrix(float fov, float aspect, float nearZ, float farZ)
{
    return glm::perspectiveLH_ZO(fov, aspect, nearZ, farZ);
}

inline Matrix4x4 CreateLookAtMatrix(const Vector3& eye, const Vector3& target, const Vector3& up)
{
    return glm::lookAtLH(eye, target, up);
}

inline Matrix4x4 CreateTranslationMatrix(const Vector3& translation)
{
    return glm::translate(glm::mat4(1.0f), translation);
}

inline Matrix4x4 CreateScaleMatrix(const Vector3& scale)
{
    return glm::scale(glm::mat4(1.0f), scale);
}

inline Matrix4x4 CreateRotationMatrixX(float angle)
{
    return glm::rotate(glm::mat4(1.0f), angle, glm::vec3(1, 0, 0));
}

inline Matrix4x4 CreateRotationMatrixY(float angle)
{
    return glm::rotate(glm::mat4(1.0f), angle, glm::vec3(0, 1, 0));
}

inline Matrix4x4 CreateRotationMatrixZ(float angle)
{
    return glm::rotate(glm::mat4(1.0f), angle, glm::vec3(0, 0, 1));
}

} // namespace Engine
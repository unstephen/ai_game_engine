/**
 * @file MSVCCompat.h
 * @brief MSVC 兼容性修复 - 提供 POSIX 风格数学函数
 * 
 * DirectXMath 使用 floorf/fabsf/modff 等 POSIX 函数名
 * MSVC 的 <math.h> 默认不提供这些名称
 */

#pragma once

#ifdef _MSC_VER

// 确保 C 运行时提供 POSIX 函数名
#ifndef _CRT_DECLARE_NONSTDC_NAMES
#define _CRT_DECLARE_NONSTDC_NAMES 1
#endif

#include <math.h>

// 如果 <math.h> 仍未提供，手动定义
#ifndef floorf
#define floorf(x)  ((float)floor(x))
#endif
#ifndef ceilf
#define ceilf(x)   ((float)ceil(x))
#endif
#ifndef fabsf
#define fabsf(x)   ((float)fabs(x))
#endif
#ifndef sqrtf
#define sqrtf(x)   ((float)sqrt(x))
#endif
#ifndef sinf
#define sinf(x)    ((float)sin(x))
#endif
#ifndef cosf
#define cosf(x)    ((float)cos(x))
#endif
#ifndef tanf
#define tanf(x)    ((float)tan(x))
#endif
#ifndef asinf
#define asinf(x)   ((float)asin(x))
#endif
#ifndef acosf
#define acosf(x)   ((float)acos(x))
#endif
#ifndef atanf
#define atanf(x)   ((float)atan(x))
#endif
#ifndef atan2f
#define atan2f(y,x) ((float)atan2(y,x))
#endif
#ifndef powf
#define powf(x,y)  ((float)pow(x,y))
#endif
#ifndef expf
#define expf(x)    ((float)exp(x))
#endif
#ifndef logf
#define logf(x)    ((float)log(x))
#endif
#ifndef log10f
#define log10f(x)  ((float)log10(x))
#endif
#ifndef fmodf
#define fmodf(x,y) ((float)fmod(x,y))
#endif

// modff 特殊处理（有指针参数）
inline float modff(float x, float* iptr) {
    double d_iptr;
    float result = (float)modf((double)x, &d_iptr);
    *iptr = (float)d_iptr;
    return result;
}

#endif // _MSC_VER
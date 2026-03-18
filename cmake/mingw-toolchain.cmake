# CMake mingw-w64 交叉编译工具链
# 用于在 Linux 下快速验证 Windows 代码的语法正确性

set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

# 编译器
set(CMAKE_C_COMPILER x86_64-w64-mingw32-gcc)
set(CMAKE_CXX_COMPILER x86_64-w64-mingw32-g++)

# 查找工具
set(CMAKE_FIND_ROOT_PATH /usr/x86_64-w64-mingw32)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# Windows 特定定义
add_definitions(-D_WIN32 -DWIN32_LEAN_AND_MEAN -DNOMINMAX)

# mingw 特定定义（用于条件编译）
add_definitions(-D__MINGW32__ -D_MINGW)

# 禁用示例项目（需要 D3D12）
set(ENGINE_BUILD_SAMPLES OFF CACHE BOOL "Disable samples for mingw check" FORCE)
set(ENGINE_BUILD_TESTS OFF CACHE BOOL "Disable tests for mingw check" FORCE)

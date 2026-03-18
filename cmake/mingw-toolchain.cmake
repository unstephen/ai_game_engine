# CMake mingw-w64 交叉编译工具链
# 用于在 Linux 下快速验证 Windows 代码的语法正确性

set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

# 编译器
set(CMAKE_C_COMPILER x86_64-w64-mingw32-gcc)
set(CMAKE_CXX_COMPILER x86_64-w64-mingw32-g++)

# mingw 9.3 使用 C++2a 代替 C++20
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++2a")

# 查找工具 - 允许查找宿主系统的程序
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# Windows 特定定义
add_definitions(-D_WIN32 -DWIN32_LEAN_AND_MEAN -DNOMINMAX)

# mingw 特定定义（用于条件编译）
add_definitions(-D__MINGW32__ -D_MINGW)

# mingw 没有 D3D12 头文件，禁用 D3D12 后端和示例
set(ENGINE_RHI_D3D12 OFF CACHE BOOL "Disable D3D12 for mingw (no headers)" FORCE)
set(ENGINE_BUILD_SAMPLES OFF CACHE BOOL "Disable samples for mingw (require D3D12)" FORCE)

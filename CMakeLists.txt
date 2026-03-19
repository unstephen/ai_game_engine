# =============================================================================
# 自研 3D 游戏引擎 - CMake 主配置
# =============================================================================

cmake_minimum_required(VERSION 3.20)

# 项目信息
project(Engine
    VERSION 0.1.0
    DESCRIPTION "自研 3D 游戏引擎 - 目标超越 Unreal Engine"
    LANGUAGES CXX
)

# =============================================================================
# CMake 选项
# =============================================================================

option(ENGINE_BUILD_EDITOR "构建编辑器" OFF)
option(ENGINE_BUILD_SAMPLES "构建示例项目" ON)
option(ENGINE_RHI_D3D12 "启用 D3D12 后端" ON)
option(ENGINE_RHI_VULKAN "启用 Vulkan 后端" OFF)
option(ENGINE_DEBUG_LAYER "启用调试层" ON)
option(ENGINE_WARNINGS_AS_ERRORS "警告视为错误" OFF)

# =============================================================================
# C++ 标准
# =============================================================================

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# =============================================================================
# 输出目录
# =============================================================================

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# =============================================================================
# 编译选项
# =============================================================================

# 平台检测
if(WIN32)
    add_definitions(-DWIN32_LEAN_AND_MEAN)
    add_definitions(-DNOMINMAX)
endif()

# MSVC 特定设置
if(MSVC)
    # 解决 MSVC cmath/cstdlib 兼容性问题
    add_compile_definitions(
        _CRT_NONSTDC_NO_WARNINGS
        _CRT_SECURE_NO_WARNINGS
        _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS
    )
    # 使用严格标准一致性
    add_compile_options(/permissive-)
    # 正确报告 C++ 版本
    add_compile_options(/Zc:__cplusplus)
endif()

# 调试层
if(ENGINE_DEBUG_LAYER)
    add_definitions(-DRHI_DEBUG)
    message(STATUS "RHI 调试层：启用")
endif()

# 警告级别
if(MSVC)
    add_compile_options(/W4 /MP)  # 多处理器编译
    if(ENGINE_WARNINGS_AS_ERRORS)
        add_compile_options(/WX)
    endif()
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
    if(ENGINE_WARNINGS_AS_ERRORS)
        add_compile_options(-Werror)
    endif()
endif()

# =============================================================================
# 查找依赖
# =============================================================================

# DirectX 12
if(ENGINE_RHI_D3D12)
    find_package(directx-headers CONFIG QUIET)
    find_package(dx12-agility CONFIG QUIET)
    
    if(NOT directx-headers_FOUND)
        message(STATUS "DirectX Headers 未找到，使用 Windows SDK 默认版本")
    endif()
endif()

# =============================================================================
# 核心库
# =============================================================================

# Engine::Core - 核心系统
add_library(EngineCore STATIC
    Source/Runtime/Core/Private/Core.cpp
    Source/Runtime/Core/Private/Log.cpp
    Source/Runtime/Core/Private/Memory.cpp
    Source/Runtime/Core/Private/Math.cpp
)

target_include_directories(EngineCore
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/Runtime/Core/Public
)

target_compile_definitions(EngineCore
    PRIVATE
        ENGINE_CORE_EXPORTS
)

# Engine::Window - 窗口管理
add_library(EngineWindow STATIC
    Source/Runtime/Window/Private/Window.cpp
    Source/Runtime/Window/Private/WindowManager.cpp
)

target_include_directories(EngineWindow
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/Runtime/Window/Public
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/Runtime/Core/Public
)

target_link_libraries(EngineWindow
    PUBLIC
        EngineCore
)

if(WIN32)
    target_link_libraries(EngineWindow PRIVATE user32)
endif()

# =============================================================================
# RHI 库
# =============================================================================

# Engine::RHI - RHI 核心接口
add_library(EngineRHI STATIC
    Source/Runtime/RHI/Private/RHI.cpp
    Source/Runtime/RHI/Private/Device.cpp
    Source/Runtime/RHI/Private/CommandList.cpp
    Source/Runtime/RHI/Private/Resource.cpp
    Source/Runtime/RHI/Private/FrameResourceManager.cpp
    Source/Runtime/RHI/Private/DescriptorAllocator.cpp
    Source/Runtime/RHI/Private/UploadManager.cpp
    Source/Runtime/RHI/Private/SwapChain.cpp
)

target_include_directories(EngineRHI
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/Runtime/RHI/Public
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/Runtime/Core/Public
)

target_link_libraries(EngineRHI
    PUBLIC
        EngineCore
        EngineWindow
)

# D3D12 后端
if(ENGINE_RHI_D3D12)
    add_library(EngineRHID3D12 STATIC
        Source/Runtime/RHI/D3D12/Private/D3D12Device.cpp
        Source/Runtime/RHI/D3D12/Private/D3D12CommandList.cpp
        Source/Runtime/RHI/D3D12/Private/D3D12Buffer.cpp
        Source/Runtime/RHI/D3D12/Private/D3D12Texture.cpp
        Source/Runtime/RHI/D3D12/Private/D3D12PipelineState.cpp
        Source/Runtime/RHI/D3D12/Private/D3D12RootSignature.cpp
        Source/Runtime/RHI/D3D12/Private/D3D12SwapChain.cpp
        Source/Runtime/RHI/D3D12/Private/D3D12DescriptorAllocator.cpp
        Source/Runtime/RHI/D3D12/Private/D3D12UploadManager.cpp
        Source/Runtime/RHI/D3D12/Private/D3D12FrameResourceManager.cpp
    )

    target_include_directories(EngineRHID3D12
        PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}/Source/Runtime/RHI/D3D12/Public
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/Source/Runtime/RHI/Public
    )

    # mingw 下使用 DirectX Headers (如果可用)
    if(MINGW)
        # mingw-w64 通常包含基本的 DirectX 头文件
        # 如果需要完整的 D3D12 支持，可以安装 DirectX-Headers
        target_link_libraries(EngineRHID3D12
            PUBLIC
                EngineRHI
        )
    else()
        # MSVC 下链接完整的 D3D12 库
        target_link_libraries(EngineRHID3D12
            PUBLIC
                EngineRHI
            PRIVATE
                d3d12
                dxgi
                dxguid
        )
    endif()

    target_compile_definitions(EngineRHID3D12
        PRIVATE
            ENGINE_RHI_D3D12_EXPORTS
            ENGINE_RHI_D3D12=1
    )
endif()

# =============================================================================
# 示例项目
# =============================================================================

if(ENGINE_BUILD_SAMPLES)
    # Triangle 示例
    add_executable(Sample_Triangle
        Samples/Triangle/main.cpp
        Samples/Triangle/TriangleApp.cpp
        Samples/Triangle/TriangleApp.h
    )
    
    target_include_directories(Sample_Triangle
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/Samples/Triangle
    )

    target_link_libraries(Sample_Triangle
        PRIVATE
            EngineCore
            EngineWindow
            EngineRHI
    )

    if(ENGINE_RHI_D3D12)
        target_link_libraries(Sample_Triangle
            PRIVATE
                EngineRHID3D12
                d3d12
                dxgi
                d3dcompiler
        )
        target_compile_definitions(Sample_Triangle
            PRIVATE
                ENGINE_RHI_D3D12
        )
    endif()
endif()

# =============================================================================
# 安装规则
# =============================================================================

install(TARGETS EngineCore EngineWindow EngineRHI
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

install(DIRECTORY Source/Runtime/Core/Public/
    DESTINATION include/Engine/Core
    FILES_MATCHING PATTERN "*.h"
)

install(DIRECTORY Source/Runtime/RHI/Public/
    DESTINATION include/Engine/RHI
    FILES_MATCHING PATTERN "*.h"
)

# =============================================================================
# 测试
# =============================================================================

option(ENGINE_BUILD_TESTS "构建测试" ON)

if(ENGINE_BUILD_TESTS)
    enable_testing()
    
    # 查找 Google Test
    find_package(GTest QUIET)
    
    if(NOT GTest_FOUND)
        message(STATUS "Google Test 未找到，测试将被禁用")
    else()
        message(STATUS "Google Test 已找到")
        
        # D3D12Device 测试
        if(ENGINE_RHI_D3D12)
            add_executable(Test_D3D12Device
                Tests/D3D12DeviceTest.cpp
            )
            
            target_link_libraries(Test_D3D12Device
                PRIVATE
                    EngineRHID3D12
                    GTest::gtest
                    GTest::gtest_main
                    GTest::gmock
            )
            
            target_compile_definitions(Test_D3D12Device
                PRIVATE
                    ENGINE_RHI_D3D12
            )
            
            # 添加测试
            add_test(NAME D3D12DeviceTest COMMAND Test_D3D12Device)
            
            # D3D12Buffer 测试
            add_executable(Test_D3D12Buffer
                Tests/D3D12BufferTest.cpp
            )
            
            target_link_libraries(Test_D3D12Buffer
                PRIVATE
                    EngineRHID3D12
                    GTest::gtest
                    GTest::gtest_main
                    GTest::gmock
            )
            
            target_compile_definitions(Test_D3D12Buffer
                PRIVATE
                    ENGINE_RHI_D3D12
            )
            
            # 添加测试
            add_test(NAME D3D12BufferTest COMMAND Test_D3D12Buffer)
            
            # D3D12Texture 测试
            add_executable(Test_D3D12Texture
                Tests/D3D12TextureTest.cpp
            )
            
            target_link_libraries(Test_D3D12Texture
                PRIVATE
                    EngineRHID3D12
                    GTest::gtest
                    GTest::gtest_main
                    GTest::gmock
            )
            
            target_compile_definitions(Test_D3D12Texture
                PRIVATE
                    ENGINE_RHI_D3D12
            )
            
            # 添加测试
            add_test(NAME D3D12TextureTest COMMAND Test_D3D12Texture)
            
            # D3D12SwapChain 测试
            add_executable(Test_D3D12SwapChain
                Tests/D3D12SwapChainTest.cpp
            )
            
            target_link_libraries(Test_D3D12SwapChain
                PRIVATE
                    EngineRHID3D12
                    GTest::gtest
                    GTest::gtest_main
                    GTest::gmock
            )
            
            target_compile_definitions(Test_D3D12SwapChain
                PRIVATE
                    ENGINE_RHI_D3D12
            )
            
            # 添加测试
            add_test(NAME D3D12SwapChainTest COMMAND Test_D3D12SwapChain)
            
            # D3D12FrameResourceManager 测试
            add_executable(Test_D3D12FrameResourceManager
                Tests/D3D12FrameResourceManagerTest.cpp
            )
            
            target_link_libraries(Test_D3D12FrameResourceManager
                PRIVATE
                    EngineRHID3D12
                    GTest::gtest
                    GTest::gtest_main
                    GTest::gmock
            )
            
            target_compile_definitions(Test_D3D12FrameResourceManager
                PRIVATE
                    ENGINE_RHI_D3D12
            )
            
            # 添加测试
            add_test(NAME D3D12FrameResourceManagerTest COMMAND Test_D3D12FrameResourceManager)
            
            # D3D12UploadManager 测试
            add_executable(Test_D3D12UploadManager
                Tests/D3D12UploadManagerTest.cpp
            )
            
            target_link_libraries(Test_D3D12UploadManager
                PRIVATE
                    EngineRHID3D12
                    GTest::gtest
                    GTest::gtest_main
                    GTest::gmock
            )
            
            target_compile_definitions(Test_D3D12UploadManager
                PRIVATE
                    ENGINE_RHI_D3D12
            )
            
            # 添加测试
            add_test(NAME D3D12UploadManagerTest COMMAND Test_D3D12UploadManager)
            
            # D3D12CommandList 测试
            add_executable(Test_D3D12CommandList
                Tests/D3D12CommandListTest.cpp
            )
            
            target_link_libraries(Test_D3D12CommandList
                PRIVATE
                    EngineRHID3D12
                    GTest::gtest
                    GTest::gtest_main
                    GTest::gmock
            )
            
            target_compile_definitions(Test_D3D12CommandList
                PRIVATE
                    ENGINE_RHI_D3D12
            )
            
            # 添加测试
            add_test(NAME D3D12CommandListTest COMMAND Test_D3D12CommandList)
        endif()
    endif()
endif()

# =============================================================================
# 配置摘要
# =============================================================================

message(STATUS "")
message(STATUS "========================================")
message(STATUS "引擎配置摘要")
message(STATUS "========================================")
message(STATUS "版本：${PROJECT_VERSION}")
message(STATUS "构建编辑器：${ENGINE_BUILD_EDITOR}")
message(STATUS "构建示例：${ENGINE_BUILD_SAMPLES}")
message(STATUS "构建测试：${ENGINE_BUILD_TESTS}")
message(STATUS "D3D12 后端：${ENGINE_RHI_D3D12}")
message(STATUS "Vulkan 后端：${ENGINE_RHI_VULKAN}")
message(STATUS "调试层：${ENGINE_DEBUG_LAYER}")
message(STATUS "警告视为错误：${ENGINE_WARNINGS_AS_ERRORS}")
message(STATUS "========================================")
message(STATUS "")

// =============================================================================
// D3D12SwapChainTest.cpp - D3D12SwapChain 单元测试
// =============================================================================

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "D3D12SwapChain.h"
#include "D3D12Device.h"

#include <Windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>

using namespace Engine::RHI;
using namespace Engine::RHI::D3D12;

// =============================================================================
// 测试辅助函数
// =============================================================================

/**
 * @brief 创建测试窗口
 * @return HWND 窗口句柄
 */
static HWND CreateTestWindow()
{
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = [](HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) -> LRESULT {
        switch (message) {
            case WM_DESTROY:
                PostQuitMessage(0);
                break;
            default:
                return DefWindowProcW(hWnd, message, wParam, lParam);
        }
        return 0;
    };
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.lpszClassName = L"TestWindowClass";
    
    RegisterClassExW(&wc);
    
    HWND hWnd = CreateWindowExW(
        0,
        L"TestWindowClass",
        L"Test Window",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        800,
        600,
        nullptr,
        nullptr,
        GetModuleHandleW(nullptr),
        nullptr
    );
    
    return hWnd;
}

/**
 * @brief 销毁测试窗口
 * @param hWnd 窗口句柄
 */
static void DestroyTestWindow(HWND hWnd)
{
    if (hWnd) {
        DestroyWindow(hWnd);
    }
}

// =============================================================================
// 测试固件
// =============================================================================

class D3D12SwapChainTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 创建设备
        DeviceDesc deviceDesc = {};
        deviceDesc.enableDebug = true;
        deviceDesc.enableValidation = true;
        
        m_device = std::make_unique<D3D12Device>();
        RHIResult result = m_device->Initialize(deviceDesc);
        
        // 如果设备创建失败，跳过测试
        if (IsFailure(result)) {
            GTEST_SKIP() << "D3D12 设备初始化失败，跳过测试";
        }
        
        // 创建测试窗口
        m_testWindow = CreateTestWindow();
        if (!m_testWindow) {
            GTEST_SKIP() << "创建测试窗口失败，跳过测试";
        }
    }
    
    void TearDown() override {
        m_swapChain.reset();
        
        if (m_testWindow) {
            DestroyTestWindow(m_testWindow);
            m_testWindow = nullptr;
        }
        
        if (m_device) {
            m_device->Shutdown();
            m_device.reset();
        }
    }
    
    // 创建交换链并验证结果
    RHIResult CreateSwapChain(uint32_t width = 800, uint32_t height = 600,
                               uint32_t bufferCount = 2, bool vsync = true)
    {
        SwapChainDesc desc = {};
        desc.windowHandle = m_testWindow;
        desc.width = width;
        desc.height = height;
        desc.format = Format::R8G8B8A8_UNorm;
        desc.bufferCount = bufferCount;
        desc.vsync = vsync;
        desc.debugName = "TestSwapChain";
        
        m_swapChain = std::make_unique<D3D12SwapChain>();
        return m_swapChain->Initialize(m_device.get(), desc);
    }
    
    std::unique_ptr<D3D12Device> m_device;
    std::unique_ptr<D3D12SwapChain> m_swapChain;
    HWND m_testWindow = nullptr;
};

// =============================================================================
// 测试用例 1：交换链创建
// =============================================================================

TEST_F(D3D12SwapChainTest, CreateSwapChain_Success)
{
    // Arrange
    SwapChainDesc desc = {};
    desc.windowHandle = m_testWindow;
    desc.width = 1920;
    desc.height = 1080;
    desc.format = Format::R8G8B8A8_UNorm;
    desc.bufferCount = 2;
    desc.vsync = true;
    desc.debugName = "TestSwapChain";
    
    // Act
    m_swapChain = std::make_unique<D3D12SwapChain>();
    RHIResult result = m_swapChain->Initialize(m_device.get(), desc);
    
    // Assert
    EXPECT_EQ(result, RHIResult::Success);
    ASSERT_NE(m_swapChain, nullptr);
    
    // 验证基本属性
    EXPECT_EQ(m_swapChain->GetWidth(), 1920u);
    EXPECT_EQ(m_swapChain->GetHeight(), 1080u);
    EXPECT_EQ(m_swapChain->GetBufferCount(), 2u);
    EXPECT_EQ(m_swapChain->GetFormat(), Format::R8G8B8A8_UNorm);
    
    // 验证 D3D12 对象
    EXPECT_NE(m_swapChain->GetDXGISwapChain(), nullptr);
    EXPECT_NE(m_swapChain->GetRTVHeap(), nullptr);
}

TEST_F(D3D12SwapChainTest, CreateSwapChain_DefaultDesc)
{
    // Act
    RHIResult result = CreateSwapChain();
    
    // Assert
    EXPECT_EQ(result, RHIResult::Success);
    EXPECT_NE(m_swapChain->GetDXGISwapChain(), nullptr);
}

TEST_F(D3D12SwapChainTest, CreateSwapChain_DifferentBufferCounts)
{
    // 测试双缓冲
    RHIResult result = CreateSwapChain(800, 600, 2);
    EXPECT_EQ(result, RHIResult::Success);
    EXPECT_EQ(m_swapChain->GetBufferCount(), 2u);
    
    m_swapChain.reset();
    
    // 测试三缓冲
    result = CreateSwapChain(800, 600, 3);
    EXPECT_EQ(result, RHIResult::Success);
    EXPECT_EQ(m_swapChain->GetBufferCount(), 3u);
}

TEST_F(D3D12SwapChainTest, CreateSwapChain_DifferentFormats)
{
    // 测试 R8G8B8A8_UNorm
    SwapChainDesc desc = {};
    desc.windowHandle = m_testWindow;
    desc.width = 800;
    desc.height = 600;
    desc.format = Format::R8G8B8A8_UNorm;
    desc.bufferCount = 2;
    
    m_swapChain = std::make_unique<D3D12SwapChain>();
    RHIResult result = m_swapChain->Initialize(m_device.get(), desc);
    EXPECT_EQ(result, RHIResult::Success);
    
    m_swapChain.reset();
    
    // 测试 R8G8B8A8_UNorm_SRGB
    desc.format = Format::R8G8B8A8_UNorm_SRGB;
    m_swapChain = std::make_unique<D3D12SwapChain>();
    result = m_swapChain->Initialize(m_device.get(), desc);
    EXPECT_EQ(result, RHIResult::Success);
    
    m_swapChain.reset();
    
    // 测试 R16G16B16A16_Float
    desc.format = Format::R16G16B16A16_Float;
    m_swapChain = std::make_unique<D3D12SwapChain>();
    result = m_swapChain->Initialize(m_device.get(), desc);
    EXPECT_EQ(result, RHIResult::Success);
}

// =============================================================================
// 测试用例 2：后备缓冲区访问
// =============================================================================

TEST_F(D3D12SwapChainTest, GetBackBufferResource_ValidIndex)
{
    // Arrange
    RHIResult result = CreateSwapChain(800, 600, 3);
    ASSERT_EQ(result, RHIResult::Success);
    
    // Act & Assert - 验证每个后备缓冲区
    for (uint32_t i = 0; i < 3; i++) {
        ID3D12Resource* resource = m_swapChain->GetBackBufferResource(i);
        EXPECT_NE(resource, nullptr) << "后备缓冲区 " << i << " 为空";
    }
}

TEST_F(D3D12SwapChainTest, GetBackBufferResource_CurrentIndex)
{
    // Arrange
    RHIResult result = CreateSwapChain();
    ASSERT_EQ(result, RHIResult::Success);
    
    // Act
    uint32_t currentIndex = m_swapChain->GetCurrentBackBufferIndex();
    ID3D12Resource* currentBuffer = m_swapChain->GetCurrentBackBufferResource();
    
    // Assert
    EXPECT_NE(currentBuffer, nullptr);
    EXPECT_LT(currentIndex, m_swapChain->GetBufferCount());
}

TEST_F(D3D12SwapChainTest, GetBackBufferResource_InvalidIndex)
{
    // Arrange
    RHIResult result = CreateSwapChain();
    ASSERT_EQ(result, RHIResult::Success);
    
    // Act - 使用越界索引
    ID3D12Resource* resource = m_swapChain->GetBackBufferResource(100);
    
    // Assert
    EXPECT_EQ(resource, nullptr);
}

TEST_F(D3D12SwapChainTest, GetRTV_ValidHandle)
{
    // Arrange
    RHIResult result = CreateSwapChain(800, 600, 2);
    ASSERT_EQ(result, RHIResult::Success);
    
    // Act & Assert - 验证每个 RTV 句柄
    for (uint32_t i = 0; i < 2; i++) {
        D3D12_CPU_DESCRIPTOR_HANDLE rtv = m_swapChain->GetRTV(i);
        EXPECT_NE(rtv.ptr, 0ull) << "RTV 句柄 " << i << " 为空";
    }
}

TEST_F(D3D12SwapChainTest, GetCurrentRTV_ValidHandle)
{
    // Arrange
    RHIResult result = CreateSwapChain();
    ASSERT_EQ(result, RHIResult::Success);
    
    // Act
    D3D12_CPU_DESCRIPTOR_HANDLE rtv = m_swapChain->GetCurrentRTV();
    
    // Assert
    EXPECT_NE(rtv.ptr, 0ull);
}

// =============================================================================
// 测试用例 3：Resize 测试
// =============================================================================

TEST_F(D3D12SwapChainTest, Resize_Success)
{
    // Arrange
    RHIResult result = CreateSwapChain(800, 600);
    ASSERT_EQ(result, RHIResult::Success);
    
    // Act
    m_swapChain->Resize(1920, 1080);
    
    // Assert
    EXPECT_EQ(m_swapChain->GetWidth(), 1920u);
    EXPECT_EQ(m_swapChain->GetHeight(), 1080u);
    
    // 验证后备缓冲区仍然有效
    EXPECT_NE(m_swapChain->GetCurrentBackBufferResource(), nullptr);
    EXPECT_NE(m_swapChain->GetCurrentRTV().ptr, 0ull);
}

TEST_F(D3D12SwapChainTest, Resize_SameSize)
{
    // Arrange
    RHIResult result = CreateSwapChain(800, 600);
    ASSERT_EQ(result, RHIResult::Success);
    
    // Act - 调整到相同尺寸
    m_swapChain->Resize(800, 600);
    
    // Assert - 应该保持不变
    EXPECT_EQ(m_swapChain->GetWidth(), 800u);
    EXPECT_EQ(m_swapChain->GetHeight(), 600u);
}

TEST_F(D3D12SwapChainTest, Resize_MultipleTimes)
{
    // Arrange
    RHIResult result = CreateSwapChain(800, 600);
    ASSERT_EQ(result, RHIResult::Success);
    
    // Act - 多次调整
    m_swapChain->Resize(1280, 720);
    EXPECT_EQ(m_swapChain->GetWidth(), 1280u);
    EXPECT_EQ(m_swapChain->GetHeight(), 720u);
    
    m_swapChain->Resize(1920, 1080);
    EXPECT_EQ(m_swapChain->GetWidth(), 1920u);
    EXPECT_EQ(m_swapChain->GetHeight(), 1080u);
    
    m_swapChain->Resize(640, 480);
    EXPECT_EQ(m_swapChain->GetWidth(), 640u);
    EXPECT_EQ(m_swapChain->GetHeight(), 480u);
}

TEST_F(D3D12SwapChainTest, Resize_SmallSize)
{
    // Arrange
    RHIResult result = CreateSwapChain(1920, 1080);
    ASSERT_EQ(result, RHIResult::Success);
    
    // Act - 调整到小尺寸
    m_swapChain->Resize(320, 240);
    
    // Assert
    EXPECT_EQ(m_swapChain->GetWidth(), 320u);
    EXPECT_EQ(m_swapChain->GetHeight(), 240u);
    EXPECT_NE(m_swapChain->GetCurrentBackBufferResource(), nullptr);
}

// =============================================================================
// 测试用例 4：VSync 控制
// =============================================================================

TEST_F(D3D12SwapChainTest, VSync_EnableDisable)
{
    // Arrange
    RHIResult result = CreateSwapChain(800, 600, 2, true);
    ASSERT_EQ(result, RHIResult::Success);
    
    // Assert - 初始状态
    EXPECT_TRUE(m_swapChain->IsVSyncEnabled());
    
    // Act - 禁用 VSync
    m_swapChain->SetVSync(false);
    
    // Assert
    EXPECT_FALSE(m_swapChain->IsVSyncEnabled());
    
    // Act - 重新启用 VSync
    m_swapChain->SetVSync(true);
    
    // Assert
    EXPECT_TRUE(m_swapChain->IsVSyncEnabled());
}

TEST_F(D3D12SwapChainTest, VSync_InitialValueFromDesc)
{
    // Arrange & Act - 创建时启用 VSync
    RHIResult result = CreateSwapChain(800, 600, 2, true);
    ASSERT_EQ(result, RHIResult::Success);
    
    // Assert
    EXPECT_TRUE(m_swapChain->IsVSyncEnabled());
    
    m_swapChain.reset();
    
    // Arrange & Act - 创建时禁用 VSync
    result = CreateSwapChain(800, 600, 2, false);
    ASSERT_EQ(result, RHIResult::Success);
    
    // Assert
    EXPECT_FALSE(m_swapChain->IsVSyncEnabled());
}

// =============================================================================
// 测试用例 5：帧索引更新
// =============================================================================

TEST_F(D3D12SwapChainTest, FrameIndex_ValidRange)
{
    // Arrange
    RHIResult result = CreateSwapChain(800, 600, 2);
    ASSERT_EQ(result, RHIResult::Success);
    
    // Act
    uint32_t frameIndex = m_swapChain->GetCurrentBackBufferIndex();
    
    // Assert
    EXPECT_LT(frameIndex, 2u);
}

TEST_F(D3D12SwapChainTest, FrameIndex_AfterPresent)
{
    // Arrange
    RHIResult result = CreateSwapChain(800, 600, 3);
    ASSERT_EQ(result, RHIResult::Success);
    
    // 获取初始帧索引
    uint32_t initialIndex = m_swapChain->GetCurrentBackBufferIndex();
    
    // Act - 呈现一帧
    m_swapChain->Present();
    
    // Assert - 帧索引应该更新（或保持相同，取决于时机）
    uint32_t newIndex = m_swapChain->GetCurrentBackBufferIndex();
    EXPECT_LT(newIndex, 3u);
    
    // 注意：帧索引可能在 Present 后变化，也可能不变
    // 这取决于交换链的实现
    RHI_LOG_INFO("FrameIndex: %u -> %u", initialIndex, newIndex);
}

TEST_F(D3D12SwapChainTest, FrameIndex_AfterResize)
{
    // Arrange
    RHIResult result = CreateSwapChain(800, 600, 2);
    ASSERT_EQ(result, RHIResult::Success);
    
    // Act - 调整大小
    m_swapChain->Resize(1024, 768);
    
    // Assert - 帧索引应该重置
    uint32_t frameIndex = m_swapChain->GetCurrentBackBufferIndex();
    EXPECT_LT(frameIndex, 2u);
}

// =============================================================================
// 测试用例 6：错误处理
// =============================================================================

TEST_F(D3D12SwapChainTest, ErrorHandling_NullDevice)
{
    // Arrange
    SwapChainDesc desc = {};
    desc.windowHandle = m_testWindow;
    desc.width = 800;
    desc.height = 600;
    
    // Act - 使用空设备
    D3D12SwapChain swapChain;
    RHIResult result = swapChain.Initialize(nullptr, desc);
    
    // Assert
    EXPECT_EQ(result, RHIResult::InvalidParameter);
}

TEST_F(D3D12SwapChainTest, ErrorHandling_NullWindow)
{
    // Arrange
    SwapChainDesc desc = {};
    desc.windowHandle = nullptr;  // 空窗口
    desc.width = 800;
    desc.height = 600;
    
    // Act
    D3D12SwapChain swapChain;
    RHIResult result = swapChain.Initialize(m_device.get(), desc);
    
    // Assert
    EXPECT_EQ(result, RHIResult::InvalidParameter);
}

TEST_F(D3D12SwapChainTest, ErrorHandling_ZeroWidth)
{
    // Arrange
    SwapChainDesc desc = {};
    desc.windowHandle = m_testWindow;
    desc.width = 0;  // 无效宽度
    desc.height = 600;
    
    // Act
    D3D12SwapChain swapChain;
    RHIResult result = swapChain.Initialize(m_device.get(), desc);
    
    // Assert
    EXPECT_EQ(result, RHIResult::InvalidParameter);
}

TEST_F(D3D12SwapChainTest, ErrorHandling_ZeroHeight)
{
    // Arrange
    SwapChainDesc desc = {};
    desc.windowHandle = m_testWindow;
    desc.width = 800;
    desc.height = 0;  // 无效高度
    
    // Act
    D3D12SwapChain swapChain;
    RHIResult result = swapChain.Initialize(m_device.get(), desc);
    
    // Assert
    EXPECT_EQ(result, RHIResult::InvalidParameter);
}

TEST_F(D3D12SwapChainTest, ErrorHandling_InvalidBufferCount)
{
    // Arrange
    SwapChainDesc desc = {};
    desc.windowHandle = m_testWindow;
    desc.width = 800;
    desc.height = 600;
    desc.bufferCount = 1;  // 无效缓冲区数量（最小为 2）
    
    // Act
    D3D12SwapChain swapChain;
    RHIResult result = swapChain.Initialize(m_device.get(), desc);
    
    // Assert
    EXPECT_EQ(result, RHIResult::InvalidParameter);
}

// =============================================================================
// 测试用例 7：移动语义
// =============================================================================

TEST_F(D3D12SwapChainTest, MoveConstructor_TransferOwnership)
{
    // Arrange
    RHIResult result = CreateSwapChain(800, 600);
    ASSERT_EQ(result, RHIResult::Success);
    
    IDXGISwapChain3* originalSwapChain = m_swapChain->GetDXGISwapChain();
    uint32_t originalWidth = m_swapChain->GetWidth();
    
    // Act - 移动构造
    D3D12SwapChain movedSwapChain(std::move(*m_swapChain));
    
    // Assert
    EXPECT_EQ(movedSwapChain.GetWidth(), originalWidth);
    EXPECT_EQ(movedSwapChain.GetDXGISwapChain(), originalSwapChain);
    EXPECT_EQ(m_swapChain->GetDXGISwapChain(), nullptr);  // 原对象应该为空
}

TEST_F(D3D12SwapChainTest, MoveAssignment_TransferOwnership)
{
    // Arrange
    RHIResult result = CreateSwapChain(800, 600);
    ASSERT_EQ(result, RHIResult::Success);
    
    // 创建另一个交换链
    D3D12SwapChain anotherSwapChain;
    
    // Act - 移动赋值
    anotherSwapChain = std::move(*m_swapChain);
    
    // Assert
    EXPECT_EQ(anotherSwapChain.GetWidth(), 800u);
    EXPECT_NE(anotherSwapChain.GetDXGISwapChain(), nullptr);
}

// =============================================================================
// 测试用例 8：Present 测试
// =============================================================================

TEST_F(D3D12SwapChainTest, Present_Success)
{
    // Arrange
    RHIResult result = CreateSwapChain(800, 600);
    ASSERT_EQ(result, RHIResult::Success);
    
    // Act & Assert - Present 不应该崩溃
    EXPECT_NO_THROW(m_swapChain->Present());
}

TEST_F(D3D12SwapChainTest, Present_MultipleFrames)
{
    // Arrange
    RHIResult result = CreateSwapChain(800, 600);
    ASSERT_EQ(result, RHIResult::Success);
    
    // Act & Assert - 多次 Present 不应该崩溃
    for (int i = 0; i < 10; i++) {
        EXPECT_NO_THROW(m_swapChain->Present());
    }
}

TEST_F(D3D12SwapChainTest, Present_WithVSync)
{
    // Arrange
    RHIResult result = CreateSwapChain(800, 600, 2, true);
    ASSERT_EQ(result, RHIResult::Success);
    
    // Act & Assert - 启用 VSync 的 Present
    EXPECT_NO_THROW(m_swapChain->Present());
}

TEST_F(D3D12SwapChainTest, Present_WithoutVSync)
{
    // Arrange
    RHIResult result = CreateSwapChain(800, 600, 2, false);
    ASSERT_EQ(result, RHIResult::Success);
    
    // Act & Assert - 禁用 VSync 的 Present
    EXPECT_NO_THROW(m_swapChain->Present());
}

// =============================================================================
// 测试用例 9：关闭和重新初始化
// =============================================================================

TEST_F(D3D12SwapChainTest, Shutdown_CanBeCalledMultipleTimes)
{
    // Arrange
    RHIResult result = CreateSwapChain();
    ASSERT_EQ(result, RHIResult::Success);
    
    // Act - 调用两次关闭应该不会崩溃
    m_swapChain->Shutdown();
    m_swapChain->Shutdown();
    
    // Assert - 如果到达这里，说明多次关闭是安全的
    SUCCEED();
}

TEST_F(D3D12SwapChainTest, Initialize_CanBeCalledAfterShutdown)
{
    // Arrange
    RHIResult result = CreateSwapChain(800, 600);
    ASSERT_EQ(result, RHIResult::Success);
    
    // Act
    m_swapChain->Shutdown();
    
    SwapChainDesc desc = {};
    desc.windowHandle = m_testWindow;
    desc.width = 1024;
    desc.height = 768;
    desc.format = Format::R8G8B8A8_UNorm;
    desc.bufferCount = 2;
    
    RHIResult reinitResult = m_swapChain->Initialize(m_device.get(), desc);
    
    // Assert
    EXPECT_EQ(reinitResult, RHIResult::Success);
    EXPECT_EQ(m_swapChain->GetWidth(), 1024u);
    EXPECT_EQ(m_swapChain->GetHeight(), 768u);
}

TEST_F(D3D12SwapChainTest, Destructor_CleansUpProperly)
{
    // Arrange
    {
        D3D12SwapChain swapChain;
        
        SwapChainDesc desc = {};
        desc.windowHandle = m_testWindow;
        desc.width = 800;
        desc.height = 600;
        
        RHIResult result = swapChain.Initialize(m_device.get(), desc);
        ASSERT_EQ(result, RHIResult::Success);
        // swapChain 离开作用域时自动析构
    }
    
    // Assert - 如果到达这里，说明析构函数正常工作
    SUCCEED();
}

// =============================================================================
// 测试用例 10：边缘情况
// =============================================================================

TEST_F(D3D12SwapChainTest, LargeResolution)
{
    // Arrange & Act - 创建大分辨率交换链
    RHIResult result = CreateSwapChain(3840, 2160);  // 4K
    
    // Assert
    EXPECT_EQ(result, RHIResult::Success);
    EXPECT_EQ(m_swapChain->GetWidth(), 3840u);
    EXPECT_EQ(m_swapChain->GetHeight(), 2160u);
}

TEST_F(D3D12SwapChainTest, SmallResolution)
{
    // Arrange & Act - 创建小分辨率交换链
    RHIResult result = CreateSwapChain(320, 240);
    
    // Assert
    EXPECT_EQ(result, RHIResult::Success);
    EXPECT_EQ(m_swapChain->GetWidth(), 320u);
    EXPECT_EQ(m_swapChain->GetHeight(), 240u);
}

TEST_F(D3D12SwapChainTest, GetBackBuffer_ReturnsNullForInvalidIndex)
{
    // Arrange
    RHIResult result = CreateSwapChain();
    ASSERT_EQ(result, RHIResult::Success);
    
    // Act - 获取越界索引的后备缓冲区
    ITexture* texture = m_swapChain->GetBackBuffer(100);
    
    // Assert
    EXPECT_EQ(texture, nullptr);
}

// =============================================================================
// 主函数
// =============================================================================

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
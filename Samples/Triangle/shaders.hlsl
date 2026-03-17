// =============================================================================
// shaders.hlsl - 三角形着色器
// =============================================================================

// 顶点着色器输入
struct VSInput {
    float3 position : POSITION;
    float3 color    : COLOR;
};

// 顶点着色器输出 / 像素着色器输入
struct VSOutput {
    float4 position : SV_POSITION;
    float3 color    : COLOR;
};

// =============================================================================
// 顶点着色器
// =============================================================================

VSOutput VSMain(VSInput input) {
    VSOutput output;
    
    // 直接传递位置（已经在标准化设备坐标）
    output.position = float4(input.position, 1.0);
    
    // 传递颜色
    output.color = input.color;
    
    return output;
}

// =============================================================================
// 像素着色器
// =============================================================================

float4 PSMain(VSOutput input) : SV_TARGET {
    // 返回插值后的颜色
    return float4(input.color, 1.0);
}
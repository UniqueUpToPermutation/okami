#pragma once

#include <okami/diligent/Glfw.hpp>
#include <okami/diligent/Shader.hpp>

#include <okami/VertexFormat.hpp>
#include <okami/Texture.hpp>
#include <okami/Camera.hpp>
#include <okami/Transform.hpp>

#include <EngineFactory.h>
#include <RenderDevice.h>
#include <DeviceContext.h>
#include <SwapChain.h>
#include <BasicMath.hpp>

namespace okami::graphics::diligent {

    namespace DG = Diligent;
    
    inline DG::float2 ToDiligent(const glm::vec2& v) {
        return DG::float2(v.x, v.y);
    }
    inline DG::float3 ToDiligent(const glm::vec3& v) {
        return DG::float3(v.x, v.y, v.z);
    }
    inline DG::float4 ToDiligent(const glm::vec4& v) {
        return DG::float4(v.x, v.y, v.z, v.w);
    }
    inline DG::float4x4 ToDiligent(const glm::mat4x4& m) {
        return DG::float4x4(
            m[0][0], m[0][1], m[0][2], m[0][3],
            m[1][0], m[1][1], m[1][2], m[1][3],
            m[2][0], m[2][1], m[2][2], m[2][3],
            m[3][0], m[3][1], m[3][2], m[3][3]);
    }

    DG::SURFACE_TRANSFORM ToDiligent(
        SurfaceTransform transform);

    inline DG::float4x4 ToMatrix(
        const core::Transform& transform) {
        return ToDiligent(transform.ToMatrix());
    }

    DG::float4x4 GetProjection(
        const core::Camera* camera,
        const DG::SwapChainDesc& scDesc, 
        bool bIsGL);

    DG::float4x4 GetSurfacePretransformMatrix(
        const DG::SwapChainDesc& scDesc, 
		const DG::float3& f3CameraViewAxis);
    
    DG::float4x4 GetAdjustedProjectionMatrix(
        const DG::SwapChainDesc& scDesc, 
		bool bIsGL, 
		float FOV, 
		float NearPlane, 
		float FarPlane);

    DG::float4x4 GetAdjustedOrthoMatrix(
		bool bIsGL,
		const DG::float2& fCameraSize,
		float zNear, 
        float zFar);

    struct InputLayoutDiligent {
        std::vector<DG::LayoutElement> mElements;
    };

    void WritePassShaderMacros(
        const RenderPass& pass, 
        ShaderPreprocessorConfig& config);

    typedef std::function<void(
        DG::RENDER_DEVICE_TYPE DeviceType, 
        DG::EngineCreateInfo& EngineCI, 
        DG::SwapChainDesc& scDesc)> get_engine_initialization_attribs;

    void CreateDevice(
        GraphicsBackend backend,
        DG::IEngineFactory** factory,
        DG::IRenderDevice** device,
        std::vector<DG::IDeviceContext*>& contexts,
        DG::SwapChainDesc* scDefaultDesc,
        const get_engine_initialization_attribs& attribsFunc);

    void CreateSwapChain(
        DG::IRenderDevice* device,
        DG::IDeviceContext* immediateContext,
        DG::ISwapChain** swapChain,
        DG::SwapChainDesc scDefaultDesc,
        DG::IEngineFactory* factory,
        IWindow* window);

    DG::RESOURCE_DIMENSION ToDiligent(core::ResourceDimension dim);
    DG::VALUE_TYPE ToDiligent(core::ValueType valueType);
    DG::INPUT_ELEMENT_FREQUENCY ToDiligent(core::InputElementFrequency frequency);
    InputLayoutDiligent ToDiligent(const core::VertexFormat& layout);
    DG::TEXTURE_FORMAT ToDiligent(const core::TextureFormat& format);

    core::ResourceDimension ToOkami(DG::RESOURCE_DIMENSION dim);
    core::ValueType ToOkami(DG::VALUE_TYPE valueType);
    core::InputElementFrequency ToOkami(DG::INPUT_ELEMENT_FREQUENCY frequency);
    core::TextureFormat ToOkami(DG::TEXTURE_FORMAT format);

    uint GetTypeSize(DG::VALUE_TYPE type);
	DG::VALUE_TYPE GetComponentType(DG::TEXTURE_FORMAT texFormat);
	int GetComponentCount(DG::TEXTURE_FORMAT texFormat);
	int GetSize(DG::VALUE_TYPE v);
	int GetPixelByteSize(DG::TEXTURE_FORMAT format);
	bool GetIsNormalized(DG::TEXTURE_FORMAT format);
	bool GetIsSRGB(DG::TEXTURE_FORMAT format);
}
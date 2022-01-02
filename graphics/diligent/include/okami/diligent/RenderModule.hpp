#pragma once

#include <okami/Graphics.hpp>
#include <okami/Camera.hpp>

#include <DeviceContext.h>
#include <RenderDevice.h>
#include <SwapChain.h>
#include <BasicMath.hpp>

namespace DG = Diligent;

namespace okami::graphics::diligent {
    
    enum class RenderPass {
        UNKNOWN,
        DEPTH,
        COLOR,
        COLOR_FINAL,
        OVERLAY
    };

    struct RenderModuleParams {
        bool bRenderEntityIds = false;
    };

    struct RenderModuleGlobals {
        DG::float4x4 mView;
        DG::float4x4 mProjection;
        DG::float2 mViewportSize;
        DG::float3 mViewOrigin;
        DG::float3 mViewDirection;
        DG::float3 mWorldUp;
        core::Camera mCamera;
        core::Time mTime;
    };
    
    class IRenderModule : public IGraphicsObject {
    public:
        virtual void Startup(
            core::ISystem* renderer, 
            DG::IRenderDevice* device, 
            DG::ISwapChain* swapChain,
            const RenderModuleParams& params) = 0;
        virtual void QueueCommands(
            DG::IDeviceContext* context, 
            RenderPass pass,
            const RenderModuleGlobals& globals) = 0;
        virtual void Shutdown() = 0;
    };
}
#pragma once

#include <okami/Graphics.hpp>

#include <DeviceContext.h>
#include <RenderDevice.h>
#include <SwapChain.h>

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
    
    class IRenderModule : public IGraphicsObject {
    public:
        virtual void Startup(
            core::ISystem* renderer, 
            DG::IRenderDevice* device, 
            DG::ISwapChain* swapChain,
            const RenderModuleParams& params) = 0;
        virtual void QueueCommands(
            DG::IDeviceContext* context, 
            RenderPass pass) = 0;
        virtual void Shutdown() = 0;
    };
}
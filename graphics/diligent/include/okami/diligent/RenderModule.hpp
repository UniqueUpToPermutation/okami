#pragma once

#include <okami/Graphics.hpp>

#include <DeviceContext.h>
#include <RenderDevice.h>
#include <SwapChain.h>

namespace DG = Diligent;

namespace okami::graphics::diligent {
    class IRenderModule : public IGraphicsObject {
    public:
        virtual void Startup(DG::IRenderDevice* device, DG::ISwapChain* swapChain) = 0;
        virtual void QueueCommands(DG::IDeviceContext* context) = 0;
        virtual void Shutdown() = 0;
    };
}
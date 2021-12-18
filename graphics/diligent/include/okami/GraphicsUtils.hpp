#pragma once

#include <okami/Display.hpp>

#include <EngineFactory.h>
#include <RenderDevice.h>
#include <DeviceContext.h>
#include <SwapChain.h>

namespace DG = Diligent;

namespace okami::graphics {
    typedef std::function<void(
        DG::RENDER_DEVICE_TYPE DeviceType, 
        DG::EngineCreateInfo& EngineCI, 
        DG::SwapChainDesc& SCDesc)> get_engine_initialization_attribs;

    void CreateDeviceAndSwapChain(
        IDisplay* display,
        INativeWindowProvider* windowProvider,
        DG::IEngineFactory** factory,
        DG::IRenderDevice** device,
        std::vector<DG::IDeviceContext*>& contexts,
        DG::ISwapChain** swapChain,
        const get_engine_initialization_attribs& attribsFunc);
}
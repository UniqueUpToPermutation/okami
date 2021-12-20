#pragma once

#include <okami/Display.hpp>
#include <okami/VertexLayout.hpp>
#include <okami/Texture.hpp>

#include <EngineFactory.h>
#include <RenderDevice.h>
#include <DeviceContext.h>
#include <SwapChain.h>

namespace DG = Diligent;

namespace okami::graphics {
    struct InputLayoutDiligent {
        std::vector<DG::LayoutElement> mElements;
    };

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

    DG::RESOURCE_DIMENSION ToDiligent(core::ResourceDimension dim);
    DG::VALUE_TYPE ToDiligent(core::ValueType valueType);
    DG::INPUT_ELEMENT_FREQUENCY ToDiligent(core::InputElementFrequency frequency);
    InputLayoutDiligent ToDiligent(const core::VertexLayout& layout);
    DG::TEXTURE_FORMAT ToDiligent(const core::TextureFormat& format);
}
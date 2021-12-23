#include <okami/Graphics.hpp>

#include <okami/diligent/BasicRenderer.hpp>
#include <okami/diligent/Display.hpp>

namespace okami::graphics {

    std::unique_ptr<core::ISystem> CreateDisplay(
        const RealtimeGraphicsParams& params) {
        return CreateGLFWDisplay(params);
    }

    std::unique_ptr<core::ISystem> CreateRenderer(core::ISystem* displaySystem, core::ResourceInterface& resources) {
        return std::make_unique<diligent::BasicRenderer>(displaySystem, resources);
    }

    std::unique_ptr<core::ISystem> CreateGLFWDisplay(const RealtimeGraphicsParams& params) {
        return std::make_unique<diligent::DisplayGLFW>(params);
    }
}
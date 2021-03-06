#include <okami/Graphics.hpp>

#include <okami/diligent/BasicRenderer.hpp>
#include <okami/diligent/Glfw.hpp>

namespace okami::graphics {
    std::unique_ptr<core::ISystem> CreateRenderer(
        IDisplay* display, core::ResourceManager& resources) {
        return std::make_unique<diligent::BasicRenderer>(display, resources);
    }

    std::unique_ptr<core::ISystem> CreateGLFWDisplay(
        core::ResourceManager* resourceInterface,
        const RealtimeGraphicsParams& params) {
        return std::make_unique<diligent::DisplayGLFW>(resourceInterface, params);
    }
}
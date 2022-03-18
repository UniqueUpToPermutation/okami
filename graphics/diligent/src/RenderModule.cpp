#include <okami/diligent/RenderModule.hpp>

namespace okami::graphics::diligent {
    void* IRenderModule::GetBackend() {
        return this;
    }

    void* IOverlayModule::GetUserData() {
        return GetBackend();
    }

    void IOverlayModule::RegisterVertexFormats(
        core::VertexLayoutRegistry& registry) {
    }

    void IOverlayModule::RegisterResourceInterfaces(
        core::ResourceManager& resourceInterface) {

    }
}
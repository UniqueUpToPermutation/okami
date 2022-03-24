#include <okami/Graphics.hpp>

using namespace entt;

namespace okami::graphics {

    entt::meta_type RenderCanvas::GetType() const {
        return entt::resolve<RenderCanvas>();
    }

    void RenderCanvas::Register() {
        entt::meta<RenderCanvas>()
            .type("RenderCanvas"_hs);
    }

    RenderCanvas::~RenderCanvas() {
        for (auto overlay : mOverlays) {
            overlay->OnDettach(this);
        }
    }

    bool RenderPass::operator==(const RenderPass& pass) const {
        bool bResult = pass.mAttributeCount == mAttributeCount;

        for (int i = 0; i < MaxAttributes; ++i) {
            bResult = bResult && 
                (i >= mAttributeCount || 
                mAttributes[i] == pass.mAttributes[i]);
        }

        return bResult;
    }

    std::size_t RenderPass::Hasher::operator()(const RenderPass& p) const noexcept {
        auto hasher = std::hash<uint>{};
        std::size_t result = hasher(p.mAttributeCount);

        for (int i = 0; i < MaxAttributes; ++i) {
            result ^= (i < p.mAttributeCount) 
                * hasher((uint)p.mAttributes[i]);
        }

        return result;
    }
}
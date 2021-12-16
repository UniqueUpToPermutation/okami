#include <okami/Frame.hpp>

using namespace entt;

namespace okami::core {
    entt::meta_type Frame::GetType() const {
        return entt::resolve<Frame>();
    }

    void Frame::Register() {
        entt::meta<Frame>()
            .type("Frame"_hs);
    }
}
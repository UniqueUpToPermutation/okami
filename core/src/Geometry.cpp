#include <okami/Geometry.hpp>

using namespace entt;

namespace okami::core {
    entt::meta_type Geometry::GetType() const {
        return entt::resolve<Geometry>();
    }

    void Geometry::Register() {
        entt::meta<Geometry>()
            .type("Geometry"_hs);
    }
}
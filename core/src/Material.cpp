#include <okami/Material.hpp>

using namespace entt;

namespace okami::core {
    entt::meta_type BaseMaterial::GetType() const {
        return entt::resolve<BaseMaterial>();
    }

    void BaseMaterial::Register() {
        entt::meta<BaseMaterial>()
            .type("BaseMaterial"_hs);
    }

    bool BaseMaterial::HasLoadParams() const {
        return false;
    }

    std::filesystem::path BaseMaterial::GetPath() const {
        throw std::runtime_error("BaseMaterial has no path!");
    }
}
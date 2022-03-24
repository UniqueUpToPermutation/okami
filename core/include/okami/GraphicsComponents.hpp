#pragma once

#include <okami/PlatformDefs.hpp>
#include <okami/Geometry.hpp>
#include <okami/Material.hpp>

namespace okami::core {

    class StaticMeshMaterial final : public BaseMaterial {
    public:
        StaticMeshMaterial() = default;
        StaticMeshMaterial(const BaseMaterial::Data& data) : 
            BaseMaterial(data) {    
        }

        inline const LoadParams<StaticMeshMaterial>& GetLoadParams() const {
            throw std::runtime_error("Materials don't have load params!");
        }
    };

    struct StaticMesh {
        resource_id_t mGeometry = INVALID_RESOURCE;
        resource_id_t mMaterial = INVALID_RESOURCE;
    };

    struct Sprite {
        resource_id_t mTexture;
        glm::vec4 mColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
        glm::vec2 mOrigin = glm::vec2(0.0f, 0.0f);
        int mLayer = 0;
    };
}
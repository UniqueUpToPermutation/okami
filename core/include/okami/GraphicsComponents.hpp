#pragma once

#include <okami/PlatformDefs.hpp>
#include <okami/Geometry.hpp>
#include <okami/Material.hpp>

namespace okami::core {

    class StaticMeshMaterial : public BaseMaterial {
    public:
        inline StaticMeshMaterial(const Data& data) : BaseMaterial(data) {
        }

        inline StaticMeshMaterial() = default;
    };

    struct StaticMesh {
        Handle<Geometry> mGeometry;
        Handle<StaticMeshMaterial> mMaterial;
    };

    struct Sprite {
        Handle<Texture> mTexture;
        glm::vec4 mColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
        glm::vec2 mOrigin = glm::vec2(0.0f, 0.0f);
        int mLayer = 0;
    };
}
#pragma once

#include <okami/PlatformDefs.hpp>
#include <okami/Geometry.hpp>
#include <okami/Material.hpp>

#include <glm/ext/scalar_constants.hpp>

namespace okami::core {
    struct StaticMesh {
        resource_id_t mGeometry = INVALID_RESOURCE;
        resource_id_t mMaterial = INVALID_RESOURCE;
    };

    struct PointLight {
        glm::vec3 mColor = glm::vec3(1.0f, 1.0f, 1.0f);
        float mRadiantFlux = 4.0f * glm::pi<float>();
        float mRadianceFalloff = 1.0f;
    };

    struct DirectionalLight {
        glm::vec3 mColor = glm::vec3(1.0f, 1.0f, 1.0f);
        float mIrradiance = 1.0f;
    };

    struct Sprite {
        resource_id_t mTexture;
        glm::vec4 mColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
        glm::vec2 mOrigin = glm::vec2(0.0f, 0.0f);
        int mLayer = 0;
    };
}
#pragma once

#include <glm/vec3.hpp>
#include <glm/matrix.hpp>
#include <glm/gtc/constants.hpp>

namespace okami::core {    
    struct Camera {
        enum class Type {
            PERSPECTIVE,
            ORTHOGRAPHIC
        };

        float mFieldOfView = glm::pi<float>() / 4.0f;
        float mNearPlane = 0.1f;
        float mFarPlane = 100.0f;

        glm::vec2 mOrthoSize = glm::vec2(0.0f, 0.0f);
        Type mType = Type::PERSPECTIVE;
    };
}
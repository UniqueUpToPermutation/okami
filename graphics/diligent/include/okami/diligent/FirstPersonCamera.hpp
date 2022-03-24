#pragma once

#include <okami/System.hpp>
#include <okami/Transform.hpp>
#include <okami/Input.hpp>

#include <glm/vec3.hpp>

namespace okami::graphics {
    struct FirstPersonController {
        struct Input {
            bool bForward = false;
            bool bBackward = false;
            bool bLeft = false;
            bool bRight = false;
            float mRotateX = 0.0f;
            float mRotateY = 0.0f;
        };

        float mMoveSpeed = 5.0f;
        float mRotateSpeed = 0.25f;

        bool bEnabled = true;
        bool bInitialized = false;

        glm::vec3 mDPos;
        glm::quat mQX;
        glm::quat mQY;
        
        void PrepareUpdate(const Input& input, 
            const core::Transform& transform,
            const core::Time& time);
        void FlushUpdate(core::Transform& output);
    };

    std::unique_ptr<core::ISystem> CreateFPSCameraSystem(core::IInputProvider* input);
}
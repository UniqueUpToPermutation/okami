#include <okami/Transform.hpp>
#include <okami/Meta.hpp>

#include <entt/entt.hpp>

using namespace entt;

namespace okami::core {
    void Transform::Register() {
        meta<Transform>()
            .type("Transform"_hs)
            .ctor()
            .data<&Transform::mRotation>("Rotation"_hs)
            .data<&Transform::mScale>("Scale"_hs)
            .data<&Transform::mTranslation>("Translation"_hs);

        RegisterConcept<Transform, ICopyableConcept>();
    }
}
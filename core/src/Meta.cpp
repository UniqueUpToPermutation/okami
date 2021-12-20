#include <okami/Meta.hpp>
#include <okami/Transform.hpp>
#include <okami/Geometry.hpp>
#include <okami/Frame.hpp>
#include <okami/Texture.hpp>
#include <okami/Material.hpp>

using namespace entt;

namespace okami::core {

    struct ConceptKey {
        entt::id_type mComponentType;
        entt::id_type mConceptType;

        struct Hasher {
            std::size_t operator()(const ConceptKey& k) const {
                return std::hash<entt::id_type>()(k.mComponentType) 
                    ^ std::hash<entt::id_type>()(k.mConceptType);
            }
        };

        inline bool operator==(const ConceptKey& other) const {
            return mComponentType == other.mComponentType &&
                mConceptType == other.mConceptType;
        }
    };

    std::unordered_map<ConceptKey, 
        std::unique_ptr<IComponentConcept>, 
        ConceptKey::Hasher> gConceptMap;

    void RegisterConcept(std::unique_ptr<IComponentConcept>&& concept_) {
        ConceptKey key;
        key.mComponentType = concept_->GetComponentType().id();
        key.mConceptType = concept_->GetType().id();

        gConceptMap.emplace(key, std::move(concept_));
    }

    IComponentConcept* GetConceptImpl(
        entt::id_type type, 
        entt::id_type conceptType) {

        ConceptKey key{type, conceptType};

        auto it = gConceptMap.find(key);

        if (it != gConceptMap.end()) {
            return it->second.get();
        } else {
            return nullptr;
        }
    }

    void Meta::Register() {
        meta<glm::vec2>()
            .type("vec2"_hs)
            .ctor()
            .data<&glm::vec2::x>("x"_hs)
            .data<&glm::vec2::y>("y"_hs);
        
        meta<glm::vec3>()
            .type("vec3"_hs)
            .ctor()
            .data<&glm::vec3::x>("x"_hs)
            .data<&glm::vec3::y>("y"_hs)
            .data<&glm::vec3::z>("z"_hs);

        meta<glm::vec4>()
            .type("vec4"_hs)
            .ctor()
            .data<&glm::vec4::x>("x"_hs)
            .data<&glm::vec4::y>("y"_hs)
            .data<&glm::vec4::z>("z"_hs)
            .data<&glm::vec4::w>("w"_hs);

        meta<glm::quat>()
            .type("quat"_hs)
            .ctor()
            .data<&glm::quat::x>("x"_hs)
            .data<&glm::quat::y>("y"_hs)
            .data<&glm::quat::z>("z"_hs)
            .data<&glm::quat::w>("w"_hs);

        Transform::Register();
        Geometry::Register();
        Frame::Register();
        Texture::Register();
        BaseMaterial::Register();
    }
}
#pragma once 

#include <entt/entt.hpp>

namespace okami::core {

    class IComponentConcept {
    public:
        virtual entt::meta_type GetType() const = 0;
        virtual entt::meta_type GetComponentType() const = 0;
    };

    class ICopyableConcept : public IComponentConcept {
    public:
        virtual void Copy(
            entt::registry& dest_registry, 
            const std::vector<entt::entity>& dest_entities,
            entt::registry& src_registry, 
            const std::vector<entt::entity>& src_entities) = 0;
        
        entt::meta_type GetType() const override {
            return entt::resolve<ICopyableConcept>();
        }

        template <typename ComponentT>
        static std::unique_ptr<ICopyableConcept> Spawn(); 
    };

    template <typename ComponentT>
    class CopyableConceptImpl : public ICopyableConcept {
    public:
        void Copy(entt::registry& dest_registry, 
            const std::vector<entt::entity>& dest_entities,
            entt::registry& src_registry, 
            const std::vector<entt::entity>& src_entities) override {
            assert(dest_entities.size() == src_entities.size());

            for (size_t i = 0; i < dest_entities.size(); ++i) {
                auto src_entity = src_entities[i];
                auto dest_entity = dest_entities[i];
                dest_registry.emplace<ComponentT>(dest_entity, 
                    src_registry.get<ComponentT>(src_entity));
            }
        }

        entt::meta_type GetComponentType() const override {
            return entt::resolve<ComponentT>();
        }
    };

    template <typename ComponentT>
    std::unique_ptr<ICopyableConcept> ICopyableConcept::Spawn() {
        return std::make_unique<CopyableConceptImpl<ComponentT>>();
    }

    void RegisterConcept(std::unique_ptr<IComponentConcept>&& concept_);
    IComponentConcept* GetConceptImpl(entt::id_type type, entt::id_type conceptType);

    template <typename TypeT, typename ConceptT>
    inline void RegisterConcept() {
        RegisterConcept(ConceptT::template Spawn<TypeT>());
    }

    template <typename ConceptT>
    inline ConceptT* GetConcept(entt::id_type type) {
        return dynamic_cast<ConceptT*>(GetConceptImpl(type, entt::resolve<ConceptT>().id()));
    }

    template <typename TypeT, typename ConceptT>
    inline ConceptT* GetConcept() {
        return GetConcept<ConceptT>(entt::resolve<TypeT>().id());
    }

    struct Meta {
        static void Register();
    };
}
#pragma once

#include <marl/containers.h>

#include <okami/Frame.hpp>

namespace okami::core {

    enum class ObserverType {
        ON_CONSTRUCT,
        ON_DESTROY,
        ON_UPDATE
    };
    
    template <
        typename ComponentType, 
        ObserverType ObsType, 
        uint BaseCapacity = 32>
    class Observer {
    private:
        marl::containers::vector<
            entt::entity, BaseCapacity> mQueue;
        entt::registry* mRegistry = nullptr;

        inline void invoke(entt::registry& reg, entt::entity e) {
            mQueue.emplace_back(std::move(e));
        }
    public:
        Observer() = default;

        inline auto begin() const {
            return mQueue.begin();
        }

        inline auto end() const {
            return mQueue.end();
        }

        inline void clear() {
            mQueue.resize(0);
        }

        void dettach() {
            if (mRegistry) {
                if constexpr (ObsType == ObserverType::ON_CONSTRUCT) {
                    mRegistry->on_construct<ComponentType>().template disconnect<
                        &Observer<ComponentType, ObsType, BaseCapacity>::invoke>(this);
                } else if constexpr (ObsType == ObserverType::ON_DESTROY) {
                    mRegistry->on_destroy<ComponentType>().template disconnect<
                        &Observer<ComponentType, ObsType, BaseCapacity>::invoke>(this);
                } else if constexpr (ObsType == ObserverType::ON_UPDATE) {
                    mRegistry->on_update<ComponentType>().template disconnect<
                        &Observer<ComponentType, ObsType, BaseCapacity>::invoke>(this);
                }
            }
        }

        void attach(entt::registry& registry) {
            dettach();

            mRegistry = &registry;
            
            if constexpr (ObsType == ObserverType::ON_CONSTRUCT) {
                registry.on_construct<ComponentType>().template connect<
                    &Observer<ComponentType, ObsType, BaseCapacity>::invoke>(this);
            } else if constexpr (ObsType == ObserverType::ON_DESTROY) {
                registry.on_destroy<ComponentType>().template connect<
                    &Observer<ComponentType, ObsType, BaseCapacity>::invoke>(this);
            } else if constexpr (ObsType == ObserverType::ON_UPDATE) {
                registry.on_update<ComponentType>().template connect<
                    &Observer<ComponentType, ObsType, BaseCapacity>::invoke>(this);
            }
        }

        inline void attach(Frame& frame) {
            attach(frame.Registry());
        }
    };
}
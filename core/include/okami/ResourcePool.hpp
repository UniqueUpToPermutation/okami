#pragma once

#include <filesystem>
#include <unordered_map>
#include <atomic>

#include <entt/entt.hpp>

namespace okami::core {

    typedef uint32_t resource_id_t;
    typedef int32_t ref_count_t;

    struct ResourceIdentifier {
        entt::entity mFrameId;
        resource_id_t mFrameResourceId;
    };

    struct RefCounter {
        std::atomic<ref_count_t> mCount;
    };

    struct FrameHeader {
        std::filesystem::path mPath;
    };

    class ResourcePool {
    private:
        std::unordered_map<std::filesystem::path, entt::entity> mPathToFrame;
        entt::registry mRegistry;

    public:
        class Handle {
        private:
            ResourcePool* mPool = nullptr;
            entt::entity mEntity = entt::null;

        protected:
            template <typename T>
            inline T& Get() {
                return mPool->mRegistry.get<T>();
            }

        public:
            inline Handle(const Handle& handle) : 
                mPool(handle.mPool), 
                mEntity(handle.mEntity) {
                Get<RefCounter>().mCount++;
            }

            inline Handle& operator=(const Handle& handle) {
                mPool = handle.mPool;
                mEntity = handle.mEntity;
                Get<RefCounter>().mCount++;
            }

            inline Handle(Handle&& handle) = default;
            inline Handle& operator=(Handle&& handle) = default;

            inline ~Handle() {
                Get<RefCounter>().mCount--;
            }
        };
    };

    template <typename desc_t>
    class ResourceHandle {
    private:
        ResourcePool::Handle mUnderlying;

    public:
        inline desc_t& Desc() {
            return mUnderlying.Get<desc_t>();
        }

        inline ResourceIdentifier& Id() {
            return mUnderlying.Get<ResourceIdentifier>();
        }
    };
}
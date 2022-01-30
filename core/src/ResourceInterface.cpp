#include <okami/ResourceInterface.hpp>

namespace okami::core {
    void ResourceInterface::Destroy(WeakHandle<Resource> object) {
        {
            marl::lock lock(mResourcesMut);

            auto it = mResourceToPath.find(object);

            if (it != mResourceToPath.end()) {
                mPathToResource.erase(it->second);
                mResourceToPath.erase(it);
            }

            auto resIt = mOrphanedResources.find(object);
            if (resIt != mOrphanedResources.end()) {
                mOrphanedResources.erase(object);
            }
        }

        object.Free();
    }

    void ResourceInterface::DestroyAllOrphaned() {
        set_t resourcesCopy;
        {
            marl::lock lock(mResourcesMut);
        
            resourcesCopy = std::move(mOrphanedResources);

            mResourceToPath.clear();
            mPathToResource.clear();
        }

        for (auto res : resourcesCopy) {
            res.Free();
        }
    }

    void ResourceInterfaceDestructor(void* owner, WeakHandle<Resource> object) {
        reinterpret_cast<ResourceInterface*>(owner)->Destroy(object);
    }
}
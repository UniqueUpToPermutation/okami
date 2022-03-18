#pragma once

#include <filesystem>
#include <unordered_map>
#include <atomic>

#include <entt/entt.hpp>

#include <okami/PlatformDefs.hpp>
#include <okami/Resource.hpp>
#include <okami/Async.hpp>
#include <okami/Hashers.hpp>
#include <okami/Frame.hpp>
#include <okami/Pool.hpp>

namespace okami::core {

    struct ResourceDigraph;

    class IResourcePoolAbstract {
    public:
        virtual entt::meta_type GetType() const = 0;
        virtual void RemoveNoResult(resource_id_t) = 0;
    };

    template <typename T>
    class ResourcePool final :
        public StablePool<resource_id_t, T>,
        public IResourcePoolAbstract {
    public:
        inline ResourcePool(uint32_t blockSize = 64) :
            StablePool<resource_id_t, T>(blockSize) {
        }

        entt::meta_type GetType() const override {
            return entt::resolve<T>();
        }

        void RemoveNoResult(resource_id_t id) override {
            StablePool<resource_id_t, T>::Dealloc(id);
        }
    };

    class PoolCollection {
    private:
        std::unordered_map<entt::meta_type, 
            std::unique_ptr<IResourcePoolAbstract>, 
            TypeHash> mPools;

    public:
        template <typename T>
        inline ResourcePool<T>* GetPool() {
            auto it = mPools.find(entt::resolve<T>());
            if (it != mPools.end()) {
                return dynamic_cast<ResourcePool<T>*>(it->second.get());
            } else {
                throw std::runtime_error("No pool for type!");
            }
        }

        template <typename T>
        inline void MakePool() {
            mPools[entt::resolve<T>()] = std::make_unique<ResourcePool<T>>();
        }

        template <typename T>
        inline void DestroyPool() {
            mPools.erase(entt::resolve<T>());
        }

        template <typename T>
        inline T& Add(resource_id_t id, T&& item) {
            auto pool = GetPool<T>();
            return pool->Add(id, std::move(item));
        }

        template <typename T>
        inline T Remove(resource_id_t id) {
            auto pool = GetPool<T>();
            return pool->Remove(id);
        }

        inline void Remove(resource_id_t id, const entt::meta_type& type) {
            auto poolIt = mPools.find(type);

            if (poolIt != mPools.end()) {
                poolIt->second->RemoveNoResult(id);
            } else {
                throw std::runtime_error("No pool for type!");
            }
        }
    };

    template <typename T>
    class IResourceBackend {
    public:
        // Implementation should be thread safe!
        virtual void NotifyAdd(resource_id_t id, T& frontend) = 0;
        virtual void NotifyDestroy(resource_id_t id, T& frontend) = 0;
    };

    struct ResourceDesc {
        bool bHasLoadParams;
        bool bIsManaged;
        std::filesystem::path mPath;
        entt::meta_type mType;
        entt::meta_any mPointer;
    };

    typedef std::function<void(entt::meta_any)> resource_updater_any_t;
    
    template <typename T>
    using resource_updater_t = std::function<void(T&)>;

    // This object can only be called from main thread!
    class ResourceManager {
    private:
        PoolCollection mPools;

        std::unordered_map<entt::meta_type, 
            entt::meta_any, 
            TypeHash> mLoaderInterfaces;

        std::unordered_map<std::filesystem::path,
            resource_id_t,
            PathHash> mPathToResource;

        std::unordered_map<entt::meta_type,
            std::function<void(resource_id_t, entt::meta_any)>,
            TypeHash> mDestroyNotifiers;

        std::unordered_map<resource_id_t, 
            ResourceDesc> mResourceDescs;
        
        resource_id_t mCurrentId = 0;

        std::unique_ptr<ResourceDigraph> mDependencies;
        std::set<resource_id_t> mGarbage;
       
        resource_id_t MakeNode();

        template <typename T>
        void InitResource(T* obj, resource_id_t id, bool isManaged) {
            bool bHasLoadParams = obj->HasLoadParams();
            std::filesystem::path path;

            if (bHasLoadParams) {
                path = obj->GetPath();
                auto pathIt = mPathToResource.find(path);

                // This resource has already been added!
                if (pathIt != mPathToResource.end()) {
                    throw std::runtime_error(
                        "ResourceManager already has a resource at the specified path!");
                } 

                mPathToResource.emplace(path, id);
            }

            obj->SetResourceId(id);

            ResourceDesc desc;
            desc.bHasLoadParams = bHasLoadParams;
            desc.mPath = path;
            desc.mType = entt::resolve<T>();
            desc.mPointer = obj;
            desc.bIsManaged = isManaged;

            mResourceDescs.emplace(id, desc);
        }

    public:
        ResourceManager();
        ~ResourceManager();

        template <typename T>
        inline T* TryGet(resource_id_t res) {
            auto it = mResourceDescs.find(res);
        
            if (it != mResourceDescs.end()) {
                return it->second.mPointer.cast<T*>();
            } else {
                return nullptr;
            }
        }

        template <typename T>
        inline T& Get(resource_id_t res) {
            auto it = mResourceDescs.find(res);

            if (it != mResourceDescs.end()) {
                return *(it->second.mPointer.cast<T*>());
            } else {
                throw std::runtime_error("Failed to find resource!");
            }
        }

        template <typename T>
        inline void Register(IResourceBackend<T>* loader) {
            mLoaderInterfaces.emplace(entt::resolve<T>(), loader);
            mPools.MakePool<T>();
            mDestroyNotifiers.emplace(entt::resolve<T>(),
                [loader, this](resource_id_t id, entt::meta_any res) {
                    loader->NotifyDestroy(id, *(res.cast<T*>()));
            });
        }

        template <typename T>
        inline void Unregister() {
            mLoaderInterfaces.erase(entt::resolve<T>());
            mPools.DestroyPool<T>();
            mDestroyNotifiers.erase(entt::resolve<T>());
        }

        void AddDependency(
            resource_id_t child, 
            resource_id_t parent);

        template <typename T>
        resource_id_t Add(T* obj) {
            auto resourceId = MakeNode();
            InitResource<T>(obj, resourceId, false);

            auto type = entt::resolve<T>();
            auto it = mLoaderInterfaces.find(type);

            if (it != mLoaderInterfaces.end()) {
                auto loader = it->second.template cast<IResourceBackend<T>*>();
                loader->NotifyAdd(resourceId, *obj);
            }

            return resourceId;
        };

        // Must be called from thread 1!
        template <typename T>
        resource_id_t Add(T&& obj, resource_id_t parent) {
            auto type = entt::resolve<T>();
            auto it = mLoaderInterfaces.find(type);

            if (it == mLoaderInterfaces.end()) {
                throw std::runtime_error("Loader for type unregistered!");
            }

            auto loader = it->second.template cast<IResourceBackend<T>*>();

            // Transfer ownership to the relevant pool
            auto resourceId = MakeNode();
            auto& objInPool = mPools.Add<T>(resourceId, std::move(obj));
            InitResource<T>(&objInPool, resourceId, true);

            if (parent != INVALID_RESOURCE) {
                AddDependency(resourceId, parent);
            }

            // Let the loader know it should do its thing
            loader->NotifyAdd(resourceId, objInPool);
            return resourceId;
        }

        template <typename T>
        inline resource_id_t Add(T&& obj) {
            return Add<T>(std::move(obj), INVALID_RESOURCE);
        }

        template <typename T>
        inline resource_id_t Add(T&& obj, const Frame& parentFrame) {
            return Add<T>(std::move(obj), parentFrame.GetResourceId());
        }

        void SendToGarbage(resource_id_t item);
        void CollectGarbage();

        inline void Free(resource_id_t item) {
            SendToGarbage(item);
            CollectGarbage();
        }

        template <typename T>
        inline void Free(T* obj) {
            Free(obj->GetResourceId());
        }

        template <typename T>
        inline void SendToGarbage(T* obj) {
            SendToGarbage(obj->GetResourceId());
        }
    };
}
#pragma once

#include <filesystem>
#include <unordered_map>
#include <atomic>

#include <entt/entt.hpp>

#include <okami/PlatformDefs.hpp>
#include <okami/Resource.hpp>
#include <okami/Async.hpp>
#include <okami/Hashers.hpp>

namespace okami::core {

    template <typename T>
    struct LoadParams {
    };

    class IResourceManagerAbstract {
    public:
        virtual entt::meta_type GetType() const = 0;
    };

    template <typename T>
    class IResourceManager : public IResourceManagerAbstract {
    public:
        virtual Handle<T> Load(
            const std::filesystem::path& path, 
            const LoadParams<T>& params, 
            resource_id_t newResId) = 0;

        virtual Handle<T> Add(T&& obj, 
            resource_id_t newResId) = 0;
        virtual Handle<T> Add(T&& obj, 
            const std::filesystem::path& path, 
            resource_id_t newResId) = 0;

        entt::meta_type GetType() const override {
            return entt::resolve<T>();
        }
    };

    void ResourceInterfaceDestructor(void* owner, WeakHandle<Resource> object);

    class ResourceInterface {
    private:
        std::unordered_map<entt::meta_type, 
            IResourceManagerAbstract*, TypeHash> mLoaderInterfaces;
        std::atomic<resource_id_t> mCurrentId = 0;

        marl::mutex mResourcesMut;

        typedef std::unordered_set<WeakHandle<Resource>,
            typename WeakHandle<Resource>::Hasher> set_t; 
            
        set_t mOrphanedResources;

        // All resources registered with a path
        std::unordered_map<std::filesystem::path,
            typename set_t::iterator, PathHash> mPathToResource;
        std::unordered_map<WeakHandle<Resource>,
            typename std::unordered_map<std::filesystem::path,
                typename set_t::iterator, PathHash>::iterator,
            typename WeakHandle<Resource>::Hasher> mResourceToPath;

        void Destroy(WeakHandle<Resource> object);
       
    public:
        void DestroyAllOrphaned();

        inline ~ResourceInterface() {
            DestroyAllOrphaned();
        }

        template <typename T>
        inline void Register(IResourceManager<T>* loader) {
            mLoaderInterfaces.emplace(entt::resolve<T>(), loader);
        }

        template <typename T>
        Handle<T> Load(const std::filesystem::path& path, 
            const LoadParams<T>& params = LoadParams<T>()) {
            auto type = entt::resolve<T>();
            auto it = mLoaderInterfaces.find(type);

            if (it == mLoaderInterfaces.end()) {
                marl::lock lock(mResourcesMut);

                auto it = mPathToResource.find(path);

                if (it == mPathToResource.end()) {
                    throw std::runtime_error("Type does not have a "
                        "ResourceManager! And an instance of this object "
                        "has not been created!");
                }

                Handle<Resource> handle = *it->second;
                return handle.TryCast<T>();
            }

            auto lookup = dynamic_cast<IResourceManager<T>*>(it->second);

            auto newId = mCurrentId.fetch_add(1);
            return lookup->Load(path, params, newId);
        }

        template <typename T>
        Handle<T> Add(T&& obj) {
            auto type = entt::resolve<T>();
            auto it = mLoaderInterfaces.find(type);

            auto newId = mCurrentId.fetch_add(1);

            if (it == mLoaderInterfaces.end()) {
                marl::lock lock(mResourcesMut);
                
                Handle<T> h(std::move(obj), this, &ResourceInterfaceDestructor);
                h->SetId(newId);
                mOrphanedResources.emplace(h.template DownCast<Resource, true>());

                return h;
            }

            auto lookup = dynamic_cast<IResourceManager<T>*>(it->second);

            return lookup->Add(std::move(obj), newId);
        }

        template <typename T>
        Handle<T> Add(T&& obj, const std::filesystem::path& path) {
            auto type = entt::resolve<T>();
            auto it = mLoaderInterfaces.find(type);

            auto newId = mCurrentId.fetch_add(1);

            if (it == mLoaderInterfaces.end()) {
                marl::lock lock(mResourcesMut);

                Handle<T> h(std::move(obj), this, &ResourceInterfaceDestructor);
                h->SetId(newId);

                auto it = mOrphanedResources.emplace(h.template DownCast<Resource, true>());
                
                auto it2 = mPathToResource.emplace(path, it);
                mResourceToPath.emplace(h.Ptr(), it2);

                return h;
            }

            auto lookup = dynamic_cast<IResourceManager<T>*>(it->second);

            return lookup->Add(std::move(obj), path, newId);
        }

        friend void ResourceInterfaceDestructor(void* owner, WeakHandle<Resource> object);
    };
}